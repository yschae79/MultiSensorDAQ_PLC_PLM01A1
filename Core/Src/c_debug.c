/**
  ******************************************************************************
  * @file           : c_debug.c
  * @brief          : 비동기 DMA 기반 디버그 printf (Dedicated Logger Task 방식)
  * @details        : syscalls.c의 weak _write()를 재정의하여
  *                   출력 문자열을 Byte Pool에 할당 후 Message Queue로 전송.
  *                   백그라운드의 Debug Task가 큐를 모니터링하며 DMA로 출력 처리.
  *                   호출하는 태스크는 Blocking이나 Mutex 경합 없이 즉시 리턴됨.
  ******************************************************************************
  */

#include "c_debug.h"
#include "main.h"
#include "tx_api.h"
#include <string.h>

/* ----------------------- 외부 HAL 핸들 ---------------------------------- */
extern UART_HandleTypeDef DEBUG_UART_INSTANCE;

/* ----------------------- RTOS 객체 & 메모리 ----------------------------- */
#define DEBUG_STACK_SIZE    1024u
#define DEBUG_EVENT_TX_DONE (1u << 0)

static TX_BYTE_POOL         s_debug_pool;
static TX_QUEUE             s_debug_queue;
static TX_THREAD            s_debug_thread;
static TX_EVENT_FLAGS_GROUP s_debug_events;

/* 정적 메모리 할당 (App_ThreadX_Init의 Pool을 사용하지 않고 독립적으로 구성) */
static uint8_t s_pool_memory[DEBUG_BYTE_POOL_SIZE];
static uint8_t s_debug_stack[DEBUG_STACK_SIZE];
static ULONG   s_queue_memory[DEBUG_QUEUE_SIZE * 2]; /* 포인터 1, 길이 1 = 총 2 ULONG */

static uint8_t s_initDone = 0;

/* ----------------------- 오버플로우 정책 설정 --------------------------- */
#if (DEBUG_OVERFLOW_BLOCK == 1)
  #define ALLOC_WAIT TX_WAIT_FOREVER
#else
  #define ALLOC_WAIT TX_NO_WAIT
#endif

/* ----------------------- Private 헬퍼 함수 ------------------------------ */

/**
 * @brief  Debug Logger 전용 백그라운드 태스크
 */
static void Debug_Task_Entry(ULONG argument)
{
    (void)argument;
    ULONG msg[2];
    ULONG actual_events;

    for (;;)
    {
        /* 1. 큐에서 메시지 대기 (무한 대기) */
        if (tx_queue_receive(&s_debug_queue, msg, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            uint8_t *ptr = (uint8_t *)msg[0];
            uint32_t len = (uint32_t)msg[1];

            /* 2. DMA 전송 시작 */
            HAL_UART_Transmit_DMA(&DEBUG_UART_INSTANCE, ptr, (uint16_t)len);

            /* 3. DMA 완료 대기 */
            tx_event_flags_get(&s_debug_events, DEBUG_EVENT_TX_DONE, 
                               TX_AND_CLEAR, &actual_events, TX_WAIT_FOREVER);

            /* 4. 메모리 해제 */
            tx_byte_release(ptr);
        }
    }
}

/* ----------------------- Public API ------------------------------------- */

/**
 * @brief  비동기 디버그 출력 초기화
 * @note   App_ThreadX_Init 내부에서 커널 시작 전(또는 직후)에 호출됨.
 */
void Debug_Init(void)
{
    if (s_initDone) return;

    tx_byte_pool_create(&s_debug_pool, "dbg_pool", s_pool_memory, sizeof(s_pool_memory));
    tx_queue_create(&s_debug_queue, "dbg_que", 2 /* 2 ULONGs per msg */, s_queue_memory, sizeof(s_queue_memory));
    tx_event_flags_create(&s_debug_events, "dbg_evt");

    /* Debug Task는 우선순위 10으로 생성 (LCD=5, PLC=3 보다 낮음. 필요시 조정) */
    tx_thread_create(&s_debug_thread, "Debug Task", Debug_Task_Entry, 0,
                     s_debug_stack, sizeof(s_debug_stack),
                     10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);

    s_initDone = 1;
}

/**
 * @brief  DMA TX 완료 핸들러 (ISR 컨텍스트)
 * @note   HAL_UART_TxCpltCallback 에서 huart == &DEBUG_UART_INSTANCE 일 때 호출
 */
void Debug_TxCpltHandler(void)
{
    if (s_initDone) {
        /* DMA 완료 이벤트를 Task로 전달 */
        tx_event_flags_set(&s_debug_events, DEBUG_EVENT_TX_DONE, TX_OR);
    }
}

/* ----------------------- _write 재정의 (syscalls.c weak 함수 대체) ------- */

/**
 * @brief  _write() 재정의 — printf()가 Non-blocking으로 메시지 큐를 타도록 처리
 * @param  file  파일 디스크립터 (미사용)
 * @param  ptr   출력할 데이터 포인터
 * @param  len   출력할 바이트 수
 * @retval 요청된 len (일부 드롭 시에도 동일)
 */
int _write(int file, char *ptr, int len)
{
    (void)file;

    if (len <= 0 || !s_initDone) return len;
    
    /* 스케줄러 구동 전이거나 ISR 컨텍스트라면 블로킹 방식으로 즉시 출력하여 로그 유실 방지 */
    if (tx_thread_identify() == TX_NULL) {
        HAL_UART_Transmit(&DEBUG_UART_INSTANCE, (uint8_t*)ptr, len, HAL_MAX_DELAY);
        return len;
    }

    uint8_t *mem;
    
    /* 1. 메모리 풀에서 공간 할당 */
    if (tx_byte_allocate(&s_debug_pool, (VOID **)&mem, len, ALLOC_WAIT) != TX_SUCCESS)
    {
        return len; /* 할당 실패 시 DROP (또는 Timeout) */
    }

    /* 2. 데이터 복사 */
    memcpy(mem, ptr, len);

    /* 3. 큐에 포인터와 길이 전송 */
    ULONG msg[2] = { (ULONG)mem, (ULONG)len };
    if (tx_queue_send(&s_debug_queue, msg, ALLOC_WAIT) != TX_SUCCESS)
    {
        /* 큐 전송 실패 시 메모리 반환 */
        tx_byte_release(mem);
    }

    return len;
}

/**
 * @brief  Binary 데이터를 UART2 DMA를 통해 전송 (TraceX 덤프 전용 등)
 * @note   매우 긴 데이터일 경우 풀 크기에 맞춰 청크(Chunk) 단위로 분할하여 큐에 전송.
 */
void Debug_SendBinary(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0u || !s_initDone) return;
    
    /* 스케줄러 구동 전이거나 ISR 컨텍스트라면 블로킹 방식으로 즉시 출력 */
    if (tx_thread_identify() == TX_NULL) {
        HAL_UART_Transmit(&DEBUG_UART_INSTANCE, (uint8_t*)data, len, HAL_MAX_DELAY);
        return;
    }

    uint32_t offset = 0;
    
    /* Pool의 최대 할당 가능한 크기는 약간의 오버헤드를 제외한 사이즈. 안전하게 절반씩 전송. */
    const uint32_t MAX_CHUNK = DEBUG_BYTE_POOL_SIZE / 2;

    while (offset < len)
    {
        uint32_t toWrite = len - offset;
        if (toWrite > MAX_CHUNK) {
            toWrite = MAX_CHUNK;
        }

        uint8_t *mem;
        if (tx_byte_allocate(&s_debug_pool, (VOID **)&mem, toWrite, ALLOC_WAIT) != TX_SUCCESS)
        {
            break; /* DROP 정책 시 남은 데이터 포기 */
        }

        memcpy(mem, data + offset, toWrite);

        ULONG msg[2] = { (ULONG)mem, (ULONG)toWrite };
        if (tx_queue_send(&s_debug_queue, msg, ALLOC_WAIT) != TX_SUCCESS)
        {
            tx_byte_release(mem);
            break;
        }

        offset += toWrite;
    }
}
