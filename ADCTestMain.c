// ADCTestMain.c
// Runs on TM4C123
// This program periodically samples ADC channel 0 and stores the
// result to a global variable that can be accessed with the JTAG
// debugger and viewed with the variable watch feature.
// Daniel Valvano
// Jan 18, 2021

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// center of X-ohm potentiometer connected to PE3/AIN0
// bottom of X-ohm potentiometer connected to ground
// top of X-ohm potentiometer connected to +3.3V 
#include <stdint.h>
#include "../inc/ADCSWTrigger.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/PLL.h"
#include "../inc/LaunchPad.h"
#include "../inc/CortexM.h"
#include "../inc/TExaS.h"
#include "../inc/ST7735.h"
#include "../inc/ST7735.c"
#include "sound.h"
#include "UART.h"
#include "controls.h"
#include "graphics.h"
#include "game.h"
#include "ports.h"

#define PF1       (*((volatile uint32_t *)0x40025008))

void OutCRLF(void)
{
  UART_OutChar(CR);
  UART_OutChar(LF);
}

int main(void)
{
	DisableInterrupts();
	PLL_Init(Bus80MHz);
	Unified_Port_Init();
	ADC_Init89();
	//ST7735_InitB();
	ST7735_InitR(INITR_REDTAB);
	UART_Init();
	Timer2A_Init(1333333,5);
	Timer1A_Init(6000000,3);
	ST7735_FillScreen(ST7735_WHITE);
	
	//DAC_Init(0);
	//playSong();
	
	//uint32_t n = 0;
	
	EnableInterrupts();
	
	while(1)
	{
		//for(int i = 0; i < 1000000; i++);
		//uint32_t test = UART_InChar();
		receive_puck_info();
		
		//ST7735_OutChar(test);
		/*
    n = (n + 1) % 212;
		UART_OutUDec(n);
		OutCRLF();
		uint32_t result = UART_InUDec();
		ST7735_OutUDec(result);
		//RxFifo_Init();
		//TxFifo_Init();
		*/
  }
}


