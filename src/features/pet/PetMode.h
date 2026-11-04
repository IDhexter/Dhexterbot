// PetMode.h — DhexterBot 2.1: NOC Cyber Girl (Super Expressive Anime) + Neural AI & Hermes Bot
// Repository: https://github.com/IDhexter/Dhexterbot
#pragma once
#include <Arduino.h>
#include "Mode.h"
#include "Settings.h"

enum PetMood : uint8_t {
  PET_MOOD_AUTO      = 0,
  PET_MOOD_NORMAL    = 1,
  PET_MOOD_HAPPY     = 2,
  PET_MOOD_ANGRY     = 3,
  PET_MOOD_CURIOUS   = 4,
  PET_MOOD_SURPRISED = 5,
  PET_MOOD_LOVE      = 6,
  PET_MOOD_CYBER     = 7,
  PET_MOOD_TIRED     = 8,
  PET_MOOD_SLEEP     = 9,
  PET_MOOD_COOL      = 10,
  PET_MOOD_EXCITED   = 11,
  PET_MOOD_COUNT
};

enum PetAction : uint8_t {
  PET_ACT_NONE      = 0,
  PET_ACT_COFFEE    = 1,
  PET_ACT_CHARGE    = 2,
  PET_ACT_PETTING   = 3,
  PET_ACT_HACKER    = 4,
  PET_ACT_CELEBRATE = 5
};

struct TargetStatus {
  bool     checked;
  bool     up;
  uint16_t latencyMs;
};

class PetMode : public DisplayMode {
 public:
  const char* id() const override { return "pet"; }
  uint8_t modeConst() const override { return MODE_PET; }

  void begin(const Settings& s) override;
  void service(const Settings& s) override;
  void invalidate(const Settings& s) override;
  void wake(const Settings& s) override;

  // Live interactive triggers from Web UI, Neural AI, or Hermes Bot (/api/pet, /api/ai/chat, /api/hermes/webhook)
  void triggerMood(uint8_t mood, uint16_t holdSec = 25);
  void triggerAction(uint8_t action, uint16_t holdSec = 10);
  void triggerBlink();
  void triggerSay(const char* text, uint8_t mood = PET_MOOD_AUTO, uint16_t ttlSec = 25, const char* source = "NOC-GIRL");
  void triggerGaze(int8_t gx, int8_t gy, uint16_t holdMs = 3500);
  void startPomodoro(uint16_t minutes);
  void stopPomodoro();
  void forceSystemCheck();
  void pushExternalAlert(const char* source, const char* message, int alertsCount);

  // Neural Brain & Hermes Bot Integration
  bool triggerNeuralThought(const Settings& s, const char* customPrompt = nullptr, const char* sourceTag = "IA NEURAL");
  void pushHermesEvent(const char* eventType, const char* message, const char* moodStr, int alertCount = -1);

  // State getters for /api/pet & /api/hermes/status
  uint8_t     currentMood()       const { return _activeMood; }
  uint8_t     forcedMood()        const { return _forcedMood; }
  uint8_t     currentAction()     const { return _activeAction; }
  const char* currentSpeech()     const { return _speechActive ? _speechText : ""; }
  const char* speechSource()      const { return _speechSource; }
  const char* lastAiReply()       const { return _lastAiReply; }
  uint32_t    aiThoughtsCount()   const { return _aiThoughtsCount; }
  uint32_t    hermesEventsCount() const { return _hermesEventsCount; }

  bool        weatherValid()      const { return _weatherValid; }
  float       weatherTemp()       const { return _weatherTemp; }
  int         weatherCode()       const { return _weatherCode; }

  bool        pomodoroActive()    const { return _pomoActive; }
  uint32_t    pomodoroRemSec()    const;

  const TargetStatus& targetStatus(uint8_t idx) const { return _targetStatus[idx < MAX_WATCH_TARGETS ? idx : 0]; }
  int         zbxProblems()       const { return _zbxProblems; }
  bool        anySystemDown(const Settings& s) const;

 private:
  bool     _fullRepaint       = true;
  bool     _hudDirty          = true;
  bool     _footerDirty       = true;
  uint8_t  _lastViewMode      = 0xFF;

  // Mood & Action state
  uint8_t  _forcedMood        = PET_MOOD_AUTO;
  uint8_t  _activeMood        = PET_MOOD_NORMAL;
  uint32_t _forcedUntil       = 0;
  uint32_t _nextBrainMs       = 0;

  uint8_t  _activeAction      = PET_ACT_NONE;
  uint32_t _actionUntil       = 0;

  // Spring-damper eye physics
  float    _curX              = 0.0f;
  float    _curY              = 0.0f;
  float    _velX              = 0.0f;
  float    _velY              = 0.0f;
  float    _tgtX              = 0.0f;
  float    _tgtY              = 0.0f;
  bool     _gazeForced        = false;
  uint32_t _gazeUntil         = 0;
  uint32_t _nextSaccadeMs     = 0;

  // Blink state
  uint32_t _nextBlinkMs       = 0;
  uint8_t  _blinkStage        = 0;
  uint32_t _blinkStageMs      = 0;
  bool     _doubleBlink       = false;
  float    _openL             = 1.0f;
  float    _openR             = 1.0f;

  // Frame timing
  uint32_t _lastFrameMs       = 0;
  uint16_t _animTick          = 0;

  // Pomodoro timer
  bool     _pomoActive        = false;
  uint32_t _pomoEndMs         = 0;
  uint32_t _pomoTotalMs       = 0;
  uint16_t _lastPomoSec       = 0xFFFF;

  // Speech bubble & rotating phrases (68 chars -> 2 full lines of 33 chars!)
  char     _speechText[68]    = "";
  char     _speechSource[16]  = "NOC-GIRL";
  bool     _speechActive      = false;
  uint32_t _speechUntil       = 0;
  uint8_t  _phraseIdx         = 0;
  uint32_t _nextPhraseMs      = 0;

  // Neural AI & Hermes state
  char     _lastAiReply[68]   = "Pronta no NOC! Integrada ao Hermes Bot e Zabbix.";
  uint32_t _lastAiMs          = 0;
  uint32_t _aiThoughtsCount   = 0;
  uint32_t _hermesEventsCount = 0;
  bool     _pendingAutoAi     = false;

  // Weather (Open-Meteo HTTP)
  float    _weatherTemp       = 0.0f;
  int      _weatherCode       = 0;
  bool     _weatherValid      = false;
  uint32_t _lastWeatherMs     = 0;

  // System Watchdog + Zabbix state
  TargetStatus _targetStatus[MAX_WATCH_TARGETS];
  uint8_t  _nextTargetIdx     = 0;
  uint32_t _lastSysCheckMs    = 0;
  uint32_t _lastZbxCheckMs    = 0;
  int      _zbxProblems       = -1;

  // Clock cache
  int      _lastClockMin      = -1;
  int      _lastClockSec      = -1;

  void updateBrain(const Settings& s);
  void updatePhysics(const Settings& s);
  void updateWeather(const Settings& s);
  void updateSystems(const Settings& s);
  void updateNeuralBrain(const Settings& s);
  void drawHud(const Settings& s);
  void drawFaceRegion(const Settings& s, int16_t areaY, int16_t areaH);
  void drawBottomPanel(const Settings& s);
  void drawBigClockPanel(const Settings& s);
  void drawNocPanel(const Settings& s);
};

extern PetMode g_petMode;
const char* petMoodName(uint8_t m);
uint8_t     petMoodFromStr(const char* s);
const char* petActionName(uint8_t a);
uint8_t     petActionFromStr(const char* s);
