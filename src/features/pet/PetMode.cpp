// PetMode.cpp — DhexterBot 2.1: NOC Cyber Girl (Super Expressive Anime) + Neural AI & Hermes Bot
// Repository: https://github.com/IDhexter/Dhexterbot
#include "PetMode.h"
#include "Gfx.h"
#include "Clock.h"
#include "Net.h"
#include "Platform.h"
#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#if defined(SMALLTV_ESP8266)
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
#else
  #include <HTTPClient.h>
  #include <WiFiClient.h>
#endif
#include <math.h>

PetMode g_petMode;

const char* petMoodName(uint8_t m) {
  switch (m) {
    case PET_MOOD_NORMAL:    return "normal";
    case PET_MOOD_HAPPY:     return "happy";
    case PET_MOOD_ANGRY:     return "angry";
    case PET_MOOD_CURIOUS:   return "curious";
    case PET_MOOD_SURPRISED: return "surprised";
    case PET_MOOD_LOVE:      return "love";
    case PET_MOOD_CYBER:     return "cyber";
    case PET_MOOD_TIRED:     return "tired";
    case PET_MOOD_SLEEP:     return "sleep";
    case PET_MOOD_COOL:      return "cool";
    case PET_MOOD_EXCITED:   return "excited";
    default:                 return "auto";
  }
}

uint8_t petMoodFromStr(const char* s) {
  if (!s || !s[0]) return PET_MOOD_AUTO;
  String v(s);
  if (v.equalsIgnoreCase("normal"))    return PET_MOOD_NORMAL;
  if (v.equalsIgnoreCase("happy"))     return PET_MOOD_HAPPY;
  if (v.equalsIgnoreCase("angry") || v.equalsIgnoreCase("alert")) return PET_MOOD_ANGRY;
  if (v.equalsIgnoreCase("curious"))   return PET_MOOD_CURIOUS;
  if (v.equalsIgnoreCase("surprised")) return PET_MOOD_SURPRISED;
  if (v.equalsIgnoreCase("love"))      return PET_MOOD_LOVE;
  if (v.equalsIgnoreCase("cyber") || v.equalsIgnoreCase("neural") || v.equalsIgnoreCase("hermes")) return PET_MOOD_CYBER;
  if (v.equalsIgnoreCase("tired"))     return PET_MOOD_TIRED;
  if (v.equalsIgnoreCase("sleep"))     return PET_MOOD_SLEEP;
  if (v.equalsIgnoreCase("cool"))      return PET_MOOD_COOL;
  if (v.equalsIgnoreCase("excited"))   return PET_MOOD_EXCITED;
  return PET_MOOD_AUTO;
}

const char* petActionName(uint8_t a) {
  switch (a) {
    case PET_ACT_COFFEE:    return "coffee";
    case PET_ACT_CHARGE:    return "charge";
    case PET_ACT_PETTING:   return "pet";
    case PET_ACT_HACKER:    return "hacker";
    case PET_ACT_CELEBRATE: return "celebrate";
    default:                return "none";
  }
}

uint8_t petActionFromStr(const char* s) {
  if (!s || !s[0]) return PET_ACT_NONE;
  String v(s);
  if (v.equalsIgnoreCase("coffee") || v.equalsIgnoreCase("cafe")) return PET_ACT_COFFEE;
  if (v.equalsIgnoreCase("charge") || v.equalsIgnoreCase("bateria")) return PET_ACT_CHARGE;
  if (v.equalsIgnoreCase("pet") || v.equalsIgnoreCase("carinho")) return PET_ACT_PETTING;
  if (v.equalsIgnoreCase("hacker") || v.equalsIgnoreCase("matrix")) return PET_ACT_HACKER;
  if (v.equalsIgnoreCase("celebrate") || v.equalsIgnoreCase("festa")) return PET_ACT_CELEBRATE;
  return PET_ACT_NONE;
}

static uint16_t presetColor565(uint8_t preset, uint8_t activeMood, uint8_t activeAction) {
  if (activeAction == PET_ACT_HACKER) return 0x07EC; // Matrix Green
  if (activeMood == PET_MOOD_ANGRY)   return 0xF924; // Alert Red-Crimson
  if (activeMood == PET_MOOD_LOVE)    return 0xF993; // Hot Pink
  if (activeMood == PET_MOOD_EXCITED) return 0xFFE0; // Golden Star
  if (activeMood == PET_MOOD_CYBER)   return 0xB33F; // Neural Purple-Cyan
  if (activeMood == PET_MOOD_HAPPY)   return 0x07EC; // Emerald UP Green
  switch (preset) {
    case 0:  return 0x073F; // Electric Cyan (NOC Girl Signature Neon)
    case 1:  return 0x07EC; // Matrix Neon Green
    case 2:  return 0xB33F; // Cyber Purple
    case 3:  return 0xFD60; // Amber Gold
    case 4:  return 0x3D1F; // Ice Blue
    case 5:  return 0xF924; // Crimson Red
    case 6:  return 0xFFFF; // Pure White
    default: return 0x073F;
  }
}

static uint16_t scaleColor565(uint16_t c, uint8_t pct) {
  uint32_t r = ((c >> 11) & 0x1F) * pct / 100;
  uint32_t g = ((c >> 5)  & 0x3F) * pct / 100;
  uint32_t b = ( c        & 0x1F) * pct / 100;
  if (r > 31) r = 31;
  if (g > 63) g = 63;
  if (b > 31) b = 31;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

// Normalize incoming text while preserving Portuguese UTF-8 glyphs.
static void normalizeDialogText(const char* in, char* out, size_t maxLen) {
  if (!in || maxLen == 0) {
    if (maxLen > 0) out[0] = 0;
    return;
  }
  size_t j = 0;
  bool lastSpace = true;
  for (size_t i = 0; in[i] != 0 && j + 1 < maxLen; i++) {
    uint8_t c = (uint8_t)in[i];
    if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    if (c == ' ') {
      if (lastSpace) continue;
      out[j++] = ' ';
      lastSpace = true;
    } else if (c >= 32 && c <= 126) {
      out[j++] = (char)c;
      lastSpace = false;
    } else if (c == 0xC3 && in[i + 1] != 0 && j + 2 < maxLen) {
      out[j++] = (char)c;
      out[j++] = in[++i];
      lastSpace = false;
    }
  }
  while (j > 0 && out[j - 1] == ' ') j--;
  out[j] = 0;
}

void PetMode::begin(const Settings& s) {
  _fullRepaint   = true;
  _hudDirty      = true;
  _footerDirty   = true;
  _lastViewMode  = s.pet.viewMode;
  _nextBrainMs   = millis() + 2000;
  _nextBlinkMs   = millis() + 1800;
  _nextSaccadeMs = millis() + 1200;
  _nextPhraseMs  = millis() + 12000;
  _nextDialogPageMs = millis() + 4000;
  _lastWeatherMs = 0;
  _lastSysCheckMs = 0;
  _lastAiMs      = millis();
  _pendingAutoAi = true; // Trigger first autonomous greeting shortly after boot!
  for (uint8_t i = 0; i < MAX_WATCH_TARGETS; i++) {
    _targetStatus[i].checked   = false;
    _targetStatus[i].up        = true;
    _targetStatus[i].latencyMs = 0;
  }
}

void PetMode::invalidate(const Settings& s) {
  _fullRepaint  = true;
  _hudDirty     = true;
  _footerDirty  = true;
}

void PetMode::wake(const Settings& s) {
  _fullRepaint  = true;
  _hudDirty     = true;
  _footerDirty  = true;
}

void PetMode::triggerMood(uint8_t mood, uint16_t holdSec) {
  _forcedMood  = mood;
  if (mood != PET_MOOD_AUTO) _activeMood = mood;
  _forcedUntil = (mood == PET_MOOD_AUTO) ? 0 : (millis() + (uint32_t)holdSec * 1000UL);
  _nextBrainMs = 0;
  _hudDirty    = true;
  _footerDirty = true;
}

void PetMode::triggerAction(uint8_t action, uint16_t holdSec) {
  _activeAction = action;
  _actionUntil  = (action == PET_ACT_NONE) ? 0 : (millis() + (uint32_t)holdSec * 1000UL);
  if (action == PET_ACT_COFFEE) {
    triggerSay("Cafezinho especial do plantao NOC! Energia a mil!", PET_MOOD_HAPPY, holdSec, "NOC-GIRL");
  } else if (action == PET_ACT_CHARGE) {
    triggerSay("Headphones neon e sistemas em potencia maxima!", PET_MOOD_CYBER, holdSec, "NOC-GIRL");
  } else if (action == PET_ACT_PETTING) {
    triggerSay("Obrigada pelo carinho! Pronta para vigiar a rede!", PET_MOOD_LOVE, holdSec, "NOC-GIRL");
  } else if (action == PET_ACT_HACKER) {
    triggerSay("Analisando pacotes dos switches Aruba e links WAN...", PET_MOOD_CYBER, holdSec, "NOC-GIRL");
  } else if (action == PET_ACT_CELEBRATE) {
    triggerSay("Zero perda de pacotes! Todos os links 100% UP!", PET_MOOD_EXCITED, holdSec, "NOC-GIRL");
  }
  _hudDirty    = true;
  _footerDirty = true;
}

void PetMode::triggerBlink() {
  _blinkStage   = 1;
  _blinkStageMs = millis();
  _doubleBlink  = true;
}

void PetMode::triggerSay(const char* text, uint8_t mood, uint16_t ttlSec, const char* source) {
  if (!text || !text[0]) {
    _speechActive  = false;
    _speechText[0] = 0;
  } else {
    normalizeDialogText(text, _speechText, sizeof(_speechText));
    strlcpy(_speechSource, (source && source[0]) ? source : "NOC-GIRL", sizeof(_speechSource));
    _speechActive = true;
    _dialogPage = 0;
    _nextDialogPageMs = millis() + 4000;
    if (ttlSec < 2)   ttlSec = 2;
    if (ttlSec > 300) ttlSec = 300;
    _speechUntil = millis() + (uint32_t)ttlSec * 1000UL;
  }
  if (mood != PET_MOOD_AUTO) {
    triggerMood(mood, ttlSec);
  }
  _footerDirty = true;
}

void PetMode::triggerGaze(int8_t gx, int8_t gy, uint16_t holdMs) {
  _tgtX       = constrain((int)gx, -22, 22);
  _tgtY       = constrain((int)gy, -12, 12);
  _gazeForced = true;
  _gazeUntil  = millis() + holdMs;
}

void PetMode::startPomodoro(uint16_t minutes) {
  if (minutes == 0) {
    stopPomodoro();
    return;
  }
  if (minutes > 180) minutes = 180;
  _pomoActive  = true;
  _pomoTotalMs = (uint32_t)minutes * 60000UL;
  _pomoEndMs   = millis() + _pomoTotalMs;
  _lastPomoSec = 0xFFFF;
  char msg[64];
  snprintf(msg, sizeof(msg), "Modo Foco %um ativado! Fones no maximo!", (unsigned)minutes);
  triggerSay(msg, PET_MOOD_COOL, 12, "FOCO");
  _hudDirty    = true;
  _footerDirty = true;
}

void PetMode::stopPomodoro() {
  _pomoActive  = false;
  _lastPomoSec = 0xFFFF;
  _hudDirty    = true;
  _footerDirty = true;
}

uint32_t PetMode::pomodoroRemSec() const {
  if (!_pomoActive) return 0;
  int32_t rem = (int32_t)(_pomoEndMs - millis());
  return (rem > 0) ? (uint32_t)(rem / 1000) : 0;
}

void PetMode::forceSystemCheck() {
  _lastSysCheckMs = 0;
  _lastZbxCheckMs = 0;
}

void PetMode::pushExternalAlert(const char* source, const char* message, int alertsCount) {
  _zbxProblems = alertsCount;
  if (alertsCount > 0) {
    if (message && message[0]) {
      triggerSay(message, PET_MOOD_ANGRY, 45, source ? source : "ZABBIX");
    } else {
      char msg[96];
      snprintf(msg, sizeof(msg), "ALERTA %s: %d incidente(s) ativos na rede!", source ? source : "NOC", alertsCount);
      triggerSay(msg, PET_MOOD_ANGRY, 45, source ? source : "ZABBIX");
    }
  } else {
    triggerAction(PET_ACT_CELEBRATE, 10);
    if (message && message[0]) triggerSay(message, PET_MOOD_EXCITED, 25, source ? source : "ZABBIX");
  }
  _hudDirty    = true;
  _footerDirty = true;
}

void PetMode::pushHermesEvent(const char* eventType, const char* message, const char* moodStr, int alertCount) {
  _hermesEventsCount++;
  if (alertCount >= 0) _zbxProblems = alertCount;

  uint8_t m = PET_MOOD_CYBER;
  if (moodStr && moodStr[0]) {
    m = petMoodFromStr(moodStr);
  } else if (eventType) {
    String ev(eventType);
    if (ev.equalsIgnoreCase("alert") || ev.equalsIgnoreCase("critical") || ev.equalsIgnoreCase("error") || ev.equalsIgnoreCase("danger")) {
      m = PET_MOOD_ANGRY;
    } else if (ev.equalsIgnoreCase("success") || ev.equalsIgnoreCase("task_done") || ev.equalsIgnoreCase("excited") || ev.equalsIgnoreCase("celebrate")) {
      m = PET_MOOD_EXCITED;
      triggerAction(PET_ACT_CELEBRATE, 8);
    } else if (ev.equalsIgnoreCase("ok") || ev.equalsIgnoreCase("recovery") || ev.equalsIgnoreCase("happy")) {
      m = PET_MOOD_HAPPY;
    } else if (ev.equalsIgnoreCase("cool")) {
      m = PET_MOOD_COOL;
    } else if (ev.equalsIgnoreCase("curious") || ev.equalsIgnoreCase("thinking")) {
      m = PET_MOOD_CURIOUS;
    }
  }

  const char* msg = (message && message[0]) ? message : "Hermes Agent conectado e sincronizado!";
  normalizeDialogText(msg, _lastAiReply, sizeof(_lastAiReply));
  triggerSay(msg, m, 40, "HERMES AI");
  _hudDirty    = true;
  _footerDirty = true;
}

bool PetMode::anySystemDown(const Settings& s) const {
  if (_zbxProblems > 0) return true;
  for (uint8_t i = 0; i < MAX_WATCH_TARGETS; i++) {
    if (s.pet.targets[i].enabled && _targetStatus[i].checked && !_targetStatus[i].up) {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Hermes Buddy Brain Client (ESP8266 = Body, Hermes = Brain with Memory & Auth)
// Zero cloud API keys stored in firmware!
// ---------------------------------------------------------------------------
static bool isHourActive(uint8_t curH, uint8_t startH, uint8_t endH) {
  if (startH == endH) return true;
  if (startH < endH)  return (curH >= startH && curH <= endH);
  return (curH >= startH || curH <= endH);
}

static void clampThreeScreenLines(const char* in, char* out, size_t outSize) {
  normalizeDialogText(in, out, outSize);
}

static const char* kOfflinePhrases[] = {
  "Rede 100% monitorada aqui no NOC!",
  "Fones sintonizados nos seus pacotes!",
  "Pronta para mais um turno com voce!",
  "Links estaveis e olhos atentos!",
  "Tomando um cafe virtual no plantao!",
  "Sistemas verificados, tudo em ordem!"
};

bool PetMode::triggerNeuralThought(const Settings& s, const char* customPrompt, const char* sourceTag) {
  if (!netConnected() || !s.pet.hermesUrl[0]) {
    // Offline fallback phrase so the buddy body always responds charismatically
    const char* fb = kOfflinePhrases[(millis() / 1000) % 6];
    clampThreeScreenLines(fb, _lastAiReply, sizeof(_lastAiReply));
    triggerSay(_lastAiReply, PET_MOOD_HAPPY, 25, "OFFLINE");
    return false;
  }

  struct tm t;
  char timeStr[12] = "agora";
  if (clockNow(t)) {
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", t.tm_hour, t.tm_min);
    // Respect configured active hours for autonomous thoughts
    if (!customPrompt && !isHourActive((uint8_t)t.tm_hour, s.pet.activeStartHour, s.pet.activeEndHour)) {
      return false;
    }
  }

  int upCount = 0, downCount = 0;
  for (uint8_t i = 0; i < MAX_WATCH_TARGETS; i++) {
    if (s.pet.targets[i].enabled && _targetStatus[i].checked) {
      if (_targetStatus[i].up) upCount++;
      else downCount++;
    }
  }

  String hUrl = String(s.pet.hermesUrl);
  hUrl.trim();
  int schemeIdx = hUrl.indexOf("://");
  int slashIdx  = hUrl.indexOf('/', schemeIdx >= 0 ? schemeIdx + 3 : 0);
  if (slashIdx < 0) {
    hUrl += "/api/buddy/think";
  } else if (slashIdx == (int)hUrl.length() - 1) {
    hUrl += "api/buddy/think";
  }

  JsonDocument reqDoc;
  reqDoc["event"]           = (customPrompt && customPrompt[0]) ? "chat" : "autonomous";
  reqDoc["prompt"]          = (customPrompt && customPrompt[0]) ? customPrompt : "";
  reqDoc["persona"]         = s.pet.hermesPersona;
  reqDoc["time"]            = timeStr;
  reqDoc["tempC"]           = _weatherValid ? _weatherTemp : 26.0f;
  reqDoc["hostsUp"]         = upCount;
  reqDoc["hostsDown"]       = downCount;
  reqDoc["zbxAlerts"]       = _zbxProblems > 0 ? _zbxProblems : 0;
  reqDoc["maxLines"]        = 3;
  reqDoc["maxCharsPerLine"] = 18;

  String body;
  serializeJson(reqDoc, body);
  reqDoc.clear();

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(5000);

  if (http.begin(client, hUrl)) {
    http.addHeader("Content-Type", "application/json");
    if (s.pet.hermesToken[0]) {
      http.addHeader("X-Buddy-Token", s.pet.hermesToken);
      http.addHeader("Authorization", String("Bearer ") + s.pet.hermesToken);
    }
    int code = http.POST(body);
    if (code == 200) {
      String resp = http.getString();
      http.end();
      JsonDocument resDoc;
      if (!deserializeJson(resDoc, resp)) {
        const char* rawReply = resDoc["say"] |
                               (resDoc["reply"] |
                               (resDoc["message"] |
                               (resDoc["output"] |
                               (resDoc["choices"][0]["message"]["content"] | ""))));
        const char* md  = resDoc["mood"] | "cyber";
        const char* act = resDoc["action"] | "";
        if (act && act[0]) {
          triggerAction(petActionFromStr(act), 10);
        }
        if (rawReply && rawReply[0]) {
          clampThreeScreenLines(rawReply, _lastAiReply, sizeof(_lastAiReply));
          _aiThoughtsCount++;
          triggerSay(_lastAiReply, petMoodFromStr(md), 35, sourceTag ? sourceTag : "HERMES AI");
          return true;
        }
      }
    } else {
      http.end();
    }
  }

  // Graceful offline fallback if Hermes Bridge API is temporarily unreachable
  const char* fb = kOfflinePhrases[(millis() / 1000) % 6];
  clampThreeScreenLines(fb, _lastAiReply, sizeof(_lastAiReply));
  triggerSay(_lastAiReply, PET_MOOD_NORMAL, 25, "OFFLINE");
  return false;
}

void PetMode::updateNeuralBrain(const Settings& s) {
  if (!s.pet.hermesEnabled || !netConnected()) return;
  uint32_t now = millis();

  // First autonomous thought 18 seconds after boot once Wi-Fi & weather are ready
  if (_pendingAutoAi && now > 18000) {
    _pendingAutoAi = false;
    _lastAiMs = now;
    triggerNeuralThought(s, nullptr, "HERMES AI");
    return;
  }

  if (s.pet.hermesIntervalMin == 0) return;
  uint32_t intervalMs = (uint32_t)s.pet.hermesIntervalMin * 60000UL;
  if ((now - _lastAiMs) >= intervalMs) {
    _lastAiMs = now;
    triggerNeuralThought(s, nullptr, "HERMES AI");
  }
}

// ---------------------------------------------------------------------------
// Autonomous Weather Fetch (Open-Meteo plain HTTP -> zero TLS heap overhead)
// ---------------------------------------------------------------------------
void PetMode::updateWeather(const Settings& s) {
  if (!netConnected()) return;
  uint32_t now = millis();
  if (_lastWeatherMs != 0 && (now - _lastWeatherMs) < 900000UL) return; // every 15 min
  if (_lastWeatherMs == 0 && now < 3500) return;

  _lastWeatherMs = now | 1;

  char url[160];
  snprintf(url, sizeof(url),
           "http://api.open-meteo.com/v1/forecast?latitude=%.3f&longitude=%.3f&current=temperature_2m,weather_code",
           s.pet.lat, s.pet.lon);

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(3000);
  if (http.begin(client, url)) {
    int code = http.GET();
    if (code == 200) {
      String payload = http.getString();
      JsonDocument doc;
      if (!deserializeJson(doc, payload)) {
        JsonObjectConst cur = doc["current"];
        if (!cur.isNull()) {
          _weatherTemp  = cur["temperature_2m"] | 0.0f;
          _weatherCode  = cur["weather_code"]   | 0;
          _weatherValid = true;
          _hudDirty     = true;
          _footerDirty  = true;
        }
      }
    }
    http.end();
  }
}

// ---------------------------------------------------------------------------
// Autonomous System Guardian (TCP Watchdog + Zabbix JSON-RPC API)
// ---------------------------------------------------------------------------
void PetMode::updateSystems(const Settings& s) {
  if (!netConnected()) return;
  uint32_t now = millis();

  uint32_t stepMs = (uint32_t)max((uint16_t)10, s.pet.sysPollSec) * 250UL;
  if (_lastSysCheckMs == 0 ? (now > 6000) : ((now - _lastSysCheckMs) >= stepMs)) {
    _lastSysCheckMs = now | 1;

    for (uint8_t tries = 0; tries < MAX_WATCH_TARGETS; tries++) {
      uint8_t idx = (_nextTargetIdx + tries) % MAX_WATCH_TARGETS;
      const WatchTarget& wt = s.pet.targets[idx];
      if (!wt.enabled || !wt.host[0]) continue;

      _nextTargetIdx = (idx + 1) % MAX_WATCH_TARGETS;
      bool wasChecked = _targetStatus[idx].checked;
      bool wasUp      = _targetStatus[idx].up;

      WiFiClient tcp;
      tcp.setTimeout(450);
      uint32_t t0 = millis();
      bool ok = tcp.connect(wt.host, wt.port ? wt.port : 80);
      uint16_t lat = (uint16_t)(millis() - t0);
      if (ok) tcp.stop();

      _targetStatus[idx].checked   = true;
      _targetStatus[idx].up        = ok;
      _targetStatus[idx].latencyMs = ok ? lat : 0;

      if (wasChecked && wasUp && !ok) {
        char alertMsg[64];
        snprintf(alertMsg, sizeof(alertMsg), "ALERTA CRITICO: %s ficou OFFLINE!", wt.name[0] ? wt.name : wt.host);
        triggerSay(alertMsg, PET_MOOD_ANGRY, 40, "WATCHDOG");
      } else if (wasChecked && !wasUp && ok) {
        char recMsg[64];
        snprintf(recMsg, sizeof(recMsg), "RECUPERADO: %s voltou UP (%ums)!", wt.name[0] ? wt.name : wt.host, (unsigned)lat);
        triggerAction(PET_ACT_CELEBRATE, 8);
        triggerSay(recMsg, PET_MOOD_EXCITED, 25, "WATCHDOG");
      }
      _hudDirty    = true;
      _footerDirty = true;
      break;
    }
  }

  uint32_t zbxIntervalMs = (uint32_t)max((uint16_t)15, s.pet.sysPollSec) * 1000UL;
  if (s.pet.zbxEnabled && s.pet.zbxUrl[0] && s.pet.zbxToken[0] &&
      (_lastZbxCheckMs == 0 ? (now > 9000) : ((now - _lastZbxCheckMs) >= zbxIntervalMs))) {
    _lastZbxCheckMs = now | 1;

    WiFiClient client;
    HTTPClient http;
    http.setTimeout(2500);
    if (http.begin(client, s.pet.zbxUrl)) {
      http.addHeader("Content-Type", "application/json");
      char reqBody[280];
      snprintf(reqBody, sizeof(reqBody),
               "{\"jsonrpc\":\"2.0\",\"method\":\"problem.get\",\"params\":{\"countOutput\":true,\"recent\":true,\"severity_from\":%u},\"auth\":\"%s\",\"id\":1}",
               (unsigned)s.pet.zbxMinSev, s.pet.zbxToken);
      int code = http.POST((uint8_t*)reqBody, strlen(reqBody));
      if (code == 200) {
        String resp = http.getString();
        JsonDocument doc;
        if (!deserializeJson(doc, resp) && !doc["result"].isNull()) {
          int prev = _zbxProblems;
          int cnt  = doc["result"].is<const char*>() ? atoi(doc["result"].as<const char*>()) : (doc["result"] | 0);
          _zbxProblems = cnt;
          if (cnt > 0 && cnt != prev) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Zabbix NOC: %d incidente(s) ativos agora!", cnt);
            triggerSay(msg, PET_MOOD_ANGRY, 35, "ZABBIX NOC");
          } else if (cnt == 0 && prev > 0) {
            triggerAction(PET_ACT_CELEBRATE, 8);
            triggerSay("Zabbix 100% Limpo! Zero incidentes na rede!", PET_MOOD_EXCITED, 25, "ZABBIX NOC");
          }
          _hudDirty    = true;
          _footerDirty = true;
        }
      }
      http.end();
    }
  }
}

// ---------------------------------------------------------------------------
// Autonomous Mood Brain & Pomodoro
// ---------------------------------------------------------------------------
void PetMode::updateBrain(const Settings& s) {
  uint32_t now = millis();

  if (_lastViewMode != s.pet.viewMode) {
    _lastViewMode = s.pet.viewMode;
    _fullRepaint  = true;
    _hudDirty     = true;
    _footerDirty  = true;
  }

  if (_activeAction != PET_ACT_NONE && _actionUntil != 0 && (int32_t)(now - _actionUntil) >= 0) {
    _activeAction = PET_ACT_NONE;
    _hudDirty     = true;
    _footerDirty  = true;
  }

  if (_pomoActive) {
    if ((int32_t)(now - _pomoEndMs) >= 0) {
      _pomoActive = false;
      triggerAction(PET_ACT_CELEBRATE, 12);
      triggerSay("Pomodoro concluido! Hora de esticar e tomar cafe!", PET_MOOD_EXCITED, 30, "FOCO");
      _hudDirty    = true;
      _footerDirty = true;
    } else {
      uint16_t remS = (uint16_t)pomodoroRemSec();
      if (remS != _lastPomoSec) {
        _lastPomoSec = remS;
        _hudDirty    = true;
      }
    }
  }

  if (_speechActive && (int32_t)(now - _speechUntil) >= 0) {
    _speechActive = false;
    _footerDirty  = true;
  }

  if (_forcedMood != PET_MOOD_AUTO && _forcedUntil != 0 && (int32_t)(now - _forcedUntil) >= 0) {
    _forcedMood = PET_MOOD_AUTO;
    _hudDirty   = true;
  }

  if (_gazeForced && (int32_t)(now - _gazeUntil) >= 0) {
    _gazeForced = false;
  }

  uint8_t prevMood = _activeMood;
  if (_activeAction == PET_ACT_PETTING) {
    _activeMood = PET_MOOD_LOVE;
  } else if (_activeAction == PET_ACT_CELEBRATE) {
    _activeMood = PET_MOOD_EXCITED;
  } else if (_activeAction == PET_ACT_COFFEE) {
    _activeMood = PET_MOOD_HAPPY;
  } else if (_forcedMood != PET_MOOD_AUTO) {
    _activeMood = _forcedMood;
  } else if (anySystemDown(s)) {
    _activeMood = PET_MOOD_ANGRY;
  } else {
    bool shouldSleep = false;
    if (s.pet.autoSleep) {
      if (clockNightActive()) {
        shouldSleep = true;
      } else {
        struct tm t;
        if (clockNow(t)) {
          if (t.tm_hour >= 23 || t.tm_hour < 7) shouldSleep = true;
        }
      }
    }

    if (shouldSleep) {
      _activeMood = PET_MOOD_SLEEP;
    } else if ((int32_t)(now - _nextBrainMs) >= 0) {
      uint32_t baseHold = (s.pet.activity == 0) ? 9000 : (s.pet.activity == 2) ? 3800 : 6000;
      _nextBrainMs = now + baseHold + random(0, 4000);

      uint8_t roll = random(0, 100);
      if (roll < 36)      _activeMood = PET_MOOD_NORMAL;
      else if (roll < 56) _activeMood = PET_MOOD_HAPPY;
      else if (roll < 70) _activeMood = PET_MOOD_CURIOUS;
      else if (roll < 80) _activeMood = PET_MOOD_COOL;
      else if (roll < 88) _activeMood = PET_MOOD_EXCITED;
      else if (roll < 95) _activeMood = PET_MOOD_CYBER;
      else                _activeMood = PET_MOOD_LOVE;
    }
  }

  if (_activeMood != prevMood) {
    _hudDirty    = true;
    _footerDirty = true;
  }

  if (!_speechActive && (int32_t)(now - _nextPhraseMs) >= 0) {
    _nextPhraseMs = now + 10000;
    _phraseIdx    = (_phraseIdx + 1) % 6;
    _dialogPage   = 0;
    _nextDialogPageMs = now + 4000;
    _footerDirty  = true;
  }

  if (s.pet.viewMode == 0 && (int32_t)(now - _nextDialogPageMs) >= 0) {
    _dialogPage++;
    _nextDialogPageMs = now + 4000;
    _footerDirty = true;
  }

  struct tm t;
  if (clockNow(t)) {
    if (t.tm_min != _lastClockMin) {
      _lastClockMin = t.tm_min;
      _hudDirty     = true;
    }
    if (s.pet.viewMode == 1 && t.tm_sec != _lastClockSec) {
      _lastClockSec = t.tm_sec;
      _footerDirty  = true;
    }
  }
}

// ---------------------------------------------------------------------------
// Spring-Damper Eye & Blink Physics
// ---------------------------------------------------------------------------
void PetMode::updatePhysics(const Settings& s) {
  uint32_t now = millis();

  if (!_gazeForced && (int32_t)(now - _nextSaccadeMs) >= 0) {
    if (_activeMood == PET_MOOD_SLEEP) {
      _tgtX = 0;
      _tgtY = 4;
      _nextSaccadeMs = now + 2000;
    } else if (_activeMood == PET_MOOD_CYBER) {
      _tgtX = (_tgtX <= 0) ? 14.0f : -14.0f;
      _tgtY = 0;
      _nextSaccadeMs = now + 1100;
    } else {
      uint32_t minWait = (s.pet.activity == 2) ? 650  : (s.pet.activity == 0) ? 2400 : 1200;
      uint32_t maxWait = (s.pet.activity == 2) ? 1900 : (s.pet.activity == 0) ? 5000 : 3200;
      _nextSaccadeMs = now + random(minWait, maxWait);

      if (random(0, 100) < 38) {
        _tgtX = 0;
        _tgtY = 0;
      } else {
        _tgtX = (float)random(-16, 17);
        _tgtY = (float)random(-8, 9);
      }
    }
  }

  const float stiffness = 0.24f;
  const float damping   = 0.62f;
  _velX = (_velX + (_tgtX - _curX) * stiffness) * damping;
  _velY = (_velY + (_tgtY - _curY) * stiffness) * damping;
  _curX += _velX;
  _curY += _velY;

  if (_activeMood == PET_MOOD_SLEEP) {
    _openL = 0.08f;
    _openR = 0.08f;
    _blinkStage = 0;
    return;
  }

  if (_blinkStage == 0 && (int32_t)(now - _nextBlinkMs) >= 0) {
    _blinkStage   = 1;
    _blinkStageMs = now;
    _doubleBlink  = (random(0, 100) < 28);
  }

  if (_blinkStage == 1) {
    float dt = (float)(now - _blinkStageMs) / 60.0f;
    if (dt >= 1.0f) {
      _openL = 0.05f;
      _openR = 0.05f;
      _blinkStage   = 2;
      _blinkStageMs = now;
    } else {
      _openL = 1.0f - dt * 0.95f;
      _openR = _openL;
    }
  } else if (_blinkStage == 2) {
    if (now - _blinkStageMs >= 35) {
      _blinkStage   = 3;
      _blinkStageMs = now;
    }
  } else if (_blinkStage == 3) {
    float dt = (float)(now - _blinkStageMs) / 80.0f;
    if (dt >= 1.0f) {
      _openL = 1.0f;
      _openR = 1.0f;
      if (_doubleBlink) {
        _doubleBlink  = false;
        _blinkStage   = 1;
        _blinkStageMs = now + 50;
      } else {
        _blinkStage  = 0;
        _nextBlinkMs = now + random(2200, 5600);
      }
    } else {
      _openL = 0.05f + dt * 0.95f;
      _openR = _openL;
    }
  } else {
    if (_activeMood == PET_MOOD_TIRED) {
      _openL = 0.48f;
      _openR = 0.48f;
    } else if (_activeMood == PET_MOOD_CURIOUS) {
      _openL = 0.85f;
      _openR = 1.08f;
    } else if (_activeMood == PET_MOOD_SURPRISED || _activeMood == PET_MOOD_EXCITED) {
      _openL = 1.12f;
      _openR = 1.12f;
    } else {
      _openL = 1.0f;
      _openR = 1.0f;
    }
  }
}

// ---------------------------------------------------------------------------
// Top HUD Bar (y = 0..22): Name / Pomodoro | NOC Badge | Weather | Clock
// ---------------------------------------------------------------------------
void PetMode::drawHud(const Settings& s) {
  auto* g = gfxDev();
  if (!s.pet.showHud) {
    g->fillRect(0, 0, 240, 23, 0x0000);
    return;
  }

  uint16_t hudBg   = gfxTint(0x0842);
  uint16_t rawEye  = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t accent  = gfxTint(rawEye);
  uint16_t lineCol = gfxTint(scaleColor565(rawEye, 45));

  g->fillRect(0, 0, 240, 22, hudBg);
  g->drawFastHLine(0, 22, 240, lineCol);

  bool alert = anySystemDown(s);
  uint16_t dotCol = alert ? gfxTint(0xF800) : accent;
  g->fillCircle(10, 11, 3, dotCol);

  g->setTextSize(1);
  if (_pomoActive) {
    uint32_t rem = pomodoroRemSec();
    char pbuf[18];
    snprintf(pbuf, sizeof(pbuf), "FOCO %02u:%02u", (unsigned)(rem / 60), (unsigned)(rem % 60));
    g->setTextColor(gfxTint(0xFFE0), hudBg);
    g->setCursor(18, 7);
    g->print(pbuf);
  } else {
    g->setTextColor(accent, hudBg);
    g->setCursor(18, 7);
    g->print(s.pet.name[0] ? s.pet.name : "NOC-GIRL");
  }

  int16_t curX = 96;
  g->setCursor(curX, 7);
  if (alert) {
    g->setTextColor(gfxTint(0xF800), hudBg);
    g->print("!NOC ");
  } else {
    g->setTextColor(gfxTint(0x07E0), hudBg);
    g->print("OK ");
  }

  if (_weatherValid) {
    char wbuf[12];
    snprintf(wbuf, sizeof(wbuf), "%.0fC", _weatherTemp);
    g->setTextColor(gfxTint(0xFFE0), hudBg);
    g->print(wbuf);
  }

  struct tm t;
  char cbuf[10];
  if (clockNow(t)) {
    snprintf(cbuf, sizeof(cbuf), "%02d:%02d", t.tm_hour, t.tm_min);
  } else {
    snprintf(cbuf, sizeof(cbuf), "--:--");
  }
  g->setTextColor(gfxTint(0xFFFF), hudBg);
  g->setCursor(204, 7);
  g->print(cbuf);
}

static inline bool inHeart(int dx, int dy, int hw, int hh) {
  if (hw < 4 || hh < 4) return false;
  int x = (dx * 32) / hw;
  int y = (dy * 32) / hh;
  if (abs(x) > 34 || abs(y) > 34) return false;
  if (y < -4) {
    int lx = abs(x) - 15;
    int ly = y + 12;
    return (lx * lx + ly * ly) <= 240;
  }
  int maxAx = (28 - y) * 31 / 32;
  return (y <= 28) && (abs(x) <= maxAx);
}

static inline bool inStar(int dx, int dy, int r) {
  int ax = abs(dx), ay = abs(dy);
  if (ax > r || ay > r) return false;
  return (ax * 3 + ay <= r * 2) || (ay * 3 + ax <= r * 2) || (ax * ax + ay * ay <= (r * r) / 4);
}

// ---------------------------------------------------------------------------
// MODEL 3: NOC CYBER GIRL — CLOSE-UP ANIME (SUPER EXPRESSIVA)
// Flicker-Free Scanline Renderer (Full 240px width, 30 FPS)
// ---------------------------------------------------------------------------
void PetMode::drawFaceRegion(const Settings& s, int16_t areaY, int16_t areaH) {
  auto* g = gfxDev();
  const int16_t areaW = 240;
  const bool compact  = (areaH < 120);

  uint16_t rawNeon    = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t colNeonDim = gfxTint(scaleColor565(rawNeon, 45));

  // -------------------------------------------------------------------------
  // Mandatory Cyber-Anime Blue Buddy Fixed Palette (RGB565)
  // -------------------------------------------------------------------------
  uint16_t colBlack        = gfxTint(0x0000); // Deep near-black background (#000000)
  uint16_t colNavy         = gfxTint(0x0865); // Dark navy for shadows, depth & suit (#0a1228)
  uint16_t colSapphire     = gfxTint(0x1A7B); // Royal sapphire blue for hair & iris (#1d4ed8)
  uint16_t colElectricBlue = gfxTint(0x1BF7); // Luminous electric blue highlights (#1a7fff)
  uint16_t colCyan         = gfxTint(0x07FF); // Vibrant glowing neon cyan (#00ffff)
  uint16_t colViolet       = gfxTint(0x497B); // Subtle violet-blue reflection (#4c38d8)

  uint16_t colHeadbandDark  = gfxTint(0x0926); // Modern dark headphones arch (#0c1830)

  // Natural Anime Features
  uint16_t colSkin          = gfxTint(0xFED6); // Warm porcelain anime skin (#fde2d4)
  uint16_t colSkinShad      = gfxTint(0xEDA7); // Soft contour shadow (#e0b4a4)
  uint16_t colBlush         = gfxTint(0xFA94); // Rosy pink anime blush (#f77b94)
  uint16_t colLash          = gfxTint(0x0842); // Deep obsidian eyeliner & pupil (#080d1a)
  uint16_t colWhite         = gfxTint(0xFFFF); // Pure white catchlights & shine (#ffffff)
  uint16_t colSuit          = gfxTint(0x08A6); // Cyber suit navy (#0d1936)
  uint16_t colLipLine       = gfxTint(0x9188); // Subtle anime lip contour
  uint16_t colLipGloss      = gfxTint(0xF38E); // Soft natural lip gloss (#f07185)
  uint16_t colSclera        = gfxTint(0xF7FF); // Soft crisp blue-white sclera (#eef8ff)
  uint16_t colIrisTop       = gfxTint(0x090A); // Deep navy upper iris (#09152b)
  uint16_t colIrisMid       = gfxTint(0x1A7B); // Sapphire blue mid iris
  uint16_t colIrisBot       = gfxTint(0x07BF); // Luminous cyan lower iris (#00ddff)
  uint16_t colRedAlert      = gfxTint(0xF8A4); // Crimson red alert LED

  bool alert = anySystemDown(s);

  // Subtle breathing & head movement following gaze
  float breath = sinf((float)_animTick * 0.09f) * 2.0f;
  int gx = (int)roundf(_curX);
  int gy = (int)roundf(_curY + breath);

  // Head center
  int hx = 120 + (gx / 4);
  int hy = (compact ? 48 : 61) + (gy / 3);
  int scalePct = compact ? 72 : 92;

  // Eye centers & dimensions
  int eyeSep = (26 * scalePct) / 100;
  int lx = hx - eyeSep;
  int rx = hx + eyeSep;
  int ey = hy + (4 * scalePct) / 100;

  int eyeHW = (16 * scalePct) / 100;
  int baseEyeHH = (17 * scalePct) / 100;
  int lhh = max(1, (int)roundf(baseEyeHH * _openL));
  int rhh = max(1, (int)roundf(baseEyeHH * _openR));

  // Iris gaze offset inside the eye
  int igx = constrain(gx / 3, -5, 5);
  int igy = constrain((int)roundf(_curY) / 3, -4, 4);
  int irisHW = (_activeMood == PET_MOOD_ANGRY || _activeMood == PET_MOOD_SURPRISED)
               ? ((10 * scalePct) / 100) : ((12 * scalePct) / 100);
  int irisHH = (_activeMood == PET_MOOD_ANGRY || _activeMood == PET_MOOD_SURPRISED)
               ? ((12 * scalePct) / 100) : ((15 * scalePct) / 100);

  // Mouth position & lip-sync openness
  int mouthX = hx;
  int mouthY = hy + (33 * scalePct) / 100;
  bool talking = _speechActive || (_activeAction == PET_ACT_COFFEE);
  int talkH = talking ? (2 + (int)(fabsf(sinf((float)_animTick * 0.55f)) * 6.0f)) : 0;

  // Headphone earcup centers
  int cupOffset = (58 * scalePct) / 100;
  int lCupX = hx - cupOffset;
  int rCupX = hx + cupOffset;
  int cupY  = hy - (2 * scalePct) / 100;
  int cupRX = (12 * scalePct) / 100;
  int cupRY = (22 * scalePct) / 100;

  bool pulseBright = ((_animTick / 4) & 1) == 0;
  uint16_t colRingActive = alert ? (pulseBright ? colRedAlert : colNeonDim) : colCyan;

  uint8_t acc = s.pet.accessory;
  if (_activeMood == PET_MOOD_COOL) acc = 1; // Sunglasses

  uint16_t rowBuf[areaW];

  for (int y = 0; y < areaH; y++) {
    // -----------------------------------------------------------------------
    // 1. NEAR-BLACK BACKGROUND (High contrast, pitch dark)
    // -----------------------------------------------------------------------
    for (int x = 0; x < areaW; x++) {
      rowBuf[x] = colBlack;
    }

    int dy = ((y - hy) * 100) / scalePct;

    // -----------------------------------------------------------------------
    // 2. BLUE BOB HAIR BACK & SIDES (Chanel Silhouette)
    // -----------------------------------------------------------------------
    if (dy >= -68 && dy <= 66) {
      int hairHW = 0;
      if (dy < -18) {
        int d = dy + 18;
        int r2 = 50 * 50 - d * d;
        hairHW = (r2 > 0) ? (22 + (int)sqrtf((float)r2)) : 0;
      } else if (dy <= 22) {
        hairHW = 72 + ((dy + 18) * 8) / 40;
      } else if (dy <= 46) {
        int t = dy - 22;
        hairHW = 80 - (t * t * 14) / (24 * 24);
      } else {
        hairHW = 66 - ((dy - 46) * 26) / 20;
      }

      if (hairHW > 0) {
        int realHW = (hairHW * scalePct) / 100;
        int x0 = max(0, hx - realHW - 1);
        int x1 = min(areaW - 1, hx + realHW + 1);
        for (int x = x0; x <= x1; x++) {
          int adx = abs(x - hx);
          int normAdx = (adx * 100) / scalePct;

          if (dy >= 45 && normAdx <= 22) {
            continue; // Open space for neck & cyber collar
          }

          if (adx >= realHW) {
            rowBuf[x] = colNavy;
          } else {
            uint16_t pix = colSapphire;
            if (normAdx >= 64) {
              pix = ((adx + dy) % 3 == 0) ? colElectricBlue : colSapphire;
            } else if (dy >= 30 && normAdx <= 42) {
              pix = colNavy;
            } else {
              pix = colSapphire;
            }

            // Violet-blue undertone sheen
            if (normAdx >= 54 && dy >= 8 && dy <= 40 && ((x + y) % 6 == 0)) {
              pix = colViolet;
            }

            // Crown cyan curved highlight arc
            if (dy >= -54 && dy <= -42) {
              int crownY = -48 + (normAdx * normAdx) / 140;
              if (abs(dy - crownY) <= 2 && normAdx <= 45) {
                pix = (abs(dy - crownY) <= 1) ? colCyan : colElectricBlue;
              }
            }

            rowBuf[x] = pix;
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 3. NECK & CYBER SUIT (High Collar with Cyan Piping & Chest V-Chevron)
    // -----------------------------------------------------------------------
    if (dy >= 44) {
      int neckHW = (14 * scalePct) / 100;
      if (dy <= 58) {
        for (int x = hx - neckHW; x <= hx + neckHW; x++) {
          if (x >= 0 && x < areaW) {
            rowBuf[x] = (dy <= 50) ? colSkinShad : colSkin;
          }
        }
      }

      if (dy >= 52) {
        int shHW = ((20 + (dy - 52) * 4) * scalePct) / 100;
        int vOpeningHW = max(0, ((dy - 52) * 14) / 28);

        for (int x = max(0, hx - shHW); x <= min(areaW - 1, hx + shHW); x++) {
          int adx = abs(x - hx);
          int normAdx = (adx * 100) / scalePct;

          if (dy <= 66 && normAdx <= vOpeningHW) {
            if (abs(normAdx - vOpeningHW) <= 2) {
              rowBuf[x] = colCyan; // Collar glowing cyan piping
            } else if (dy <= 56) {
              rowBuf[x] = colSkinShad;
            } else {
              rowBuf[x] = colNavy; // Dark undersuit
            }
            continue;
          }

          if (dy <= 64 && normAdx <= 34) {
            if (dy == 52 || normAdx >= 32) {
              rowBuf[x] = colCyan; // Outer collar piping
            } else {
              rowBuf[x] = colSuit;
            }
            continue;
          }

          uint16_t pix = colSuit;
          if (adx >= shHW - 2) {
            pix = colNavy;
          } else if (dy >= 64 && dy <= 80) {
            int vCenter = ((dy - 64) * 22) / 16;
            if (abs(normAdx - vCenter) <= 2) {
              pix = colCyan; // Glowing cyan chest V-chevron
            } else if (normAdx < vCenter) {
              pix = colNavy;
            } else if ((normAdx == 32 || normAdx == 33) && dy >= 68) {
              pix = colCyan;
            }
          }

          rowBuf[x] = pix;
        }
      }
    }

    // -----------------------------------------------------------------------
    // 4. NATURAL ANIME FACE JAWLINE CONTOUR & BLUSH
    // -----------------------------------------------------------------------
    if (dy >= -44 && dy <= 48) {
      int faceHW = 0;
      if (dy < 6) {
        faceHW = 51;
      } else {
        int t = dy - 6;
        faceHW = 51 - (t * t * 37) / (42 * 42) - (t * 8) / 42;
      }

      if (faceHW > 0) {
        int realFHW = (faceHW * scalePct) / 100;
        for (int x = hx - realFHW; x <= hx + realFHW; x++) {
          if (x >= 0 && x < areaW) {
            int adx = abs(x - hx);
            if (adx >= realFHW - 1 || (dy >= 45 && adx <= 8)) {
              rowBuf[x] = colSkinShad;
            } else {
              rowBuf[x] = colSkin;
            }
          }
        }
      }

      // Cheeks & Soft Anime Blush (dy = 16..24)
      if (s.pet.showBlush && dy >= 16 && dy <= 24) {
        int bdy = dy - 20;
        for (int side = 0; side < 2; side++) {
          int bx = (side == 0) ? (hx - (30 * scalePct) / 100) : (hx + (30 * scalePct) / 100);
          int bhw = (11 * scalePct) / 100;
          for (int x = bx - bhw; x <= bx + bhw; x++) {
            int bdx = x - bx;
            if ((bdx * bdx) / 14 + (bdy * bdy) <= 6) {
              if (((x + y) % 3 == 0) || _activeMood == PET_MOOD_LOVE || _activeMood == PET_MOOD_HAPPY) {
                rowBuf[x] = colBlush;
              } else {
                rowBuf[x] = colSkinShad;
              }
            }
          }
        }
      }

      // Delicate Anime Nose (dy = 18..20)
      if (dy >= 18 && dy <= 20) {
        if (dy == 20) {
          rowBuf[hx]     = colSkinShad;
          rowBuf[hx - 1] = colSkinShad;
        } else if (dy == 18) {
          rowBuf[hx - 1] = colWhite; // Nose specular catchlight
        }
      }
    }

    // -----------------------------------------------------------------------
    // 5. LARGE SAPPHIRE ANIME EYES & EYEBROWS
    // -----------------------------------------------------------------------
    int edy = y - ey;
    if (abs(edy) <= baseEyeHH + 10) {
      for (int side = 0; side < 2; side++) {
        int cx = (side == 0) ? lx : rx;
        int hh = (side == 0) ? lhh : rhh;
        int dir = (side == 0) ? -1 : 1;

        if (hh <= 3 || _activeMood == PET_MOOD_SLEEP) {
          for (int x = cx - eyeHW; x <= cx + eyeHW; x++) {
            int edx = x - cx;
            int curveY = 3 - (edx * edx) / 48;
            if (abs(edy - curveY) <= 1) rowBuf[x] = colLash;
          }
          continue;
        }

        if (_activeMood == PET_MOOD_HAPPY) {
          for (int x = cx - eyeHW - 3; x <= cx + eyeHW + 3; x++) {
            int edx = x - cx;
            int archY = -4 + (edx * edx) / 28;
            if (abs(edy - archY) <= 2 && abs(edx) <= eyeHW + 2) {
              rowBuf[x] = colLash;
            }
          }
          continue;
        }

        if (abs(edy) <= hh + 2) {
          for (int x = cx - eyeHW - 4; x <= cx + eyeHW + 4; x++) {
            int edx = x - cx;
            if (edx * dir >= eyeHW - 2 && edx * dir <= eyeHW + 4) {
              int wingY = -hh + 3 - ((edx * dir - (eyeHW - 2)) * 3) / 2;
              if (abs(edy - wingY) <= 1) {
                rowBuf[x] = colLash;
                continue;
              }
            }

            int normDist = (edx * edx * 100) / max(1, eyeHW * eyeHW) +
                           (edy * edy * 100) / max(1, hh * hh);
            if (normDist <= 100) {
              int topArchY = -hh + (edx * edx) / 40;
              if (edy <= topArchY + 3) {
                rowBuf[x] = colLash;
                continue;
              }
              if (normDist >= 90 && edy > hh / 2) {
                rowBuf[x] = colSkinShad;
                continue;
              }

              uint16_t pix = colSclera;
              int idx = edx - igx;
              int idy = edy - igy;
              int irisDist = (idx * idx * 100) / max(1, irisHW * irisHW) +
                             (idy * idy * 100) / max(1, irisHH * irisHH);

              if (irisDist <= 100) {
                if (irisDist >= 84) {
                  pix = colNavy; // Limbal ring
                } else {
                  if (idy < -irisHH / 4)      pix = colIrisTop;
                  else if (idy < irisHH / 3)  pix = colIrisMid;
                  else                        pix = colIrisBot;

                  if (_activeMood == PET_MOOD_LOVE && inHeart(idx, idy, irisHW - 2, irisHH - 2)) {
                    pix = colBlush;
                  } else if (_activeMood == PET_MOOD_EXCITED && inStar(idx, idy, irisHW - 2)) {
                    pix = gfxTint(0xFFE0);
                  } else if (irisDist <= 24) {
                    pix = colLash; // Deep obsidian pupil
                  } else if ((_activeMood == PET_MOOD_CYBER || _activeMood == PET_MOOD_COOL) && irisDist >= 30 && irisDist <= 46) {
                    pix = colCyan; // Concentric glowing cyber data ring
                  }
                }

                // Two Catchlights (Top-Left big + Lower-Right sparkle)
                int c1x = idx + irisHW / 3;
                int c1y = idy + irisHH / 3;
                int c2x = idx - irisHW / 3;
                int c2y = idy - irisHH / 3;
                if (c1x * c1x + c1y * c1y <= 11) {
                  pix = colWhite;
                } else if (c2x * c2x + c2y * c2y <= 4) {
                  pix = colWhite;
                }
              }

              rowBuf[x] = pix;
            }
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 6. COOL CYBER SUNGLASSES OVERLAY (acc == 1 or PET_MOOD_COOL)
    // -----------------------------------------------------------------------
    if (acc == 1 && abs(edy) <= 14) {
      if (edy >= -12 && edy <= -9) {
        for (int x = lx - eyeHW - 4; x <= rx + eyeHW + 4; x++) rowBuf[x] = colCyan;
      }
      for (int side = 0; side < 2; side++) {
        int cx = (side == 0) ? lx : rx;
        int lhw = eyeHW + 4;
        for (int x = cx - lhw; x <= cx + lhw; x++) {
          int dx = x - cx;
          if (edy >= -11 && edy <= 11) {
            bool border = (abs(dx) >= lhw - 1 || edy <= -10 || edy >= 10);
            int glint = dx + edy;
            if (border) rowBuf[x] = colCyan;
            else if ((glint >= -8 && glint <= -4) || (glint >= 2 && glint <= 4)) rowBuf[x] = colWhite;
            else rowBuf[x] = colNavy;
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 7. ELEGANT ANIME MOUTH WITH LIP-SYNC
    // -----------------------------------------------------------------------
    int mdy = y - mouthY;
    if (s.pet.showMouth && abs(mdy) <= 10) {
      int mhw = (8 * scalePct) / 100;
      if (talking && talkH >= 3) {
        for (int x = mouthX - mhw; x <= mouthX + mhw; x++) {
          int mdx = x - mouthX;
          if (mdy >= -2 && mdy <= talkH + 1 && (mdx * mdx) / 12 + (mdy * mdy) / max(2, talkH) <= 6) {
            if (mdy == -2 || mdy == -1) rowBuf[x] = colWhite;
            else if (mdy >= talkH - 1)  rowBuf[x] = colBlush;
            else                        rowBuf[x] = colLash;
          }
        }
      } else if (_activeMood == PET_MOOD_HAPPY || _activeMood == PET_MOOD_EXCITED || _activeMood == PET_MOOD_LOVE) {
        for (int x = mouthX - mhw; x <= mouthX + mhw; x++) {
          int mdx = x - mouthX;
          int botY = 5 - (mdx * mdx) / 14;
          if (mdy >= 0 && mdy <= botY) {
            rowBuf[x] = (mdy == 0 || mdy == botY) ? colLash : colBlush;
          }
        }
      } else if (_activeMood == PET_MOOD_ANGRY || _activeMood == PET_MOOD_SURPRISED) {
        for (int x = mouthX - 5; x <= mouthX + 5; x++) {
          int mdx = x - mouthX;
          int d2 = mdx * mdx + mdy * mdy;
          if (d2 <= 20) rowBuf[x] = (d2 >= 12) ? colLash : colRedAlert;
        }
      } else {
        // Delicate anime smile (contour + gloss)
        for (int x = mouthX - 6; x <= mouthX + 6; x++) {
          int mdx = x - mouthX;
          int curveY = (mdx * mdx) / 24;
          if (mdy == curveY) {
            rowBuf[x] = colLipLine;
          } else if (mdy == curveY + 1 && abs(mdx) <= 3) {
            rowBuf[x] = colLipGloss;
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 8. BLUE BANGS & FACE-FRAMING STRANDS (Forehead Layer)
    // -----------------------------------------------------------------------
    if (dy >= -64 && dy <= 38) {
      int bangHW = (62 * scalePct) / 100;
      for (int x = max(0, hx - bangHW); x <= min(areaW - 1, hx + bangHW); x++) {
        int bdx = ((x - hx) * 100) / scalePct;
        int topLim = -60 + (bdx * bdx) / 120;

        int botLim = -16;
        if (bdx >= -28 && bdx < -14) {
          botLim = -30 + (abs(bdx + 20) * 12) / 10;
        } else if (bdx >= -14 && bdx <= -2) {
          botLim = -24 + ((bdx + 14) * 14) / 12;
        } else if (bdx > -2 && bdx <= 14) {
          botLim = -18 + (abs(bdx - 6) * 10) / 8;
        } else if (bdx > 14 && bdx <= 28) {
          botLim = -22 + ((bdx - 14) * 8) / 14;
        } else if (bdx < -28) {
          botLim = 18 + ((abs(bdx) - 28) * 12) / 10;
        } else if (bdx > 28) {
          botLim = 16 + ((bdx - 28) * 12) / 10;
        }

        if (dy >= topLim && dy <= botLim) {
          uint16_t pix = colSapphire;

          // Flowing cyan highlight band across bangs
          if (dy >= -36 && dy <= -20) {
            int hCenter = -28 - bdx / 5;
            if (abs(dy - hCenter) <= 2) {
              pix = (abs(dy - hCenter) <= 1) ? colCyan : colElectricBlue;
            }
          } else if (abs(bdx) >= 30 && dy >= -8) {
            if (abs(bdx) == 32 || ((x + y) % 4 == 0)) {
              pix = colElectricBlue;
            }
          }

          if (dy == botLim) {
            pix = colNavy;
          }

          rowBuf[x] = pix;
        }
      }
    }

    // Eyebrows overlay across fringe
    if (dy >= -17 && dy <= -11) {
      for (int side = 0; side < 2; side++) {
        int cx = (side == 0) ? lx : rx;
        int dir = (side == 0) ? -1 : 1;
        for (int x = cx - eyeHW + 2; x <= cx + eyeHW - 2; x++) {
          int edx = x - cx;
          int browY = -14;
          if (_activeMood == PET_MOOD_ANGRY) {
            browY = -12 - (edx * dir) / 4;
          } else if (_activeMood == PET_MOOD_CURIOUS && side == 1) {
            browY = -16 + (edx * edx) / 55;
          } else if (_activeMood == PET_MOOD_HAPPY || _activeMood == PET_MOOD_EXCITED) {
            browY = -15 + (edx * edx) / 50;
          } else {
            browY = -14 + (edx * edx) / 55;
          }
          if (abs(dy - browY) <= 1) rowBuf[x] = colNavy;
        }
      }
    }

    // -----------------------------------------------------------------------
    // 9. MODERN HEADPHONES (Dark Headband + Cyan Neon Strip + Earcups)
    // -----------------------------------------------------------------------
    if (dy >= -68 && dy <= -40) {
      int hbHW = (62 * scalePct) / 100;
      for (int x = max(0, hx - hbHW); x <= min(areaW - 1, hx + hbHW); x++) {
        int bdx = ((x - hx) * 100) / scalePct;
        int archY = -64 + (bdx * bdx) / 165;
        if (abs(dy - archY) <= 3) {
          if (abs(dy - archY) <= 1 && abs(bdx) <= 46) {
            rowBuf[x] = colCyan; // Cyan Neon LED Light Strip
          } else {
            rowBuf[x] = colHeadbandDark;
          }
        }
      }
    }

    // Left & Right Over-Ear Headphone Earcups
    int cdy = y - cupY;
    if (abs(cdy) <= cupRY + 2) {
      for (int side = 0; side < 2; side++) {
        int cx = (side == 0) ? lCupX : rCupX;
        int dir = (side == 0) ? -1 : 1;
        for (int x = max(0, cx - cupRX - 2); x <= min(areaW - 1, cx + cupRX + 2); x++) {
          int cdx = x - cx;
          int ell = (cdx * cdx * 100) / max(1, cupRX * cupRX) +
                    (cdy * cdy * 100) / max(1, cupRY * cupRY);
          if (ell <= 100) {
            if (ell >= 56 && ell <= 86) {
              rowBuf[x] = colRingActive;
            } else if (ell > 86) {
              rowBuf[x] = colNavy;
            } else {
              if (cdy >= 4 && cdy <= 10 && cdx * dir >= 2 && cdx * dir <= 6) {
                rowBuf[x] = colCyan;
              } else {
                rowBuf[x] = colHeadbandDark;
              }
            }
          }
        }
      }
    }

    // Headphone cable glowing downwards from right earcup
    if (y > cupY + cupRY && y < areaH) {
      int cableX = rCupX + ((y - (cupY + cupRY)) / 5);
      if (cableX >= 0 && cableX + 1 < areaW) {
        rowBuf[cableX]     = colRingActive;
        rowBuf[cableX + 1] = colNeonDim;
      }
    }

    g->draw16bitRGBBitmap(0, areaY + y, rowBuf, areaW, 1);
  }
}

// ---------------------------------------------------------------------------
// View 0 Bottom Panel: Large Cyberpunk Dialogue Box (TextSize 2!) (y = 160..239)
// ---------------------------------------------------------------------------
enum AccentMark : uint8_t {
  ACCENT_NONE,
  ACCENT_ACUTE,
  ACCENT_GRAVE,
  ACCENT_CIRCUMFLEX,
  ACCENT_TILDE,
  ACCENT_DIAERESIS,
  ACCENT_CEDILLA
};

struct PortugueseGlyph {
  char base;
  AccentMark accent;
  uint8_t bytes;
};

static PortugueseGlyph decodePortugueseGlyph(const char* p) {
  if (!p || !p[0]) return {'?', ACCENT_NONE, 0};
  uint8_t c = (uint8_t)p[0];
  if (c < 0x80) return {(char)c, ACCENT_NONE, 1};
  if (c != 0xC3 || !p[1]) return {'?', ACCENT_NONE, 1};

  switch ((uint8_t)p[1]) {
    case 0x80: return {'A', ACCENT_GRAVE, 2};
    case 0x81: return {'A', ACCENT_ACUTE, 2};
    case 0x82: return {'A', ACCENT_CIRCUMFLEX, 2};
    case 0x83: return {'A', ACCENT_TILDE, 2};
    case 0x84: return {'A', ACCENT_DIAERESIS, 2};
    case 0x87: return {'C', ACCENT_CEDILLA, 2};
    case 0x88: return {'E', ACCENT_GRAVE, 2};
    case 0x89: return {'E', ACCENT_ACUTE, 2};
    case 0x8A: return {'E', ACCENT_CIRCUMFLEX, 2};
    case 0x8B: return {'E', ACCENT_DIAERESIS, 2};
    case 0x8C: return {'I', ACCENT_GRAVE, 2};
    case 0x8D: return {'I', ACCENT_ACUTE, 2};
    case 0x8E: return {'I', ACCENT_CIRCUMFLEX, 2};
    case 0x8F: return {'I', ACCENT_DIAERESIS, 2};
    case 0x92: return {'O', ACCENT_GRAVE, 2};
    case 0x93: return {'O', ACCENT_ACUTE, 2};
    case 0x94: return {'O', ACCENT_CIRCUMFLEX, 2};
    case 0x95: return {'O', ACCENT_TILDE, 2};
    case 0x96: return {'O', ACCENT_DIAERESIS, 2};
    case 0x99: return {'U', ACCENT_GRAVE, 2};
    case 0x9A: return {'U', ACCENT_ACUTE, 2};
    case 0x9B: return {'U', ACCENT_CIRCUMFLEX, 2};
    case 0x9C: return {'U', ACCENT_DIAERESIS, 2};
    case 0xA0: return {'a', ACCENT_GRAVE, 2};
    case 0xA1: return {'a', ACCENT_ACUTE, 2};
    case 0xA2: return {'a', ACCENT_CIRCUMFLEX, 2};
    case 0xA3: return {'a', ACCENT_TILDE, 2};
    case 0xA4: return {'a', ACCENT_DIAERESIS, 2};
    case 0xA7: return {'c', ACCENT_CEDILLA, 2};
    case 0xA8: return {'e', ACCENT_GRAVE, 2};
    case 0xA9: return {'e', ACCENT_ACUTE, 2};
    case 0xAA: return {'e', ACCENT_CIRCUMFLEX, 2};
    case 0xAB: return {'e', ACCENT_DIAERESIS, 2};
    case 0xAC: return {'i', ACCENT_GRAVE, 2};
    case 0xAD: return {'i', ACCENT_ACUTE, 2};
    case 0xAE: return {'i', ACCENT_CIRCUMFLEX, 2};
    case 0xAF: return {'i', ACCENT_DIAERESIS, 2};
    case 0xB2: return {'o', ACCENT_GRAVE, 2};
    case 0xB3: return {'o', ACCENT_ACUTE, 2};
    case 0xB4: return {'o', ACCENT_CIRCUMFLEX, 2};
    case 0xB5: return {'o', ACCENT_TILDE, 2};
    case 0xB6: return {'o', ACCENT_DIAERESIS, 2};
    case 0xB9: return {'u', ACCENT_GRAVE, 2};
    case 0xBA: return {'u', ACCENT_ACUTE, 2};
    case 0xBB: return {'u', ACCENT_CIRCUMFLEX, 2};
    case 0xBC: return {'u', ACCENT_DIAERESIS, 2};
    default:   return {'?', ACCENT_NONE, 2};
  }
}

static void drawAccent(Arduino_GFX* g, int16_t x, int16_t y, AccentMark mark,
                       uint8_t textSize, uint16_t color) {
  int16_t center = (textSize == 2) ? 5 : 2;
  switch (mark) {
    case ACCENT_ACUTE:
      g->drawPixel(x + center + 1, y, color); g->drawPixel(x + center, y + 1, color); break;
    case ACCENT_GRAVE:
      g->drawPixel(x + center - 1, y, color); g->drawPixel(x + center, y + 1, color); break;
    case ACCENT_CIRCUMFLEX:
      g->drawPixel(x + center - 1, y + 1, color); g->drawPixel(x + center, y, color);
      g->drawPixel(x + center + 1, y + 1, color); break;
    case ACCENT_TILDE:
      g->drawPixel(x + center - 2, y + 1, color); g->drawPixel(x + center - 1, y, color);
      g->drawPixel(x + center, y, color); g->drawPixel(x + center + 1, y + 1, color); break;
    case ACCENT_DIAERESIS:
      g->drawPixel(x + center - 2, y, color); g->drawPixel(x + center + 2, y, color); break;
    case ACCENT_CEDILLA: {
      int16_t cy = y + ((textSize == 2) ? 15 : 7);
      g->drawPixel(x + center, cy, color); g->drawPixel(x + center - 1, cy + 1, color); break;
    }
    default: break;
  }
}

static uint8_t countPortugueseGlyphs(const char* text, uint8_t maxGlyphs) {
  uint8_t count = 0;
  const char* p = text;
  while (p && *p && count < maxGlyphs) {
    PortugueseGlyph glyph = decodePortugueseGlyph(p);
    p += glyph.bytes ? glyph.bytes : 1;
    count++;
  }
  return count;
}

static uint8_t drawPortugueseText(Arduino_GFX* g, int16_t x0, int16_t y0, const char* text,
                                  const char* end, uint8_t maxGlyphs, uint8_t textSize,
                                  uint16_t textColor, uint16_t bgColor) {
  if (!text) return 0;
  g->setTextSize(textSize);
  g->setTextColor(textColor, bgColor);
  const char* p = text;
  uint8_t col = 0;
  int16_t cellW = 6 * textSize;
  int16_t baseOffset = (textSize == 2) ? 2 : 1;
  while (*p && (!end || p < end) && col < maxGlyphs) {
    PortugueseGlyph glyph = decodePortugueseGlyph(p);
    int16_t x = x0 + col * cellW;
    g->setCursor(x, y0 + baseOffset);
    g->write((uint8_t)glyph.base);
    drawAccent(g, x, y0, glyph.accent, textSize, textColor);
    p += glyph.bytes ? glyph.bytes : 1;
    col++;
  }
  return col;
}

static uint8_t printWrappedBig(Arduino_GFX* g, int16_t x0, int16_t y0, const char* text,
                               uint8_t requestedPage, uint16_t textColor, uint16_t bgColor) {
  if (!text || !text[0]) return 0;
  struct WrappedLine { const char* start; const char* end; };
  WrappedLine lines[32];
  uint8_t lineCount = 0;
  const char* p = text;

  while (*p && lineCount < 32) {
    while (*p == ' ') p++;
    if (!*p) break;
    const char* start = p;
    const char* q = p;
    const char* lastSpace = nullptr;
    uint8_t cols = 0;
    uint8_t spaceCol = 0;
    while (*q && cols < 18) {
      if (*q == ' ') { lastSpace = q; spaceCol = cols; }
      PortugueseGlyph glyph = decodePortugueseGlyph(q);
      q += glyph.bytes ? glyph.bytes : 1;
      cols++;
    }
    const char* end = q;
    if (*q && lastSpace && spaceCol >= 8) {
      end = lastSpace;
      q = lastSpace + 1;
    }
    lines[lineCount++] = {start, end};
    p = q;
  }

  uint8_t pageCount = (lineCount + 2) / 3;
  uint8_t page = pageCount ? requestedPage % pageCount : 0;
  uint8_t firstLine = page * 3;
  g->setTextSize(2);
  g->setTextColor(textColor, bgColor);

  for (uint8_t row = 0; row < 3 && firstLine + row < lineCount; row++) {
    const WrappedLine& line = lines[firstLine + row];
    int16_t y = y0 + row * 18;
    drawPortugueseText(g, x0, y, line.start, line.end, 18, 2, textColor, bgColor);
  }
  return pageCount;
}

void PetMode::drawBottomPanel(const Settings& s) {
  auto* g = gfxDev();
  g->fillRect(0, 160, 240, 80, 0x0000);
  if (!s.pet.showFooter) return;

  uint16_t rawMain   = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t borderCol = gfxTint(rawMain);
  uint16_t bubbleBg  = gfxTint(0x0842);
  uint16_t textCol   = _speechActive ? gfxTint(0xFFE0) : gfxTint(0xFFFF);

  // Large Cyberpunk Speech Window (y = 161..238, height = 77px, width = 232px)
  g->fillRoundRect(4, 161, 232, 77, 8, bubbleBg);
  g->drawRoundRect(4, 161, 232, 77, 8, borderCol);
  g->drawRoundRect(5, 162, 230, 75, 7, borderCol);

  // Header badge inside speech bubble: [IA NEURAL] / [HERMES BOT] / [ZABBIX NOC]
  const char* srcBadge = _speechActive ? _speechSource : "NOC CYBER GIRL";
  g->setTextSize(1);
  g->setTextColor(borderCol, bubbleBg);
  g->setCursor(11, 166);
  g->print("[");
  g->print(srcBadge);
  g->print("]");

  const char* pName = s.pet.hermesEnabled ? "HERMES" : "OFFLINE";
  int16_t px = 228 - (int16_t)(strlen(pName) * 6);
  g->setTextColor(gfxTint(0x74DF), bubbleBg);
  g->setCursor(px, 166);
  g->print(pName);

  g->drawFastHLine(10, 176, 220, gfxTint(scaleColor565(rawMain, 35)));

  const char* msg = nullptr;
  char autoBuf[68];

  if (_speechActive && _speechText[0]) {
    msg = _speechText;
  } else if (_lastAiReply[0] && (_phraseIdx == 0 || _phraseIdx == 3)) {
    msg = _lastAiReply;
  } else {
    switch (_phraseIdx) {
      case 1:
        if (anySystemDown(s)) {
          snprintf(autoBuf, sizeof(autoBuf), "ALERTA: Queda no NOC! Verifique agora!");
        } else {
          snprintf(autoBuf, sizeof(autoBuf), "Switches Aruba e links WAN 100%% UP!");
        }
        msg = autoBuf;
        break;
      case 2:
        if (_weatherValid) {
          snprintf(autoBuf, sizeof(autoBuf), "Clima: %.1f C | Hermes Bot ativo!", _weatherTemp);
        } else {
          snprintf(autoBuf, sizeof(autoBuf), "Monitorando pacotes com Hermes Bot!");
        }
        msg = autoBuf;
        break;
      case 4:
        snprintf(autoBuf, sizeof(autoBuf), "IP %s | IA: %u pensamentos",
                 netIP().c_str(), (unsigned)_aiThoughtsCount);
        msg = autoBuf;
        break;
      default:
        msg = s.pet.statusText[0] ? s.pet.statusText : "Pronta no NOC! Fale comigo na Web UI.";
        break;
    }
  }

  printWrappedBig(g, 11, 181, msg, _dialogPage, textCol, bubbleBg);
}

// ---------------------------------------------------------------------------
// View 1 Bottom Panel: Big Digital Clock + Date + Speech (y = 128..239)
// ---------------------------------------------------------------------------
void PetMode::drawBigClockPanel(const Settings& s) {
  auto* g = gfxDev();
  uint16_t cardBg    = gfxTint(0x0862);
  uint16_t rawMain   = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t accent    = gfxTint(rawMain);
  uint16_t borderCol = gfxTint(scaleColor565(rawMain, 55));

  g->fillRoundRect(10, 130, 220, 104, 10, cardBg);
  g->drawRoundRect(10, 130, 220, 104, 10, borderCol);

  struct tm t;
  char hhmm[8] = "--:--";
  char ss[6]   = ":--";
  char dateBuf[32] = "Sincronizando NTP...";
  static const char* kDays[] = {"DOM", "SEG", "TER", "QUA", "QUI", "SEX", "SAB"};

  if (clockNow(t)) {
    snprintf(hhmm, sizeof(hhmm), "%02d:%02d", t.tm_hour, t.tm_min);
    snprintf(ss, sizeof(ss), ":%02d", t.tm_sec);
    if (_weatherValid) {
      snprintf(dateBuf, sizeof(dateBuf), "%s %02d/%02d  |  %.1f C",
               kDays[t.tm_wday % 7], t.tm_mday, t.tm_mon + 1, _weatherTemp);
    } else {
      snprintf(dateBuf, sizeof(dateBuf), "%s %02d/%02d/%04d",
               kDays[t.tm_wday % 7], t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    }
  }

  g->setTextSize(4);
  g->setTextColor(gfxTint(0xFFFF), cardBg);
  g->setCursor(32, 144);
  g->print(hhmm);

  g->setTextSize(2);
  g->setTextColor(accent, cardBg);
  g->setCursor(156, 156);
  g->print(ss);

  g->setTextSize(1);
  g->setTextColor(gfxTint(0xFFE0), cardBg);
  int16_t dx = 120 - (int16_t)(strlen(dateBuf) * 3);
  g->setCursor(max((int16_t)18, dx), 188);
  g->print(dateBuf);

  const char* botMsg = _speechActive ? _speechText : (_lastAiReply[0] ? _lastAiReply : "NOC Cyber Girl Online");
  uint8_t botGlyphs = countPortugueseGlyphs(botMsg, 33);
  int16_t mx = 120 - (int16_t)(botGlyphs * 3);
  drawPortugueseText(g, max((int16_t)18, mx), 210, botMsg, nullptr, 33, 1, accent, cardBg);
}

// ---------------------------------------------------------------------------
// View 2 Bottom Panel: NOC System Guardian Matrix (y = 124..239)
// ---------------------------------------------------------------------------
void PetMode::drawNocPanel(const Settings& s) {
  auto* g = gfxDev();
  uint16_t cardBg    = gfxTint(0x0862);
  uint16_t rawMain   = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t borderCol = gfxTint(scaleColor565(rawMain, 55));

  g->fillRoundRect(8, 124, 224, 112, 8, cardBg);
  g->drawRoundRect(8, 124, 224, 112, 8, borderCol);

  g->setTextSize(1);
  for (uint8_t i = 0; i < MAX_WATCH_TARGETS; i++) {
    int16_t ry = 132 + i * 20;
    const WatchTarget& wt = s.pet.targets[i];
    const TargetStatus& st = _targetStatus[i];

    uint16_t dotCol = !wt.enabled ? gfxTint(0x4208)
                    : !st.checked ? gfxTint(0xFFE0)
                    : st.up       ? gfxTint(0x07E0)
                                  : gfxTint(0xF800);
    g->fillCircle(20, ry + 4, 3, dotCol);

    g->setTextColor(wt.enabled ? gfxTint(0xFFFF) : gfxTint(0x8410), cardBg);
    g->setCursor(30, ry);
    g->print(wt.name[0] ? wt.name : wt.host);

    char rightBuf[18];
    if (!wt.enabled) {
      strlcpy(rightBuf, "OFF", sizeof(rightBuf));
      g->setTextColor(gfxTint(0x8410), cardBg);
    } else if (!st.checked) {
      strlcpy(rightBuf, "WAIT...", sizeof(rightBuf));
      g->setTextColor(gfxTint(0xFFE0), cardBg);
    } else if (st.up) {
      snprintf(rightBuf, sizeof(rightBuf), "UP %ums", (unsigned)st.latencyMs);
      g->setTextColor(gfxTint(0x07E0), cardBg);
    } else {
      strlcpy(rightBuf, "DOWN!", sizeof(rightBuf));
      g->setTextColor(gfxTint(0xF800), cardBg);
    }
    int16_t rx = 222 - (int16_t)(strlen(rightBuf) * 6);
    g->setCursor(rx, ry);
    g->print(rightBuf);
  }

  g->drawFastHLine(14, 212, 212, borderCol);
  char sumBuf[64];
  const char* sumText = sumBuf;
  uint16_t sumColor;
  if (_speechActive && _speechText[0]) {
    sumText = _speechText;
    sumColor = gfxTint(0xFFE0);
  } else if (s.pet.zbxEnabled && _zbxProblems >= 0) {
    snprintf(sumBuf, sizeof(sumBuf), "Zabbix API: %d incidente(s)", _zbxProblems);
    sumColor = _zbxProblems > 0 ? gfxTint(0xF800) : gfxTint(0x07E0);
  } else {
    snprintf(sumBuf, sizeof(sumBuf), "Hermes Events: %u | Neural: %u", (unsigned)_hermesEventsCount, (unsigned)_aiThoughtsCount);
    sumColor = gfxTint(rawMain);
  }
  uint8_t sumGlyphs = countPortugueseGlyphs(sumText, 34);
  int16_t sx = 120 - (int16_t)(sumGlyphs * 3);
  drawPortugueseText(g, max((int16_t)16, sx), 219, sumText, nullptr, 34, 1, sumColor, cardBg);
}

// ---------------------------------------------------------------------------
// Main Service Loop (~30 FPS)
// ---------------------------------------------------------------------------
void PetMode::service(const Settings& s) {
  uint32_t now = millis();
  if ((now - _lastFrameMs) < 33) return;
  _lastFrameMs = now;
  _animTick++;

  if (_fullRepaint) {
    gfxDev()->fillScreen(0x0000);
    _fullRepaint = false;
    _hudDirty    = true;
    _footerDirty = true;
  }

  updateWeather(s);
  updateSystems(s);
  updateNeuralBrain(s);
  updateBrain(s);
  updatePhysics(s);

  if (_hudDirty) {
    drawHud(s);
    _hudDirty = false;
  }

  if (s.pet.viewMode == 1) {
    drawFaceRegion(s, 23, 103);
    if (_footerDirty) {
      drawBigClockPanel(s);
      _footerDirty = false;
    }
  } else if (s.pet.viewMode == 2) {
    drawFaceRegion(s, 23, 99);
    if (_footerDirty) {
      drawNocPanel(s);
      _footerDirty = false;
    }
  } else {
    drawFaceRegion(s, 23, 137);
    if (_footerDirty) {
      drawBottomPanel(s);
      _footerDirty = false;
    }
  }
}
