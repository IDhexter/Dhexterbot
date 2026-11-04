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
  #include <WiFiClientSecureBearSSL.h>
#else
  #include <HTTPClient.h>
  #include <WiFiClient.h>
  #include <WiFiClientSecure.h>
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

// Helper to sanitize UTF-8 Portuguese accents to clean ASCII for the 5x7 bitmap font
static void sanitizeAscii(const char* in, char* out, size_t maxLen) {
  if (!in || maxLen == 0) {
    if (maxLen > 0) out[0] = 0;
    return;
  }
  size_t j = 0;
  for (size_t i = 0; in[i] != 0 && j + 1 < maxLen; i++) {
    uint8_t c = (uint8_t)in[i];
    if (c == 0xC3 && in[i + 1] != 0) {
      uint8_t n = (uint8_t)in[++i];
      char rep = 0;
      if (n >= 0xA0 && n <= 0xA5) rep = 'a';
      else if (n >= 0x80 && n <= 0x85) rep = 'A';
      else if (n >= 0xA8 && n <= 0xAB) rep = 'e';
      else if (n >= 0x88 && n <= 0x8B) rep = 'E';
      else if (n >= 0xAC && n <= 0xAF) rep = 'i';
      else if (n >= 0x8C && n <= 0x8F) rep = 'I';
      else if (n >= 0xB2 && n <= 0xB6) rep = 'o';
      else if (n >= 0x92 && n <= 0x96) rep = 'O';
      else if (n >= 0xB9 && n <= 0xBC) rep = 'u';
      else if (n >= 0x99 && n <= 0x9C) rep = 'U';
      else if (n == 0xA7) rep = 'c';
      else if (n == 0x87) rep = 'C';
      if (rep) out[j++] = rep;
    } else if (c >= 32 && c <= 126) {
      out[j++] = (char)c;
    }
  }
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
    sanitizeAscii(text, _speechText, sizeof(_speechText));
    strlcpy(_speechSource, (source && source[0]) ? source : "NOC-GIRL", sizeof(_speechSource));
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
    char msg[68];
    if (message && message[0]) {
      strlcpy(msg, message, sizeof(msg));
    } else {
      snprintf(msg, sizeof(msg), "ALERTA %s: %d incidente(s) ativos na rede!", source ? source : "NOC", alertsCount);
    }
    triggerSay(msg, PET_MOOD_ANGRY, 45, source ? source : "ZABBIX");
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
  strlcpy(_lastAiReply, msg, sizeof(_lastAiReply));
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
// Autonomous Neural Brain (NVIDIA NIM / Hermes Bot / OpenRouter OAuth / OpenAI / Gemini / Groq)
// ---------------------------------------------------------------------------
bool PetMode::triggerNeuralThought(const Settings& s, const char* customPrompt, const char* sourceTag) {
  if (!netConnected()) return false;
  if (!s.pet.aiKey[0] && s.pet.aiProvider != 1) return false;

  // Build live context string (time, weather, NOC status)
  struct tm t;
  char timeStr[12] = "agora";
  if (clockNow(t)) {
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", t.tm_hour, t.tm_min);
  }

  int upCount = 0, downCount = 0;
  for (uint8_t i = 0; i < MAX_WATCH_TARGETS; i++) {
    if (s.pet.targets[i].enabled && _targetStatus[i].checked) {
      if (_targetStatus[i].up) upCount++;
      else downCount++;
    }
  }

  char promptBuf[360];
  if (customPrompt && customPrompt[0]) {
    snprintf(promptBuf, sizeof(promptBuf),
             "Voce e DhexterBot (NOC Cyber Girl anime de fones neon azuis), parceira do Alex e do Hermes Bot. Hora:%s Clima:%.1fC HostsUP:%d Down:%d. Alex disse: \"%s\". Responda em Portugues sem acentos, max 50 chars, APENAS JSON: {\"mood\":\"happy|normal|curious|excited|cool|cyber|angry|love\",\"say\":\"frase\"}",
             timeStr, _weatherValid ? _weatherTemp : 26.0f, upCount, downCount, customPrompt);
  } else {
    snprintf(promptBuf, sizeof(promptBuf),
             "Voce e DhexterBot (NOC Cyber Girl anime de fones neon azuis), guardiana de rede integrada ao Hermes Bot e Zabbix. Hora:%s Clima:%.1fC HostsUP:%d Down:%d ZbxAlerts:%d. Gere um comentario espontaneo carismatico em Portugues sem acentos, max 50 chars, APENAS JSON: {\"mood\":\"happy|normal|curious|excited|cool|cyber|angry\",\"say\":\"frase\"}",
             timeStr, _weatherValid ? _weatherTemp : 26.0f, upCount, downCount, _zbxProblems > 0 ? _zbxProblems : 0);
  }

  // Prepare JSON body
  JsonDocument reqDoc;
  uint8_t prov = s.pet.aiProvider;

  // If provider 1 (Universal Hermes Agent Gateway — OpenAI-compatible or REST/Webhook) is selected
  if (prov == 1 && s.pet.hermesUrl[0]) {
    String hUrl = String(s.pet.hermesUrl);
    hUrl.trim();
    // Auto-normalize URL: if it ends with /v1, append /chat/completions; if bare host:port, append /v1/chat/completions
    if (hUrl.endsWith("/v1")) {
      hUrl += "/chat/completions";
    } else if (hUrl.endsWith("/v1/")) {
      hUrl += "chat/completions";
    } else {
      int schemeIdx = hUrl.indexOf("://");
      int slashIdx  = hUrl.indexOf('/', schemeIdx >= 0 ? schemeIdx + 3 : 0);
      if (slashIdx < 0) {
        hUrl += "/v1/chat/completions";
      } else if (slashIdx == (int)hUrl.length() - 1) {
        hUrl += "v1/chat/completions";
      }
    }

    reqDoc["model"]    = s.pet.aiModel[0] ? s.pet.aiModel : "hermes-agent";
    reqDoc["message"]  = promptBuf;
    reqDoc["prompt"]   = promptBuf;
    reqDoc["max_tokens"] = 90;
    JsonArray hMsgs = reqDoc["messages"].to<JsonArray>();
    JsonObject hm0 = hMsgs.add<JsonObject>();
    hm0["role"]    = "user";
    hm0["content"] = promptBuf;

    String body;
    serializeJson(reqDoc, body);
    reqDoc.clear();

    HTTPClient http;
    http.setTimeout(5000);
    bool begun = false;
#if defined(SMALLTV_ESP8266)
    std::unique_ptr<BearSSL::WiFiClientSecure> hSec;
#else
    std::unique_ptr<WiFiClientSecure> hSec;
#endif
    WiFiClient hPlain;

    if (hUrl.startsWith("https://")) {
#if defined(SMALLTV_ESP8266)
      hSec.reset(new BearSSL::WiFiClientSecure());
      hSec->setInsecure();
      hSec->setBufferSizes(1024, 512);
#else
      hSec.reset(new WiFiClientSecure());
      hSec->setInsecure();
#endif
      begun = http.begin(*hSec, hUrl);
    } else {
      begun = http.begin(hPlain, hUrl);
    }

    if (begun) {
      http.addHeader("Content-Type", "application/json");
      if (s.pet.aiKey[0]) {
        http.addHeader("Authorization", String("Bearer ") + s.pet.aiKey);
        http.addHeader("X-API-Key", s.pet.aiKey);
      }
      int code = http.POST(body);
      if (code == 200) {
        String resp = http.getString();
        http.end();
        JsonDocument resDoc;
        if (!deserializeJson(resDoc, resp)) {
          const char* rawReply = resDoc["choices"][0]["message"]["content"] |
                                 (resDoc["say"] | (resDoc["reply"] | (resDoc["message"] | (resDoc["output"] | (resDoc["response"] | "")))));
          const char* md = resDoc["mood"] | "cyber";
          if (rawReply && rawReply[0]) {
            String cStr(rawReply);
            cStr.trim();
            int js = cStr.indexOf('{');
            int je = cStr.lastIndexOf('}');
            String finalSay = cStr;
            uint8_t finalMood = petMoodFromStr(md);
            if (js >= 0 && je > js) {
              JsonDocument inner;
              if (!deserializeJson(inner, cStr.substring(js, je + 1))) {
                if (inner["say"].is<const char*>())  finalSay  = inner["say"].as<const char*>();
                if (inner["mood"].is<const char*>()) finalMood = petMoodFromStr(inner["mood"].as<const char*>());
              }
            }
            sanitizeAscii(finalSay.c_str(), _lastAiReply, sizeof(_lastAiReply));
            _aiThoughtsCount++;
            triggerSay(_lastAiReply, finalMood, 35, "HERMES AI");
            return true;
          }
        }
      } else {
        http.end();
      }
    }
    // If Hermes endpoint is unreachable and an API key is configured, fall back to cloud LLM
    if (!s.pet.aiKey[0]) return false;
    prov = 0;
  }

  const char* url = "https://integrate.api.nvidia.com/v1/chat/completions";
  const char* defModel = "nvidia/nemotron-3.5-super-vl-120b-a12b";
  if (prov == 2) {
    url = "https://openrouter.ai/api/v1/chat/completions";
    defModel = "openai/gpt-4o-mini";
  } else if (prov == 3) {
    url = "https://api.openai.com/v1/chat/completions";
    defModel = "gpt-4o-mini";
  } else if (prov == 5) {
    url = "https://api.groq.com/openai/v1/chat/completions";
    defModel = "llama-3.3-70b-versatile";
  }

  const char* modelToUse = (s.pet.aiModel[0]) ? s.pet.aiModel : defModel;

#if defined(SMALLTV_ESP8266)
  std::unique_ptr<BearSSL::WiFiClientSecure> sec(new BearSSL::WiFiClientSecure());
  sec->setInsecure();
  sec->setBufferSizes(1024, 512);
#else
  std::unique_ptr<WiFiClientSecure> sec(new WiFiClientSecure());
  sec->setInsecure();
#endif

  HTTPClient https;
  https.setTimeout(7500);

  if (prov == 4) {
    // Google Gemini REST API
    String gemUrl = String("https://generativelanguage.googleapis.com/v1beta/models/") +
                    (s.pet.aiModel[0] ? s.pet.aiModel : "gemini-2.0-flash") +
                    ":generateContent?key=" + s.pet.aiKey;
    if (!https.begin(*sec, gemUrl)) return false;
    https.addHeader("Content-Type", "application/json");
    JsonArray contents = reqDoc["contents"].to<JsonArray>();
    JsonObject c0 = contents.add<JsonObject>();
    JsonArray parts = c0["parts"].to<JsonArray>();
    parts.add<JsonObject>()["text"] = promptBuf;
  } else {
    // OpenAI-compatible Chat Completions (NVIDIA NIM, OpenRouter, OpenAI, Groq)
    if (!https.begin(*sec, url)) return false;
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", String("Bearer ") + s.pet.aiKey);
    reqDoc["model"] = modelToUse;
    reqDoc["max_tokens"] = 90;
    reqDoc["temperature"] = 0.75;
    if (prov == 0) {
      // Disable chain-of-thought reasoning tokens on NVIDIA Nemotron so reply is immediate and compact!
      reqDoc["chat_template_kwargs"]["enable_thinking"] = false;
    }
    JsonArray msgs = reqDoc["messages"].to<JsonArray>();
    JsonObject m0 = msgs.add<JsonObject>();
    m0["role"] = "user";
    m0["content"] = promptBuf;
  }

  String reqBody;
  serializeJson(reqDoc, reqBody);
  reqDoc.clear();

  int httpCode = https.POST(reqBody);
  if (httpCode != 200) {
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  JsonDocument respDoc;
  if (deserializeJson(respDoc, payload)) return false;

  const char* rawContent = nullptr;
  if (prov == 4) {
    rawContent = respDoc["candidates"][0]["content"]["parts"][0]["text"];
  } else {
    rawContent = respDoc["choices"][0]["message"]["content"];
  }
  if (!rawContent || !rawContent[0]) return false;

  // Extract inner JSON {"mood":"...","say":"..."} if present
  String contentStr(rawContent);
  contentStr.trim();
  int jStart = contentStr.indexOf('{');
  int jEnd   = contentStr.lastIndexOf('}');
  String sayText = contentStr;
  uint8_t chosenMood = PET_MOOD_CYBER;

  if (jStart >= 0 && jEnd > jStart) {
    String sub = contentStr.substring(jStart, jEnd + 1);
    JsonDocument inner;
    if (!deserializeJson(inner, sub)) {
      if (inner["say"].is<const char*>())  sayText = inner["say"].as<const char*>();
      if (inner["mood"].is<const char*>()) chosenMood = petMoodFromStr(inner["mood"].as<const char*>());
    }
  }

  sanitizeAscii(sayText.c_str(), _lastAiReply, sizeof(_lastAiReply));
  _aiThoughtsCount++;
  triggerSay(_lastAiReply, chosenMood, 35, sourceTag ? sourceTag : "IA NEURAL");
  return true;
}

void PetMode::updateNeuralBrain(const Settings& s) {
  if (!s.pet.aiEnabled || !netConnected()) return;
  uint32_t now = millis();

  // First autonomous thought 18 seconds after boot once Wi-Fi & weather are ready
  if (_pendingAutoAi && now > 18000) {
    _pendingAutoAi = false;
    _lastAiMs = now;
    triggerNeuralThought(s, nullptr, "IA NEURAL");
    return;
  }

  if (s.pet.aiIntervalMin == 0) return;
  uint32_t intervalMs = (uint32_t)s.pet.aiIntervalMin * 60000UL;
  if ((now - _lastAiMs) >= intervalMs) {
    _lastAiMs = now;
    triggerNeuralThought(s, nullptr, "IA NEURAL");
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
    _footerDirty  = true;
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
  uint16_t colNeon    = gfxTint(rawNeon);
  uint16_t colNeonDim = gfxTint(scaleColor565(rawNeon, 45));

  // Signature NOC Cyber Girl Palette (RGB565)
  uint16_t colHair     = gfxTint(0x0842); // Jet-black / midnight navy hair (#080d1a)
  uint16_t colSkin     = gfxTint(0xE77F); // Cool blue-tinted porcelain skin (#e4effa)
  uint16_t colSkinShad = gfxTint(0x74DF); // Comic-noir contour shadow (#7c9bc0)
  uint16_t colNeck     = gfxTint(0xC6DF); // Shaded neck porcelain (#c5d8ee)
  uint16_t colWhite    = gfxTint(0xFFFF); // Pure white shine / catchlights / headband
  uint16_t colHeadband = gfxTint(0xDF1D); // White-silver headband arch
  uint16_t colSclera   = gfxTint(0xF7FF); // Crisp white-blue sclera
  uint16_t colIrisTop  = gfxTint(0x08A5); // Deep navy upper iris (#09152b)
  uint16_t colIrisMid  = gfxTint(0x1A7B); // Royal sapphire mid iris (#1d4ed8)
  uint16_t colIrisBot  = gfxTint(0x3DFF); // Luminous cyan lower iris (#38bdf8)
  uint16_t colLash     = gfxTint(0x0022); // Deep obsidian eyeliner & pupil
  uint16_t colBlush    = gfxTint(0xF993); // Rosy pink anime blush
  uint16_t colLipUp    = gfxTint(0x29EB); // Defined upper lip
  uint16_t colLipBot   = gfxTint(0x8D7B); // Glossy lower lip
  uint16_t colRedAlert = gfxTint(0xF8A4); // Crimson red alert LED
  uint16_t colCyanLed  = gfxTint(0x07FF); // Switch port cyan LED
  uint16_t colGreenLed = gfxTint(0x07EC); // Switch port green LED
  uint16_t colRackBg   = gfxTint(0x0884); // Server rack frame
  uint16_t colSwitchBg = gfxTint(0x1107); // 1U switch faceplate

  bool alert = anySystemDown(s);

  // Subtle breathing & head movement following gaze
  float breath = sinf((float)_animTick * 0.09f) * 2.0f;
  int gx = (int)roundf(_curX);
  int gy = (int)roundf(_curY + breath);

  // Head center
  int hx = 120 + (gx / 4);
  int hy = (compact ? 48 : 63) + (gy / 3);
  int scalePct = compact ? 72 : 92;

  // Eye centers & dimensions
  int eyeSep = (27 * scalePct) / 100;
  int lx = hx - eyeSep;
  int rx = hx + eyeSep;
  int ey = hy + (4 * scalePct) / 100;

  int eyeHW = (16 * scalePct) / 100;
  int baseEyeHH = (18 * scalePct) / 100;
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
  int mouthY = hy + (32 * scalePct) / 100;
  bool talking = _speechActive || (_activeAction == PET_ACT_COFFEE);
  int talkH = talking ? (2 + (int)(fabsf(sinf((float)_animTick * 0.55f)) * 6.0f)) : 0;

  // Headphone earcup centers
  int cupOffset = (62 * scalePct) / 100;
  int lCupX = hx - cupOffset;
  int rCupX = hx + cupOffset;
  int cupY  = hy - (2 * scalePct) / 100;
  int cupRX = (14 * scalePct) / 100;
  int cupRY = (24 * scalePct) / 100;

  bool pulseBright = ((_animTick / 4) & 1) == 0;
  uint16_t colRingActive = alert ? (pulseBright ? colRedAlert : colNeonDim) : colNeon;

  uint8_t acc = s.pet.accessory;
  if (_activeMood == PET_MOOD_COOL) acc = 1; // Sunglasses

  uint16_t rowBuf[areaW];

  for (int y = 0; y < areaH; y++) {
    // -----------------------------------------------------------------------
    // 1. CLEAN DARK STUDIO BACKDROP (No switches behind her!)
    // -----------------------------------------------------------------------
    uint16_t bgCol = (y < areaH / 2) ? gfxTint(0x0021) : gfxTint(0x0842);
    for (int x = 0; x < areaW; x++) {
      rowBuf[x] = bgCol;
    }

    int dy = ((y - hy) * 100) / scalePct; // Normalized vertical coordinate relative to head center

    // -----------------------------------------------------------------------
    // 2. JET-BLACK BOB HAIR BACK + UPTURNED FLIPPED TIPS (FIXED HAIR COLOR!)
    // -----------------------------------------------------------------------
    uint16_t colHairRim = gfxTint(0x1A56); // Fixed subtle studio blue-slate rim (hair never changes color!)
    if (dy >= -68 && dy <= 66) {
      int hairHW = 0;
      if (dy < -20) {
        // Upper dome of hair
        int d = dy + 20;
        int r2 = 48 * 48 - d * d;
        hairHW = (r2 > 0) ? (22 + (int)sqrtf((float)r2)) : 0;
      } else if (dy <= 32) {
        // Sleek bob sides widening gently downward
        hairHW = 70 + ((dy + 20) * 10) / 52;
      } else if (dy <= 56) {
        // Signature upturned flipped-out bob curls on left & right!
        int curlProgress = dy - 32; // 0..24
        hairHW = 80 + (12 - abs(curlProgress - 10));
      } else {
        // Lower tapered hair bottom
        hairHW = 72 - ((dy - 56) * 28) / 10;
      }

      if (hairHW > 0) {
        int realHW = (hairHW * scalePct) / 100;
        int x0 = max(0, hx - realHW - 1);
        int x1 = min(areaW - 1, hx + realHW + 1);
        for (int x = x0; x <= x1; x++) {
          int adx = abs(x - hx);
          int normAdx = (adx * 100) / scalePct;
          if (dy >= 24 && dy <= 40 && normAdx >= 72 && normAdx <= 78 && dy < 32 + (normAdx - 72)) {
            continue;
          }
          if (adx >= realHW) {
            rowBuf[x] = colHairRim; // Fixed hair outline color!
          } else {
            rowBuf[x] = colHair;
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 3. NECK, BLACK CHOKER & WHITE TEE COLLAR
    // -----------------------------------------------------------------------
    if (dy >= 40 && dy <= 76) {
      int neckHW = (14 * scalePct) / 100;
      if (dy <= 58) {
        for (int x = hx - neckHW; x <= hx + neckHW; x++) {
          if (x >= 0 && x < areaW) {
            if (dy >= 49 && dy <= 54) {
              rowBuf[x] = colLash; // Fixed black choker
            } else {
              rowBuf[x] = colNeck;
            }
          }
        }
      } else {
        // White T-Shirt Shoulders & Collar
        int shHW = ((16 + (dy - 58) * 3) * scalePct) / 100;
        for (int x = max(0, hx - shHW); x <= min(areaW - 1, hx + shHW); x++) {
          int adx = abs(x - hx);
          if (adx >= shHW - 1) rowBuf[x] = colSkinShad;
          else rowBuf[x] = colHeadband;
        }
      }
    }

    // -----------------------------------------------------------------------
    // 4. PORCELAIN ANIME FACE CONTOUR + CYBER BLUSH
    // -----------------------------------------------------------------------
    if (dy >= -44 && dy <= 47) {
      int faceHW = 0;
      if (dy < 8) {
        faceHW = 54;
      } else {
        // Smooth anime V-jaw tapering to chin at dy = 47
        int t = dy - 8; // 0..39
        faceHW = 54 - (t * t * 40) / (39 * 39) - (t * 14) / 39;
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

      // Rosy/Cyber Anime Blush on cheeks (dy = 15..23)
      if (s.pet.showBlush && dy >= 15 && dy <= 23) {
        int bdy = dy - 19;
        for (int side = 0; side < 2; side++) {
          int bx = (side == 0) ? (hx - (33 * scalePct) / 100) : (hx + (33 * scalePct) / 100);
          int bhw = (10 * scalePct) / 100;
          for (int x = bx - bhw; x <= bx + bhw; x++) {
            int bdx = x - bx;
            if ((bdx * bdx) / 16 + (bdy * bdy) <= 5) {
              if (((x + y) % 4) == 0) rowBuf[x] = colBlush;
              else if (_activeMood == PET_MOOD_LOVE || _activeMood == PET_MOOD_HAPPY) rowBuf[x] = colBlush;
              else rowBuf[x] = colSkinShad;
            }
          }
        }
      }

      // Subtle comic anime nose shadow (dy = 16..20)
      if (dy >= 16 && dy <= 20) {
        if (dy == 20) {
          rowBuf[hx - 1] = colSkinShad;
          rowBuf[hx]     = colSkinShad;
        } else {
          rowBuf[hx - 1] = colSkinShad;
        }
      }
    }

    // -----------------------------------------------------------------------
    // 5. SUPER-EXPRESSIVE MULTI-LAYERED SAPPHIRE ANIME EYES
    // -----------------------------------------------------------------------
    int edy = y - ey;
    if (abs(edy) <= baseEyeHH + 10) {
      for (int side = 0; side < 2; side++) {
        int cx = (side == 0) ? lx : rx;
        int hh = (side == 0) ? lhh : rhh;
        int dir = (side == 0) ? -1 : 1; // -1 left eye, +1 right eye

        // A. Sleepy / Blinking closed eye arch
        if (hh <= 3 || _activeMood == PET_MOOD_SLEEP) {
          for (int x = cx - eyeHW; x <= cx + eyeHW; x++) {
            int edx = x - cx;
            int curveY = 3 - (edx * edx) / 48;
            if (abs(edy - curveY) <= 1) rowBuf[x] = colLash;
          }
          continue;
        }

        // B. Happy closed anime eye arch (^ ^) with lush winged lashes!
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

        // C. Open Multi-Layered Sapphire Anime Eye
        if (abs(edy) <= hh + 2) {
          for (int x = cx - eyeHW - 4; x <= cx + eyeHW + 4; x++) {
            int edx = x - cx;

            // Winged upper eyeliner flick on outer edge
            if (edx * dir >= eyeHW - 2 && edx * dir <= eyeHW + 4) {
              int wingY = -hh + 4 - ((edx * dir - (eyeHW - 2)) * 3) / 2;
              if (abs(edy - wingY) <= 1) {
                rowBuf[x] = colLash;
                continue;
              }
            }

            // Rounded anime eye boundary: (edx/eyeHW)^2 + (edy/hh)^2 <= 1
            int normDist = (edx * edx * 100) / max(1, eyeHW * eyeHW) +
                           (edy * edy * 100) / max(1, hh * hh);
            if (normDist <= 100) {
              // Thick upper anime eyelash arch
              int topArchY = -hh + (edx * edx) / 36;
              if (edy <= topArchY + 3) {
                rowBuf[x] = colLash;
                continue;
              }
              // Delicate lower lash rim
              if (normDist >= 90 && edy > hh / 2) {
                rowBuf[x] = colSkinShad;
                continue;
              }

              // Sclera base
              uint16_t pix = colSclera;

              // Iris check (centered at igx, igy)
              int idx = edx - igx;
              int idy = edy - igy;
              int irisDist = (idx * idx * 100) / max(1, irisHW * irisHW) +
                             (idy * idy * 100) / max(1, irisHH * irisHH);

              if (irisDist <= 100) {
                if (irisDist >= 84) {
                  // Dark outer limbal ring of the anime iris
                  pix = colLash;
                } else {
                  // 3-stop Sapphire Vertical Gradient Iris!
                  if (idy < -irisHH / 4)      pix = colIrisTop;
                  else if (idy < irisHH / 3)  pix = colIrisMid;
                  else                        pix = colIrisBot;

                  // Inner Pupil or Special Mood Symbol (Heart / Star / Cyber Ring)
                  if (_activeMood == PET_MOOD_LOVE && inHeart(idx, idy, irisHW - 2, irisHH - 2)) {
                    pix = colBlush;
                  } else if (_activeMood == PET_MOOD_EXCITED && inStar(idx, idy, irisHW - 2)) {
                    pix = gfxTint(0xFFE0);
                  } else if (irisDist <= 26) {
                    pix = colLash; // Deep obsidian pupil
                  } else if (_activeMood == PET_MOOD_CYBER && irisDist >= 30 && irisDist <= 46) {
                    pix = colNeon; // Glowing Persocom/Cyber data ring inside iris!
                  }
                }

                // Dual Crystal White Catchlights (Upper-Left big + Lower-Right sparkle)
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
        for (int x = lx - eyeHW - 4; x <= rx + eyeHW + 4; x++) rowBuf[x] = colNeon;
      }
      for (int side = 0; side < 2; side++) {
        int cx = (side == 0) ? lx : rx;
        int lhw = eyeHW + 4;
        for (int x = cx - lhw; x <= cx + lhw; x++) {
          int dx = x - cx;
          if (edy >= -11 && edy <= 11) {
            bool border = (abs(dx) >= lhw - 1 || edy <= -10 || edy >= 10);
            int glint = dx + edy;
            if (border) rowBuf[x] = colNeon;
            else if ((glint >= -8 && glint <= -4) || (glint >= 2 && glint <= 4)) rowBuf[x] = colWhite;
            else rowBuf[x] = colHair;
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 7. EXPRESSIVE ANIME MOUTH WITH LIP-SYNC
    // -----------------------------------------------------------------------
    int mdy = y - mouthY;
    if (s.pet.showMouth && abs(mdy) <= 10) {
      int mhw = (9 * scalePct) / 100;
      if (talking && talkH >= 3) {
        for (int x = mouthX - mhw; x <= mouthX + mhw; x++) {
          int mdx = x - mouthX;
          if (mdy >= -2 && mdy <= talkH + 1 && (mdx * mdx) / 12 + (mdy * mdy) / max(2, talkH) <= 6) {
            if (mdy == -2 || mdy == -1) rowBuf[x] = colWhite; // Upper teeth
            else if (mdy >= talkH - 1)  rowBuf[x] = colBlush; // Tongue
            else                        rowBuf[x] = colLash;
          }
        }
      } else if (_activeMood == PET_MOOD_HAPPY || _activeMood == PET_MOOD_EXCITED || _activeMood == PET_MOOD_LOVE) {
        // Open anime happy smile
        for (int x = mouthX - mhw; x <= mouthX + mhw; x++) {
          int mdx = x - mouthX;
          int botY = 6 - (mdx * mdx) / 14;
          if (mdy >= 0 && mdy <= botY) {
            if (mdy == 0 || mdy == botY) rowBuf[x] = colLash;
            else rowBuf[x] = colBlush;
          }
        }
      } else if (_activeMood == PET_MOOD_ANGRY || _activeMood == PET_MOOD_SURPRISED) {
        // Alert oval mouth
        for (int x = mouthX - 6; x <= mouthX + 6; x++) {
          int mdx = x - mouthX;
          int d2 = mdx * mdx + mdy * mdy;
          if (d2 <= 25) rowBuf[x] = (d2 >= 15) ? colLash : colRedAlert;
        }
      } else {
        // Signature Comic-Noir Defined Lips (from the reference photo!)
        for (int x = mouthX - 7; x <= mouthX + 7; x++) {
          int mdx = x - mouthX;
          if (mdy >= -1 && mdy <= 1 && abs(mdx) <= 7) {
            rowBuf[x] = colLipUp;
          } else if (mdy >= 2 && mdy <= 4 && abs(mdx) <= 6 - (mdy - 2) * 2) {
            rowBuf[x] = colLipBot;
          }
        }
      }
    }

    // -----------------------------------------------------------------------
    // 8. FULL STRAIGHT DARK BANGS + COMIC-NOIR WHITE GLOSS STREAKS
    // -----------------------------------------------------------------------
    if (dy >= -62 && dy <= -12) {
      int bangHW = (60 * scalePct) / 100;
      for (int x = max(0, hx - bangHW); x <= min(areaW - 1, hx + bangHW); x++) {
        int bdx = ((x - hx) * 100) / scalePct;
        // Curved top of bangs
        int topLim = -60 + (bdx * bdx) / 120;
        // Scalloped fringe bottom with natural part dips over forehead
        int botLim = -14;
        if (abs(bdx - 18) <= 5)      botLim = -22 + abs(bdx - 18);
        else if (abs(bdx + 22) <= 5) botLim = -21 + abs(bdx + 22);

        if (dy >= topLim && dy <= botLim) {
          // Comic-noir white hair shine streaks across the bangs (dy = -40..-26)
          bool isWhiteStreak = false;
          if (dy >= -40 && dy <= -26) {
            int sMod = (bdx + 60) % 18;
            int sWidth = (dy + 40) / 5;
            if (sMod >= 7 - sWidth && sMod <= 7 + sWidth) isWhiteStreak = true;
          }
          rowBuf[x] = isWhiteStreak ? colWhite : colHair;
        }
      }
    }

    // Expressive Eyebrows (visible cleanly at the edge of the fringe, dy = -16..-11)
    if (dy >= -16 && dy <= -11) {
      for (int side = 0; side < 2; side++) {
        int cx = (side == 0) ? lx : rx;
        int dir = (side == 0) ? -1 : 1;
        for (int x = cx - eyeHW + 2; x <= cx + eyeHW - 2; x++) {
          int edx = x - cx;
          int browY = -14;
          if (_activeMood == PET_MOOD_ANGRY) {
            browY = -12 - (edx * dir) / 4; // Furrowed inward
          } else if (_activeMood == PET_MOOD_CURIOUS && side == 1) {
            browY = -15 + (edx * edx) / 60; // Raised right eyebrow
          } else {
            browY = -14 + (edx * edx) / 55;
          }
          if (abs(dy - browY) <= 1) rowBuf[x] = colLash;
        }
      }
    }

    // -----------------------------------------------------------------------
    // 9. WHITE HEADBAND ARCH + OVER-EAR HEADPHONES WITH PULSING NEON RINGS
    // -----------------------------------------------------------------------
    // Upper White Headband Arch (dy = -68..-46)
    if (dy >= -68 && dy <= -42) {
      int hbHW = (62 * scalePct) / 100;
      for (int x = max(0, hx - hbHW); x <= min(areaW - 1, hx + hbHW); x++) {
        int bdx = ((x - hx) * 100) / scalePct;
        int archY = -65 + (bdx * bdx) / 165;
        if (abs(dy - archY) <= 3) {
          rowBuf[x] = colHeadband;
        }
      }
    }

    // Left & Right Over-Ear Headphone Cups with Glowing Neon Rings!
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
            // Glowing Neon Ring at ell = 58..86
            if (ell >= 56 && ell <= 86) {
              rowBuf[x] = colRingActive;
            } else if (ell > 86) {
              rowBuf[x] = colNeonDim;
            } else {
              // Inner glossy crescent highlight on earcup
              if (cdy >= 4 && cdy <= 10 && cdx * dir >= 2 && cdx * dir <= 6) {
                rowBuf[x] = colNeon;
              } else {
                rowBuf[x] = colHair;
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
static void printWrappedBig(Arduino_GFX* g, int16_t x0, int16_t y0, const char* text) {
  if (!text || !text[0]) return;
  g->setTextSize(2); // 12x16 big legible font! Max 18 chars per line inside 224px box
  const int maxCols = 18;
  const int maxLines = 3;
  const char* p = text;

  for (int line = 0; line < maxLines && *p; line++) {
    while (*p == ' ') p++;
    if (!*p) break;

    int len = (int)strlen(p);
    int take = len;
    if (take > maxCols) {
      take = maxCols;
      // Try to break cleanly at a space
      for (int k = maxCols; k >= 8; k--) {
        if (p[k] == ' ') {
          take = k;
          break;
        }
      }
    }

    char buf[22];
    memcpy(buf, p, take);
    buf[take] = 0;
    g->setCursor(x0, y0 + line * 18);
    g->print(buf);
    p += take;
  }
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

  const char* provNames[] = {"NIM-120B", "HERMES", "OAUTH", "OPENAI", "GEMINI", "GROQ"};
  const char* pName = provNames[s.pet.aiProvider <= 5 ? s.pet.aiProvider : 0];
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

  g->setTextColor(textCol, bubbleBg);
  printWrappedBig(g, 11, 181, msg);
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

  g->drawFastHLine(14, 212, 212, borderCol);
  char sumBuf[36];
  if (_speechActive && _speechText[0]) {
    strlcpy(sumBuf, _speechText, sizeof(sumBuf));
    g->setTextColor(gfxTint(0xFFE0), cardBg);
  } else if (s.pet.zbxEnabled && _zbxProblems >= 0) {
    snprintf(sumBuf, sizeof(sumBuf), "Zabbix API: %d incidente(s)", _zbxProblems);
    g->setTextColor(_zbxProblems > 0 ? gfxTint(0xF800) : gfxTint(0x07E0), cardBg);
  } else {
    snprintf(sumBuf, sizeof(sumBuf), "Hermes Events: %u | Neural: %u", (unsigned)_hermesEventsCount, (unsigned)_aiThoughtsCount);
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
