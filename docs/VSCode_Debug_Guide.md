# VS Code 디버그 환경 가이드

> **대상**: STM32CubeIDE → VS Code + CMake + Clang + ThreadX 전환 팀원
> **프로젝트**: MultiSensorDAQ_PLC_PLM01A1 (STM32F446RE, NUCLEO-F446RE)
> **작성일**: 2026-05-06

---

## 목차

1. [개발환경 전환 개요](#1-개발환경-전환-개요)
2. [필수 설치 항목](#2-필수-설치-항목)
3. [프로젝트 빌드](#3-프로젝트-빌드)
4. [디버그 구성 (launch.json)](#4-디버그-구성-launchjson)
5. [디버그 기능별 사용법](#5-디버그-기능별-사용법)
6. [VS Code vs CubeIDE 기능 비교](#6-vs-code-vs-cubeide-기능-비교)
7. [알려진 문제 및 해결 방법](#7-알려진-문제-및-해결-방법)
8. [FAQ](#8-faq)

---

## 1. 개발환경 전환 개요

### 전환 배경

| 항목 | STM32CubeIDE | VS Code + CMake |
|---|---|---|
| 편집기 | Eclipse 기반 | VS Code (최신 UX) |
| 컴파일러 | GCC (arm-none-eabi) | **Clang (starm-clang)** |
| 빌드 시스템 | Make | **CMake + Ninja** |
| RTOS | FreeRTOS → ThreadX | **Azure RTOS ThreadX** |
| 버전관리 통합 | 제한적 | Git 완전 통합 |

### 툴체인 구성

```
STM32 Cube Bundles Manager (ST VS Code 확장 내장, CubeIDE 불필요)
├── 컴파일러: st-arm-clang 21.1.1+st.7     (LLVM 기반 ARM 최적화)
├── GDB:      gnu-gdb-for-stm32 14.3.1+st.2
├── GCC:      gnu-tools-for-stm32 14.3.1+st.2
├── CMake:    cmake 4.3.1+st.1
├── Ninja:    ninja 1.13.2+st.1
├── 디버그:   stlink-gdbserver 7.13.0+st.3  (ST-Link GDB Server — 기본 디버그 서버)
└── RTOS:     rtos-proxy 0.19.0+st.1        (ThreadX/FreeRTOS 뷰)

⚠️  OpenOCD는 Bundles Manager에 포함되지 않음
    → Cortex-Debug + OpenOCD 구성은 STM32CubeIDE 설치가 별도로 필요

VS Code 확장
├── ST.stm32-vscode-extension (ST 공식, 위 툴체인 자동 관리 포함)
└── marus25.cortex-debug (서드파티, 옵션 — OpenOCD 사용 시)
```

> **Bundles Manager**는 프로젝트·시스템별로 툴 버전을 독립 관리합니다.
> STM32CubeIDE와 완전히 별개로 동작하며, **ST-Link GDB Server 기반 디버그는 CubeIDE 없이 사용 가능**합니다.
> OpenOCD 기반 디버그(Cortex-Debug 구성)는 STM32CubeIDE 설치가 필요합니다.

---

## 2. 필수 설치 항목

### 2.1 소프트웨어

| 소프트웨어 | 버전 | 용도 |
|---|---|---|
| [VS Code](https://code.visualstudio.com/) | 최신 | 편집기 |
| [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) | 최신 | CMake + Clang 프로젝트 생성 (.ioc → CMakeLists.txt) |
| [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) | 최신 | ST-Link 플래시 프로그래밍 (별도 설치 필수) |
| Git | 최신 | 버전 관리 |

> STM32CubeIDE는 **기본 워크플로우에서는 설치 불필요**합니다.
> 컴파일러, GDB, CMake, Ninja, ST-Link GDB Server, RTOS 프록시 등 대부분의 툴체인은
> ST VS Code 확장에 내장된 **STM32 Cube Bundles Manager**가 자동으로 다운로드·관리합니다.
> 단, **STM32CubeProgrammer는 별도 설치**가 필요합니다. ST-Link GDB Server가 플래시 시 `STM32_Programmer_CLI.exe`를 직접 호출합니다.
>
> ⚠️ **Cortex-Debug + OpenOCD 구성을 사용하려면 STM32CubeIDE 설치가 필요합니다.**
> OpenOCD는 Bundles Manager에 포함되어 있지 않으며, CubeIDE 플러그인 경로의 OpenOCD를 참조합니다.
### 2.2 VS Code 확장

Extensions 탭(`Ctrl+Shift+X`)에서 설치:

| 확장 ID | 필수 여부 | 용도 |
|---|---|---|
| `ST.stm32-vscode-extension` | **필수** | ST 공식 빌드/디버그 통합 + 툴체인 자동 관리 |
| `marus25.cortex-debug` | 권장 | RTOS 스레드 뷰, OpenOCD 지원 |
| `llvm-vs-code-extensions.vscode-clangd` | 권장 | Clang 기반 IntelliSense |
| `ms-vscode.cmake-tools` | 권장 | CMake 통합 |

### 2.3 Bundles Manager — 관리 툴 목록

ST VS Code 확장 설치 후 `STM32CUBE BUNDLES` 패널에서 버전 확인 및 업데이트 가능:

| 번들 | 현재 버전 | 역할 |
|---|---|---|
| `st-arm-clang` | 21.1.1+st.7 | LLVM 기반 ARM 컴파일러 |
| `gnu-tools-for-stm32` | 14.3.1+st.2 | GCC 툴체인 (링커, 어셈블러) |
| `gnu-gdb-for-stm32` | 14.3.1+st.2 | GDB 디버거 |
| `cmake` | 4.3.1+st.1 | 빌드 시스템 |
| `ninja` | 1.13.2+st.1 | 빌드 실행기 |
| `programmer` | 2.22.0+st.1 | 용도 불명확 (ST 내부 도구로 추정, 플래시는 별도 설치한 STM32CubeProgrammer CLI 사용) |
| `rtos-proxy` | 0.19.0+st.1 | ThreadX/FreeRTOS RTOS 뷰 |
| `pack-manager` | 0.5.3 | CMSIS Pack 관리 |
| `node` | 22.22.0+st.1 | 확장 런타임 |
| `adoptium-jre` | 21.0.8+9.st.2 | 일부 ST 도구 런타임 |
| `cube-wrapper` | 0.10.2 | `cube-cmake` CLI 래퍼 |

---

## 3. 프로젝트 빌드

### 3.1 빌드 프리셋

이 프로젝트는 CMake 프리셋을 사용합니다. 두 가지 빌드 타입이 있습니다:

| 프리셋 | 최적화 | 디버그 심볼 | 용도 |
|---|---|---|---|
| `Debug` | `-Og` | `-g3` | **디버깅용** (소스 레벨 스텝, RTOS 뷰) |
| `Release` | `-O3` | 없음 | 양산/플래시 배포용 |

> ⚠️ **Release 빌드로는 소스 레벨 디버깅 불가.** 반드시 Debug 빌드를 사용할 것.

### 3.2 VS Code 태스크로 빌드

하단 상태바 또는 `Ctrl+Shift+P` → `Tasks: Run Task`:

| 태스크 이름 | 동작 |
|---|---|
| `Build (Debug)` | Debug 프리셋 빌드 → `.elf` 생성 |
| `Build (Release)` | Release 프리셋 빌드 |
| `Flash (Release)` | Release ELF를 STM32CubeProgrammer로 플래시 |
| `Clean (Release)` | Release 빌드 출력 삭제 |

### 3.3 터미널에서 직접 빌드

```powershell
# Debug 빌드
cube-cmake --build --preset Debug

# Release 빌드
cube-cmake --build --preset Release
```

---

## 4. 디버그 구성 (launch.json)

`.vscode/launch.json`에 두 가지 구성이 정의되어 있습니다.

### 4.1 구성 선택 방법

`Ctrl+Shift+D` (Run & Debug 패널) → 드롭다운에서 선택 → `F5`

### 4.2 구성 ① — ST-Link GDB Server (권장)

```json
"type": "stlinkgdbtarget",
"name": "STM32Cube: Debug (ST-Link GDB Server)"
```

**특징:**
- ST 공식 지원 구성
- ThreadX RTOS 뷰 지원 (`serverRtos.driver: "threadx"`)
- Live Watch (실시간 변수 모니터링, 읽기 전용)
- 자동 빌드 후 플래시 → 디버그 진입

**사용 시나리오:** 일반 디버깅, RTOS 태스크 상태 확인

### 4.3 구성 ② — Cortex-Debug + OpenOCD

> ⚠️ **이 구성은 STM32CubeIDE 설치가 필요합니다.**
> OpenOCD는 ST Bundles Manager에 포함되어 있지 않습니다.
> CubeIDE가 없으면 구성 ①(ST-Link GDB Server)만 사용 가능합니다.

```json
"type": "cortex-debug",
"name": "Cortex-Debug: Debug (OpenOCD)"
```

**특징:**
- VARIABLES/WATCH 패널 완전 지원
- ThreadX RTOS 스레드 인식 (`"rtos": "ThreadX"`)
- SVD 기반 페리페럴 레지스터 뷰
- OpenOCD 직접 제어 (고급 설정 가능)

> ⚠️ **OpenOCD swj-dp.tcl 버그 패치 적용됨**: `.vscode/openocd/target/swj-dp.tcl`
> ST OpenOCD 0.12.0에서 `hla newtap` → `swj_newdap` 무한재귀 버그를 패치한 파일입니다.
> 삭제하지 마세요.

---

## 5. 디버그 기능별 사용법

### 5.1 기본 디버그 플로우

```
F5 누르기
  └→ Debug 빌드 자동 실행
      └→ ELF 플래시
          └→ main() 진입점에서 일시 정지
              └→ F5 (Continue) → 코드 실행
```

### 5.2 중단점 (Breakpoints)

- 소스 파일 라인 번호 왼쪽 클릭 → 빨간 점 생성
- `F9`: 현재 라인 중단점 토글
- 어셈블리 파일에 중단점 설정 시: `설정(⚙️) → Allow Breakpoints Everywhere` 활성화 필요

### 5.3 CORTEX LIVE WATCH (실시간 변수 모니터링)

SWD를 통해 GDB를 우회하여 **코드 실행 중에도** 변수 값을 읽습니다.

**사용 방법:**
1. 디버그 패널 하단 **CORTEX LIVE WATCH** 탭 클릭
2. `+` 버튼 → 전역 변수명 입력 (예: `g_livewatch_val`)
3. `F5`로 코드 실행 → 실시간 값 갱신 확인

> ⚠️ **읽기 전용**: 실행 중 값 수정은 불가합니다. (→ 5.5 참고)

### 5.4 STM32CUBE RTOS 뷰 (ThreadX 태스크 목록)

**⚠️ 중요**: 타겟이 실행 중일 때는 RTOS 뷰가 갱신되지 않습니다.

**태스크 목록 확인 방법:**
1. `F5`로 코드 실행
2. 확인하고 싶을 때 ⏸️ **일시정지** (또는 태스크 내부 중단점 히트)
3. 하단 **STM32CUBE RTOS** 탭에서 Threads 목록 확인
4. ▶ **Continue** (`F5`)로 재실행

**이 프로젝트의 태스크 목록:**

| 태스크 이름 | 우선순위 | 스택 | 역할 |
|---|---|---|---|
| PLC Task | 3 (높음) | 2KB | PLC 통신, 상태머신 |
| LCD Task | 5 | 2KB | ILI9341 렌더링 |
| DisplayTask | 6 | (LCD 내부) | LCD 커맨드 큐 |
| LiveWatch Task | 8 | 1KB | 디버그용 값 출력 |
| Debug Task | 10 (낮음) | - | 비동기 UART 출력 |

### 5.5 실행 중 변수 값 수정 (Memory View 활용)

**정지 후 수정 (일반적인 방법):**
1. ⏸️ 일시정지
2. VARIABLES 패널 → 변수 더블클릭 → 값 입력

**실행 중 메모리 직접 수정:**
1. Debug Console 탭에서 주소 확인:
   ```
   >p &g_livewatch_val
   ```
2. `View → Open View → Memory` → 주소 입력
3. 해당 주소의 바이트를 직접 수정 → SWD로 즉시 반영

### 5.6 페리페럴 레지스터 뷰 (Cortex-Debug 구성 전용)

Cortex-Debug 구성으로 실행 시 **STM32F446 SVD** 파일을 통해 페리페럴 레지스터를 확인할 수 있습니다.

`Ctrl+Shift+P` → `Cortex-Debug: View Peripheral Registers`

---

## 6. VS Code vs CubeIDE 기능 비교

| 기능 | STM32CubeIDE | VS Code (ST 확장) | 비고 |
|---|---|---|---|
| 소스 레벨 디버깅 | ✅ | ✅ | Debug 빌드 필수 |
| 중단점 | ✅ | ✅ | |
| VARIABLES/WATCH | ✅ | ✅ | |
| 정지 후 변수 수정 | ✅ | ✅ | |
| RTOS 태스크 뷰 | ✅ 실행 중 | ⚠️ 정지 시만 | 구조적 한계 |
| Live Watch (읽기) | ✅ | ✅ | |
| **실행 중 변수 쓰기** | ✅ Force Value | ❌ 미지원 | ST-Link 전용 API |
| 페리페럴 레지스터 뷰 | ✅ | ✅ (Cortex-Debug) | SVD 파일 필요 |
| 메모리 뷰/수정 | ✅ | ✅ | 실행 중 수정 가능 |
| SWO/ITM 트레이스 | ✅ | △ 제한적 | |
| 멀티코어 디버그 | ✅ | △ 설정 복잡 | H7 시리즈 해당 |

> **"실행 중 변수 쓰기" 미지원 이유**: CubeIDE의 Force Value는 GDB를 거치지 않고 ST-Link API로 SWD 버스에 직접 메모리를 씁니다. 이 API는 VS Code extension에 공개되어 있지 않으며 ST 로드맵에 추가 예정입니다.

---

## 7. 알려진 문제 및 해결 방법

### 문제 ① — OpenOCD 실행 직후 종료 (swj-dp.tcl 무한재귀)

**증상:**
```
OpenOCD: GDB Server Quit Unexpectedly
```

**원인:** ST OpenOCD 0.12.0이 `hla newtap`을 deprecated 처리하면서 `swj_newdap`으로 리다이렉트했으나, ST 자신의 `swj-dp.tcl`이 여전히 `hla newtap`을 호출하여 무한재귀 발생.

**해결:** `.vscode/openocd/target/swj-dp.tcl` 패치 파일이 적용되어 있습니다. `searchDir`에 이 디렉토리가 ST 스크립트보다 먼저 지정되어 있어 자동으로 패치 버전이 사용됩니다.

> ⚠️ `.vscode/openocd/` 디렉토리를 삭제하거나 `launch.json`의 `searchDir` 순서를 바꾸지 마세요.

---

### 문제 ② — "디버그 형식이 인식되지 않습니다" (openocdtarget)

**증상:** launch.json에서 `"type": "openocdtarget"` 경고 표시

**원인:** 설치된 ST VS Code extension 버전이 `openocdtarget` 타입을 미지원.

**해결:** `stlinkgdbtarget` 구성을 사용하세요. ST extension 업데이트 시 지원될 예정입니다.

---

### 문제 ③ — RTOS 뷰 "Not started" 또는 "No data available"

**증상:** STM32CUBE RTOS 패널에 스레드 목록이 표시되지 않음

**원인 및 해결:**

| 원인 | 해결 |
|---|---|
| `main()` 진입점에서 정지 상태 | `F5` Continue로 실행 후 ⏸️ 일시정지 |
| 타겟이 실행 중 상태 | ⏸️ 일시정지 또는 태스크 내 중단점 설정 |
| `tx_kernel_enter()` 미진입 | `App_ThreadX_Init()` 내 `Error_Handler()` 진입 여부 확인 |

---

### 문제 ④ — LiveWatch Task 스택 오버플로우

**증상:** LiveWatch Task의 첫 번째 `printf` 출력 후 전체 시스템 멈춤

**원인:** `printf` → `_write` → `tx_byte_allocate` 호출 체인이 ~400B 이상 스택을 소비하여 512B 스택 오버플로우 발생.

**해결:** LiveWatch Task 스택을 1024B로 설정 (현재 적용됨).

```c
ret = tx_byte_allocate(byte_pool, &p_stack, 1024u, TX_NO_WAIT);
```

---

### 문제 ⑤ — Debug 빌드 출력 파일이 없음

**증상:** `build/Debug/` 폴더가 없거나 `.elf` 파일이 없음

**원인:** Debug 프리셋으로 한 번도 빌드하지 않음

**해결:**
```powershell
cube-cmake --build --preset Debug
```

또는 VS Code 태스크: `Build (Debug)`

---

## 8. FAQ

**Q. Release 빌드로 디버그하면 변수가 "optimized out"으로 표시됩니다.**

A. Release 빌드는 `-O3 -g0`으로 컴파일되어 디버그 심볼이 없습니다. 소스 레벨 디버깅은 반드시 `Debug 프리셋 빌드 → Debug 구성`으로 진행하세요.

---

**Q. 디버그 세션 시작 시 "ST-LINK firmware needs update" 메시지가 나옵니다.**

A. VS Code 좌측 `STM32CUBE DEVICES AND BOARDS` 패널에서 해당 ST-Link 옆의 ↓ (다운로드) 버튼을 클릭하여 펌웨어를 업데이트하세요.

---

**Q. 두 보드(NUCLEO-F446RE × 2)를 동시에 연결했을 때 어떤 보드에 연결됩니까?**

A. ST-Link GDB Server가 첫 번째 감지된 보드에 연결됩니다. 특정 보드를 지정하려면 `launch.json`의 `serverArgs`에 Serial Number를 추가해야 합니다:

```json
"serverArgs": ["-serialNumber", "066DFF515250898367075515"]
```

---

**Q. `cube-cmake` 명령어를 찾을 수 없습니다.**

A. ST VS Code extension이 설치되어 있어야 `cube-cmake` 래퍼가 PATH에 등록됩니다. Extension을 재설치하거나 VS Code를 재시작하세요.

---

## 참고 링크

- [ST 공식 launch.json 가이드](https://community.st.com/t5/stm32-mcus/stm32cubeide-for-vs-code-debug-configuration-launch-json/ta-p/866870)
- [STM32CubeIDE for VS Code 공식 문서](https://dev.st.com/stm32cube-docs/stm32cubeide-vscode/1.0.1/en/docs/markup/development/debug.html)
- [Cortex-Debug 위키](https://github.com/Marus/cortex-debug/wiki)
- [Azure RTOS ThreadX 문서](https://learn.microsoft.com/en-us/azure/rtos/threadx/)
