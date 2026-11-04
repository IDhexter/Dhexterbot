// main.cpp — DhexterBot OS Firmware Entrypoint
// Repository: https://github.com/IDhexter/Dhexterbot
#include <Arduino.h>
#include "Platform.h"
#include "config.h"
#include "Settings.h"
#include "Net.h"
#include "Gfx.h"
#include "WebPortal.h"
#include "Mode.h"
#include "Clock.h"
#include "features/pet/PetMode.h"
#if WITH_NOTIFY
#include "NotifyMode.h"
#endif

static Settings g_settings;
static String   g_resetReason;
static bool     g_safeMode = false;
static char     g_epcStr[16]  = "";
static char     g_addrStr[16] = "";
static int      g_lastBr = -1;
static uint32_t g_btnWakeUntil = 0;
#if HAS_LDR
static uint32_t g_lastAutoBr = 0;
static uint8_t  g_ldrCache   = DEFAULT_BRIGHTNESS;
#endif

static uint8_t appEffectiveBrightness() {
  if (g_btnWakeUntil && millis() < g_btnWakeUntil) return g_settings.brightness;
  if (clockNightActive()) return g_settings.clock.nightLevel;
#if HAS_LDR
  if (g_settings.autoBrightness) {
    if (millis() - g_lastAutoBr > 2000) {
      g_lastAutoBr = millis();
      int raw = analogRead(LDR_PIN);
      g_ldrCache = (uint8_t)constrain(raw * 100 / ADC_MAX, 5, 100);
    }
    return g_ldrCache;
  }
#endif
  return g_settings.brightness;
}

void appApplyBrightness() {
  uint8_t t = appEffectiveBrightness();
  if ((int)t != g_lastBr) {
    gfxSetBrightness(t, g_settings.backlightInverted);
    g_lastBr = t;
  }
}

const char* appResetReason() { return g_resetReason.c_str(); }

void appInvalidate() {
  g_petMode.invalidate(g_settings);
}

void appRelease() {
  g_petMode.freeForRequest();
}

static void bootProgress(const char* msg) {
  gfxBoot("DhexterBot", msg);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(FW_NAME " " FW_VERSION);

  PlatformReset pr = platformResetInfo();
  Serial.print("[boot] reset reason: ");
  Serial.println(pr.reason);

  if (pr.wasCrash) {
    g_safeMode = true;
    strlcpy(g_epcStr,  pr.epc,  sizeof(g_epcStr));
    strlcpy(g_addrStr, pr.addr, sizeof(g_addrStr));
    char rich[80];
    snprintf(rich, sizeof(rich), "%s epc %s addr %s", pr.reason.c_str(),
             g_epcStr[0] ? g_epcStr : "-", g_addrStr[0] ? g_addrStr : "-");
    g_resetReason = rich;
  } else {
    g_resetReason = pr.reason;
  }

  settingsBegin();
  loadSettings(g_settings);

  gfxBegin(g_settings);
  gfxBoot(g_safeMode ? "SafeMode" : "DhexterBot", FW_VERSION);

  netAllowRetry(!g_safeMode);
  netBegin(g_settings, bootProgress);
  if (!g_safeMode) clockReapply(g_settings);

  webPortalBegin(g_settings);
  g_petMode.begin(g_settings);

  if (netMode() == NET_AP) {
    gfxApInfo(g_settings.apSsid.c_str(), g_settings.apPass.c_str(), netIP().c_str(), netFailReason());
  } else if (g_safeMode) {
    gfxCrash(g_epcStr, g_addrStr, netIP().c_str());
  } else {
    gfxStaInfo(netSSID().c_str(), netIP().c_str(), g_settings.hostname.c_str());
    delay(2200);
  }
}

void loop() {
  netLoop();
  webPortalLoop();

  if (webPortalRebootDue()) {
    delay(120);
    ESP.restart();
  }

  if (g_safeMode) {
    delay(5);
    return;
  }

  if (netMode() == NET_AP) {
    static uint16_t shownFail = netFailSeq();
    if (netFailSeq() != shownFail) {
      shownFail = netFailSeq();
      gfxApInfo(g_settings.apSsid.c_str(), g_settings.apPass.c_str(), netIP().c_str(), netFailReason());
    }
    delay(5);
    return;
  }

  clockService(g_settings);
  appApplyBrightness();

#if WITH_NOTIFY
  static bool wasNotifying = false;
  if (g_notifyMode.active()) {
    wasNotifying = true;
    g_notifyMode.service(g_settings);
    delay(5);
    return;
  }
  if (wasNotifying) {
    wasNotifying = false;
    g_petMode.wake(g_settings);
  }
#endif

  g_petMode.service(g_settings);
  delay(5);
}
