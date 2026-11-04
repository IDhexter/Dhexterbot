// PetMode.cpp — DhexterBot 2.0: Charismatic Desk Companion & NOC System Guardian
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
  if (v.equalsIgnoreCase("cyber"))     return PET_MOOD_CYBER;
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
  if (activeMood == PET_MOOD_ANGRY)   return 0xF924; // Alert Red-Orange
  if (activeMood == PET_MOOD_LOVE)    return 0xF993; // Hot Pink / Coral
  if (activeMood == PET_MOOD_EXCITED) return 0xFFE0; // Golden Star Yellow
  switch (preset) {
    case 0:  return 0x073F; // Electric Cyan (Cozmo / Emo)
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

void PetMode::begin(const Settings& s) {
  _fullRepaint   = true;
  _hudDirty      = true;
  _footerDirty   = true;
  _lastViewMode  = s.pet.viewMode;
  _nextBrainMs   = millis() + 2000;
  _nextBlinkMs   = millis() + 1800;
  _nextSaccadeMs = millis() + 1200;
  _nextPhraseMs  = millis() + 12000;
  _lastWeatherMs = 0;
  _lastSysCheckMs = 0;
  _lastDecayMs   = millis();
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
    _coffee    = (uint8_t)min(100, (int)_coffee + 45);
    _energy    = (uint8_t)min(100, (int)_energy + 30);
    _happiness = (uint8_t)min(100, (int)_happiness + 15);
    _xp       += 10;
    triggerSay("Hmm! Cafezinho top! +Energia!", PET_MOOD_HAPPY, holdSec);
  } else if (action == PET_ACT_CHARGE) {
    _energy    = 100;
    _xp       += 10;
    triggerSay("Bateria 100% recarregada!", PET_MOOD_CYBER, holdSec);
  } else if (action == PET_ACT_PETTING) {
    _happiness = 100;
    _xp       += 15;
    triggerSay("Adoro cafune! Voce e demais!", PET_MOOD_LOVE, holdSec);
  } else if (action == PET_ACT_HACKER) {
    _xp       += 12;
    triggerSay("Varrendo pacotes na rede...", PET_MOOD_CYBER, holdSec);
  } else if (action == PET_ACT_CELEBRATE) {
    _happiness = 100;
    _xp       += 20;
    triggerSay("UHUUUL! Tudo comemorando!", PET_MOOD_EXCITED, holdSec);
  }
  _hudDirty    = true;
  _footerDirty = true;
}

void PetMode::triggerBlink() {
  _blinkStage   = 1;
  _blinkStageMs = millis();
  _doubleBlink  = true;
}

void PetMode::triggerSay(const char* text, uint8_t mood, uint16_t ttlSec) {
  if (!text || !text[0]) {
    _speechActive  = false;
    _speechText[0] = 0;
  } else {
    strlcpy(_speechText, text, sizeof(_speechText));
    _speechActive = true;
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
  _tgtX       = constrain((int)gx, -28, 28);
  _tgtY       = constrain((int)gy, -16, 16);
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
  char msg[44];
  snprintf(msg, sizeof(msg), "Modo Foco %um iniciado! Vamos la!", (unsigned)minutes);
  triggerSay(msg, PET_MOOD_COOL, 10);
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
    char msg[48];
    if (message && message[0]) {
      strlcpy(msg, message, sizeof(msg));
    } else {
      snprintf(msg, sizeof(msg), "ALERTA %s: %d incidente(s)!", source ? source : "NOC", alertsCount);
    }
    triggerSay(msg, PET_MOOD_ANGRY, 45);
  } else {
    triggerAction(PET_ACT_CELEBRATE, 10);
    if (message && message[0]) triggerSay(message, PET_MOOD_EXCITED, 20);
  }
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

  // 1. Round-robin check of one WatchTarget every (sysPollSec / 4) seconds
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
      tcp.setTimeout(450); // Fast LAN/WAN probe so animation stays smooth
      uint32_t t0 = millis();
      bool ok = tcp.connect(wt.host, wt.port ? wt.port : 80);
      uint16_t lat = (uint16_t)(millis() - t0);
      if (ok) tcp.stop();

      _targetStatus[idx].checked   = true;
      _targetStatus[idx].up        = ok;
      _targetStatus[idx].latencyMs = ok ? lat : 0;

      if (wasChecked && wasUp && !ok) {
        // Host just went DOWN!
        char alertMsg[48];
        snprintf(alertMsg, sizeof(alertMsg), "ALERTA: %s OFFLINE!", wt.name[0] ? wt.name : wt.host);
        triggerSay(alertMsg, PET_MOOD_ANGRY, 40);
      } else if (wasChecked && !wasUp && ok) {
        // Host recovered!
        char recMsg[48];
        snprintf(recMsg, sizeof(recMsg), "ONLINE: %s voltou! (%ums)", wt.name[0] ? wt.name : wt.host, (unsigned)lat);
        triggerAction(PET_ACT_CELEBRATE, 8);
        triggerSay(recMsg, PET_MOOD_EXCITED, 20);
      }
      _hudDirty    = true;
      _footerDirty = true;
      break;
    }
  }

  // 2. Optional Zabbix JSON-RPC Problem Counter
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
            char msg[48];
            snprintf(msg, sizeof(msg), "Zabbix NOC: %d incidente(s) ativos!", cnt);
            triggerSay(msg, PET_MOOD_ANGRY, 35);
          } else if (cnt == 0 && prev > 0) {
            triggerAction(PET_ACT_CELEBRATE, 8);
            triggerSay("Zabbix 100% Limpo! Zero incidentes!", PET_MOOD_EXCITED, 20);
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
// Autonomous AI Brain, Tamagotchi Needs & Pomodoro
// ---------------------------------------------------------------------------
void PetMode::updateBrain(const Settings& s) {
  uint32_t now = millis();

  // View mode switch detection
  if (_lastViewMode != s.pet.viewMode) {
    _lastViewMode = s.pet.viewMode;
    _fullRepaint  = true;
    _hudDirty     = true;
    _footerDirty  = true;
  }

  // Expire active action
  if (_activeAction != PET_ACT_NONE && _actionUntil != 0 && (int32_t)(now - _actionUntil) >= 0) {
    _activeAction = PET_ACT_NONE;
    _hudDirty     = true;
    _footerDirty  = true;
  }

  // Pomodoro timer check
  if (_pomoActive) {
    if ((int32_t)(now - _pomoEndMs) >= 0) {
      _pomoActive = false;
      triggerAction(PET_ACT_CELEBRATE, 12);
      triggerSay("Pomodoro concluido! Hora do cafe!", PET_MOOD_EXCITED, 30);
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

  // Gradual Tamagotchi stat decay every 4 minutes
  if ((now - _lastDecayMs) >= 240000UL) {
    _lastDecayMs = now;
    if (_activeMood != PET_MOOD_SLEEP) {
      if (_energy > 15)    _energy--;
      if (_coffee > 10)    _coffee--;
      if (_happiness > 20) _happiness--;
    } else {
      // Sleeping slowly restores energy!
      if (_energy < 100) _energy = (uint8_t)min(100, (int)_energy + 3);
    }
    if (s.pet.viewMode == 0) _footerDirty = true;
  }

  // Expire speech bubble
  if (_speechActive && (int32_t)(now - _speechUntil) >= 0) {
    _speechActive = false;
    _footerDirty  = true;
  }

  // Expire forced mood
  if (_forcedMood != PET_MOOD_AUTO && _forcedUntil != 0 && (int32_t)(now - _forcedUntil) >= 0) {
    _forcedMood = PET_MOOD_AUTO;
    _hudDirty   = true;
  }

  // Expire forced gaze
  if (_gazeForced && (int32_t)(now - _gazeUntil) >= 0) {
    _gazeForced = false;
  }

  // Determine active mood
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
    // Automatically look alert/angry if a watched system is down!
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
    } else if (_coffee < 25 || _energy < 25) {
      _activeMood = PET_MOOD_TIRED;
    } else if ((int32_t)(now - _nextBrainMs) >= 0) {
      uint32_t baseHold = (s.pet.activity == 0) ? 9000 : (s.pet.activity == 2) ? 3800 : 6000;
      _nextBrainMs = now + baseHold + random(0, 4000);

      uint8_t roll = random(0, 100);
      if (roll < 38)      _activeMood = PET_MOOD_NORMAL;
      else if (roll < 56) _activeMood = PET_MOOD_HAPPY;
      else if (roll < 68) _activeMood = PET_MOOD_CURIOUS;
      else if (roll < 77) _activeMood = PET_MOOD_COOL;
      else if (roll < 85) _activeMood = PET_MOOD_EXCITED;
      else if (roll < 92) _activeMood = PET_MOOD_CYBER;
      else if (roll < 96) _activeMood = PET_MOOD_SURPRISED;
      else                _activeMood = PET_MOOD_LOVE;
    }
  }

  if (_activeMood != prevMood) {
    _hudDirty    = true;
    _footerDirty = true;
  }

  // Rotate idle charismatic phrases in footer when no custom speech is active
  if (!_speechActive && (int32_t)(now - _nextPhraseMs) >= 0) {
    _nextPhraseMs = now + 11000;
    _phraseIdx    = (_phraseIdx + 1) % 7;
    _footerDirty  = true;
  }

  // Check if clock minute/second changed
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
      _tgtX = (_tgtX <= 0) ? 20.0f : -20.0f;
      _tgtY = 0;
      _nextSaccadeMs = now + 1100;
    } else {
      uint32_t minWait = (s.pet.activity == 2) ? 650  : (s.pet.activity == 0) ? 2400 : 1200;
      uint32_t maxWait = (s.pet.activity == 2) ? 1900 : (s.pet.activity == 0) ? 5000 : 3200;
      _nextSaccadeMs = now + random(minWait, maxWait);

      if (random(0, 100) < 35) {
        _tgtX = 0;
        _tgtY = 0;
      } else {
        _tgtX = (float)random(-22, 23);
        _tgtY = (float)random(-10, 11);
      }
    }
  }

  // Elastic spring-damper physics (gives lively Cozmo/Emo bounce!)
  const float stiffness = 0.24f;
  const float damping   = 0.62f;
  _velX = (_velX + (_tgtX - _curX) * stiffness) * damping;
  _velY = (_velY + (_tgtY - _curY) * stiffness) * damping;
  _curX += _velX;
  _curY += _velY;

  // Natural blinking state machine
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
      _openL = 0.46f;
      _openR = 0.46f;
    } else if (_activeMood == PET_MOOD_CURIOUS) {
      _openL = 0.72f;
      _openR = 1.12f;
    } else if (_activeMood == PET_MOOD_SURPRISED || _activeMood == PET_MOOD_EXCITED) {
      _openL = 1.15f;
      _openR = 1.15f;
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

  uint16_t hudBg   = gfxTint(0x0862);
  uint16_t rawEye  = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t accent  = gfxTint(rawEye);
  uint16_t lineCol = gfxTint(scaleColor565(rawEye, 40));

  g->fillRect(0, 0, 240, 22, hudBg);
  g->drawFastHLine(0, 22, 240, lineCol);

  // Left indicator LED + Name (or Pomodoro Timer if active)
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
    g->print(s.pet.name[0] ? s.pet.name : "DHEXTERBOT");
  }

  // Center NOC Status Pill + Weather Temp
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

  // Right: Clock HH:MM
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

// ---------------------------------------------------------------------------
// Procedural Shapes for Scanline Renderer (Zero Heap, Ultra-Fast)
// ---------------------------------------------------------------------------
static inline bool inRoundRect(int dx, int dy, int hw, int hh, int r) {
  int ax = abs(dx), ay = abs(dy);
  if (ax > hw || ay > hh) return false;
  int ix = hw - r, iy = hh - r;
  if (ax <= ix || ay <= iy) return true;
  int cx = ax - ix, cy = ay - iy;
  return (cx * cx + cy * cy) <= (r * r);
}

static inline bool inHeart(int dx, int dy, int hw, int hh) {
  if (hw < 6 || hh < 6) return false;
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
  // 4-point sparkle star + core
  return (ax * 3 + ay <= r * 2) || (ay * 3 + ax <= r * 2) || (ax * ax + ay * ay <= (r * r) / 5);
}

// ---------------------------------------------------------------------------
// Flicker-Free Scanline Face Renderer (Eyes + Accessories + Blush + Mouth + Props)
// ---------------------------------------------------------------------------
void PetMode::drawFaceRegion(const Settings& s, int16_t areaY, int16_t areaH) {
  auto* g = gfxDev();
  const int16_t areaX = 12;
  const int16_t areaW = 216;
  const bool compact  = (areaH < 120);

  uint16_t rawMain  = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t colMain  = gfxTint(rawMain);
  uint16_t colScan  = gfxTint(scaleColor565(rawMain, 74));
  uint16_t colDim   = gfxTint(scaleColor565(rawMain, 30));
  uint16_t colBg    = gfxTint(0x0000);
  uint16_t colFrame = gfxTint(0x0841);
  uint16_t colSpec  = gfxTint(0xFFFF);
  uint16_t colBlush = gfxTint(0xF993); // Coral pink cheeks
  uint16_t colGold  = gfxTint(0xFFE0);
  uint16_t colMug   = gfxTint(0xFD20); // Coffee mug amber/orange

  float breath = sinf((float)_animTick * 0.09f) * 2.2f;
  int gx = (int)roundf(_curX);
  int gy = (int)roundf(_curY + breath);

  int baseEyeY = compact ? 42 : 56;
  int lx = 62  + gx;
  int ly = baseEyeY + gy;
  int rx = 154 + gx;
  int ry = baseEyeY + gy;

  int baseHW = compact ? 25 : 28;
  int baseHH = compact ? 27 : 34;

  int lhw = baseHW, rhw = baseHW;
  int lhh = max(2, (int)roundf(baseHH * _openL));
  int rhh = max(2, (int)roundf(baseHH * _openR));

  if (_activeMood == PET_MOOD_CURIOUS) {
    lhw = baseHW - 3;
    rhw = baseHW + 4;
  } else if (_activeMood == PET_MOOD_LOVE) {
    int pulse = (int)(sinf((float)_animTick * 0.25f) * 3.0f);
    lhw = baseHW + 2 + pulse; rhw = lhw;
    lhh = baseHW + pulse;     rhh = lhh;
  }

  int radL = min(13, min(lhw, lhh));
  int radR = min(13, min(rhw, rhh));

  int scanX = 108 + (int)(sinf((float)_animTick * 0.18f) * 78.0f);

  // Mouth center & openness
  int mouthX = 108 + (gx / 2);
  int mouthY = compact ? (areaH - 16) : 114;
  bool talking = _speechActive || (_activeAction == PET_ACT_COFFEE);
  int talkH = talking ? (2 + (int)(fabsf(sinf((float)_animTick * 0.45f)) * 7.0f)) : 0;

  // Effective accessory (Cool mood forces sunglasses; Pomodoro forces NOC headset if none set)
  uint8_t acc = s.pet.accessory;
  if (_activeMood == PET_MOOD_COOL) acc = 1;
  else if (_pomoActive && acc == 0) acc = 2;

  bool showBlush = s.pet.showBlush &&
                   (_activeMood == PET_MOOD_HAPPY || _activeMood == PET_MOOD_LOVE ||
                    _activeMood == PET_MOOD_EXCITED || _activeAction == PET_ACT_PETTING);

  bool raining = s.pet.weatherFx && _weatherValid && (_weatherCode >= 51);

  uint16_t rowBuf[areaW];

  for (int y = 0; y < areaH; y++) {
    uint16_t bgLine = (y == 0 || y == areaH - 1) ? colFrame : colBg;
    for (int x = 0; x < areaW; x++) {
      rowBuf[x] = bgLine;
    }
    rowBuf[0] = colFrame;
    rowBuf[areaW - 1] = colFrame;

    uint16_t eyeCol = (s.pet.scanlines && ((y & 1) == 1)) ? colScan : colMain;

    // 1. Background FX: Matrix Rain (ACT_HACKER) or Weather Raindrops
    if (_activeAction == PET_ACT_HACKER) {
      int colIdx = y / 4;
      for (int c = 8; c < areaW - 8; c += 18) {
        int dropY = ((_animTick * 4 + c * 7) % areaH);
        if (abs(y - dropY) <= 5 && ((c + y) & 1)) {
          rowBuf[c] = (y == dropY) ? colSpec : colDim;
        }
      }
    } else if (raining) {
      for (int c = 6; c < areaW - 6; c += 34) {
        if (c > 36 && c < 180) continue; // keep rain on edges
        int dropY = ((_animTick * 5 + c * 11) % areaH);
        if (y >= dropY && y <= dropY + 4) {
          rowBuf[c] = gfxTint(0x3D1F);
        }
      }
    }

    // 2. Top Accessory: Robot Antenna (acc == 3) or Golden Crown (acc == 4)
    if (acc == 3 && y >= 2 && y <= 20) {
      int ax = 108 + (gx / 3);
      if (y <= 8) {
        // Glowing antenna orb
        int dyOrb = y - 5;
        for (int x = ax - 5; x <= ax + 5; x++) {
          int dxOrb = x - ax;
          if (dxOrb * dxOrb + dyOrb * dyOrb <= 20) {
            rowBuf[x] = ((_animTick & 4) || (dxOrb * dxOrb + dyOrb * dyOrb <= 6)) ? colSpec : colMain;
          }
        }
      } else {
        rowBuf[ax - 1] = colDim;
        rowBuf[ax]     = colMain;
        rowBuf[ax + 1] = colDim;
      }
    } else if (acc == 4 && y >= 4 && y <= 18) {
      // Pixel Golden Crown
      int cx = 108 + (gx / 3);
      int cy = y - 4; // 0..14
      for (int x = cx - 22; x <= cx + 22; x++) {
        int dx = abs(x - cx);
        bool inCrown = (cy >= 9) ||
                       (dx <= 6 && cy >= dx) ||
                       (dx >= 10 && dx <= 22 && cy >= abs(dx - 16));
        if (inCrown) {
          if (cy == 11 && (dx == 0 || dx == 14)) rowBuf[x] = gfxTint(0xF800); // Ruby gems
          else rowBuf[x] = colGold;
        }
      }
    }

    // 3. NOC Headset Headband & Earpads (acc == 2)
    if (acc == 2) {
      int hx = 108 + (gx / 2);
      if (y >= 8 && y <= 14) {
        for (int x = hx - 82; x <= hx + 82; x++) {
          if (x >= 2 && x < areaW - 2) {
            int arcY = 8 + (abs(x - hx) * abs(x - hx)) / 950;
            if (abs(y - arcY) <= 1) rowBuf[x] = colMain;
          }
        }
      }
      if (y >= baseEyeY - 16 && y <= baseEyeY + 18) {
        // Left & right earpads
        for (int x = 10; x <= 18; x++) rowBuf[x] = colMain;
        for (int x = areaW - 19; x <= areaW - 11; x++) rowBuf[x] = colMain;
      }
      // Boom mic reaching from left earpad toward mouth
      if (y >= baseEyeY + 16 && y <= mouthY + 2) {
        int mx = 16 + (y - (baseEyeY + 16)) * 2;
        if (mx >= 12 && mx < mouthX - 18) {
          rowBuf[mx]     = colDim;
          rowBuf[mx + 1] = colMain;
          if (y >= mouthY && y <= mouthY + 2) {
            rowBuf[mx + 2] = gfxTint(0x07E0); // Green active mic LED
            rowBuf[mx + 3] = gfxTint(0x07E0);
          }
        }
      }
    }

    // 4. Eyes (12 Moods)
    if (_activeMood == PET_MOOD_CYBER) {
      int dy = y - ly;
      if (abs(dy) <= 13) {
        for (int x = 22; x < 194; x++) {
          int dx = abs(x - 108);
          if (dx <= 84 && abs(dy) <= 11) {
            int distFromBeam = abs(x - scanX);
            if (distFromBeam < 12)      rowBuf[x] = colSpec;
            else if (distFromBeam < 32) rowBuf[x] = eyeCol;
            else if ((x % 4) == 0)      rowBuf[x] = colDim;
          }
        }
      }
    } else {
      for (int side = 0; side < 2; side++) {
        int cx  = (side == 0) ? lx  : rx;
        int cy  = (side == 0) ? ly  : ry;
        int hw  = (side == 0) ? lhw : rhw;
        int hh  = (side == 0) ? lhh : rhh;
        int rad = (side == 0) ? radL : radR;

        int dy = y - cy;
        if (abs(dy) > hh + 8) continue;

        int x0 = max(2, cx - hw - 2);
        int x1 = min(areaW - 3, cx + hw + 2);

        for (int x = x0; x <= x1; x++) {
          int dx = x - cx;
          bool lit  = false;
          bool spec = false;

          if (_activeMood == PET_MOOD_LOVE) {
            lit = inHeart(dx, dy, hw, hh);
            if (lit && dx >= -hw / 2 && dx <= -hw / 5 && dy >= -hh / 2 && dy <= -hh / 5) spec = true;
          } else if (_activeMood == PET_MOOD_EXCITED) {
            if (inRoundRect(dx, dy, hw, hh, rad)) {
              lit = true;
              if (inStar(dx, dy, hw - 6)) spec = true;
            }
          } else if (_activeMood == PET_MOOD_HAPPY) {
            if (inRoundRect(dx, dy, hw, hh, rad)) {
              int cutY = cy + 9 + (dx * dx) / 26;
              if (y < cutY) lit = true;
            }
          } else if (_activeMood == PET_MOOD_SLEEP) {
            int arcY = cy + 6 - (dx * dx) / 40;
            if (abs(dx) <= hw - 4 && abs(y - arcY) <= 2) lit = true;
          } else {
            if (inRoundRect(dx, dy, hw, hh, rad)) {
              lit = true;
              if (_activeMood == PET_MOOD_ANGRY) {
                int innerDx = (side == 0) ? dx : -dx;
                int slopeY  = cy - hh + 12 + ((innerDx + hw) * 18) / (2 * hw);
                if (y < slopeY) lit = false;
              } else if (_activeMood == PET_MOOD_TIRED) {
                if (y < cy - 3) lit = false;
              }
              if (lit && hh > 12) {
                int sx = dx + hw / 3 - gx / 4;
                int sy = dy + hh / 3 - gy / 4;
                if (sx * sx + sy * sy <= 22) spec = true;
                int sx2 = dx - hw / 3;
                int sy2 = dy - hh / 3;
                if (sx2 * sx2 + sy2 * sy2 <= 6) spec = true;
              }
            }
          }

          if (lit) {
            rowBuf[x] = spec ? colSpec : eyeCol;
          }
        }
      }
    }

    // 5. Cool Sunglasses Overlay (acc == 1 or PET_MOOD_COOL)
    if (acc == 1) {
      int sy = ly;
      int dy = y - sy;
      if (dy >= -18 && dy <= 16) {
        // Bridge + side temples
        if (dy >= -16 && dy <= -12) {
          for (int x = max(4, lx - 32); x <= min(areaW - 5, rx + 32); x++) {
            rowBuf[x] = colSpec;
          }
        }
        // Left & right dark lenses with neon border and diagonal white reflection
        for (int side = 0; side < 2; side++) {
          int cx = (side == 0) ? lx : rx;
          int lensHW = 30;
          int lensHH = 16;
          for (int x = max(4, cx - lensHW); x <= min(areaW - 5, cx + lensHW); x++) {
            int dx = x - cx;
            if (abs(dx) <= lensHW && dy >= -lensHH && dy <= lensHH - (abs(dx) / 5)) {
              bool border = (abs(dx) >= lensHW - 2) || (dy <= -lensHH + 2) || (dy >= lensHH - (abs(dx) / 5) - 2);
              int Glint = (dx + dy);
              if (border) {
                rowBuf[x] = colMain;
              } else if (Glint >= -10 && Glint <= -5 || Glint >= 2 && Glint <= 4) {
                rowBuf[x] = colSpec; // Deal With It diagonal lens shine!
              } else {
                rowBuf[x] = gfxTint(0x0862); // Dark tinted lens
              }
            }
          }
        }
      }
    }

    // 6. Rosy Blushing Cheeks (below outer eye edges)
    if (showBlush) {
      int blushY = ly + baseHH + 5;
      int dyB = y - blushY;
      if (abs(dyB) <= 5) {
        for (int side = 0; side < 2; side++) {
          int bx = (side == 0) ? (lx - 22) : (rx + 22);
          for (int x = max(4, bx - 12); x <= min(areaW - 5, bx + 12); x++) {
            int dxB = x - bx;
            if ((dxB * dxB) / 28 + (dyB * dyB) / 5 <= 5) {
              // Cute anime diagonal blush slashes inside pink oval
              if (((x + y) % 4) == 0) rowBuf[x] = colSpec;
              else rowBuf[x] = colBlush;
            }
          }
        }
      }
    }

    // 7. Animated Expressive Mouth
    if (s.pet.showMouth && _activeMood != PET_MOOD_CYBER) {
      int dyM = y - mouthY;
      if (abs(dyM) <= 12) {
        if (talking && talkH >= 3) {
          // Open talking mouth D-shape / oval
          for (int x = mouthX - 12; x <= mouthX + 12; x++) {
            int dxM = x - mouthX;
            if (dyM >= -2 && dyM <= talkH + 2 && (dxM * dxM) / 18 + (dyM * dyM) / max(2, talkH) <= 8) {
              bool rim = (dyM == -2) || (abs(dxM) >= 10) || (dyM >= talkH);
              rowBuf[x] = rim ? colMain : (dyM >= talkH - 2 ? colBlush : colBg);
            }
          }
        } else if (_activeMood == PET_MOOD_HAPPY || _activeMood == PET_MOOD_LOVE || _activeMood == PET_MOOD_EXCITED) {
          // Cute cat-mouth 'w' or wide smile arc
          for (int x = mouthX - 15; x <= mouthX + 15; x++) {
            int dxM = x - mouthX;
            int curveY = -2 + (dxM * dxM) / 28;
            if (abs(dyM - curveY) <= 1) rowBuf[x] = colMain;
          }
        } else if (_activeMood == PET_MOOD_ANGRY) {
          // Frown / jagged mouth
          for (int x = mouthX - 13; x <= mouthX + 13; x++) {
            int dxM = x - mouthX;
            int curveY = 4 - (dxM * dxM) / 28;
            if (abs(dyM - curveY) <= 1) rowBuf[x] = colMain;
          }
        } else if (_activeMood == PET_MOOD_SURPRISED) {
          // Small 'O' mouth
          for (int x = mouthX - 7; x <= mouthX + 7; x++) {
            int dxM = x - mouthX;
            int d2 = dxM * dxM + dyM * dyM;
            if (d2 >= 16 && d2 <= 42) rowBuf[x] = colMain;
          }
        } else if (_activeMood == PET_MOOD_COOL) {
          // Confident asymmetric smirk
          for (int x = mouthX - 10; x <= mouthX + 14; x++) {
            int dxM = x - mouthX;
            int smirkY = 2 - (dxM + 10) / 5;
            if (abs(dyM - smirkY) <= 1) rowBuf[x] = colMain;
          }
        } else if (_activeMood != PET_MOOD_SLEEP) {
          // Gentle neutral smile
          for (int x = mouthX - 10; x <= mouthX + 10; x++) {
            int dxM = x - mouthX;
            int curveY = (dxM * dxM) / 36;
            if (abs(dyM - curveY) <= 1) rowBuf[x] = colMain;
          }
        }
      }
    }

    // 8. Interactive Action Props (Coffee Mug, Electric Bolts, Floating Hearts/Stars, Sleep zZz)
    if (_activeAction == PET_ACT_COFFEE && !compact) {
      // Steaming Coffee Mug at bottom-right of face (x = 158..190, y = 96..134)
      int mugX = 172, mugY = 118;
      int dyC = y - mugY;
      if (dyC >= -14 && dyC <= 14) {
        for (int x = mugX - 12; x <= mugX + 16; x++) {
          int dxC = x - mugX;
          if (dxC >= -11 && dxC <= 11 && abs(dyC) <= 12) {
            bool rim = (abs(dxC) >= 9 || dyC <= -10 || dyC >= 10);
            rowBuf[x] = rim ? colSpec : colMug;
          } else if (dxC > 11 && dxC <= 15 && abs(dyC) <= 7) {
            if (dxC >= 14 || abs(dyC) >= 5) rowBuf[x] = colSpec; // Mug handle
          }
        }
      } else if (dyC >= -28 && dyC < -14) {
        // Rising steam wisps (~ ~)
        int wisp = (int)(sinf((float)(y + _animTick) * 0.35f) * 3.0f);
        rowBuf[mugX - 5 + wisp] = colSpec;
        rowBuf[mugX + 4 - wisp] = colSpec;
      }
    } else if (_activeAction == PET_ACT_CHARGE) {
      // Electric lightning bolts on left and right sides
      if ((_animTick & 1) == 0 && y >= 24 && y <= areaH - 20) {
        int zig = ((y / 8) & 1) ? 4 : -4;
        int lxBolt = 18 + zig + (y % 8);
        int rxBolt = areaW - 18 - zig - (y % 8);
        rowBuf[lxBolt] = colGold; rowBuf[lxBolt + 1] = colSpec;
        rowBuf[rxBolt] = colGold; rowBuf[rxBolt - 1] = colSpec;
      }
    } else if (_activeAction == PET_ACT_PETTING || _activeAction == PET_ACT_CELEBRATE) {
      // Floating hearts or celebration stars rising on left & right
      for (int p = 0; p < 2; p++) {
        int px = (p == 0) ? 24 : (areaW - 24);
        int py = areaH - 15 - (((_animTick * 2) + p * 45) % (areaH - 25));
        if (abs(y - py) <= 6) {
          for (int x = px - 6; x <= px + 6; x++) {
            if (_activeAction == PET_ACT_PETTING) {
              if (inHeart(x - px, y - py, 6, 6)) rowBuf[x] = colBlush;
            } else {
              if (inStar(x - px, y - py, 6)) rowBuf[x] = (p == 0) ? colGold : colMain;
            }
          }
        }
      }
    } else if (_activeMood == PET_MOOD_SLEEP) {
      int zPhase = (_animTick * 2) % 60;
      int zy1 = 44 - (zPhase / 2);
      int zx1 = 166 + (zPhase / 3);
      if (y >= zy1 && y < zy1 + 10 && zx1 + 8 < areaW) {
        int ryZ = y - zy1;
        for (int rxZ = 0; rxZ < 8; rxZ++) {
          if (ryZ == 0 || ryZ == 9 || (rxZ + ryZ == 8)) rowBuf[zx1 + rxZ] = eyeCol;
        }
      }
    }

    g->draw16bitRGBBitmap(areaX, areaY + y, rowBuf, areaW, 1);
  }
}

// ---------------------------------------------------------------------------
// View 0 Bottom Panel: Speech Bubble + Tamagotchi Stat Bars (y = 167..239)
// ---------------------------------------------------------------------------
void PetMode::drawBottomPanel(const Settings& s) {
  auto* g = gfxDev();
  g->fillRect(0, 166, 240, 74, 0x0000);
  if (!s.pet.showFooter) return;

  uint16_t rawMain   = presetColor565(s.pet.colorPreset, _activeMood, _activeAction);
  uint16_t borderCol = gfxTint(scaleColor565(rawMain, 65));
  uint16_t bubbleBg  = gfxTint(0x0862);
  uint16_t textCol   = _speechActive ? gfxTint(0xFFE0) : gfxTint(0xFFFF);

  // Speech bubble card (y = 170..213)
  g->fillRoundRect(10, 170, 220, 42, 8, bubbleBg);
  g->drawRoundRect(10, 170, 220, 42, 8, borderCol);
  g->fillTriangle(114, 170, 126, 170, 120, 165, borderCol);

  const char* msg = nullptr;
  char autoBuf[46];

  if (_speechActive && _speechText[0]) {
    msg = _speechText;
  } else {
    switch (_phraseIdx) {
      case 0:
        msg = s.pet.statusText[0] ? s.pet.statusText : "DhexterBot 2.0 na escuta!";
        break;
      case 1:
        if (anySystemDown(s)) {
          snprintf(autoBuf, sizeof(autoBuf), "ATENCAO: Verifique o painel NOC!");
          msg = autoBuf;
        } else {
          msg = "Sistemas monitorados 100% OK!";
        }
        break;
      case 2:
        if (_weatherValid) {
          snprintf(autoBuf, sizeof(autoBuf), "Clima agora: %.1f C | Nivel %u", _weatherTemp, (unsigned)level());
          msg = autoBuf;
        } else {
          msg = "Que tal um cafezinho agora?";
        }
        break;
      case 3:
        snprintf(autoBuf, sizeof(autoBuf), "IP: %s | Heap: %uK", netIP().c_str(), (unsigned)(ESP.getFreeHeap() / 1024));
        msg = autoBuf;
        break;
      case 4:
        msg = (_coffee < 40) ? "Preciso de cafe! Me da um gole?" : "Energia total! Bora codar!";
        break;
      case 5:
        msg = "Mande comandos em /api/pet!";
        break;
      default:
        msg = "Seu companheiro fiel de bancada!";
        break;
    }
  }

  g->setTextSize(1);
  g->setTextColor(textCol, bubbleBg);
  size_t len = strlen(msg);
  if (len <= 33) {
    int16_t tx = 120 - (int16_t)(len * 3);
    if (tx < 16) tx = 16;
    g->setCursor(tx, 187);
    g->print(msg);
  } else {
    char line1[34];
    strlcpy(line1, msg, sizeof(line1));
    g->setCursor(18, 179);
    g->print(line1);
    g->setCursor(18, 194);
    g->print(msg + 33);
  }

  // Mini Tamagotchi Status Bars (y = 218..236): ⚡ Energy | ☕ Coffee | ♥ Happy | LV.X
  uint16_t barBg = gfxTint(0x1082);
  // ⚡ Energy
  g->setTextColor(gfxTint(0x073F), 0x0000);
  g->setCursor(12, 222);
  g->print("E:");
  g->fillRect(24, 223, 36, 6, barBg);
  g->fillRect(24, 223, (int16_t)(36 * _energy / 100), 6, gfxTint(0x073F));

  // ☕ Coffee
  g->setTextColor(gfxTint(0xFD60), 0x0000);
  g->setCursor(68, 222);
  g->print("C:");
  g->fillRect(80, 223, 36, 6, barBg);
  g->fillRect(80, 223, (int16_t)(36 * _coffee / 100), 6, gfxTint(0xFD60));

  // ♥ Happiness
  g->setTextColor(gfxTint(0xF993), 0x0000);
  g->setCursor(124, 222);
  g->print("H:");
  g->fillRect(136, 223, 36, 6, barBg);
  g->fillRect(136, 223, (int16_t)(36 * _happiness / 100), 6, gfxTint(0xF993));

  // 🏆 Level badge
  char lvBuf[12];
  snprintf(lvBuf, sizeof(lvBuf), "LV.%u", (unsigned)level());
  g->setTextColor(gfxTint(0xFFE0), 0x0000);
  g->setCursor(184, 222);
  g->print(lvBuf);
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

  const char* botMsg = _speechActive ? _speechText : (s.pet.statusText[0] ? s.pet.statusText : "DhexterBot Clock Mode");
  char clip[34];
  strlcpy(clip, botMsg, sizeof(clip));
  g->setTextColor(accent, cardBg);
  int16_t mx = 120 - (int16_t)(strlen(clip) * 3);
  g->setCursor(max((int16_t)18, mx), 210);
  g->print(clip);
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

  // Bottom status / Zabbix summary line inside NOC card
  g->drawFastHLine(14, 212, 212, borderCol);
  char sumBuf[36];
  if (_speechActive && _speechText[0]) {
    strlcpy(sumBuf, _speechText, sizeof(sumBuf));
    g->setTextColor(gfxTint(0xFFE0), cardBg);
  } else if (s.pet.zbxEnabled && _zbxProblems >= 0) {
    snprintf(sumBuf, sizeof(sumBuf), "Zabbix API: %d incidente(s)", _zbxProblems);
    g->setTextColor(_zbxProblems > 0 ? gfxTint(0xF800) : gfxTint(0x07E0), cardBg);
  } else {
    snprintf(sumBuf, sizeof(sumBuf), "IP: %s | Guardiao Ativo", netIP().c_str());
    g->setTextColor(gfxTint(rawMain), cardBg);
  }
  int16_t sx = 120 - (int16_t)(strlen(sumBuf) * 3);
  g->setCursor(max((int16_t)16, sx), 219);
  g->print(sumBuf);
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
  updateBrain(s);
  updatePhysics(s);

  if (_hudDirty) {
    drawHud(s);
    _hudDirty = false;
  }

  if (s.pet.viewMode == 1) {
    drawFaceRegion(s, 24, 102);
    if (_footerDirty) {
      drawBigClockPanel(s);
      _footerDirty = false;
    }
  } else if (s.pet.viewMode == 2) {
    drawFaceRegion(s, 24, 98);
    if (_footerDirty) {
      drawNocPanel(s);
      _footerDirty = false;
    }
  } else {
    drawFaceRegion(s, 24, 140);
    if (_footerDirty) {
      drawBottomPanel(s);
      _footerDirty = false;
    }
  }
}
