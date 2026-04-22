cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME MultiSensorDAQ_PLC_PLM01A1)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # 애플리케이션 소스
    Core/Src/c_plc_appli.c
    # BSP: X-NUCLEO-PLM01A1
    Drivers/BSP/X-NUCLEO-PLM01A1/stm32_plm01a1.c
    # BSP: STM32F4xx Nucleo (LED/버튼 추상화)
    Drivers/BSP/STM32F4xx_Nucleo/stm32f4xx_nucleo.c
    # ST7580 PLC 모뎀 드라이버
    Drivers/BSP/Components/ST7580/ST7580_Library/Src/ST7580_Serial.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # 애플리케이션 헤더
    Core/Inc
    # BSP: X-NUCLEO-PLM01A1
    Drivers/BSP/X-NUCLEO-PLM01A1
    # BSP: STM32F4xx Nucleo
    Drivers/BSP/STM32F4xx_Nucleo
    # ST7580 라이브러리 헤더
    Drivers/BSP/Components/ST7580/ST7580_Library/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # STM32F4xx Nucleo BSP 및 cube_hal.h 조건부 컴파일 활성화
    USE_STM32F4XX_NUCLEO
    # CubeMX가 stm32f4xx_hal_msp.c에서 HAL_UART_MspInit를 생성하므로
    # BSP(stm32_plm01a1.c)의 중복 정의 비활성화
    USE_CUBEMX_MSP
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
