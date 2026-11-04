#include "Settings.h"
#include "AtomicJson.h"
#include "DebouncedSave.h"
#include <LittleFS.h>

static const char* CFG_PATH = "/config.json";
static const char* CFG_TMP  = "/config.tmp";
static size_t g_storedSize = 0;

size_t settingsStoredSize() { return g_storedSize; }

static String fmtHHMM(uint16_t m) {
  if (m >= 1440) m = 0;
  char b[6];
  snprintf(b, sizeof(b), "%02u:%02u", (unsigned)(m / 60), (unsigned)(m % 60));
  return String(b);
}

static bool parseHHMM(const char* s, uint16_t& out) {
  if (!s) return false;
  unsigned h = 0, m = 0;
  char tail = 0;
  if (sscanf(s, "%u:%u%c", &h, &m, &tail) != 2) return false;
  if (h > 23 || m > 59) return false;
  out = (uint16_t)(h * 60 + m);
  return true;
}

void ClockSettings::setDefaults() {
  tz            = DEFAULT_TZ_NAME;
  tzPosix       = DEFAULT_TZ_POSIX;
  lang          = "pt";
  nightEnabled  = DEFAULT_NIGHT_ENABLED;
  nightStartMin = DEFAULT_NIGHT_START_MIN;
  nightEndMin   = DEFAULT_NIGHT_END_MIN;
  nightLevel    = DEFAULT_NIGHT_LEVEL;
}

void ClockSettings::toJson(JsonObject o) const {
  o["tz"]           = tz;
  o["tzPosix"]      = tzPosix;
  o["lang"]         = lang;
  o["nightEnabled"] = nightEnabled;
  o["nightStart"]   = fmtHHMM(nightStartMin);
  o["nightEnd"]     = fmtHHMM(nightEndMin);
  o["nightLevel"]   = nightLevel;
}

void ClockSettings::fromJson(JsonObjectConst o) {
  if (o["tz"].is<const char*>()) {
    tz = o["tz"].as<const char*>();
    tz.trim();
    if (tz.length() > 40) tz = tz.substring(0, 40);
  }
  if (o["tzPosix"].is<const char*>()) {
    String p = o["tzPosix"].as<const char*>();
    p.trim();
    if (p.length() > 48) p = p.substring(0, 48);
    tzPosix = p.length() ? p : "<-03>3";
  }
  if (o["lang"].is<const char*>()) {
    String l = o["lang"].as<const char*>();
    l.trim();
    l.toLowerCase();
    lang = (l == "en" || l == "pt" || l == "es") ? l : "pt";
  }
  if (o["nightEnabled"].is<bool>()) nightEnabled = o["nightEnabled"].as<bool>();
  if (o["nightStart"].is<const char*>()) parseHHMM(o["nightStart"].as<const char*>(), nightStartMin);
  if (o["nightEnd"].is<const char*>())   parseHHMM(o["nightEnd"].as<const char*>(),   nightEndMin);
  if (o["nightLevel"].is<int>()) {
    nightLevel = (uint8_t)constrain(o["nightLevel"].as<int>(), 0, 100);
  }
}

void DisplaySettings::setDefaults() {
  colorOrder = DEFAULT_COLOR_ORDER;
  invert     = DEFAULT_COLOR_INVERT;
  rGain      = DEFAULT_COLOR_GAIN;
  gGain      = 136; // Calibrated default for SmallTV ST7789 panel
  bGain      = DEFAULT_COLOR_GAIN;
}

void DisplaySettings::toJson(JsonObject o) const {
  const char* ord = (colorOrder == COLOR_ORDER_RGB) ? "rgb"
                  : (colorOrder == COLOR_ORDER_BGR) ? "bgr" : "auto";
  o["colorOrder"] = ord;
  o["invert"]     = invert;
  o["rGain"]      = rGain;
  o["gGain"]      = gGain;
  o["bGain"]      = bGain;
}

void DisplaySettings::fromJson(JsonObjectConst o) {
  if (o["colorOrder"].is<const char*>()) {
    String s = o["colorOrder"].as<const char*>();
    if      (s == "rgb") colorOrder = COLOR_ORDER_RGB;
    else if (s == "bgr") colorOrder = COLOR_ORDER_BGR;
    else                 colorOrder = COLOR_ORDER_AUTO;
  }
  if (o["invert"].is<bool>()) invert = o["invert"].as<bool>();
  if (o["rGain"].is<int>()) rGain = (uint8_t)constrain(o["rGain"].as<int>(), MIN_COLOR_GAIN, MAX_COLOR_GAIN);
  if (o["gGain"].is<int>()) gGain = (uint8_t)constrain(o["gGain"].as<int>(), MIN_COLOR_GAIN, MAX_COLOR_GAIN);
  if (o["bGain"].is<int>()) bGain = (uint8_t)constrain(o["bGain"].as<int>(), MIN_COLOR_GAIN, MAX_COLOR_GAIN);
}

void AuthSettings::setDefaults() {
  enabled = false;
  user    = DEFAULT_AUTH_USER;
  pass    = "";
}

void AuthSettings::toJson(JsonObject o, bool includeSecrets) const {
  o["enabled"] = enabled;
  o["user"]    = user;
  if (includeSecrets) o["pass"] = pass;
  else                o["passSet"] = pass.length() > 0;
}

void AuthSettings::fromJson(JsonObjectConst o) {
  if (o["user"].is<const char*>()) {
    String u = o["user"].as<const char*>();
    u.trim();
    if (u.length() > MAX_AUTH_USER_LEN) u = u.substring(0, MAX_AUTH_USER_LEN);
    if (u.length()) user = u;
  }
  if (o["pass"].is<const char*>()) {
    String p = o["pass"].as<const char*>();
    if (p.length() > MAX_AUTH_PASS_LEN) p = p.substring(0, MAX_AUTH_PASS_LEN);
    pass = p;
  }
  if (o["enabled"].is<bool>()) enabled = o["enabled"].as<bool>();
  if (!pass.length()) enabled = false;
}

void ButtonSettings::setDefaults() {
  sensitivity = DEFAULT_BTN_SENS;
  shortAction = DEFAULT_BTN_SHORT;
  longAction  = DEFAULT_BTN_LONG;
}

void ButtonSettings::toJson(JsonObject o) const {
  o["sensitivity"] = sensitivity;
  o["shortAction"] = shortAction;
  o["longAction"]  = longAction;
}

void ButtonSettings::fromJson(JsonObjectConst o) {
  if (o["sensitivity"].is<int>())
    sensitivity = (uint8_t)constrain(o["sensitivity"].as<int>(), 50, 99);
  if (o["shortAction"].is<int>()) {
    int a = o["shortAction"].as<int>();
    if (a >= 0 && a <= BTN_ACT_MAX) shortAction = (uint8_t)a;
  }
  if (o["longAction"].is<int>()) {
    int a = o["longAction"].as<int>();
    if (a >= 0 && a <= BTN_ACT_MAX) longAction = (uint8_t)a;
  }
}

void PetSettings::setDefaults() {
  strlcpy(name, "DHEXTERBOT", sizeof(name));
  strlcpy(statusText, "DhexterBot 2.0 Online!", sizeof(statusText));
  colorPreset = 0;   // Cyan Neon
  activity    = 1;   // Normal
  viewMode    = 0;   // Full Face
  accessory   = 0;   // None
  showHud     = true;
  showFooter  = true;
  scanlines   = true;
  autoSleep   = true;
  showMouth   = true;
  showBlush   = true;
  weatherFx   = true;
  lat         = -23.5505f;
  lon         = -46.6333f;

  sysPollSec  = 30;
  strlcpy(targets[0].name, "Internet", sizeof(targets[0].name));
  strlcpy(targets[0].host, "1.1.1.1", sizeof(targets[0].host));
  targets[0].port = 53;
  targets[0].enabled = true;

  strlcpy(targets[1].name, "Google DNS", sizeof(targets[1].name));
  strlcpy(targets[1].host, "8.8.8.8", sizeof(targets[1].host));
  targets[1].port = 53;
  targets[1].enabled = true;

  strlcpy(targets[2].name, "Zabbix NOC", sizeof(targets[2].name));
  strlcpy(targets[2].host, "10.10.1.26", sizeof(targets[2].host));
  targets[2].port = 80;
  targets[2].enabled = true;

  strlcpy(targets[3].name, "Hermes Brain", sizeof(targets[3].name));
  strlcpy(targets[3].host, "172.1.0.32", sizeof(targets[3].host));
  targets[3].port = 9119;
  targets[3].enabled = false;

  zbxEnabled  = false;
  strlcpy(zbxUrl, "http://10.10.1.26/api_jsonrpc.php", sizeof(zbxUrl));
  zbxToken[0] = 0;
  zbxMinSev   = 3;

  // ESP8266 = Buddy Body; Hermes = Brain (No cloud LLM keys stored in firmware!)
  hermesEnabled     = true;
  strlcpy(hermesUrl, "http://172.1.0.32:9119", sizeof(hermesUrl));
  hermesToken[0]    = 0;
  strlcpy(hermesPersona, "NOC Cyber Girl carismatica, prestativa e bem-humorada", sizeof(hermesPersona));
  hermesIntervalMin = 10;
  activeStartHour   = 8;
  activeEndHour     = 23;
}

void PetSettings::toJson(JsonObject o, bool includeSecrets) const {
  o["name"]        = name;
  o["statusText"]  = statusText;
  o["colorPreset"] = colorPreset;
  o["activity"]    = activity;
  o["viewMode"]    = viewMode;
  o["accessory"]   = accessory;
  o["showHud"]     = showHud;
  o["showFooter"]  = showFooter;
  o["scanlines"]   = scanlines;
  o["autoSleep"]   = autoSleep;
  o["showMouth"]   = showMouth;
  o["showBlush"]   = showBlush;
  o["weatherFx"]   = weatherFx;
  o["lat"]         = lat;
  o["lon"]         = lon;
  o["sysPollSec"]  = sysPollSec;
  o["zbxEnabled"]  = zbxEnabled;
  o["zbxUrl"]      = zbxUrl;
  o["zbxMinSev"]   = zbxMinSev;
  if (includeSecrets) {
    o["zbxToken"] = zbxToken;
  } else {
    o["zbxTokenSet"] = (zbxToken[0] != 0);
  }
  o["hermesEnabled"]     = hermesEnabled;
  o["hermesUrl"]         = hermesUrl;
  o["hermesPersona"]     = hermesPersona;
  o["hermesIntervalMin"] = hermesIntervalMin;
  o["activeStartHour"]   = activeStartHour;
  o["activeEndHour"]     = activeEndHour;
  if (includeSecrets) {
    o["hermesToken"] = hermesToken;
  } else {
    o["hermesTokenSet"] = (hermesToken[0] != 0);
  }
  JsonArray arr = o["targets"].to<JsonArray>();
  for (uint8_t i = 0; i < MAX_WATCH_TARGETS; i++) {
    JsonObject t = arr.add<JsonObject>();
    t["name"]    = targets[i].name;
    t["host"]    = targets[i].host;
    t["port"]    = targets[i].port;
    t["enabled"] = targets[i].enabled;
  }
}

void PetSettings::fromJson(JsonObjectConst o) {
  if (o["name"].is<const char*>()) {
    String n = o["name"].as<const char*>();
    n.trim();
    if (n.length() == 0) n = "DHEXTERBOT";
    strlcpy(name, n.c_str(), sizeof(name));
  }
  if (o["statusText"].is<const char*>()) {
    String st = o["statusText"].as<const char*>();
    st.trim();
    strlcpy(statusText, st.c_str(), sizeof(statusText));
  }
  if (o["colorPreset"].is<int>()) colorPreset = (uint8_t)constrain(o["colorPreset"].as<int>(), 0, 6);
  if (o["activity"].is<int>())    activity    = (uint8_t)constrain(o["activity"].as<int>(), 0, 2);
  if (o["viewMode"].is<int>())    viewMode    = (uint8_t)constrain(o["viewMode"].as<int>(), 0, 2);
  if (o["accessory"].is<int>())   accessory   = (uint8_t)constrain(o["accessory"].as<int>(), 0, 4);
  if (o["showHud"].is<bool>())    showHud     = o["showHud"].as<bool>();
  if (o["showFooter"].is<bool>()) showFooter  = o["showFooter"].as<bool>();
  if (o["scanlines"].is<bool>())  scanlines   = o["scanlines"].as<bool>();
  if (o["autoSleep"].is<bool>())  autoSleep   = o["autoSleep"].as<bool>();
  if (o["showMouth"].is<bool>())  showMouth   = o["showMouth"].as<bool>();
  if (o["showBlush"].is<bool>())  showBlush   = o["showBlush"].as<bool>();
  if (o["weatherFx"].is<bool>())  weatherFx   = o["weatherFx"].as<bool>();
  if (o["lat"].is<float>() || o["lat"].is<int>()) lat = constrain(o["lat"].as<float>(), -90.0f, 90.0f);
  if (o["lon"].is<float>() || o["lon"].is<int>()) lon = constrain(o["lon"].as<float>(), -180.0f, 180.0f);

  if (o["sysPollSec"].is<int>())  sysPollSec  = (uint16_t)constrain(o["sysPollSec"].as<int>(), 10, 600);
  if (o["zbxEnabled"].is<bool>()) zbxEnabled  = o["zbxEnabled"].as<bool>();
  if (o["zbxUrl"].is<const char*>()) {
    String u = o["zbxUrl"].as<const char*>();
    u.trim();
    strlcpy(zbxUrl, u.c_str(), sizeof(zbxUrl));
  }
  if (o["zbxToken"].is<const char*>()) {
    String tk = o["zbxToken"].as<const char*>();
    tk.trim();
    if (tk.length() > 0) strlcpy(zbxToken, tk.c_str(), sizeof(zbxToken));
  }
  if (o["zbxMinSev"].is<int>()) zbxMinSev = (uint8_t)constrain(o["zbxMinSev"].as<int>(), 0, 5);

  if (o["hermesEnabled"].is<bool>()) hermesEnabled = o["hermesEnabled"].as<bool>();
  else if (o["aiEnabled"].is<bool>()) hermesEnabled = o["aiEnabled"].as<bool>();
  if (o["hermesUrl"].is<const char*>()) {
    String h = o["hermesUrl"].as<const char*>();
    h.trim();
    if (h.length() > 0) strlcpy(hermesUrl, h.c_str(), sizeof(hermesUrl));
  }
  if (o["hermesToken"].is<const char*>()) {
    String ht = o["hermesToken"].as<const char*>();
    ht.trim();
    if (ht.length() > 0) strlcpy(hermesToken, ht.c_str(), sizeof(hermesToken));
  }
  if (o["hermesPersona"].is<const char*>()) {
    String hp = o["hermesPersona"].as<const char*>();
    hp.trim();
    if (hp.length() > 0) strlcpy(hermesPersona, hp.c_str(), sizeof(hermesPersona));
  }
  if (o["hermesIntervalMin"].is<int>()) hermesIntervalMin = (uint16_t)constrain(o["hermesIntervalMin"].as<int>(), 0, 240);
  else if (o["aiIntervalMin"].is<int>()) hermesIntervalMin = (uint16_t)constrain(o["aiIntervalMin"].as<int>(), 0, 240);
  if (o["activeStartHour"].is<int>()) activeStartHour = (uint8_t)constrain(o["activeStartHour"].as<int>(), 0, 23);
  if (o["activeEndHour"].is<int>())   activeEndHour   = (uint8_t)constrain(o["activeEndHour"].as<int>(), 0, 23);

  if (o["targets"].is<JsonArrayConst>()) {
    JsonArrayConst arr = o["targets"].as<JsonArrayConst>();
    uint8_t idx = 0;
    for (JsonObjectConst t : arr) {
      if (idx >= MAX_WATCH_TARGETS) break;
      if (t["name"].is<const char*>()) strlcpy(targets[idx].name, t["name"].as<const char*>(), sizeof(targets[idx].name));
      if (t["host"].is<const char*>()) strlcpy(targets[idx].host, t["host"].as<const char*>(), sizeof(targets[idx].host));
      if (t["port"].is<int>())         targets[idx].port = (uint16_t)constrain(t["port"].as<int>(), 1, 65535);
      if (t["enabled"].is<bool>())     targets[idx].enabled = t["enabled"].as<bool>();
      idx++;
    }
  }
}

void Settings::setDefaults() {
  for (uint8_t i = 0; i < MAX_WIFI_NETS; i++) {
    wifi[i].ssid = "";
    wifi[i].pass = "";
  }
  wifiCount         = 0;
  apSsid            = DEFAULT_AP_SSID;
  apPass            = DEFAULT_AP_PASS;
  hostname          = DEFAULT_HOSTNAME;
  mode              = MODE_PET;
  carouselSec       = DEFAULT_CAROUSEL_SEC;
  carouselPet       = true;
  httpTimeout       = DEFAULT_HTTP_TIMEOUT;
  brightness        = DEFAULT_BRIGHTNESS;
  autoBrightness    = false;
  backlightInverted = TFT_BL_DEFAULT_INVERTED;
  rotation          = 0;

  clock.setDefaults();
  display.setDefaults();
  auth.setDefaults();
  button.setDefaults();
  pet.setDefaults();
}

bool settingsBegin() {
#if defined(SMALLTV_FAMILY_ESP32)
  return LittleFS.begin(true);
#else
  if (LittleFS.begin()) return true;
  LittleFS.format();
  return LittleFS.begin();
#endif
}

bool loadSettings(Settings& s) {
  s.setDefaults();
  if (!LittleFS.exists(CFG_PATH)) return false;
  File f = LittleFS.open(CFG_PATH, "r");
  if (!f) return false;
  g_storedSize = f.size();
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  settingsApplyJson(s, doc.as<JsonObjectConst>());
  return true;
}

static bool writeSettingsNow(const Settings& s) {
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  settingsToJson(s, root, /*includeSecrets=*/true);
  if (!saveJsonAtomically(LittleFS, doc, CFG_PATH, CFG_TMP)) return false;
  g_storedSize = measureJson(doc);
  return true;
}

bool saveSettings(const Settings& s) {
  return writeSettingsNow(s);
}

void factoryReset(Settings& s) {
  LittleFS.remove(CFG_PATH);
  LittleFS.remove(CFG_TMP);
  s.setDefaults();
}

void settingsToJson(const Settings& s, JsonObject root, bool includeSecrets) {
  root["hostname"] = s.hostname;
  JsonArray w = root["wifi"].to<JsonArray>();
  for (uint8_t i = 0; i < s.wifiCount; i++) {
    JsonObject e = w.add<JsonObject>();
    e["ssid"] = s.wifi[i].ssid;
    if (includeSecrets) e["pass"] = s.wifi[i].pass;
    else                e["passSet"] = s.wifi[i].pass.length() > 0;
  }
  root["staSsid"] = s.wifiCount ? s.wifi[0].ssid : String("");
  if (includeSecrets) root["staPass"]    = s.wifiCount ? s.wifi[0].pass : String("");
  else                root["staPassSet"] = s.wifiCount && s.wifi[0].pass.length() > 0;

  root["apSsid"] = s.apSsid;
  if (includeSecrets) root["apPass"]    = s.apPass;
  else                root["apPassSet"] = s.apPass.length() > 0;

  root["mode"]              = "pet";
  root["brightness"]        = s.brightness;
  root["autoBrightness"]    = s.autoBrightness;
  root["backlightInverted"] = s.backlightInverted;
  root["rotation"]          = s.rotation;

  s.clock.toJson(root["clock"].to<JsonObject>());
  s.display.toJson(root["display"].to<JsonObject>());
  s.auth.toJson(root["auth"].to<JsonObject>(), includeSecrets);
  s.button.toJson(root["button"].to<JsonObject>());
  s.pet.toJson(root["pet"].to<JsonObject>(), includeSecrets);
}

static String findStoredWifiPass(const Settings& s, const String& ssid) {
  for (uint8_t i = 0; i < s.wifiCount; i++) {
    if (s.wifi[i].ssid == ssid) return s.wifi[i].pass;
  }
  return String();
}

void settingsApplyJson(Settings& s, JsonObjectConst root) {
  if (root["wifi"].is<JsonArrayConst>()) {
    WifiCred next[MAX_WIFI_NETS];
    uint8_t n = 0;
    for (JsonObjectConst e : root["wifi"].as<JsonArrayConst>()) {
      if (n >= MAX_WIFI_NETS) break;
      String ssid = e["ssid"] | "";
      String check = ssid; check.trim();
      if (!check.length()) continue;
      if (ssid.length() > 32) ssid = ssid.substring(0, 32);
      String pass;
      if (e["pass"].is<const char*>()) {
        String p = e["pass"].as<const char*>();
        pass = p.length() ? p : findStoredWifiPass(s, ssid);
      } else {
        pass = findStoredWifiPass(s, ssid);
      }
      if (pass.length() > 64) pass = pass.substring(0, 64);
      next[n].ssid = ssid;
      next[n].pass = pass;
      n++;
    }
    for (uint8_t i = 0; i < MAX_WIFI_NETS; i++) s.wifi[i] = (i < n) ? next[i] : WifiCred{};
    s.wifiCount = n;
  } else {
    if (root["staSsid"].is<const char*>()) {
      String ssid = root["staSsid"].as<const char*>();
      if (ssid.length()) {
        s.wifi[0].ssid = ssid;
        if (s.wifiCount == 0) s.wifiCount = 1;
      } else {
        s.wifiCount = 0;
      }
    }
    if (root["staPass"].is<const char*>()) {
      String p = root["staPass"].as<const char*>();
      if (p.length()) s.wifi[0].pass = p;
    }
  }

  if (root["apSsid"].is<const char*>())  s.apSsid = root["apSsid"].as<const char*>();
  if (root["apPass"].is<const char*>()) {
    String p = root["apPass"].as<const char*>();
    if (p.length() >= 8)       s.apPass = p;
    else if (p.length() == 0)  s.apPass = "";
  }
  if (root["hostname"].is<const char*>()) {
    String h = root["hostname"].as<const char*>();
    h.trim();
    if (h.length() > 24) h = h.substring(0, 24);
    for (size_t i = 0; i < h.length(); i++) {
      char c = h[i];
      bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || (c == '-' && i > 0);
      if (!ok) h.setCharAt(i, '-');
    }
    while (h.endsWith("-")) h.remove(h.length() - 1);
    s.hostname = h.length() ? h : String(DEFAULT_HOSTNAME);
  }

  s.mode = MODE_PET;
  if (root["brightness"].is<int>())
    s.brightness = (uint8_t)constrain(root["brightness"].as<int>(), 0, 100);
  if (root["autoBrightness"].is<bool>())
    s.autoBrightness = root["autoBrightness"].as<bool>();
  if (root["backlightInverted"].is<bool>())
    s.backlightInverted = root["backlightInverted"].as<bool>();
  if (root["rotation"].is<int>())
    s.rotation = (uint8_t)(root["rotation"].as<int>() & 3);

  if (root["clock"].is<JsonObjectConst>())   s.clock.fromJson(root["clock"].as<JsonObjectConst>());
  if (root["display"].is<JsonObjectConst>()) s.display.fromJson(root["display"].as<JsonObjectConst>());
  if (root["auth"].is<JsonObjectConst>())    s.auth.fromJson(root["auth"].as<JsonObjectConst>());
  if (root["button"].is<JsonObjectConst>())  s.button.fromJson(root["button"].as<JsonObjectConst>());
  if (root["pet"].is<JsonObjectConst>())     s.pet.fromJson(root["pet"].as<JsonObjectConst>());
}
