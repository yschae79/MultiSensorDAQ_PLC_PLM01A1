# PLC 통신 신뢰성 실험 계획

작성일: 2026-04-23
MCU: STM32F446RE (NUCLEO-F446RE × 2)
PLC 모뎀: ST7580 (X-NUCLEO-PLM01A1)
목적: 실 장비(센서/제어 장비 내 PLC 배선 통신) 투입 전 통신 신뢰성 사전 검증

---

## 개요

현재의 단순 P2P 루프(TRIGGER/ACK 무한 반복)를 **N회 반복 + 통계 수집 + LCD/UART 출력** 구조로 개선하고,
물리적·펌웨어 파라미터를 체계적으로 변경하면서 **PER(패킷 에러율, Packet Error Rate)**을 측정한다.

### 핵심 지표

| 지표 | 정의 |
|---|---|
| `TX_COUNT` | 총 전송 시도 횟수 |
| `RX_SUCCESS` | ACK 정상 수신 횟수 |
| `RX_FAIL_TMO` | ACK 타임아웃 실패 횟수 |
| `RX_FAIL_ERR` | 잘못된 ACK(ID 불일치 등) 횟수 |
| `PER` | `(TX_COUNT - RX_SUCCESS) / TX_COUNT × 100 [%]` |

---

## Phase 1 — 통계 수집 펌웨어 구현

### 1-1. `Core/Inc/c_plc_appli.h` 변경 사항

`PLC_Stats_t` 구조체 추가:

```c
typedef struct {
    uint32_t tx_count;
    uint32_t rx_success;
    uint32_t rx_fail_timeout;
    uint32_t rx_fail_wrong;
    float    per;           /* Packet Error Rate [%] */
} PLC_Stats_t;

#define TEST_PACKET_COUNT   100     /* 기본 테스트 패킷 수 */
#define TEST_PAYLOAD_SIZE    21     /* 기본 페이로드 크기 (최대 DL_DATALEN_MAX=242) */

const PLC_Stats_t* P2P_GetStats(void);
```

### 1-2. `Core/Src/c_plc_appli.c` 변경 사항 (AppliMasterBoard)

- 무한 `while(1)` → `for (i = 0; i < TEST_PACKET_COUNT; i++)` 루프로 변경
- 각 이터레이션에서 성공/타임아웃/잘못된 ACK 카운팅
- 슬레이브 ACK 전송 무한 재시도(`do { } while (ret != 0)`) → **최대 5회 제한** (무한 대기 방지)
- 루프 완료 후 UART2(디버그 터미널)로 통계 요약 출력:

```
=== PLC TEST RESULT ===
TX: 100  RX_OK: 97  TMO: 2  ERR: 1
PER: 3.00%  MOD: BPSKCOD  PAYLOAD: 21B
=======================
```

### 1-3. 버튼 트리거 추가 (PC13 / B1 Blue Button)

- **누름 감지** → 테스트 시작/재시작
- 케이블 교체·조건 변경 후 버튼 한 번으로 재측정 가능

---

## Phase 2 — 테스트 파라미터 가변 구조

### FRAME_MODULATION 변경 가이드

`Drivers/BSP/Components/ST7580/ST7580_Library/Inc/ST7580_Serial.h` 내 `FRAME_MODULATION` 값 변경 후 재빌드.

| 값 | 매크로 | 특성 |
|---|---|---|
| 0 | `BPSK` | 최고 속도, 노이즈에 민감 |
| 3 | `BFSK` | 노이즈 내성 최강, 속도↓ |
| 4 | `BPSKCOD` | **현재 기본값** — 균형형 (BPSK + FEC 코딩) |
| 7 | `BPSKCODPEAKAV` | 최고 신뢰성, 최저 속도 (피크 노이즈 회피 + FEC) |

> **중요 — 보드레이트 오해 주의**
> `PLM_USART_BAUDRATE`(기본값 57,600 bps)는 **MCU↔ST7580 칩 간 UART 속도**입니다.
> 실제 **전력선 PLC 전송 속도는 `FRAME_MODULATION`(변조 방식)**이 결정합니다.
> MCU UART 보드레이트를 단순 변경해도 PLC 전송 성능에 직접적 영향은 없으며,
> 변경 시 ST7580 내부 `MIB_USART_CONF(0x0C)` MIB 레지스터를 MCU와 동시 변경해야 합니다 → **복잡도 대비 효용 낮음, 후순위 처리**.

### 페이로드 크기 변경 가이드

`c_plc_appli.h`의 `TEST_PAYLOAD_SIZE` 값 수정 후 재빌드.
DL 모드(`USE_DL_DATA`) 최대 페이로드: **242 bytes**.

---

## Phase 3 — ILI9341 LCD 실시간 통계 표시

`P2P_GetStats()` 함수로 stats 구조체를 외부(LCD 레이어)에서 접근하여 실시간 갱신.

### 화면 레이아웃 (예시, 240×320 기준)

```
┌──────────────────────┐
│ [PLC RELIABILITY TEST]│
│ MODE  : BPSKCOD      │
│ PAYLOAD: 021 bytes   │
│ ────────────────────  │
│ TX  :  045 / 100     │
│ OK  :  043           │
│ TMO :  001           │
│ ERR :  001           │
│ PER :  4.44%         │
└──────────────────────┘
```

### 통합 절차

1. 기존 ILI9341 드라이버 소스/헤더를 프로젝트에 복사
2. `CMakeLists.txt`에 소스 파일 및 인클루드 경로 추가
3. LCD 갱신 함수를 `P2P_Process()` 내 매 이터레이션마다 호출

---

## Phase 4 — 물리적 테스트 실행 프로토콜

### 표준 테스트 조건 (베이스라인)

| 항목 | 값 |
|---|---|
| 전원 | 12V DC |
| 변조 방식 | BPSKCOD |
| 페이로드 크기 | 21 bytes |
| 패킷 수 | 100회 (이후 1,000회로 확장) |

---

### 테스트 매트릭스 A: 케이블 길이 (우선순위 1)

> 동일 케이블 종류(1.5mm² 비차폐)로 길이만 변경

| 케이블 길이 | TX | RX_OK | PER [%] | 비고 |
|---|---|---|---|---|
| 1 m | 100 | | | 기준점 (≈ 0% 예상) |
| 5 m | 100 | | | |
| 10 m | 100 | | | |
| 20 m | 100 | | | |
| 50 m | 100 | | | 한계 탐색 |

> PER이 5% 초과되는 시점의 길이 = **통신 한계 거리**로 기록

---

### 테스트 매트릭스 B: 변조 방식 (우선순위 2)

> 동일 케이블(1m)·페이로드(21B)에서 변조만 변경

| FRAME_MODULATION | 매크로 | PER [%] | 비고 |
|---|---|---|---|
| 0 | BPSK | | |
| 3 | BFSK | | |
| 4 | BPSKCOD | | 기준점 |
| 7 | BPSKCODPEAKAV | | |

---

### 테스트 매트릭스 C: 페이로드 크기 (우선순위 3)

> 1m 케이블, BPSKCOD, 페이로드 크기만 변경

| 페이로드 크기 | PER [%] | 비고 |
|---|---|---|
| 21 B | | 현재 기본값 |
| 50 B | | |
| 100 B | | |
| 200 B | | |
| 242 B | | DL 모드 최대 |

---

### 테스트 매트릭스 D: 케이블 종류/단면적 (우선순위 4)

> 동일 길이(10m)·변조(BPSKCOD)에서 케이블만 변경

| 케이블 종류 | 단면적 | 차폐 유무 | PER [%] | 비고 |
|---|---|---|---|---|
| 일반 전력선 | 0.75 mm² | 무 | | |
| 일반 전력선 | 1.5 mm² | 무 | | 기준점 |
| 일반 전력선 | 2.5 mm² | 무 | | |
| 차폐 케이블 | 1.5 mm² | 유 | | |

---

## 전체 진행 체크리스트 (우선순위 순)

> 위에서 아래로 순서대로 진행합니다. 앞 단계가 완료되어야 다음 단계로 이동하세요.

---

### STEP 1 — 펌웨어 구현 (Phase 1 · 2) ← **지금 해야 할 일**

#### 1-A. 통계 수집 구조 구현

- [ ] `Core/Inc/c_plc_appli.h` — `PLC_Stats_t` 구조체 추가
- [ ] `Core/Inc/c_plc_appli.h` — `TEST_PACKET_COUNT` (100), `TEST_PAYLOAD_SIZE` (21) 매크로 추가
- [ ] `Core/Inc/c_plc_appli.h` — `P2P_GetStats()` 함수 선언 추가
- [ ] `Core/Src/c_plc_appli.c` — `AppliMasterBoard()` 내 무한 `while(1)` → `for (i=0; i<TEST_PACKET_COUNT; i++)` 변경
- [ ] `Core/Src/c_plc_appli.c` — 이터레이션마다 `rx_success` / `rx_fail_timeout` / `rx_fail_wrong` 카운팅 추가
- [ ] `Core/Src/c_plc_appli.c` — 슬레이브 ACK 무한 재시도(`do{ }while(ret!=0)`) → **최대 5회 제한**으로 수정
- [ ] `Core/Src/c_plc_appli.c` — 루프 완료 후 UART2로 통계 요약 출력 구현
- [ ] `Core/Src/c_plc_appli.c` — `P2P_GetStats()` 함수 구현

#### 1-B. 버튼 트리거 추가

- [ ] `Core/Src/c_plc_appli.c` — PC13(B1) 버튼 누름 감지 후 테스트 시작/재시작 로직 추가
- [ ] `Core/Inc/c_plc_appli.h` — `FRAME_MODULATION` 옵션 주석 가이드 추가 (`ST7580_Serial.h`)

#### 1-C. 빌드 및 동작 확인

- [ ] `cube-cmake --build --preset Release` 빌드 성공 확인
- [ ] MASTER/SLAVE 두 보드에 플래싱
- [ ] **베이스라인 확인**: 1m 케이블, BPSKCOD, 100패킷 → UART2 터미널에서 `PER ≈ 0%` 수신 확인
- [ ] 슬레이브 ACK 재시도 제한 동작 정상 확인 (무한 대기 없음)
- [ ] 버튼 재시작 동작 확인

---

### STEP 2 — 실험 A: 케이블 길이 테스트 ← **우선순위 1**

> 조건 고정: BPSKCOD, 21B 페이로드, 100패킷, 12V DC, 1.5mm² 비차폐 케이블

#### 2-A. 물리 준비

- [ ] MASTER/SLAVE 보드 전원 연결 (12V DC) 및 동작 확인
- [ ] 테스트할 케이블 길이별로 준비 (1m / 5m / 10m / 20m / 50m)

#### 2-B. 길이별 측정

- [ ] 1m — 버튼 눌러 테스트 시작 → 결과 매트릭스 A 표에 기록 (기준점)
- [ ] 5m — 케이블 교체 → 버튼 → 결과 기록
- [ ] 10m — 케이블 교체 → 버튼 → 결과 기록
- [ ] 20m — 케이블 교체 → 버튼 → 결과 기록
- [ ] 50m — 케이블 교체 → 버튼 → 결과 기록
- [ ] PER 5% 초과 시점 = **통신 한계 거리**로 문서에 기록

---

### STEP 3 — 실험 B: 변조 방식 테스트 ← **우선순위 2**

> 조건 고정: 1m 케이블, 21B 페이로드, 100패킷

#### 3-A. 변조별 재빌드 및 측정

- [ ] `FRAME_MODULATION = BPSK (0)` → 재빌드 → 플래싱 → 측정 → 매트릭스 B 기록
- [ ] `FRAME_MODULATION = BFSK (3)` → 재빌드 → 플래싱 → 측정 → 기록
- [ ] `FRAME_MODULATION = BPSKCOD (4)` — 이미 측정된 베이스라인 값 옮겨 기록
- [ ] `FRAME_MODULATION = BPSKCODPEAKAV (7)` → 재빌드 → 플래싱 → 측정 → 기록
- [ ] 변조별 PER 비교 후 **실 장비 권장 변조 방식 결정** → 문서에 기록

---

### STEP 4 — LCD 통합 (Phase 3) ← **STEP 2/3 병행 가능**

> 사용자가 기존 ILI9341 드라이버 준비 완료 후 진행

- [ ] ILI9341 드라이버 소스/헤더 프로젝트에 복사
- [ ] `CMakeLists.txt` — 소스 파일 및 인클루드 경로 추가
- [ ] LCD 갱신 함수 구현 — `P2P_GetStats()` 호출 후 화면 갱신
- [ ] 빌드 및 LCD 실시간 표시 확인 (TX/OK/TMO/ERR/PER 숫자 갱신)

---

### STEP 5 — 실험 C: 페이로드 크기 테스트 ← **우선순위 3**

> 조건 고정: 1m 케이블, BPSKCOD (또는 STEP 3에서 선정한 변조), 100패킷

- [ ] `TEST_PAYLOAD_SIZE = 21` — 베이스라인 값 사용 (재측정 불필요)
- [ ] `TEST_PAYLOAD_SIZE = 50` → 재빌드 → 플래싱 → 측정 → 매트릭스 C 기록
- [ ] `TEST_PAYLOAD_SIZE = 100` → 재빌드 → 플래싱 → 측정 → 기록
- [ ] `TEST_PAYLOAD_SIZE = 200` → 재빌드 → 플래싱 → 측정 → 기록
- [ ] `TEST_PAYLOAD_SIZE = 242` (DL 최대) → 재빌드 → 플래싱 → 측정 → 기록

---

### STEP 6 — 실험 D: 케이블 종류/두께 테스트 ← **우선순위 4**

> 조건 고정: 10m, BPSKCOD, 21B 페이로드, 100패킷

- [ ] 0.75mm² 비차폐 전력선 — 연결 → 버튼 → 매트릭스 D 기록
- [ ] 1.5mm² 비차폐 전력선 — 연결 → 버튼 → 기록 (기준점)
- [ ] 2.5mm² 비차폐 전력선 — 연결 → 버튼 → 기록
- [ ] 1.5mm² 차폐 케이블 — 연결 → 버튼 → 기록
- [ ] 결과 비교 후 **실 장비 권장 케이블 사양 결정** → 문서에 기록

---

### STEP 7 — 추가 실험 (선택, STEP 1~6 완료 후)

- [ ] 패킷 수 100 → 1,000으로 증가 후 베이스라인 재측정 (통계적 신뢰도 향상)
- [ ] 장비 내 노이즈 환경 테스트: 인버터/모터 동작 중 PER 측정
- [ ] `GAIN_SELECTOR` 활성화 — TX 게인 조정으로 한계 거리 개선 가능성 확인
- [ ] `ZERO_CROSS_SYNC` 활성화 — AC 전력선 환경에서 노이즈 내성 향상 여부 확인
- [ ] 다중 노드 확장: 3대 이상 브로드캐스트/유니캐스트 테스트 (별도 계획 수립)

---

## 추가 고려사항 (Phase 4 이후)

- **노이즈 주입 테스트**: 인버터·모터 동작 중 PER 측정 → 실 장비 환경 시뮬레이션
- **다중 노드 확장**: 2노드 검증 완료 후 3대 이상 브로드캐스트/유니캐스트 테스트 (별도 계획)
- **TX 게인 조정**: `GAIN_SELECTOR` 매크로로 ST7580 송신 전력 변경 → 거리 한계 개선 가능성 탐색
- **영교차 동기(ZERO_CROSS_SYNC)**: 50/60Hz AC 전력선 환경에서 활성화 시 노이즈 내성 향상 여부 확인

---

## 현재 펌웨어 파라미터 참고

| 파라미터 | 현재값 | 위치 |
|---|---|---|
| `FRAME_MODULATION` | `BPSKCOD` (4) | `ST7580_Serial.h` |
| `PLM_USART_BAUDRATE` | 57,600 bps | `stm32_plm01a1.h` |
| `modem_config` | `{0x11}` (DL 모드) | `ST7580_Serial.c` |
| ACK 폴링 횟수 | 10회 | `c_plc_appli.c` |
| ACK 폴링 간격 | 200 ms | `c_plc_appli.c` |
| `IC_TMO` | 10 ms | `ST7580_Serial.h` |
| `ACK_TMO` | 40 ms | `ST7580_Serial.h` |
| `CMD_TMO` | 4,000 ms | `ST7580_Serial.h` |
| DL 최대 페이로드 | 242 bytes | `ST7580_Serial.h` |
