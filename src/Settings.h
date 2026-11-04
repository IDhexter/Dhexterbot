// Settings.h — Persisted configuration (LittleFS /config.json) for DhexterBot OS
// Repository: https://github.com/IDhexter/Dhexterbot
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

struct WifiCred {
  String ssid;
  String pass;
};

struct WatchTarget {
  char     name[16];
  char     host[36];
  uint16_t port;
  bool     enabled;
};

struct ClockSettings {
  String   tz;
  String   tzPosix;
  String   lang;
  bool     nightEnabled;
  uint16_t nightStartMin;
  uint16_t nightEndMin;
  uint8_t  nightLevel;

  void setDefaults();
  void toJson(JsonObject o) const;
  void fromJson(JsonObjectConst o);
};

struct AuthSettings {
  bool   enabled;
  String user;
  String pass;

  void setDefaults();
  void toJson(JsonObject o, bool includeSecrets) const;
  void fromJson(JsonObjectConst o);
};

struct DisplaySettings {
  uint8_t colorOrder;
  bool    invert;
  uint8_t rGain;
  uint8_t gGain;
  uint8_t bGain;

  void setDefaults();
  void toJson(JsonObject o) const;
  void fromJson(JsonObjectConst o);
};

struct ButtonSettings {
  uint8_t sensitivity;
  uint8_t shortAction;
  uint8_t longAction;

  void setDefaults();
  void toJson(JsonObject o) const;
  void fromJson(JsonObjectConst o);
};

// ---- DhexterBot 2.0 (Charismatic Companion & System Guardian) --------------
struct PetSettings {
  char        name[16];        // Robot name in top HUD, default "DHEXTERBOT"
  char        statusText[42];  // Default footer phrase when idle
  uint8_t     colorPreset;     // 0=Cyan, 1=Matrix Green, 2=Purple, 3=Amber, 4=Ice Blue, 5=Red, 6=White
  uint8_t     activity;        // 0=Calm, 1=Normal, 2=Hyperactive
  uint8_t     viewMode;        // 0=Full Face, 1=Big Clock + Robot, 2=NOC Monitor + Robot
  uint8_t     accessory;       // 0=None, 1=Sunglasses, 2=NOC Headset, 3=Top Antenna, 4=Crown
  bool        showHud;         // Top bar with Name + Clock + Weather + NOC badge
  bool        showFooter;      // Bottom speech bubble & mini status bars
  bool        scanlines;       // CRT scanline effect on eyes
  bool        autoSleep;       // Sleep automatically during night window
  bool        showMouth;       // Animated expressive mouth below eyes
  bool        showBlush;       // Rosy pixel cheeks when happy/loved
  bool        weatherFx;       // Weather particles (rain drops / stars / heat sweat)
  float       lat;             // Weather latitude (default -23.5505)
  float       lon;             // Weather longitude (default -46.6333)

  // System Integration: Watchdog Targets + Optional Zabbix JSON-RPC
  uint16_t    sysPollSec;      // How often to check systems (seconds, default 30)
  WatchTarget targets[MAX_WATCH_TARGETS];
  bool        zbxEnabled;
  char        zbxUrl[64];      // e.g. "http://10.10.1.26/api_jsonrpc.php"
  char        zbxToken[72];    // Zabbix API token
  uint8_t     zbxMinSev;       // Minimum severity (0..5, default 3=Average)

  // Hermes Buddy Brain Integration (ESP8266 = Body, Hermes = Brain with Memory & Auth)
  bool        hermesEnabled;     // Autonomous Hermes thoughts enabled
  char        hermesUrl[80];     // Hermes Buddy API URL (e.g. http://172.1.0.32:9119)
  char        hermesToken[64];   // Shared secret token (X-Buddy-Token / Bearer) between Pet and Hermes
  char        hermesPersona[64]; // Personality profile sent to Hermes
  uint16_t    hermesIntervalMin; // Autonomous thought frequency in minutes (default 10)
  uint8_t     activeStartHour;   // Start hour for autonomous thoughts (0..23, default 8)
  uint8_t     activeEndHour;     // End hour for autonomous thoughts (0..23, default 23)

  void setDefaults();
  void toJson(JsonObject o, bool includeSecrets = true) const;
  void fromJson(JsonObjectConst o);
};

struct Settings {
  WifiCred        wifi[MAX_WIFI_NETS];
  uint8_t         wifiCount;

  String          apSsid;
  String          apPass;
  String          hostname;

  uint8_t         mode;
  uint16_t        carouselSec;
  bool            carouselPet;

  uint16_t        httpTimeout;
  uint8_t         brightness;
  bool            autoBrightness;
  bool            backlightInverted;
  uint8_t         rotation;

  ClockSettings   clock;
  DisplaySettings display;
  AuthSettings    auth;
  ButtonSettings  button;
  PetSettings     pet;

  void setDefaults();
};

bool settingsBegin();
bool loadSettings(Settings& s);
bool saveSettings(const Settings& s);
void factoryReset(Settings& s);
size_t settingsStoredSize();

void settingsToJson(const Settings& s, JsonObject root, bool includeSecrets);
void settingsApplyJson(Settings& s, JsonObjectConst root);
