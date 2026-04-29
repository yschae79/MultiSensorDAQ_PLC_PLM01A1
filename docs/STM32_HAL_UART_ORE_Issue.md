# STM32 HAL UART ORE(Overrun Error) 수신 중단 문제 분석 및 해결

> 작성일: 2026-04-27
> 적용 대상: STM32F4xx HAL / STM32H7xx 포팅 시 참고

---

## 1. 문제 요약

STM32 HAL UART 드라이버에서 **ORE(Overrun Error)** 또는 PE/FE/NE 에러 발생 시
`HAL_UART_IRQHandler()` 내부 처리로 인해 **UART 수신이 완전히 중단**되는 현상.

HAL 초기 출시(~2014) 이후 현재까지 고쳐지지 않은 **구조적 설계 문제**.

---

## 2. 하드웨어 에러 플래그 클리어 메커니즘

STM32F4 USART 하드웨어 스펙 (RM0090):

```
에러 플래그(PE/FE/NE/ORE) 클리어 방법:
  ① USART_SR 레지스터 읽기
  ② USART_DR 레지스터 읽기
  → 이 두 단계가 반드시 연속으로 수행되어야 클리어됨
```

`__HAL_UART_CLEAR_PEFLAG` 매크로가 이를 수행:
```c
#define __HAL_UART_CLEAR_PEFLAG(__HANDLE__)     \
  do{                                           \
    __IO uint32_t tmpreg = 0x00U;               \
    tmpreg = (__HANDLE__)->Instance->SR;        \  /* ① SR 읽기 */
    tmpreg = (__HANDLE__)->Instance->DR;        \  /* ② DR 읽기 → 클리어 완료 */
    UNUSED(tmpreg);                             \
  } while(0U)
/* FE, NE, ORE 클리어 매크로도 모두 동일 시퀀스 사용 */
#define __HAL_UART_CLEAR_FEFLAG(__HANDLE__) __HAL_UART_CLEAR_PEFLAG(__HANDLE__)
#define __HAL_UART_CLEAR_NEFLAG(__HANDLE__) __HAL_UART_CLEAR_PEFLAG(__HANDLE__)
#define __HAL_UART_CLEAR_OREFLAG(__HANDLE__) __HAL_UART_CLEAR_PEFLAG(__HANDLE__)
```

---

## 3. HAL_UART_IRQHandler 내부의 문제

```c
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart)
{
    uint32_t isrflags = READ_REG(huart->Instance->SR);  /* ① SR 읽기 완료 */
    /* ...에러 플래그 감지 후 ErrorCode 설정만 함...     */
    /* ② DR 읽기 없이 콜백 호출 → 하드웨어 플래그 미클리어 */

    if (((huart->ErrorCode & HAL_UART_ERROR_ORE) != RESET) || dmarequest)
    {
        HAL_UART_ErrorCallback(huart);  /* ← DR을 읽지 않아 플래그가 살아있음 */
        /* DMA 모드: HAL_DMA_Abort() 호출 → 수신 파이프라인 완전 중단 */
    }
}
```

### 수신 중단 흐름

```
ORE 발생
    ↓
HAL_UART_IRQHandler 진입
    ↓
isrflags = SR 읽기 (① 완료)
    ↓
DR 읽기 없음 → ORE 플래그 여전히 SET
    ↓
HAL_UART_ErrorCallback 호출
    ↓
(DMA 모드) HAL_DMA_Abort() → huart->RxState = READY
    ↓
ISR 리턴 후에도 ORE 플래그 SET → 즉시 재진입 or 수신 불가
    ↓
UART 통신 완전 중단 (조용히 멈춤, 디버거로 보면 "정상" 상태)
```

### 왜 디버깅이 어려운가

- `huart->RxState == HAL_UART_STATE_READY` → 외부에서 보면 "정상"
- 수신 인터럽트 자체가 발생하지 않으므로 오류 메시지 없음
- 증상이 노이즈가 심한 환경에서만 불규칙적으로 발생

---

## 4. STM32F4 해결책: ISR 진입 시 사전 클리어

### 해결 패턴 (ST 공식 샘플 코드 방식)

`HAL_UART_IRQHandler()` 호출 **전에** 에러 플래그를 직접 클리어:

```c
void USART1_IRQHandler(void)
{
    /* USER CODE BEGIN USART1_IRQn 0 */
    UART_HandleTypeDef *huart = &pUartPlmHandle;
    uint8_t pe=0u, fe=0u, ne=0u, oe=0u;

    /* ★ HAL_UART_IRQHandler 호출 전에 에러 플래그 선클리어 ★
     * HAL 내부에서 SR만 읽고 DR을 읽지 않아 플래그가 클리어되지 않는
     * STM32F4 HAL 드라이버의 구조적 문제 우회용 */
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_PE))  { __HAL_UART_CLEAR_PEFLAG(huart);  pe=1u; }
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_FE))  { __HAL_UART_CLEAR_FEFLAG(huart);  fe=1u; }
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_NE))  { __HAL_UART_CLEAR_NEFLAG(huart);  ne=1u; }
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) { __HAL_UART_CLEAR_OREFLAG(huart); oe=1u; }
    if (pe || fe || ne || oe) { /* 에러 카운터 증가 등 처리 */ }

    /* 이후 정상 RXNE/TXE 처리 */
    /* USER CODE END USART1_IRQn 0 */
    HAL_UART_IRQHandler(&huart1);  /* 이 시점엔 SR이 클린 → 에러 경로 미진입 */
}
```

### 동작 원리

```
ORE 발생
    ↓
USART1_IRQHandler 진입
    ↓
★ __HAL_UART_CLEAR_OREFLAG: SR읽기 → DR읽기 → 플래그 클리어 완료
    ↓
HAL_UART_IRQHandler 진입 시 errorflags == RESET
    ↓
에러 처리 경로 건너뜀 → 정상 수신 경로 진행
    ↓
수신 파이프라인 유지
```

---

## 5. STM32H7 포팅 시 대응 방법

### H7의 차이점: OVRDIS 비트

STM32H7의 USART는 `CR3` 레지스터에 **OVRDIS(Overrun Disable)** 비트 추가됨:

```
USART_CR3[12] = OVRDIS
  0 (기본): 오버런 발생 시 ORE 플래그 세트 + DR 덮어쓰기 (기존 동작)
  1        : 오버런 발생 시 새 데이터 무시, ORE 플래그 세트 안 함
             → HAL 에러 처리 경로 자체를 타지 않음
```

CubeMX 설정 또는 코드에서 활성화:

```c
/* MX_USARTx_UART_Init() 이후 또는 HAL_UART_MspInit() 내부 */
__HAL_UART_DISABLE_IT(&huartx, UART_IT_ERR);  /* 에러 인터럽트 비활성화 옵션 */

/* 또는 직접 레지스터 설정 */
SET_BIT(huartx.Instance->CR3, USART_CR3_OVRDIS);
```

HAL 초기화 구조체에서도 설정 가능 (H7 HAL):
```c
huart1.Init.OverSampling    = UART_OVERSAMPLING_16;
huart1.Init.OneBitSampling  = UART_ONE_BIT_SAMPLE_DISABLE;
/* H7 전용: */
huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
/* CR3 OVRDIS는 HAL_UART_Init 후 수동 세트 권장 */
```

### H7에서 권장 접근 방법

| 방법 | 설명 | 장점 | 단점 |
|------|------|------|------|
| **OVRDIS 비트** | 오버런 발생 자체를 무시 | 가장 깔끔, HAL 수정 불필요 | ORE 카운팅 불가 |
| **사전 클리어** | H7도 동일하게 ISR 선두에서 클리어 | F4와 동일 패턴 유지 | 코드 추가 필요 |
| **LL 드라이버** | HAL 대신 LL_USART 직접 사용 | 완전한 제어권 | 개발 공수 증가 |

### H7 OVRDIS 적용 예시

```c
/* H7 포팅 시: MX_USART1_UART_Init() 호출 후 추가 */
void MX_USART1_UART_Init(void)
{
    /* ... CubeMX 생성 코드 ... */
    HAL_UART_Init(&huart1);

    /* ★ H7 전용: Overrun 하드웨어 비활성화
     * ORE 플래그 자체가 세트되지 않으므로 HAL 에러 처리 경로 미진입
     * → F4에서의 선클리어 패턴 불필요 */
    SET_BIT(huart1.Instance->CR3, USART_CR3_OVRDIS);
}
```

### H7에서도 사전 클리어가 필요한 경우

PE(Parity Error), FE(Frame Error), NE(Noise Error)는 OVRDIS와 무관하게 발생 가능.
이 경우 F4와 동일한 선클리어 패턴 유지:

```c
void USART1_IRQHandler(void)
{
    /* USER CODE BEGIN USART1_IRQn 0 */
    /* H7: OVRDIS 설정 시 ORE는 발생 안 함, 나머지는 여전히 처리 필요 */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_PE)) { __HAL_UART_CLEAR_PEFLAG(&huart1); }
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE)) { __HAL_UART_CLEAR_FEFLAG(&huart1); }
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_NE)) { __HAL_UART_CLEAR_NEFLAG(&huart1); }
    /* ORE: OVRDIS 설정 시 생략 가능 */
    /* USER CODE END USART1_IRQn 0 */
    HAL_UART_IRQHandler(&huart1);
}
```

---

## 6. 타 MCU 제조사 비교

| 제조사 | 에러 시 수신 중단 | 클리어 방식 | 비고 |
|--------|--------------|-----------|------|
| **ST (F4 HAL)** | **발생** (고질적) | SR→DR 순서 의존 | F7/H7는 OVRDIS로 우회 |
| **ST (H7 HAL)** | OVRDIS 설정 시 없음 | OVRDIS 비트 | H7부터 하드웨어 개선 |
| **TI DriverLib** | 없음 | `UARTRxErrorClear()` 전용 함수 | HW FIFO 기본 제공 |
| **Renesas FSP** | 없음 | 콜백 분리, HW 자동클리어 옵션 | FSP 설계 시 반면교사 |
| **NXP MCUXpresso** | 부분적 | `UART_ClearStatusFlags()` 독립 함수 | F4보다는 양호 |

---

## 7. 현 프로젝트 적용 현황 (STM32F446RE)

[Core/Src/stm32f4xx_it.c](../Core/Src/stm32f4xx_it.c) `USART1_IRQHandler` 에 적용:

```c
/* UART 에러 플래그 클리어 및 카운트 */
if (__HAL_UART_GET_FLAG(huart, UART_FLAG_PE))  { __HAL_UART_CLEAR_PEFLAG(huart);  pe=1u; }
if (__HAL_UART_GET_FLAG(huart, UART_FLAG_FE))  { __HAL_UART_CLEAR_FEFLAG(huart);  fe=1u; }
if (__HAL_UART_GET_FLAG(huart, UART_FLAG_NE))  { __HAL_UART_CLEAR_NEFLAG(huart);  ne=1u; }
if (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) { __HAL_UART_CLEAR_OREFLAG(huart); oe=1u; }
if (pe || fe || ne || oe) { P2P_UART1_ErrorInc(pe, fe, ne, oe); }
```

에러 발생 횟수는 `PLC_Stats_t` 구조체의 `uart_pe/fe/ne/oe` 카운터로 누적되며 LCD에 표시됨.

---

## 8. 참고

- STM32F4 Reference Manual (RM0090): Section 30.6.1 Status register (USART_SR)
- STM32H7 Reference Manual: USART_CR3 OVRDIS 비트 (Section 49.8.2)
- ST Community: "UART stops receiving after ORE error" — 2014년~현재 다수 스레드
- ST 공식 샘플: `X-NUCLEO-PLM01A1` BSP `stm32_plm01a1.c` 내 동일 패턴 적용 확인
