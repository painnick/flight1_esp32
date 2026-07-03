#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_sleep.h"

static const char *TAG = "Main";

// LED 핀 번호
#define NAVI_PIN        GPIO_NUM_4
#define STROBE1_PIN     GPIO_NUM_0
#define STROBE2_PIN     GPIO_NUM_1
#define BEACON_PIN      GPIO_NUM_10
#define LOGO_PIN        GPIO_NUM_5
#define LANDING_PIN     GPIO_NUM_6
#define NOT_USED_PIN    GPIO_NUM_7

// 버튼 핀 번호
#define BUTTON_PIN      GPIO_NUM_3

// 버튼 디바운스 (마이크로초)
#define BUTTON_DEBOUNCE_US      (200 * 1000)   // 200ms

// 스트로브 타이밍 (마이크로초)
#define STROBE_ON_DURATION_US  (70 * 1000)
#define STROBE1_PERIOD_MIN_US  (1250 * 1000)
#define STROBE1_PERIOD_MAX_US  (1300 * 1000)
#define STROBE2_PERIOD_MIN_US  (1250 * 1000)
#define STROBE2_PERIOD_MAX_US  (1300 * 1000)

// 스트로브 시작 오프셋 (마이크로초)
#define STROBE_START_OFFSET_MIN_US  (200 * 1000)
#define STROBE_START_OFFSET_MAX_US  (500 * 1000)

// 비컨 타이밍 (마이크로초)
#define BEACON_PERIOD_US       (1000 * 1000)
#define BEACON_ON_DURATION_US  (70 * 1000)

// 현재 단계 (0~5)
static volatile int step = 5;

// GPIO 출력 초기화
static void gpio_output_init(gpio_num_t pin)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

// GPIO 입력 초기화 (버튼용, 풀업)
static void gpio_input_init(gpio_num_t pin)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

// LED 전체 끄기
static void all_leds_off(void)
{
    gpio_set_level(NAVI_PIN, 0);
    gpio_set_level(STROBE1_PIN, 0);
    gpio_set_level(STROBE2_PIN, 0);
    gpio_set_level(BEACON_PIN, 0);
    gpio_set_level(LOGO_PIN, 0);
    gpio_set_level(LANDING_PIN, 0);
}

// 메인 제어 태스크
static void main_task(void *arg)
{
    int64_t last_strobe1_on = 0;
    int64_t last_strobe2_on = 0;
    int64_t last_beacon_on = 0;

    bool strobe1_active = false;
    bool strobe2_active = false;
    bool beacon_active = false;

    bool strobe_enable = false;
    bool beacon_enable = false;

    int prev_step = 5;

    int64_t strobe1_period_us = STROBE1_PERIOD_MIN_US;
    int64_t strobe2_period_us = STROBE2_PERIOD_MIN_US;

    // 버튼 디바운스
    int64_t last_button_event_us = 0;
    bool prev_button_state = true;  // 풀업이므로 유휴 상태 HIGH

    ESP_LOGI(TAG, "Start!");

    while (1) {
        int64_t now_us = esp_timer_get_time();

        // 단계 변화시 출력 제어
        if (step != prev_step) {
            all_leds_off();
            strobe_enable = false;
            beacon_enable = false;

            switch (step) {
                case 0: // NAVI ON
                    gpio_set_level(NAVI_PIN, 1);
                    break;
                case 1: // STROBE 1,2만 깜박
                    strobe_enable = true;
                    {
                        int64_t offset = esp_random() % (STROBE_START_OFFSET_MAX_US - STROBE_START_OFFSET_MIN_US + 1) + STROBE_START_OFFSET_MIN_US;
                        last_strobe1_on = now_us - offset;
                        last_strobe2_on = now_us;
                    }
                    break;
                case 2: // BEACON ON
                    beacon_enable = true;
                    break;
                case 3: // LOGO ON
                    gpio_set_level(LOGO_PIN, 1);
                    break;
                case 4: // LANDING ON
                    gpio_set_level(LANDING_PIN, 1);
                    break;
                case 5: // ALL OFF
                    break;
            }
            ESP_LOGI(TAG, "Step : %d", step);
            prev_step = step;
        }

        // ========================================
        // 버튼 처리: 풀업 기준 눌림은 LOW
        // 하강 엣지 감지 + 디바운스
        // ========================================
        bool cur_button_state = gpio_get_level(BUTTON_PIN);
        if (prev_button_state && !cur_button_state &&
            (now_us - last_button_event_us) > BUTTON_DEBOUNCE_US) {
            last_button_event_us = now_us;
            step = (step + 1) % 6;  // 0..5, 5 다음은 0
        }
        prev_button_state = cur_button_state;

        // STROBE1 처리
        if (strobe_enable) {
            if (!strobe1_active && (now_us - last_strobe1_on >= strobe1_period_us)) {
                strobe1_active = true;
                gpio_set_level(STROBE1_PIN, 1);
                last_strobe1_on = now_us;
            } else if (strobe1_active && (now_us - last_strobe1_on >= STROBE_ON_DURATION_US)) {
                strobe1_active = false;
                strobe1_period_us = esp_random() % (STROBE1_PERIOD_MAX_US - STROBE1_PERIOD_MIN_US + 1) + STROBE1_PERIOD_MIN_US;
                gpio_set_level(STROBE1_PIN, 0);
            }
            // STROBE2 처리
            if (!strobe2_active && (now_us - last_strobe2_on >= strobe2_period_us)) {
                strobe2_active = true;
                gpio_set_level(STROBE2_PIN, 1);
                last_strobe2_on = now_us;
            } else if (strobe2_active && (now_us - last_strobe2_on >= STROBE_ON_DURATION_US)) {
                strobe2_active = false;
                strobe2_period_us = esp_random() % (STROBE2_PERIOD_MAX_US - STROBE2_PERIOD_MIN_US + 1) + STROBE2_PERIOD_MIN_US;
                gpio_set_level(STROBE2_PIN, 0);
            }
        } else {
            gpio_set_level(STROBE1_PIN, 0);
            gpio_set_level(STROBE2_PIN, 0);
        }

        // BEACON 처리
        if (beacon_enable) {
            if (!beacon_active && (now_us - last_beacon_on >= BEACON_PERIOD_US)) {
                beacon_active = true;
                gpio_set_level(BEACON_PIN, 1);
                last_beacon_on = now_us;
            } else if (beacon_active && (now_us - last_beacon_on >= BEACON_ON_DURATION_US)) {
                beacon_active = false;
                gpio_set_level(BEACON_PIN, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms 대기
    }
}

void app_main(void)
{
    // GPIO 출력 초기화
    gpio_output_init(NAVI_PIN);
    gpio_output_init(STROBE1_PIN);
    gpio_output_init(STROBE2_PIN);
    gpio_output_init(BEACON_PIN);
    gpio_output_init(LOGO_PIN);
    gpio_output_init(LANDING_PIN);
    gpio_output_init(NOT_USED_PIN);

    // 버튼 입력 초기화 (풀업)
    gpio_input_init(BUTTON_PIN);

    // 모든 LED 끄기
    all_leds_off();

    // 메인 제어 태스크 생성
    xTaskCreate(main_task, "main_task", 4096, NULL, 5, NULL);
}
