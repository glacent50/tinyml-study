# Chapter 4: Voice Controlling LEDs with Edge Impulse — 상세 요약

본 장은 Edge Impulse를 활용해 간단한 음성 명령을 온디바이스로 인식하고, LED를 제어하는 TinyML 워크플로우를 끝까지 구현하는 방법을 다룹니다. 데이터 수집(보드/모바일), 특징 추출, 소형 모델 학습, 검증, 임베디드 배포(Arduino), 온디바이스 후처리와 최적화까지 포함합니다.

## 1) 목표와 결과물
- 목표: “red, green, blue, one, two, three” 같은 단어를 온디바이스로 인식하고 RGB LED의 색/밝기를 제어
- 결과물: 마이크 입력을 실시간 분석해 특정 단어 확률이 임계치 이상이면 대응 동작 수행(LED ON/OFF, 색 변경, 밝기 단계 변경)

## 2) 준비물
- 하드웨어: 마이크 내장 보드(예: Arduino Nano 33 BLE Sense/Rev2), 또는 외장 마이크
- 소프트웨어: Edge Impulse Studio 계정, Edge Impulse CLI, Arduino IDE/CLI
- 데이터: 대상 단어(키워드)와 unknown/noise 클래스 샘플

## 3) 전체 흐름 한눈에 보기
| 단계 | 도구/장소 | 핵심 작업 |
|---|---|---|
| 데이터 수집 | 보드/스마트폰 | 키워드·unknown·noise 오디오 녹음 |
| 임펄스 설계 | Edge Impulse | Audio 입력 → MFCC → Classifier 구성 |
| 학습/검증 | Edge Impulse | 모델 학습, Confusion Matrix/Live test로 평가 |
| 배포 | Edge Impulse | Arduino library로 내보내기(quantized) |
| 온디바이스 | Arduino | 마이크 캡처 → 추론 → 스무딩/임계치 → LED 제어 |

## 4) 데이터 수집
- 클래스 구성: 키워드(red, green, blue, one, two, three) + unknown + noise(배경)
- 권장 샘플(최소 가이드)
  - 키워드 각 40~60개, unknown 60~100개, noise 60~100개, 길이 1.0s, 16 kHz
- 다양성: 화자(여러 명), 억양/속도/볼륨, 환경 소음(조용/시끄러움) 다양하게 확보
- 모바일폰(QR) 수집: Studio에서 QR로 스마트폰을 연결해 빠르게 다인원·다환경 수집 가능
  - 절차: 프로젝트 → Devices/Data acquisition → “Use your mobile phone” → QR 스캔 → 레이블/길이 설정 후 녹음
  - 주의: 마이크 권한 허용, 입력 레벨(클리핑/과소입력) 확인

## 5) 임펄스 설계(Audio → MFCC → Classifier)
- 입력: Audio 16 kHz, 윈도우 1.0s(슬라이딩 가능)
- DSP: MFCC(프레임 40ms/이동 20ms, 멜 밴드 32~40, 계수 13~20 등; 디바이스 자원에 맞춰 조정)
- 모델: 소형 CNN/Dense 기반 분류기, 출력은 모든 클래스(키워드+unknown+noise)
- 자동 탐색: EON Tuner로 DSP/모델 후보를 탐색해 정확도-메모리-지연 균형 최적화 가능

## 6) 학습과 평가
- 학습: Epoch 30~100, Batch 16~64, Adam(기본 lr 1e-3 등), 조기 종료 활용
- 평가: Confusion Matrix로 혼동(예: two↔three, red↔green↔blue) 확인, Per-class metrics 점검
- Live classification: 실제 마이크로 실시간 성능 확인(임계치 초안 수립)
- 데이터 불균형/노이즈 대응: Augmentation, 클래스별 수집량 보완, unknown/noise 강화

## 7) 배포와 온디바이스 통합
- 배포: Deployment → Arduino library(zip) 다운로드 → IDE로 설치
- 온디바이스 루프: 마이크 캡처 → run_classifier 호출 → 결과 확률 → 후처리(스무딩/임계치) → LED 제어
- 메모리/지연: Edge Impulse 프로파일러로 타깃 보드 메모리/추론 지연 확인
- 최적화: int8 양자화, MFCC 차원 축소, 레이어/유닛 축소, 로깅 최소화

## 8) 신뢰도 임계치와 스무딩
- 임계치: 예) 0.7 이상일 때만 명령 확정(프로젝트/환경에 맞게 조정)
- 스무딩: N프레임 중 M프레임 이상 동일 레이블일 때 확정(플리커 방지)
- 히스테리시스/홀드: 명령 확정 후 일정 시간 유지(200~500ms)로 빠른 전환/깜빡임 방지

## 9) LED 제어 정책(예)
- 색상: red/green/blue → 해당 채널만 활성
- 밝기 단계: one/two/three → 33%/66%/100% (PWM 사용)
- 보드 주의: 일부 보드는 내장 RGB LED가 Active-Low(LOW=켜짐)로 동작하므로 듀티 반전 필요

## 10) 문제 해결
- 인식률 저하(특정 단어): 해당 클래스 데이터 추가, 잡음/억양 다양화, 임계치 완화, VAD 도입
- 오탐/플리커: 스무딩 강화, 홀드 타임 적용, unknown/noise 데이터 확충
- 메모리 부족: MFCC/모델 축소, int8 양자화, 불필요한 출력 제거
- 하드웨어: 마이크 샘플레이트/게인 확인, 내장 LED 극성/핀맵 점검, 전원/케이블 문제 확인

## 11) 확장 아이디어
- 웨이크워드(예: “hey device”) + 명령어 2단계 구조로 오탐 감소
- 다국어 지원(한국어/영어 병행) 또는 사용자별 커스텀 명령 추가
- 더 많은 디바이스 제어(서보/모터/센서 트리거)로 확장

## 12) 권장 데이터 계획(표)
| 클래스 | 최소 샘플 수 | 길이 | 메모 |
|---|---:|---:|---|
| red/green/blue | 각 40~60 | 1.0s | 억양/속도/볼륨 다양화 |
| one/two/three | 각 40~60 | 1.0s | 숫자 발음 혼동(two/three) 주의 |
| unknown | 60~100 | 1.0s | 기타 단어/잡음 포함 |
| noise | 60~100 | 1.0s | 생활소음/무음 섞기 |

## 13) Windows 환경 메모(선택)
- Edge Impulse CLI 설치 및 연결(예)
```bat
npm install -g edge-impulse-cli
edge-impulse-daemon
```
- 이후 Studio에서 데이터 수집 → 임펄스 구성(MFCC+Classifier) → 학습/Live test → Arduino library 배포 순서로 진행

---
요약: 본 장은 키워드 스팟팅을 위한 실전 파이프라인을 구축하며, 데이터 다양성·스무딩·임계치·양자화·프로파일링이 온디바이스 정확도와 안정성의 핵심임을 보여줍니다. Edge Impulse의 도구 체인을 이용하면 데이터 수집부터 디바이스 배포까지 일관된 흐름으로 구현할 수 있습니다.
