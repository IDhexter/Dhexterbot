#include "Clock.h"
#include "Platform.h"
#include "config.h"

static String            s_armedTz;
static bool              s_ntpStarted = false;
static volatile uint32_t s_lastSyncMs = 0;
static volatile bool     s_haveSync   = false;

static bool     s_nightLatched = false;
static bool     s_nightActive  = false;
static bool     s_nightHeld    = false;
static uint32_t s_lastResyncMs = 0;

static void onNtpSync() {
  s_lastSyncMs = millis();
  s_haveSync   = true;
}

static bool nightWindowContains(int now, int start, int end) {
  if (start == end) return false;
  if (start < end)  return now >= start && now < end;
  return now >= start || now < end;
}

void clockBegin(const Settings& s) {
  platformOnTimeSync(onNtpSync);
  const char* tz = s.clock.tzPosix.length() ? s.clock.tzPosix.c_str() : "<-03>3";
  platformTimeBegin(tz, NTP_SERVER1, NTP_SERVER2);
  s_armedTz = s.clock.tzPosix;
  s_ntpStarted = true;
}

void clockReapply(const Settings& s) {
  if (!s_ntpStarted || s.clock.tzPosix != s_armedTz) clockBegin(s);
}

void clockForceResync(const Settings& s) {
  const char* tz = s.clock.tzPosix.length() ? s.clock.tzPosix.c_str() : "<-03>3";
  platformTimeBegin(tz, NTP_SERVER1, NTP_SERVER2);
}

bool clockSynced() { return platformTimeValid(); }

bool clockTrusted() {
  return s_haveSync && platformTimeValid() &&
         (uint32_t)(millis() - s_lastSyncMs) <= NIGHT_NTP_TRUST_MS;
}

bool clockNow(struct tm& out) {
  if (!platformTimeValid()) return false;
  time_t now = time(nullptr);
  localtime_r(&now, &out);
  return true;
}

void clockService(const Settings& s) {
  if (!s.clock.nightEnabled) {
    s_nightLatched = s_nightActive = s_nightHeld = false;
    s_lastResyncMs = 0;
    return;
  }
  struct tm t;
  bool inWindow = clockNow(t) &&
      nightWindowContains(t.tm_hour * 60 + t.tm_min,
                          (int)s.clock.nightStartMin, (int)s.clock.nightEndMin);
  if (!inWindow) {
    s_nightLatched = s_nightActive = s_nightHeld = false;
    s_lastResyncMs = 0;
    return;
  }
  if (s_nightLatched) {
    s_nightActive = true;
    s_nightHeld   = false;
    return;
  }
  if (clockTrusted()) {
    s_nightLatched = true;
    s_nightActive  = true;
    s_nightHeld    = false;
    return;
  }
  s_nightActive = false;
  s_nightHeld   = true;
  if (s_lastResyncMs == 0 || (uint32_t)(millis() - s_lastResyncMs) >= NIGHT_NTP_RESYNC_MS) {
    s_lastResyncMs = millis() | 1;
    clockForceResync(s);
  }
}

bool clockNightActive() { return s_nightActive; }
bool clockNightHeld()   { return s_nightHeld; }

String clockTimeStr() {
  struct tm t;
  if (!clockNow(t)) return String();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);
  return String(buf);
}
