#include <Arduino.h>
#include <esp_log.h>

#undef CONFIG_BT_ENABLED

#include "esp_wifi.h"
#include "esp_bt.h"

#define MAIN_TAG "Main"

// LED가 연결된 핀 번호
#define NAVI_PIN 4
#define STROBE1_PIN 0
#define STROBE2_PIN 1
#define BEACON_PIN 3
#define LOGO_PIN 5
#define LANDING_PIN 6
#define NOT_USED_PIN 7
// 버튼 핀 번호
#define BUTTON_PIN 10

// 각 스트로브의 주기와 켜진 시간 (밀리초)
unsigned long STROBE1_PERIOD_MS = 1250; // 1.25초마다 반복
unsigned long STROBE2_PERIOD_MS = 1300; // 1.4초마다 반복
static constexpr unsigned long STROBE_ON_DURATION_MS = 70; // 100ms 동안 켜짐
static unsigned long lastStrobe1On = 300;
static unsigned long lastStrobe2On = 0;

unsigned long BEACON_PERIOD_MS = 1000; // 1초마다 반복
static constexpr unsigned long BEACON_ON_DURATION_MS = 70; // 100ms 동안 켜짐

void setup() {
  setCpuFrequencyMhz(80);

  esp_wifi_stop();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();

  pinMode(NAVI_PIN, OUTPUT);
  pinMode(STROBE1_PIN, OUTPUT);
  pinMode(STROBE2_PIN, OUTPUT);
  pinMode(BEACON_PIN, OUTPUT);
  pinMode(LOGO_PIN, OUTPUT);
  pinMode(LANDING_PIN, OUTPUT);
  pinMode(NOT_USED_PIN, OUTPUT); // 미사용 핀
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 버튼은 내부 풀업 사용, 버튼은 GND와 연결

  digitalWrite(NAVI_PIN, LOW);
  digitalWrite(STROBE1_PIN, LOW);
  digitalWrite(STROBE2_PIN, LOW);
  digitalWrite(BEACON_PIN, LOW);
  digitalWrite(LOGO_PIN, LOW);
  digitalWrite(LANDING_PIN, LOW);
  digitalWrite(NOT_USED_PIN, LOW);

  ESP_LOGI(MAIN_TAG, "Start!");
}

void loop() {
  static unsigned long lastBeaconOn = 0;
  static bool strobe1Active = false;
  static bool strobe2Active = false;
  static bool beaconActive = false;
  static int step = 0; // 0~5 값 유지
  static bool prevButtonState = HIGH; // 풀업이므로 유휴 상태 HIGH
  static unsigned long lastButtonEventMs = 0; // 디바운스용
  static int prevStep = -1; // step 변화 감지용
  static bool strobeEnable = false; // STROBE 동작 여부 플래그
  static bool beaconEnable = false; // BEACON 동작 여부 플래그

  const unsigned long now = millis();

  // step 변화시 출력 제어 (이전 step값과 비교)
  if (step != prevStep) {
    switch (step) {
      case 0: // NAVI ON
        digitalWrite(NAVI_PIN, HIGH);
        break;
      case 1: // STROBE 1,2만 깜박 (loop에서 제어)
        strobeEnable = true;
        break;
      case 2: // BEACON ON
        beaconEnable = true;
        break;
      case 3: // LOGO ON
        digitalWrite(LOGO_PIN, HIGH);
        break;
      case 4: // LANDING ON
        digitalWrite(LANDING_PIN, HIGH);
        break;
      case 5: // ALL OFF
        digitalWrite(NAVI_PIN, LOW);
        digitalWrite(STROBE1_PIN, LOW);
        digitalWrite(STROBE2_PIN, LOW);
        digitalWrite(BEACON_PIN, LOW);
        digitalWrite(LOGO_PIN, LOW);
        digitalWrite(LANDING_PIN, LOW);
        strobeEnable = false;
        beaconEnable = false;
        break;
    }
    ESP_LOGI(MAIN_TAG, "Step : %d", step);
    prevStep = step;
  }

  // 버튼 처리: 풀업 기준 눌림은 LOW. 하강 엣지 + 디바운스(200ms)
  const bool curButtonState = digitalRead(BUTTON_PIN);
  if (prevButtonState == HIGH && curButtonState == LOW && (now - lastButtonEventMs) > 200) {
    lastButtonEventMs = now;
    step = (step + 1) % 6; // 0..5, 5 다음은 0
  }
  prevButtonState = curButtonState;

  // STROBE1,2 출력 제어 (step이 아닌 strobeEnable로)
  if (strobeEnable) {
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
  } else {
    digitalWrite(STROBE1_PIN, LOW);
    digitalWrite(STROBE2_PIN, LOW);
  }

  // BEACON 처리
  if (beaconEnable) {
    if (!beaconActive && (now - lastBeaconOn >= BEACON_PERIOD_MS)) {
      beaconActive = true;
      digitalWrite(BEACON_PIN, HIGH);
      lastBeaconOn = now;
    } else if (beaconActive && (now - lastBeaconOn >= BEACON_ON_DURATION_MS)) {
      beaconActive = false;
      digitalWrite(BEACON_PIN, LOW);
    }
  }
}
