#include <Arduino.h>

#undef CONFIG_BT_ENABLED

#include "esp_wifi.h"
#include "esp_bt.h"

#define MAIN_TAG "Main"

// LED가 연결된 핀 번호
#define STROBE1_PIN 5
#define STROBE2_PIN 4
#define BEACON_PIN 6

// 각 스트로브의 주기와 켜진 시간 (밀리초)
static const unsigned long BASE_PERIOD_MS = 1250;
unsigned long STROBE1_PERIOD_MS = 1250; // 1.25초마다 반복
unsigned long STROBE2_PERIOD_MS = 1300; // 1.4초마다 반복
static const unsigned long STROBE_ON_DURATION_MS = 70; // 100ms 동안 켜짐
static unsigned long lastStrobe1On = 300;
static unsigned long lastStrobe2On = 0;

unsigned long BEACON_PERIOD_MS = 1000; // 1초마다 반복
static const unsigned long BEACON_ON_DURATION_MS = 70; // 100ms 동안 켜짐

void setup() {
  ESP_LOGI(MAIN_TAG, "Setup...");

  esp_wifi_stop();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();

  pinMode(STROBE1_PIN, OUTPUT);
  pinMode(STROBE2_PIN, OUTPUT);
  pinMode(BEACON_PIN, OUTPUT);

  digitalWrite(STROBE1_PIN, LOW);
  digitalWrite(STROBE2_PIN, LOW);
  digitalWrite(BEACON_PIN, LOW);
}

void loop() {
  static unsigned long lastBeaconOn = 0;
  static bool strobe1Active = false;
  static bool strobe2Active = false;
  static bool beaconActive = false;

  const unsigned long now = millis();

  // STROBE1 처리
  if (!strobe1Active && (now - lastStrobe1On >= STROBE1_PERIOD_MS)) {
    strobe1Active = true;
    digitalWrite(STROBE1_PIN, HIGH);
    lastStrobe1On = now;
  } else if (strobe1Active && (now - lastStrobe1On >= STROBE_ON_DURATION_MS)) {
    strobe1Active = false;
    STROBE1_PERIOD_MS = random(1250, 1300);
    digitalWrite(STROBE1_PIN, LOW);
  }

  // STROBE2 처리
  if (!strobe2Active && (now - lastStrobe2On >= STROBE2_PERIOD_MS)) {
    strobe2Active = true;
    digitalWrite(STROBE2_PIN, HIGH);
    lastStrobe2On = now;
  } else if (strobe2Active && (now - lastStrobe2On >= STROBE_ON_DURATION_MS)) {
    strobe2Active = false;
    STROBE2_PERIOD_MS = random(1250, 1300);
    digitalWrite(STROBE2_PIN, LOW);
  }

  // BEACON 처리
  if (!beaconActive && (now - lastBeaconOn >= BEACON_PERIOD_MS)) {
    beaconActive = true;
    digitalWrite(BEACON_PIN, HIGH);
    lastBeaconOn = now;
  } else if (beaconActive && (now - lastBeaconOn >= BEACON_ON_DURATION_MS)) {
    beaconActive = false;
    digitalWrite(BEACON_PIN, LOW);
  }}
