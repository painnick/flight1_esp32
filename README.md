# Flight1 ESP32-C3 LED Controller

비행기 조명 제어기 - ESP-IDF 5.5.2 기반

## 개요

Lolin C3 Mini (ESP32-C3) 보드를 사용하여 비행기 모형의 various 조명을 제어하는 펌웨어입니다.

## 하드웨어

- **보드**: Lolin C3 Mini (ESP32-C3)
- **CPU 클럭**: 80MHz (저전력 모드)
- **입력**: 버튼 (GPIO 3, 내부 풀업)
- **출력**: LED 7개 (GPIO 0, 1, 4, 5, 6, 7, 10)

### GPIO 배치

| 핀 | 기능 | 설명 |
|----|------|------|
| GPIO 0 | STROBE1 | 스트로브 LED 1 |
| GPIO 1 | STROBE2 | 스트로브 LED 2 |
| GPIO 3 | BUTTON | 버튼 입력 (풀업) |
| GPIO 4 | NAVI | 네비게이션 LED |
| GPIO 5 | LOGO | 로고 LED |
| GPIO 6 | LANDING | 착지등 LED |
| GPIO 7 | NOT_USED | 미사용 |
| GPIO 10 | BEACON | 비컨 LED |

## 기능

버튼을 눌러 6가지 모드를 순환합니다.

| Step | 모드 | 동작 |
|------|------|------|
| 0 | NAVI | 네비게이션 LED 상시 켜기 |
| 1 | STROBE | 스트로브 1,2 번갈아 깜빡이기 (1250~1300ms 랜덤) |
| 2 | BEACON | 비컨 LED 깜빡이기 (1초 주기) |
| 3 | LOGO | 로고 LED 상시 켜기 |
| 4 | LANDING | 착지등 LED 상시 켜기 |
| 5 | ALL OFF | 모든 LED 끄기 (기본값) |

### 스트로브 동작

- 스트로브1과 스트로브2는 서로 다른 주기로 동작 (1250~1300ms 랜덤)
- 각 스트로브 ON 지속 시간: 70ms
- 자연스러운 깜빡임 효과를 위한 랜덤 주기 적용

### 비컨 동작

- 주기: 1초
- ON 지속 시간: 70ms

## 버튼 설정

- **입력 방식**: GPIO 풀업 (버튼 눌림 시 LOW)
- **디바운스 시간**: 200ms
- **감지 방식**: 하강 엣지 (HIGH → LOW 전이)

## 빌드 및 플래시

### 환경 설정

```bash
# ESP-IDF 환경 초기화 (Windows)
C:\Users\<username>\esp\esp-idf\export.ps1

# 또는 Linux/Mac
source ~/esp/esp-idf/export.sh
```

### 빌드

```bash
cd flight1_esp32_idf
idf.py build
```

### 플래시 및 모니터링

```bash
# 포트 지정 (Windows 예: COM3, Linux 예: /dev/ttyUSB0)
idf.py -p COM3 flash monitor
```

### 메뉴 설정 (선택)

```bash
idf.py menuconfig
```

## WiFi/Bluetooth

전원 소비 절감을 위해 WiFi와 Bluetooth가 기본적으로 비활성화되어 있습니다.

## 로깅

- Develop 모드: INFO 레벨 로그 출력
- 3초마다 현재 step 상태 로그 출력

## 파일 구조

```
flight1_esp32_idf/
├── CMakeLists.txt          # 루트 빌드 파일
├── sdkconfig.defaults      # SDK 설정 (ESP32-C3, 80MHz)
├── .gitignore
└── main/
    ├── CMakeLists.txt      # 메인 컴포넌트 빌드 파일
    └── main.c              # 메인 제어 로직
```
