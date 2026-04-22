/**
  @page ST7580 PLC Modem Expansion Board for STM32 Nucleo Boards Point-to-Point example

  @verbatim
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    Point-to-Point Example/readme.txt
  * @author  CLAB
  * @version 1.1.0
  * @date    18-Sept-2017
  * @brief   This application is an example to show a Point-to-Point communication between 
			   two nodes using STM32 Nucleo boards and ST7580 expansion boards 
  ******************************************************************************
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  @endverbatim

@par Example Description

  This example aims to demonstrate a Point-to-Point powerline communication 
  using ST7580 driver and STM32Cube firmware. It involves two NUCLEO boards each 
  one equipped with a X-NUCLEO-PLM01A1 Expansion Board.
  
  Even if the two NUCLEO boards have two different roles in this example they are 
  programmed with the same firmware and the role of each board is assigned at boot
  according to NUCLEO USER button status. If pressed the board will assume MASTER 
  role, if released SLAVE role.
  
  After role assignment in an infinite loop the example will run the following
  actions:
	- Master board send a TRIGGER message, containing an ID character
	- Slave board receive the TRIGGER message
	- Slave board send an ACK message with the same ID back to master
	- Master board receive the ACK message
	
  Communication progress can be monitorated through the debug messages printed out 
  on the debug serial interface (Baudrate 115200, Data bit 8, Stop bit 1, No flow 
  control).
 
@par Hardware and Software environment

  - This example runs on STM32 Nucleo devices with ST7580 expansion board
    (X-NUCLEO-PLM01A1)
	
  - This example has been tested with STMicroelectronics:
    - NUCLEO-F401RE RevC board
    - NUCLEO-L053R8 RevC board

    and can be easily tailored to any other supported device and development board.


@par How to use it ? 

In order to make the program work, you must do the following:

  - WARNING: before opening the project with any toolchain be sure your folder
    installation path is not too in-depth since the toolchain may report errors
    after building.
  - The tested tool chain and environment is explained in the Release notes
  - Open the suitable toolchain (IAR, Keil, System Workbench for STM32) and open the project 
    for the required STM32 Nucleo board
  - Rebuild all files and load your image into target memory.
  - Run the example.
  - Alternatively, you can download the pre-built binaries in "Binary" 
    folder included in the distributed package.

  - IMPORTANT NOTE: To avoid issues with USB connection (mandatory if you have USB 3.0), it is   
    suggested to update the ST-Link/V2 firmware for STM32 Nucleo boards to the latest version.
    Please refer to the readme.txt file in the Applications directory for details.

Board connection:
 
  - The two NUCLEO boards, equipped with the X-NUCLEO-PLM01A1 Expansion board need to be 
    connected toghether through the CN1 screw connector. Debug serial interface is instead 
    available on NUCLEO usb connector.
  - The system composed by the NUCLEO and X-NUCLEO-PLM01A1 board need to be powered with an
    external 8-18V power supply. JP5 on the NUCLEO board need to be set on E5V position

For more details on example use and board setup and connection please refer to SW and HW 
user manual


 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
