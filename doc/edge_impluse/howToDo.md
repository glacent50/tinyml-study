# Edge Impulse로 "red, green, blue, one, two, three" 음성 분류하여 LED 제어하기 (How to do it)

본 문서는 TinyML Cookbook 4장 "Voice Controlling LEDs with Edge Impulse"의 "How to do it" 예제를 바탕으로, 영어 명령어 6개(red, green, blue, one, two, three)를 인식해 LED를 제어하는 전체 과정을 한국어로 자세히 정리하고, 실무 팁과 보완 정보를 추가했습니다.

## 무엇을 만들까요?

- 6가지 영어 단어를 실시간으로 인식합니다: red, green, blue, one, two, three
- RGB LED를 제어합니다.
  - red/green/blue: 해당 색 LED를 켬
  - one/two/three: 밝기 단계를 3단계(예: 33%/66%/100%)로 조절
- Edge Impulse로 데이터 수집 → 특징 추출(MFCC) → 소형 NN 학습 → Arduino 라이브러리로 배포 → 마이크 실시간 추론 → LED 제어

## 준비물

- 하드웨어(예시)
  - Arduino Nano 33 BLE Sense(내장 마이크, RGB LED 보유) 또는 유사 보드
  - 외장 RGB LED(필요 시)와 저항, 브레드보드, 점퍼선
- 소프트웨어/서비스
  - Edge Impulse Studio 계정
  - Edge Impulse CLI(PC에 설치)
  - Arduino IDE 또는 Arduino CLI

## 라벨과 동작 매핑

| 라벨(영어) | 한국어 의미 | 기본 동작 | 비고 |
|---|---|---|---|
| red | 빨강 | 빨간 LED 켬 | 보드 내장 LEDR 또는 외장 R핀 |
| green | 초록 | 초록 LED 켬 | LEDG |
| blue | 파랑 | 파란 LED 켬 | LEDB |
| one | 숫자 1 | 밝기 33% | PWM 사용 권장 |
| two | 숫자 2 | 밝기 66% |  |
| three | 숫자 3 | 밝기 100% |  |

참고: Nano 33 BLE Sense의 내장 RGB LED는 보통 논리 반전(LOW=켜짐)입니다. 보드에 따라 핀명/극성이 다르므로 보드 핀 정의를 확인하세요.

---

## 1. Edge Impulse 프로젝트 생성 및 장치 연결

1) Edge Impulse Studio에서 새 프로젝트 생성(예: "Voice RGB Control").
2) PC에 Edge Impulse CLI 설치(Windows CMD):

```bat
npm install -g edge-impulse-cli
edge-impulse-daemon
```

3) 장치 목록에서 보드를 선택하여 연결합니다. 내장 마이크가 있는 보드는 별도 드라이버 없이 바로 연결되는 경우가 많습니다.

팁: 보드 연결이 불안정하면 USB 케이블 교체, 전원이 충분한 포트 사용, 드라이버 업데이트를 점검하세요.

---

## 2. 데이터 수집(수동 녹음 + 보강)

권장 데이터 계획(최소 기준):

| 클래스 | 목표 샘플 수 | 각 샘플 길이 | 비고 |
|---|---:|---:|---|
| red | 40~60 | 1.0s | 다양한 화자/속도/억양 |
| green | 40~60 | 1.0s |  |
| blue | 40~60 | 1.0s |  |
| one | 40~60 | 1.0s |  |
| two | 40~60 | 1.0s |  |
| three | 40~60 | 1.0s |  |
| unknown | 60~100 | 1.0s | 다른 단어, 잡음 포함 |
| noise(또는 background) | 60~100 | 1.0s | 무음, 생활소음 |

- 녹음 팁
  - 마이크와 입 사이 거리 10~20cm, 주변 잡음 다양한 조건(조용/시끄러움) 모두 포함
  - 화자 다양화: 본인 외 1~2명 데이터 추가 시 일반화가 크게 개선
  - 단어 시작/끝에 무음이 너무 길지 않도록, 샘플 간 볼륨 편차 확보

- 데이터 분할
  - Studio의 자동 분할(Train/Test, 예: 80/20) 활성화
  - 클래스 불균형이 심하면 수집량을 늘리거나 Data Augmentation 사용

### 모바일폰(QR)로 데이터 수집

Edge Impulse는 스마트폰을 원격 수집 장치로 연결하는 QR 기반 흐름을 제공합니다. 보드 없이도 빠르게 다양한 화자/환경 데이터를 모을 수 있어 매우 유용합니다.

절차

1) Edge Impulse Studio에서 프로젝트를 연 뒤, 상단 메뉴의 Devices 또는 Data acquisition 화면으로 이동합니다.
2) "Use your mobile phone" 또는 "Connect a mobile phone" 항목을 선택하여 QR 코드를 표시합니다.
3) 스마트폰에서 다음 중 하나를 사용합니다.
  - Edge Impulse Mobile App(iOS/Android)을 설치 후 실행 → 앱 내에서 QR 코드로 프로젝트 연결
  - 또는 브라우저에서 https://edgeimpulse.com/phone 접속 → 카메라로 QR을 스캔해 프로젝트에 링크
4) 스마트폰이 Studio의 Devices 목록에 표시되면, Data acquisition 탭에서 해당 기기를 선택합니다.
5) 레이블(label), 길이(예: 1.0s), 샘플레이트(16 kHz)를 설정하고 Start sampling으로 녹음합니다.
6) 녹음이 완료되면 샘플이 자동으로 프로젝트 데이터셋에 업로드됩니다.

권장 설정/팁

- 조용/시끄러운 환경을 모두 포함해 다양한 조건에서 수집하세요.
- 화자를 늘리려면 QR를 각각의 사람에게 공유해, 각자의 스마트폰으로 수집하게 하세요(프로젝트 권한 필요).
- 스마트폰 마이크 게인을 과도하게 높이지 말고, 피크가 잘리지 않도록(클리핑 방지) 20~30cm 거리에서 발화합니다.
- Edge Impulse 앱 내 미리보기 파형으로 입력 레벨을 확인하고, 너무 작거나 큰 경우 간격/목소리 크기를 조정합니다.

간단 비교: 보드 직접 녹음 vs 모바일폰(QR)

| 방법 | 장점 | 단점 | 권장 상황 |
|---|---|---|---|
| 보드 직접 | 타깃 마이크 특성 그대로, 배포 후 성능과 유사 | 이동/다인원 수집 불편, 연결 의존성 | 최종 디바이스 환경에 근접 평가 필요할 때 |
| 모바일폰(QR) | 빠른 다인원/다환경 수집, 장치 무관 | 마이크 특성 차이, 도메인 갭 가능 | 데이터 확충/초기 프로토타입 단계 |

모바일 수집 문제 해결

- QR 스캔 후 연결이 안 되는 경우: 로그인 계정/프로젝트 권한 확인, 네트워크 상태 점검, 다른 브라우저 시도
- 마이크 권한 요청 거부됨: 브라우저/앱의 마이크 권한을 설정에서 허용
- 업로드 지연: Wi‑Fi 환경 전환, 샘플 길이 축소, 일시 저장 후 재전송

---

## 3. 임펄스 설계(Audio → MFCC → Classifier)

- 입력 블록: Audio (마이크) 16 kHz, 1.0s 윈도우
- DSP 블록: MFCC
  - 예시 파라미터
    - 프레임 길이: 40ms, 프레임 이동: 20ms
    - 멜 필터 뱅크 수: 32~40
    - 계수: 13~20(데바이스 성능에 따라 조정)
    - 전처리: 정규화/노이즈 감쇠 옵션 검토
- 학습 블록: 분류기(Classifier)
  - 소형 CNN 또는 Dense 기반 네트워크(메모리 한도 내 파라미터 최소화)
  - 출력 클래스: red, green, blue, one, two, three, unknown, noise

팁: EON Tuner를 사용하면 DSP/모델 조합을 자동 탐색하여 정확도/메모리 균형을 찾아줍니다.

---

## 4. 모델 학습과 검증

- 학습 설정
  - 에폭: 30~100(과적합 시 Early stopping)
  - 배치 크기: 16~64
  - 옵티마이저: Adam, 학습률 1e-3 근방에서 시작
- 검증
  - Confusion Matrix로 클래스 간 혼동(특히 two↔three, red↔green↔blue)을 확인
  - Per-class accuracy, F1 점검
  - Live classification로 실제 마이크 입력 테스트
- 후처리/스무딩
  - 슬라이딩 윈도우 평균, 최소 신뢰도 임계치 설정(예: 0.7)
  - 명령 유지 시간(예: N 프레임 중 M 프레임 이상이 동일 클래스면 확정)

---

## 5. 배포(Arduino 라이브러리)

- Deployment에서 "Arduino library" 선택 후 다운로드
- Arduino IDE에 ZIP 라이브러리 추가(스케치 → 라이브러리 포함하기 → .zip 라이브러리 추가)
- 제공 예제의 inferencing 코드 스니펫을 스케치에 통합

메모리 팁: RAM/Flash가 부족하면
- MFCC 밴드/계수 축소
- 모델 레이어/유닛 축소
- quantized int8 모델 사용

---

## 6. Arduino 스케치(로직 개요)

아래는 통합 로직 개요입니다(보드/라이브러리 생성물에 맞게 네임스페이스와 API는 조정).

```cpp
// ... Edge Impulse에서 생성한 헤더 포함 ...
// #include <edge-impulse-sdk/classifier/ei_run_classifier.h>

// 보드별 핀 설정 (예: Nano 33 BLE Sense 내장 RGB)
#define LED_R LEDR
#define LED_G LEDG
#define LED_B LEDB

// 논리 반전 여부
const bool LED_INVERTED = true; // LOW=켜짐

void setLed(bool r, bool g, bool b) {
  auto w = [&](int pin, bool on){
    if (LED_INVERTED) digitalWrite(pin, on ? LOW : HIGH);
    else digitalWrite(pin, on ? HIGH : LOW);
  };
  w(LED_R, r); w(LED_G, g); w(LED_B, b);
}

void setBrightness(uint8_t level /*0~255*/){
  // PWM 가능한 핀 또는 내장 LED의 PWM 지원 여부 확인 필요
  // analogWrite(LED_R, value) 등 보드에 맞게 조정
}

void setup() {
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setLed(false,false,false);
  // 마이크/모델 초기화
}

void loop() {
  // 1) 오디오 1s 캡처 → 2) MFCC/추론 → 3) 결과 해석
  // ei_impulse_result_t result;  // 추론 결과
  // run_classifier(&signal, &result, false /*debug*/);

  // 4) 신뢰도/스무딩 적용 후 클래스 결정
  // 5) LED 제어
  //   - red/green/blue: 해당 색 켬
  //   - one/two/three: 밝기 33/66/100%
}
```

클래스 매핑 예시:

| 클래스 | 조건(신뢰도) | 동작 |
|---|---|---|
| red | prob["red"] ≥ 0.7 | setLed(true,false,false) |
| green | prob["green"] ≥ 0.7 | setLed(false,true,false) |
| blue | prob["blue"] ≥ 0.7 | setLed(false,false,true) |
| one | prob["one"] ≥ 0.7 | setBrightness(85) |
| two | prob["two"] ≥ 0.7 | setBrightness(170) |
| three | prob["three"] ≥ 0.7 | setBrightness(255) |

주의: 빠르게 연속 인식 시 플리커링이 생길 수 있으므로, 200~500ms의 방아쇠(hold) 시간 또는 히스테리시스를 두면 안정적입니다.

---

## 7. 정확도 향상 팁(추가 조사)

- 키워드 발음 다양화: 미국/영국 억양, 빠르게/천천히, 큰소리/작은소리
- 잡음 강건성: 생활 소음(키보드, 선풍기, 음악) 위에서 재녹음; Noise 레이블 확충
- 음성 전처리: VAD(음성 활동 감지)를 이용해 비음성 구간을 줄이면 혼동이 감소
- 클래스 축소/통합: 초기에는 3~4 클래스부터 시작해 점진 확장
- 온디바이스 프로파일링: Edge Impulse의 Latency/Memory 프로파일러로 타깃 보드 실행 시간 확인

---

## 8. 문제 해결 체크리스트

- 마이크 입력이 0 또는 노이즈뿐: 샘플레이트/게인 설정 확인, 주변 소음 상태 점검
- 특정 단어만 인식 저하: 해당 클래스 데이터 증강, 녹음 품질 개선, 발음 예시 다양화
- 메모리 부족 에러: MFCC/모델 축소, int8 양자화, 불필요한 로그 제거
- 내장 RGB LED가 반대로 동작: 보드의 논리 반전(LOW=켜짐) 여부 확인 후 코드 조정

---

## 부록 A. Windows에서 빠른 실행 흐름(CMD)

```bat
:: 1) Edge Impulse CLI 설치 및 장치 연결
npm install -g edge-impulse-cli
edge-impulse-daemon

:: 2) Edge Impulse Studio에서 데이터 수집 → 임펄스 구성(MFCC+Classifier) → 학습 → Live test

:: 3) Deployment: Arduino library 다운로드 → Arduino IDE에 ZIP 라이브러리 추가

:: 4) 예제 스케치 수정: 클래스별 LED/밝기 로직 매핑 → 업로드
```

---

## 요약

- 6개 영어 명령(red, green, blue, one, two, three)을 Edge Impulse로 인식하고, RGB LED 색/밝기를 제어
- 핵심은 충분하고 다양한 데이터, 적절한 MFCC/모델 크기, 스무딩과 임계치 조합
- 배포는 Arduino 라이브러리로 간편하며, 보드별 핀/논리 반전에 맞춰 LED 코드를 보정하면 됩니다.

---

## 보드 전용 가이드: Arduino Nano 33 BLE Sense Rev2

이 절에서는 Arduino Nano 33 BLE Sense Rev2 보드에 맞춘 구체 설정과 코드 예시를 제공합니다.

### 하드웨어 요약(Rev2)

- 내장 PDM 마이크(보드에 장착됨)
- 내장 RGB LED 핀: `LEDR`, `LEDG`, `LEDB`
- LED 논리: Active-Low (LOW=켜짐, HIGH=꺼짐)
- PWM: 내장 RGB LED는 `analogWrite()` 지원. Active-Low이므로 듀티 반전 필요

표: 내장 RGB LED 제어 요약

| 핀 | 의미 | 논리 | PWM | 비고 |
|---|---|---|---|---|
| LEDR | 빨강 | LOW=ON | 지원 | `analogWrite(LEDR, 255 - value)` |
| LEDG | 초록 | LOW=ON | 지원 | `analogWrite(LEDG, 255 - value)` |
| LEDB | 파랑 | LOW=ON | 지원 | `analogWrite(LEDB, 255 - value)` |

참고: 외장 RGB LED를 사용할 경우, 보드의 PWM 지원 핀을 사용하고(핀아웃 문서 참고), 공통 애노드/캐소드에 맞춰 회로를 구성하세요. 내장 LED 사용이 가장 간단합니다.

### 마이크(오디오 입력) 메모

- Rev2는 PDM 마이크를 사용합니다. Edge Impulse Arduino 라이브러리의 예제는 Rev2에서 PDM을 자동 설정합니다.
- 별도로 직접 테스트할 경우, Arduino의 PDM 라이브러리를 이용해 16 kHz로 설정하세요.

예시(마이크 입력 로우레벨 확인용, EI와 별개 테스트):

```cpp
#include <PDM.h>

volatile int samplesRead = 0;
short sampleBuffer[1024];

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2; // 16-bit samples
}

void setup() {
  Serial.begin(115200);
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) { // mono, 16 kHz
    Serial.println("PDM begin failed");
    while (1) {}
  }
}

void loop() {
  if (samplesRead) {
    Serial.print("samples: "); Serial.println(samplesRead);
    samplesRead = 0;
  }
}
```

### LED 제어 유틸(Active-Low PWM 반영)

```cpp
#define LED_R LEDR
#define LED_G LEDG
#define LED_B LEDB

inline void ledWrite(int pin, uint8_t value /*0~255, 255=밝음*/){
  // Active-Low 보정: 값이 클수록 더 어둡게 되는 것을 방지
  analogWrite(pin, 255 - value);
}

void setLedRGB(uint8_t r, uint8_t g, uint8_t b){
  // 각 채널 0~255, 0=꺼짐, 255=최대 밝기
  ledWrite(LED_R, r);
  ledWrite(LED_G, g);
  ledWrite(LED_B, b);
}

void setupLed(){
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLedRGB(0,0,0);
}
```

### 명령 매핑(색상 + 밝기 단계)

다음 로직은 "red/green/blue"로 현재 색을 설정하고, "one/two/three"로 밝기를 33%/66%/100%로 조절합니다.

```cpp
enum LedColor { NONE=0, RED=1, GREEN=2, BLUE=3 };

LedColor currentColor = RED;
uint8_t currentBrightness = 85; // 33%

void applyLed(){
  uint8_t r=0,g=0,b=0;
  switch(currentColor){
    case RED:   r = currentBrightness; break;
    case GREEN: g = currentBrightness; break;
    case BLUE:  b = currentBrightness; break;
    default: break;
  }
  setLedRGB(r,g,b);
}

void onCommand(const String& label){
  if (label == "red")   { currentColor = RED;   applyLed(); }
  else if (label == "green") { currentColor = GREEN; applyLed(); }
  else if (label == "blue")  { currentColor = BLUE;  applyLed(); }
  else if (label == "one")   { currentBrightness = 85;  applyLed(); }
  else if (label == "two")   { currentBrightness = 170; applyLed(); }
  else if (label == "three") { currentBrightness = 255; applyLed(); }
}
```

### Edge Impulse 추론 루프 연결(개요)

Edge Impulse에서 생성된 Arduino 라이브러리를 포함하고, 마이크 프레임을 캡처해 `run_classifier`에 전달합니다. 라이브러리 버전에 따라 네임스페이스/헬퍼가 다를 수 있으므로, 배포 ZIP의 예제("nano_ble_sense" 대상) 코드를 기본으로 하되 아래 포인트를 반영하세요.

핵심 포인트

- 신뢰도 임계치: 0.7 권장(프로젝트에 맞게 조정)
- 스무딩: 연속 N 프레임 중 M 프레임 이상 동일 클래스일 때 확정
- 레이블 처리: `onCommand(topLabel)`로 연결

의사 코드:

```cpp
// #include <edge-impulse-sdk/classifier/ei_run_classifier.h>
// #include "microphone_inference.h" // Edge Impulse 예제 제공 시

void setup(){
  Serial.begin(115200);
  setupLed();
  // EI 마이크/모델 초기화 (예제에 따름)
}

void loop(){
  // 1) 오디오 캡처 → 2) run_classifier 호출 → 3) 결과 파싱
  // ei_impulse_result_t result;
  // run_classifier(&signal, &result, false);

  // 가장 높은 확률 레이블 선택
  // String topLabel; float topScore;
  // if (topScore >= 0.70) {
  //   onCommand(topLabel);
  // }

  // 소정의 대기 또는 다음 프레임 준비
}
```

테스트 팁

- Live classification에서 각 단어의 신뢰도를 모니터링하고, 임계치/스무딩 파라미터를 조정하세요.
- 색상 명령과 숫자 명령을 섞어서 발화하며 플리커가 없는지 확인하세요(필요 시 200~500ms 홀드 타임 적용).

