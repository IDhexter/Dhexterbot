// WebPortal.cpp — DhexterBot 2.0 Web Control Center & REST API
// Repository: https://github.com/IDhexter/Dhexterbot
#include "WebPortal.h"
#include "Platform.h"
#include "WebGuard.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "webui.h"
#include "StatusJson.h"
#include "Net.h"
#include "Gfx.h"
#include "Clock.h"
#include "SettingsTransaction.h"
#include "features/pet/PetMode.h"
#if WITH_NOTIFY
#include "NotifyMode.h"
#endif

extern void appInvalidate();
extern void appRelease();
extern const char* appResetReason();
extern void appApplyBrightness();

static WebServerClass server(80);
static Settings*      S = nullptr;
static bool           g_reboot = false;
static uint32_t       g_rebootAt = 0;

static void scheduleReboot(uint32_t inMs) {
  g_reboot   = true;
  g_rebootAt = millis() + inMs;
}

static const char* guardReject() {
  String host = server.hostHeader();
  if (netMode() != NET_AP) {
    const char* name = S->hostname.length() ? S->hostname.c_str() : DEFAULT_HOSTNAME;
    if (!webguard::hostAllowed(host.c_str(), name)) return "Forbidden: unexpected Host header";
  }
  HTTPMethod m = server.method();
  if (m != HTTP_GET && m != HTTP_HEAD && m != HTTP_OPTIONS) {
    String origin = server.header("Origin");
    String site   = server.header("Sec-Fetch-Site");
    if (!webguard::originAllowed(origin.c_str(), site.c_str(), host.c_str()))
      return "Forbidden: cross-site request";
  }
  return nullptr;
}

static bool guardAllowed() { return guardReject() == nullptr; }

static bool guardRequest() {
  const char* why = guardReject();
  if (!why) return true;
  server.send(403, "text/plain", why);
  return false;
}

static bool requireAuth() {
  if (!guardRequest()) return false;
  if (!S->auth.enabled || !S->auth.pass.length()) return true;
  if (server.authenticate(S->auth.user.c_str(), S->auth.pass.c_str())) return true;
  server.requestAuthentication(HTTPAuthMethod::DIGEST_AUTH, AUTH_REALM,
                               "Authentication required");
  return false;
}

static void sendJson(JsonDocument& doc, int code = 200) {
  String out;
  serializeJson(doc, out);
  server.send(code, "application/json", out);
}

static void handleRoot() {
  if (!requireAuth()) return;
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Content-Encoding", "gzip");
  server.send_P(200, "text/html", (PGM_P)WEBUI_HTML_GZ, WEBUI_HTML_GZ_LEN);
}

static void handleGetConfig() {
  if (!requireAuth()) return;
  appRelease();
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  settingsToJson(*S, root, /*includeSecrets=*/false);
  root["chip"] = "esp8266";
  sendJson(doc);
}

struct StatusSink : smalltv::Sink {
  bool write(const char* data, size_t count) override {
    server.sendContent(data, count);
    yield();
    return true;
  }
};

static void handleStatus() {
  if (!requireAuth()) return;
  const String ssid = netSSID(), ip = netIP();
  const String clockText = clockTimeStr();
  smalltv::StatusData d;
  d.fw         = FW_NAME;
  d.version    = FW_VERSION;
  d.variant    = FW_VARIANT;
  d.repo       = REPO_URL;
  d.updateMsg  = "";
  d.mode       = (netMode() == NET_AP) ? "ap" : "sta";
  d.connected  = netConnected();
  d.ssid       = ssid.c_str();
  d.ip         = ip.c_str();
  d.rssi       = netRSSI();
  d.wifiFail   = netFailReason();
  d.heap       = ESP.getFreeHeap();
  d.maxblk     = platformMaxFreeBlock();
  d.contstk    = platformFreeContStack();
  d.uptime     = millis() / 1000;
  d.reset      = appResetReason();
  d.synced     = clockSynced();
  d.time       = clockText.c_str();
  d.tz         = S->clock.tz.c_str();
  d.night      = clockNightActive();
  d.nightHeld  = clockNightHeld();
  d.clockFresh = clockTrusted();

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  StatusSink sink;
  smalltv::PieceWriter<128> out(sink);
  smalltv::writeStatusHead(out, d);
  smalltv::writeStatusEnd(out);
  server.sendContent("", 0);
}

static String netFingerprint(const Settings& s) {
  String f((int)s.wifiCount);
  for (uint8_t i = 0; i < s.wifiCount; i++) {
    f += '\n';
    f += s.wifi[i].ssid;
    f += '\x01';
    f += s.wifi[i].pass;
  }
  f += '\n';
  f += s.hostname;
  return f;
}

static void sendParseError(DeserializationError err) {
  if (err == DeserializationError::NoMemory)
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"not enough memory\"}");
  else
    server.send(400, "application/json", String("{\"ok\":false,\"error\":\"bad json (") + err.c_str() + ")\"}");
}

static void handlePostConfig() {
  if (!requireAuth()) return;
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "no body"); return; }

  appRelease();
  JsonDocument doc;
  if (DeserializationError err = deserializeJson(doc, server.arg("plain"))) {
    sendParseError(err);
    return;
  }

  String oldNet = netFingerprint(*S);
  if (!applyAndSaveSettings(*S,
        [&](Settings& settings) { settingsApplyJson(settings, doc.as<JsonObjectConst>()); doc.clear(); },
        [](const Settings& settings) { return saveSettings(settings); })) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"could not save settings\"}");
    return;
  }

  bool needReboot = (netFingerprint(*S) != oldNet);
  gfxSetRotation(S->rotation);
  gfxApplyColors(*S);
  clockReapply(*S);
  clockService(*S);
  appApplyBrightness();
  appInvalidate();

  server.send(200, "application/json",
              needReboot ? "{\"ok\":true,\"reboot\":true}"
                         : "{\"ok\":true,\"reboot\":false}");
  if (needReboot) scheduleReboot(800);
}

static void handleScan() {
  if (!requireAuth()) return;
  int n = WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["enc"]  = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
  }
  WiFi.scanDelete();
  sendJson(doc);
}

static void handleReboot() {
  if (!requireAuth()) return;
  server.send(200, "application/json", "{\"ok\":true}");
  scheduleReboot(400);
}

static void handleFactory() {
  if (!requireAuth()) return;
  factoryReset(*S);
  server.send(200, "application/json", "{\"ok\":true}");
  scheduleReboot(600);
}

static void handleExport() {
  if (!requireAuth()) return;
  appRelease();
  JsonDocument doc;
  settingsToJson(*S, doc.to<JsonObject>(), /*includeSecrets=*/true);
  String out;
  serializeJsonPretty(doc, out);
  server.sendHeader("Content-Disposition", "attachment; filename=\"dhexterbot-config.json\"");
  server.send(200, "application/json", out);
}

static void handleImport() {
  if (!requireAuth()) return;
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "no body"); return; }
  appRelease();
  JsonDocument doc;
  if (DeserializationError err = deserializeJson(doc, server.arg("plain"))) {
    sendParseError(err);
    return;
  }
  if (!applyAndSaveSettings(*S,
        [&](Settings& settings) { settingsApplyJson(settings, doc.as<JsonObjectConst>()); doc.clear(); },
        [](const Settings& settings) { return saveSettings(settings); })) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"could not save settings\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
  scheduleReboot(800);
}

// ---- DhexterBot 2.0 Live Interactive & System API (/api/pet) ---------------
static void handlePetGet() {
  if (!requireAuth()) return;
  JsonDocument doc;
  JsonObject o = doc.to<JsonObject>();
  o["ok"]           = true;
  o["mood"]         = petMoodName(g_petMode.currentMood());
  o["forced"]       = petMoodName(g_petMode.forcedMood());
  o["action"]       = petActionName(g_petMode.currentAction());
  o["speech"]       = g_petMode.currentSpeech();
  o["viewMode"]     = S->pet.viewMode;
  o["weatherValid"] = g_petMode.weatherValid();
  o["temp"]         = g_petMode.weatherTemp();
  o["weatherCode"]  = g_petMode.weatherCode();
  o["energy"]       = g_petMode.energy();
  o["coffee"]       = g_petMode.coffee();
  o["happiness"]    = g_petMode.happiness();
  o["xp"]           = g_petMode.xp();
  o["level"]        = g_petMode.level();
  o["pomoActive"]   = g_petMode.pomodoroActive();
  o["pomoRemSec"]   = g_petMode.pomodoroRemSec();
  o["zbxProblems"]  = g_petMode.zbxProblems();
  o["anyDown"]      = g_petMode.anySystemDown(*S);

  JsonArray tarr = o["targets"].to<JsonArray>();
  for (uint8_t i = 0; i < MAX_WATCH_TARGETS; i++) {
    JsonObject t = tarr.add<JsonObject>();
    const WatchTarget& wt = S->pet.targets[i];
    const TargetStatus& st = g_petMode.targetStatus(i);
    t["name"]      = wt.name;
    t["host"]      = wt.host;
    t["port"]      = wt.port;
    t["enabled"]   = wt.enabled;
    t["checked"]   = st.checked;
    t["up"]        = st.up;
    t["latencyMs"] = st.latencyMs;
  }
  sendJson(doc);
}

static void handlePetPost() {
  if (!requireAuth()) return;
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "no body"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }

  uint32_t holdSec = doc["hold"] | (uint32_t)25;
  uint32_t ttlSec  = doc["ttl"]  | holdSec;

  if (doc["viewMode"].is<int>()) {
    uint8_t vm = (uint8_t)constrain(doc["viewMode"].as<int>(), 0, 2);
    if (S->pet.viewMode != vm) {
      S->pet.viewMode = vm;
      saveSettings(*S);
      appInvalidate();
    }
  }
  if (doc["accessory"].is<int>()) {
    uint8_t ac = (uint8_t)constrain(doc["accessory"].as<int>(), 0, 4);
    S->pet.accessory = ac;
  }
  if (doc["action"].is<const char*>()) {
    uint8_t act = petActionFromStr(doc["action"].as<const char*>());
    g_petMode.triggerAction(act, doc["hold"] | (uint32_t)10);
  }
  if (doc["mood"].is<const char*>()) {
    PetMood m = (PetMood)petMoodFromStr(doc["mood"].as<const char*>());
    g_petMode.triggerMood(m, holdSec);
  }
  if (doc["say"].is<const char*>()) {
    const char* txt = doc["say"].as<const char*>();
    PetMood m = doc["mood"].is<const char*>() ? (PetMood)petMoodFromStr(doc["mood"].as<const char*>()) : PET_MOOD_AUTO;
    g_petMode.triggerSay(txt, m, ttlSec);
  }
  if (doc["blink"] | false) {
    g_petMode.triggerBlink();
  }
  if (doc["gx"].is<int>() || doc["gy"].is<int>()) {
    g_petMode.triggerGaze((int8_t)(doc["gx"] | 0), (int8_t)(doc["gy"] | 0));
  }
  if (doc["pomodoro"].is<int>()) {
    int mins = doc["pomodoro"].as<int>();
    if (mins > 0) g_petMode.startPomodoro((uint16_t)mins);
    else          g_petMode.stopPomodoro();
  }
  if (doc["checkSystems"] | false) {
    g_petMode.forceSystemCheck();
  }
  if (doc["alertCount"].is<int>()) {
    g_petMode.pushExternalAlert(doc["alertSource"] | "NOC",
                                doc["alertMsg"] | "",
                                doc["alertCount"].as<int>());
  }

  JsonDocument res;
  res["ok"]        = true;
  res["mood"]      = petMoodName(g_petMode.currentMood());
  res["action"]    = petActionName(g_petMode.currentAction());
  res["speech"]    = g_petMode.currentSpeech();
  res["viewMode"]  = S->pet.viewMode;
  res["energy"]    = g_petMode.energy();
  res["coffee"]    = g_petMode.coffee();
  res["happiness"] = g_petMode.happiness();
  res["level"]     = g_petMode.level();
  sendJson(res);
}

#if WITH_NOTIFY
static void handleNotify() {
  if (!requireAuth()) return;
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "no body"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "text/plain", "bad json");
    return;
  }
  const char* type = doc["type"];
  if (!type) type = doc["state"];

  NotifyRequest r;
  r.type   = type;
  r.title  = doc["title"];
  r.label  = doc["label"];
  r.anim   = doc["anim"];
  r.color  = doc["color"];
  r.ttlSec = doc["ttl"] | (uint32_t)0;
  r.prio   = doc["priority"] | (int)-1;

  g_petMode.triggerSay(r.label ? r.label : (r.title ? r.title : "Alerta!"),
                       PET_MOOD_ANGRY, r.ttlSec ? r.ttlSec : 25);

  switch (g_notifyMode.request(r)) {
    case NOTIFY_REJECTED:
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad request\"}");
      return;
    case NOTIFY_QUEUE_FULL:
      server.send(429, "application/json", "{\"ok\":false,\"error\":\"queue full\"}");
      return;
    case NOTIFY_ACCEPTED:
      break;
  }

  char body[48];
  snprintf(body, sizeof(body), "{\"ok\":true,\"queued\":%u}",
           (unsigned)g_notifyMode.queued());
  server.send(200, "application/json", body);
}
#endif

// ---- OTA ------------------------------------------------------------------
static void handleUpdateDone() {
  if (!requireAuth()) return;
  bool ok = !Update.hasError();
  server.sendHeader("Connection", "close");
  server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : platformUpdateError().c_str());
  if (ok) scheduleReboot(1200);
}

static void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  if (!guardAllowed()) {
    if (up.status != UPLOAD_FILE_START) Update.end();
    return;
  }
  if (S->auth.enabled && S->auth.pass.length() &&
      !server.authenticate(S->auth.user.c_str(), S->auth.pass.c_str())) {
    if (up.status != UPLOAD_FILE_START) Update.end();
    return;
  }
  if (up.status == UPLOAD_FILE_START) {
    appRelease();
#if defined(SMALLTV_FAMILY_ESP8266)
    WiFiUDP::stopAll();
#endif
    uint32_t maxSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSpace)) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_END) {
    if (!Update.end(true)) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.end();
  }
  yield();
}

static void handleNotFound() {
  if (netMode() == NET_AP) {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

void webPortalBegin(Settings& settings) {
  S = &settings;

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/pet", HTTP_GET, handlePetGet);
  server.on("/api/pet", HTTP_POST, handlePetPost);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/api/factory", HTTP_POST, handleFactory);
  server.on("/api/export", HTTP_GET, handleExport);
  server.on("/api/import", HTTP_POST, handleImport);
#if WITH_NOTIFY
  server.on("/api/notify", HTTP_POST, handleNotify);
#endif
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);

  server.on("/generate_204", handleNotFound);
  server.on("/gen_204", handleNotFound);
  server.on("/hotspot-detect.html", handleNotFound);
  server.on("/connecttest.txt", handleNotFound);
  server.onNotFound(handleNotFound);

#if defined(SMALLTV_FAMILY_ESP8266)
  server.collectHeaders("Origin", "Sec-Fetch-Site");
#else
  static const char* kGuardHeaders[] = {"Origin", "Sec-Fetch-Site"};
  server.collectHeaders(kGuardHeaders, 2);
#endif

  server.begin();
}

void webPortalLoop() {
  server.handleClient();
}

bool webPortalRebootDue() {
  return g_reboot && (int32_t)(millis() - g_rebootAt) >= 0;
}
