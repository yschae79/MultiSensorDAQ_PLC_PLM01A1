# ST7580 PLC 통신 속도 분석

## 1. 시스템 구성

| 항목 | 내용 |
|------|------|
| MCU | STM32F446RE (NUCLEO-F446RE × 2) |
| PLC 모뎀 | ST7580 (X-NUCLEO-PLM01A1) |
| RTOS | Azure RTOS ThreadX (1 tick = 1ms) |
| MCU ↔ ST7580 UART | **57,600 bps 고정** (칩 하드코딩, 변경 불가) |
| 주파수 대역 | CENELEC B (~95~125kHz, 중심 ~110kHz) |
| 운용 모드 | `USE_DL_DATA` (DL 레이어, MAC+CRC 포함) |

---

## 2. 현재 설정 (`ST7580_Serial.h` / `ST7580_Serial.c`)

```c
/* ST7580_Serial.h */
#define USE_DL_DATA
#define FRAME_MODULATION    BPSKCOD   // 4 → BPSK with convolutional coding (rate-1/2 FEC)
#define ZERO_CROSS_SYNC     0         // 영점 교차 동기 미사용
#define FREQUENCY_SET       1         // phy_config 주파수 설정 사용

/* DATA_OPT = 0x44 */
// bit[6:4] = 0b100 → BPSKCOD
// bit[2]   = 1     → FREQUENCY_SET
```

```c
/* ST7580_Serial.c */
const uint8_t phy_config[14] = {
    0x01,       // [0]  CENELEC B 밴드
    0xC9, 0x08, // [1~2] TX 주파수 ~110kHz
    0x01, 0x8E, 0x70,
    0x0E,       // [6]  TX 게인
    0x15, 0x00, 0x00, 0x02, 0x35, 0x9B, 0x58
};
const uint8_t modem_config[1] = {0x11}; // USE_DL_DATA: DL 레이어
```

---

## 3. ACK RTT 측정 방법 변경 이력

### 3.1 문제: 200ms 양자화 오차

초기 구현에서 ACK 폴링 간격이 200ms로 설정되어 있어 측정값이 200ms 또는 400ms로 교번 출력됨.

```c
/* 변경 전 (c_plc_appli.h) */
#define ACK_POLL_COUNT       25u
#define ACK_POLL_INTERVAL_MS 200u

/* 변경 전 (c_plc_appli.c) */
tx_thread_sleep(200);  // ACK 폴링 대기
```

**원인:** 실제 RTT가 ~100ms인데 200ms 단위로만 측정 → 200ms(1회 폴링) 또는 400ms(2회 폴링) 교번 발생

### 3.2 해결: 1ms 폴링으로 변경

```c
/* 변경 후 (c_plc_appli.h) */
#define ACK_POLL_COUNT       5000u
#define ACK_POLL_INTERVAL_MS 1u

/* 변경 후 (c_plc_appli.c) */
tx_thread_sleep(1);    // 1ms 단위 폴링
```

### 3.3 측정 방법: DWT 사이클 카운터

```c
/* P2P_Init() */
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0u;
DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

/* AppliMasterBoard() */
t_send_cyc = DWT->CYCCNT;   // TRIGGER 전송 직전 캡처
// ... 전송 및 ACK 대기 ...
s_stats.last_ack_latency_ms =
    (DWT->CYCCNT - t_send_cyc) / (SystemCoreClock / 1000u);
```

### 3.4 측정 결과

| 폴링 간격 | 측정 RTT | 비고 |
|-----------|----------|------|
| 200ms | 200ms 또는 400ms (교번) | 양자화 오차 |
| **1ms** | **100 ~ 120ms** | 실제값 반영 |

---

## 4. RTT 이론 분석 (BPSKCOD, CENELEC B)

### 4.1 전체 흐름

```
MASTER MCU          MASTER ST7580       PLC 전력선        SLAVE ST7580      SLAVE MCU
    │──DL_DATA_REQ──→│                      │                   │               │
    │                 │──[TRIGGER frame]──→──│───────────────→───│               │
    │                 │←STATUS──────────────│                   │──DL_DATA_IND──→│
    │                                        │←──[ACK frame]─────│               │
    │←DL_DATA_IND────│←───────────────────│                   │               │
```

### 4.2 단방향 전송 시간 추정

**TRIGGER 프레임 (21B 페이로드 기준):**

| 항목 | 계산 | 결과 |
|------|------|------|
| DL 페이로드 | 21B | 168 bit |
| DL 헤더 + CRC | +4B + 2B = +6B | +48 bit |
| 전체 유효 데이터 | 27B | **216 bit** |
| BPSKCOD FEC (rate-1/2) | ×2 | **432 coded bit** |
| Preamble + Sync (CENELEC B) | 고정 오버헤드 | ~10~15ms |
| BPSK 심볼 전송 (@~9600 chip/s) | 432 ÷ 9600 | ~45ms |
| **단방향 합계** | | **~48~50ms** |

**ACK 프레임 (17B 페이로드 기준):**

| 항목 | 값 |
|------|---|
| 전체 = 17B + 6B = 23B = 184 bit → FEC 후 368 coded bit | |
| 단방향 전송 | ~40~44ms |

### 4.3 RTT 합산

$$RTT = T_{\text{TRIGGER}} + T_{\text{slave 처리}} + T_{\text{ACK}}$$
$$\approx 48\text{ms} + 5\text{ms} + 44\text{ms} \approx \mathbf{97 \sim 107\text{ms}}$$

**결론: 측정값 100~120ms는 이론적으로 타당함** (잔여 10~15ms는 ST7580 내부 처리 지연 + ThreadX 스케줄러 지터)

---

## 5. 변조 방식별 비교

| 변조 방식 | 정의 | FEC | 예상 단방향 전송 | 예상 RTT | 노이즈 내성 |
|-----------|------|-----|-----------------|----------|------------|
| `BPSK` (0) | 비코딩 BPSK | 없음 | ~24ms | ~55ms | 낮음 |
| `QPSK` (1) | 비코딩 QPSK | 없음 | ~12ms | ~30ms | 낮음 |
| `PSK8` (2) | 비코딩 8PSK | 없음 | ~8ms | ~20ms | 매우 낮음 |
| `BFSK` (3) | 이진 FSK | 없음 | ~24ms | ~55ms | 중간 |
| **`BPSKCOD` (4)** | **BPSK + rate-1/2** | **있음** | **~48ms** | **~100ms** | **높음 (현재)** |
| `QPSKCOD` (5) | QPSK + rate-1/2 | 있음 | ~24ms | ~55ms | 중간 |
| `BPSKCODPEAKAV` (7) | BPSK + FEC + PNA | 있음 | ~50ms~ | ~110ms~ | 최고 (노이즈 회피) |

> **PNA (Peak Noise Avoidance):** 전력선 노이즈 피크 구간을 피해 전송하여 최고 내성을 제공하나 추가 지연 발생

### QPSKCOD로 전환 시 기대 효과

```c
// ST7580_Serial.h 수정
#define FRAME_MODULATION    QPSKCOD   // 4 → 5
```

- 예상 RTT: **~55~65ms** (현재 대비 약 45% 단축)
- 단점: 노이즈 내성 감소 → 전력선 품질이 좋을 때 유효

---

## 6. CENELEC 밴드 변경

### 6.1 CENELEC 약어 및 발음

> **C**omité **E**uropéen de **N**ormalisation **É**lectrotechnique
> (유럽 전기기술 표준화 위원회)

발음: **"세넬렉"** (프랑스어 약자, 영어권에서도 동일하게 읽음)

### 6.2 밴드 선택 (`phy_config[0]` bit[1:0])

| 값 | 밴드 | 주파수 범위 | 비고 |
|----|------|-------------|------|
| `0x00` | CENELEC A | 3 ~ 95 kHz | 전력사 전용 (EN 50065-1) |
| **`0x01`** | **CENELEC B** | **95 ~ 125 kHz** | **현재 설정, 소비자용** |
| `0x02` | CENELEC C | 125 ~ 140 kHz | 소비자용, CSMA |
| `0x03` | CENELEC D / FCC | 140 ~ 148.5 kHz | 지역별 상이 |

### 6.3 TX 주파수 계산 (`phy_config[1:2]`)

$$f_{TX} = \text{TXFREQ} \times 48.828 \text{ Hz}$$

| 밴드 | 대표 중심 주파수 | TXFREQ | `[1]`, `[2]` |
|------|----------------|--------|--------------|
| CENELEC A | ~75 kHz | 1536 = 0x0600 | `0x00, 0x06` |
| **CENELEC B** | **~110 kHz** | **2249 = 0x08C9** | **`0xC9, 0x08`** |
| CENELEC C | ~132 kHz | 2703 = 0x0A8F | `0x8F, 0x0A` |

### 6.4 코드 변경 예시 (CENELEC A로 변경 시)

```c
/* ST7580_Serial.c */
const uint8_t phy_config[14] = {
    0x00,       // [0] CENELEC A (변경)
    0x00, 0x06, // [1:2] ~75 kHz (변경)
    0x01, 0x8E, 0x70,
    0x0E,
    0x15, 0x00, 0x00, 0x02, 0x35, 0x9B, 0x58
};
```

### 6.5 ⚠️ 하드웨어 제약

X-NUCLEO-PLM01A1의 아날로그 커플링 회로(C3, C6, L1 등)는 **CENELEC B (~110kHz)에 맞게 튜닝**되어 있습니다.
밴드를 변경하면 신호 감쇠가 크게 증가하므로, 커플링 회로 부품값도 함께 재설계해야 합니다.
**현재 PLM01A1 하드웨어에서 실용적인 선택은 CENELEC B 유지.**

---

## 7. 참고: `DATA_OPT` 비트 구성

```
bit 7   : ZERO_CROSS_SYNC (0 = 미사용)
bit 6:4 : FRAME_MODULATION (BPSKCOD = 0b100 = 4)
bit 3   : GAIN_SELECTOR (0 = phy_config 게인 사용)
bit 2   : FREQUENCY_SET (1 = phy_config 주파수 사용)
bit 1   : FREQUENCY_OVERWRITE (0)
bit 0   : CUSTOM_MIB_FREQUENCY (0)

현재 DATA_OPT = 0b0100_0100 = 0x44
```

---

*최종 수정: 2026-05-14*
