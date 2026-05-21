# MultiSensorDAQ_PLC_PLM01A1 — 프로젝트 지침

> **공통 규칙**: STM32 펌웨어 작업은 `stm32-fw` 스킬, PDF(회로도·데이터시트)
> 분석은 `pdf-doc` 스킬을 적용한다. 이 문서에는 **프로젝트별 하드웨어 정보만**
> 기록한다(공통 규칙 중복 금지).

## 프로젝트 배경
- 목적: Power Line Communication(PLC) 통신 테스트 — ST7580 모뎀(X-NUCLEO-PLM01A1)으로 전력선 통신 P2P 검증
- 보드 구성: NUCLEO-F446RE(STM32F446RE) × 2 세트 + X-NUCLEO-PLM01A1 × 2 세트
- 동작: 부팅 시 USER 버튼(PC13) 상태로 MASTER/SLAVE 역할 결정
  (`BSP_PB_GetState(BUTTON_KEY)`: SET=SLAVE, RESET=MASTER)
- 샘플 참조: `sample/STM32CubeExpansion_PLM1_V1.1.0` (원본 NUCLEO-F401RE → F446RE 포팅, 핀 호환)

## 하드웨어
- **MCU**: STM32F446RE (Cortex-M4, 180MHz max, 512KB Flash, LQFP64)
- **클럭**: HSE 8MHz → PLL → 84MHz HCLK, APB1=42MHz, APB2=84MHz
- **OS**: ThreadX 6.1.10 (TIM6 타임베이스, `TX_TIMER_TICKS_PER_SECOND = 1000`)
- **STARM 라이브러리**: `STARM_NEWLIB` (`cmake/starm-clang.cmake`, `--config=newlib.cfg`, 링크 `-lcrt0-nosys`)
  - ※ PICOLIBC로 바꾸지 말 것 — crt0/링커플래그가 달라 빌드가 깨짐
- **최적화**: C Release `-O3 -g0`, CXX Release `-Oz -g0`, Debug `-Og -g3`
- **FPU**: 활성(`-mfpu=fpv4-sp-d16 -mfloat-abi=hard`) — float 연산 HW FPU
- **링커 스크립트**: `STM32F446XX_FLASH.ld`
- **컴파일 정의**: `USE_STM32F4XX_NUCLEO` 반드시 정의 (BSP/cube_hal 조건부 컴파일)
- **HAL 헤더**: `stm32f4xx_hal.h` (H7/L0 계열과 혼동 금지)

### 핀 할당 (IOC 기준 + PLM01A1)
| 핀 | 신호 | 설명 |
|---|---|---|
| PA2 | USART2_TX | 디버그 UART TX (ST-Link VCP, 115200) |
| PA3 | USART2_RX | 디버그 UART RX (ST-Link VCP) |
| PA5 | GPIO_Output | LD2(녹색 LED). ※ 샘플은 PA5=T_REQ — PLC 통합 시 재검토 |
| PA9 | USART1_TX | ST7580 PLM UART TX (57600) |
| PA10 | USART1_RX | ST7580 PLM UART RX |
| PA8 | GPIO_Output | ST7580 RESET_N |
| PC0 | GPIO_Output | ST7580 PL_TX_ON |
| PC1 | GPIO_EXTI1 | ST7580 PL_RX_ON (EXTI Rising+Falling) |
| PC13 | GPIO_EXTI13 | USER 버튼 (누르면 MASTER) |
| PA13/PA14/PB3 | SWD | SWDIO/SWCLK/SWO |

### 샘플/드라이버 구조
- `sample/.../Projects/Multi/Examples/P2P_demo/` — main.c, cube_hal_f4.c, st7580_appli.c, stm32f4xx_it.c
- BSP 통합 대상: `Drivers/BSP/X-NUCLEO-PLM01A1/` (BSP_PLM_* API), `Drivers/BSP/Components/ST7580/`
- 주요 API: `BSP_PLM_Init/Reset`, `BSP_PLM_Mib_Write/Read`, `BSP_PLM_Send_Data`, `BSP_PLM_Receive_Frame`
- MIB: `MIB_MODEM_CONF`, `MIB_PHY_CONF` (값은 `st7580_appli.h`의 `modem_config[]`, `phy_config[]`)

## 빌드/디버그
- 빌드: `cube-cmake --build --preset Release` / `--preset Debug`
- 단축키: Shift+F6 = Rebuild + Flash (R), Shift+F5 = Build + Flash (R), Shift+F7 = Clean (R)
- 디버거: ST-LINK (launch.json: ST-Link GDB Server / Attach / Cortex-Debug OpenOCD)
- USART1_IRQHandler → ST7580 수신 처리. ThreadX 미사용 시점 코드는 SysTick `HAL_Delay()` 사용 가능
