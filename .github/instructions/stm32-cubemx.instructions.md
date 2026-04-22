---
description: "STM32CubeMX 생성 CMake 프로젝트에서 코드 분석, 수정, 문제 해결 시 준수해야 할 지침. STM32 HAL, IOC, CubeMX USER CODE, GPIO, 타이머, 인터럽트, 빌드, 커밋 관련 작업 시 자동 적용."
applyTo:
  - "**/*.c"
  - "**/*.h"
  - "**/*.ioc"
  - "**/CMakeLists.txt"
  - "**/*.s"
  - "**/*.ld"
---

# STM32 CubeMX 프로젝트 에이전트 지침

## 프로젝트 배경

- **목적**: Power Line Communication (PLC) 통신 테스트 — ST7580 모뎀 칩(X-NUCLEO-PLM01A1 탑재)을 이용한 전력선 통신 P2P 검증
- **테스트 보드**: NUCLEO-F446RE (STM32F446RE) × 2 세트 + X-NUCLEO-PLM01A1 × 2 세트
- **샘플 참조**: `sample/STM32CubeExpansion_PLM1_V1.1.0` — 원본은 NUCLEO-F401RE 기준이며, **NUCLEO-F446RE 포팅 필요** (핀은 호환됨)
- **ST7580 통신 인터페이스**: USART1 (PA9=TX, PA10=RX, 57600 baud) + GPIO 제어 핀
- **디버그 UART**: USART2 (PA2=TX, PA3=RX, 115200 baud) — ST-Link VCP 연결
- **동작 방식**: 부팅 시 USER 버튼(PC13) 상태로 MASTER/SLAVE 역할 결정

---

## 핵심 원칙

### 1. 추측성 답변 절대 금지
- 확실하지 않은 정보를 추측하거나 가정하여 답변하는 것을 절대 금지합니다.
- 불확실한 사항이 있을 경우 반드시 사용자에게 명확히 알리고 확인을 요청해야 합니다.
- ❌ "아마도 이 함수는 ~할 것으로 보입니다"
- ✅ "소스코드를 확인한 결과, [파일명]의 [라인번호]에서 다음과 같이 구현되어 있습니다"

### 2. 소스코드 필수 확인
모든 답변 전에 현재 프로젝트의 실제 소스코드를 반드시 확인합니다.

### 2-1. STM32 하드웨어 리소스 분석 — 3단계 필수 절차
하드웨어 관련 질문(GPIO, 타이머, 주변장치 등)이 있을 경우 반드시 다음 순서로 확인합니다.

1. **IOC 파일 확인** (`MultiSensorDAQ_PLC_PLM01A1.ioc`) — 핀 할당, 주변장치 설정, 클럭 설정
2. **초기화 코드 확인** (`main.c`, `stm32f4xx_hal_msp.c`) — `MX_*_Init()`, `HAL_*_MspInit()`
3. **응용 코드 확인** (`Core/Src/*.c`) — 실제 제어 로직, 인터럽트 핸들러, 콜백

- ❌ IOC 파일을 확인하지 않고 핀 할당을 추측하는 것
- ❌ HAL 라이브러리 기본값을 실제 프로젝트 설정으로 가정하는 것
- ❌ STM32CubeMX 자동생성 코드와 사용자 코드를 혼동하는 것

### 3. 증거 기반 답변
모든 답변에 반드시 파일명, 라인 번호, 코드 인용을 제시합니다.

---

## 프로젝트 구조

- **MCU**: STM32F446RE (Cortex-M4, 180MHz, 512KB Flash, LQFP64)
- **보드**: NUCLEO-F446RE × 2 세트 (각각 X-NUCLEO-PLM01A1 장착)
- **PLC 모뎀 칩**: ST7580 (X-NUCLEO-PLM01A1 탑재)
- **OS**: FreeRTOS 미사용 (Bare-metal, HAL 기반)
- **빌드**: STM32CubeMX + CMake + Ninja + starm-clang (NEWLIB)
- **최적화**: C Release `-O3`, CXX Release `-Oz`, Debug `-Og -g3`
- **STARM 툴체인**: `STARM_NEWLIB` (`--config=newlib.cfg`)
- **플래시**: STM32CubeProgrammer (SWD)
- **링커 스크립트**: `STM32F446XX_FLASH.ld`
- **현재 클럭**: HSE 8MHz → PLL → 84MHz (HCLK), APB1=42MHz, APB2=84MHz

### 하드웨어 핀 할당 (현재 IOC 기준 + PLM01A1 추가 필요)

| 핀 | 신호 | 설명 |
|---|---|---|
| PA2 | USART2_TX | 디버그 UART TX (ST-Link VCP) |
| PA3 | USART2_RX | 디버그 UART RX (ST-Link VCP) |
| PA5 | GPIO_Output | LD2 (녹색 LED) |
| PA9 | USART1_TX | ST7580 PLM UART TX |
| PA10 | USART1_RX | ST7580 PLM UART RX |
| PA8 | GPIO_Output | ST7580 RESET_N 핀 |
| PA13 | SYS_JTMS-SWDIO | SWD |
| PA14 | SYS_JTCK-SWCLK | SWD |
| PB3 | SYS_JTDO-SWO | SWO |
| PC0 | GPIO_Output | ST7580 PL_TX_ON |
| PC1 | GPIO_EXTI1 | ST7580 PL_RX_ON (EXTI, Rising+Falling) |
| PC13 | GPIO_EXTI13 | USER 버튼 (Blue, 누르면 MASTER 역할) |

> **T_REQ 핀(PA5)**: 샘플에서 PA5를 T_REQ로 사용 — 현재 IOC에서 LD2 LED로 할당되어 있음. PLC 통합 시 재검토 필요.

### 샘플 펌웨어 구조 (`sample/STM32CubeExpansion_PLM1_V1.1.0`)

```
Projects/Multi/Examples/P2P_demo/
├── Src/
│   ├── main.c          — 초기화 및 메인 루프
│   ├── cube_hal_f4.c   — F4 클럭 설정
│   ├── st7580_appli.c  — P2P 애플리케이션 로직
│   └── stm32f4xx_it.c  — 인터럽트 핸들러
└── Inc/
    ├── cube_hal.h      — HAL 헤더 선택 (USE_STM32F4XX_NUCLEO)
    └── st7580_appli.h  — P2P 애플리케이션 헤더

Drivers/BSP/X-NUCLEO-PLM01A1/
├── stm32_plm01a1.c     — BSP 레이어 (UART/GPIO 초기화, BSP_PLM_* API)
└── stm32_plm01a1.h     — PLM 핀/UART 매크로 정의

Drivers/BSP/Components/ST7580/
└── ST7580_Serial.*     — ST7580 모뎀 드라이버
```

### F401RE → F446RE 포팅 시 주요 차이점

- **클럭**: F401RE는 84MHz, F446RE도 84MHz로 동일 설정 가능 (최대 180MHz)
- **USART AF**: USART1/2 모두 `GPIO_AF7_USARTx` (동일)
- **USART1_IRQn**: 동일
- **HAL 헤더**: `stm32f4xx_hal.h` (공통)
- **정의 매크로**: `USE_STM32F4XX_NUCLEO` 유지

### 소스파일 네이밍 규칙
애플리케이션 소스파일은 반드시 `c_` 접두사로 시작합니다.
- ✅ `c_plc_appli.c`, `c_debug.c`, `c_st7580.c`
- ❌ `plc_appli.c`, `PLC.c`
- 예외: CubeMX 자동생성 파일 (`main.c`, `stm32f4xx_it.c`, `stm32f4xx_hal_msp.c`, `system_stm32f4xx.c` 등)

### 소스코드 주석 규칙
소스코드 주석은 **한국어**로 작성하며, **Doxygen 규칙**을 적용합니다.
- 파일 헤더: `@file`, `@brief`, `@details` 태그 사용
- 함수: `@brief`, `@param`, `@retval`(또는 `@return`) 태그 사용
- 본문 주석(설명, 인라인)은 한국어로 작성
- ✅ `/** @brief USART3 디버그 DMA TX 완료 처리 */`
- ❌ `/* USART3 debug DMA TX complete */`
- 예외: CubeMX 자동생성 주석, 라이브러리 코드

### STM32CubeMX USER CODE 규칙
`main.c`, `stm32f4xx_it.c`, `stm32f4xx_hal_msp.c`에서 사용자 코드는 반드시 `USER CODE` 블록 내에 작성합니다.
CubeMX 재생성 시 USER CODE 블록만 보존됩니다.

```c
/* USER CODE BEGIN Includes */
#include "my_header.h"
/* USER CODE END Includes */
```

- ❌ USER CODE 블록 밖의 자동생성 코드 수정 (CubeMX에서 수정 필요)
- ❌ HAL 라이브러리 코드 직접 수정

### STM32F4 특수 고려사항
- **FPU**: 활성화됨 (`-mfpu=fpv4-sp-d16 -mfloat-abi=hard`): float 연산 시 하드웨어 FPU 사용
- **HAL 헤더**: `stm32f4xx_hal.h` (H7/L0 계열과 혼동 금지)
- **USART1 인터럽트**: `USART1_IRQHandler` — ST7580 수신 처리에 사용
- **DMA + UART**: `HAL_UART_Transmit_DMA()` 사용 시 버퍼가 전송 완료 전까지 유효해야 함
- **HAL_Delay()**: FreeRTOS 미사용이므로 SysTick 기반 `HAL_Delay()` 직접 사용 가능
- **`USE_STM32F4XX_NUCLEO`**: 컴파일 시 반드시 정의되어야 함 (BSP 및 cube_hal.h 조건부 컴파일)

### ⚠️ starm-clang (NEWLIB) 주의사항
- `volatile` 없는 루프 변수는 컴파일러에 의해 제거될 수 있음
- 릴리즈 빌드(`-O3`)에서 중단점이 원하는 위치에서 동작하지 않을 수 있음
- 상태 플래그 변수에는 반드시 `volatile` 사용
- GCC(`gcc-arm-none-eabi`)와 동작 차이 주의 (특히 printf, malloc 동작)

---

## PLM01A1 / ST7580 관련 지침

### BSP API 사용 규칙
샘플 코드의 BSP/드라이버를 `Drivers/BSP/X-NUCLEO-PLM01A1/` 및 `Drivers/BSP/Components/ST7580/`로 통합합니다.

| API | 설명 |
|---|---|
| `BSP_PLM_Init()` | ST7580 초기화 |
| `BSP_PLM_Reset()` | 모뎀 리셋 |
| `BSP_PLM_Mib_Write(index, buf, len)` | MIB 설정 쓰기 |
| `BSP_PLM_Mib_Read(index, buf, len)` | MIB 설정 읽기 |
| `BSP_PLM_Send_Data(opt, buf, len, conf)` | 데이터 전송 |
| `BSP_PLM_Receive_Frame()` | 수신 프레임 조회 |

### MIB 설정 (샘플 기준)
- `MIB_MODEM_CONF`: 모뎀 동작 모드 설정
- `MIB_PHY_CONF`: 물리 계층 설정
- 설정값은 `st7580_appli.h`의 `modem_config[]`, `phy_config[]` 배열 참조

### 역할 결정 로직
- 부팅 시 `BSP_PB_GetState(BUTTON_KEY)` 확인
- `GPIO_PIN_SET` (버튼 미누름) → SLAVE
- `GPIO_PIN_RESET` (버튼 누름) → MASTER

---

## 코드 수정 지침

### 수정 전 필수 확인
1. ✅ 기존 코드의 정확한 동작 파악
2. ✅ 수정이 미치는 영향 범위 확인
3. ✅ USER CODE 블록 규칙 준수 확인
4. ✅ 사용자에게 수정 계획 설명 및 승인 받기

### 수정 후 필수 사항
1. ✅ 수정 내용 상세 설명
2. ✅ 수정한 파일과 라인 번호 명시
3. ✅ 변경 전후 코드 비교 제시

---

## 빌드 및 커밋 워크플로우

코드 수정 완료 후 반드시 다음 순서를 따릅니다:

1. **빌드**: `cube-cmake --build --preset Release`
2. **에러 확인 및 수정**: 빌드 실패 시 에러 분석 후 수정 → 재빌드
3. **커밋/Push 금지**: 빌드 성공 후에도 커밋과 Push는 절대 자동으로 수행하지 않습니다.
   사용자가 `/커밋` 또는 `/커밋-푸시` 명령을 명시적으로 실행해야만 수행합니다.

---

## 절대 금지 행동

1. ❌ 추측으로 답변 ("아마도", "일반적으로", "보통")
2. ❌ 소스코드를 직접 확인하지 않고 답변
3. ❌ 파일명, 라인번호, 코드 인용 없이 답변
4. ❌ 사용자 확인 없이 중요 코드 변경
5. ❌ "~인 것 같습니다", "~일 수도 있습니다" 사용
