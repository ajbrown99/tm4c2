// ---------------------------------------------------------------------------
//
// File name: Unified_Port_Init.c
//
// Author: Mark McDermott
// Orig. gen date: July 12, 2020
// Last update: September 12, 2020 Fix references to SSI3
//
// Description: This is the unified Port initialization routine for
// the 2020 Labs
//
// Usage: Call Unified_Port_Init() if you want to initialize
// all ports (preferred)
// Call the individual port inits as needed.
//
// ---------------------------------------------------------------------------

#include <stdint.h>
#include "../inc/LaunchPad.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "Unified_Port_Init.h"

// ------------------------ Unified Port Init --------------------------------
void Unified_Port_Init(void)
{
	Port_A_Init();
	Port_B_Init();
	Port_C_Init();
	Port_D_Init();
	Port_E_Init();
	Port_F_Init();
}
// ----------------------------------------------------------------------------
// ---------------- PORT A Initialization --------------------------------
// ----------------------------------------------------------------------------
//
// Port A drives the ST7735 LCD
//
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) connected to PA4
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) connected to PB0 (GPIO)
// Data/CMD (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground
//
void Port_A_Init(void)
{

 SYSCTL_RCGCSSI_R |= 0x01; // Activate SSI0

 SYSCTL_RCGCGPIO_R |= 0x01; // Activate port A
 while((SYSCTL_PRGPIO_R & 0x01)==0){}; // allow time for clock to start

 GPIO_PORTA_DIR_R |= 0xC8; // make PA3,6,7 out
 GPIO_PORTA_AFSEL_R &= ~0xC8; // disable alt funct on PA3,6,7
 GPIO_PORTA_DEN_R |= 0xC8; // enable digital I/O on PA3,6,7

 // configure PA3,6,7 as GPIO
 GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R
 & 0x00FF0FFF)
 + 0x00000000;

 GPIO_PORTA_AMSEL_R &= ~0xC8; // disable analog functionality on PA3,6,7
// initialize SSI0
 GPIO_PORTA_AFSEL_R |= 0x2C; // enable alt funct on PA2,3,5
 GPIO_PORTA_DEN_R |= 0x2C; // enable digital I/O on PA2,3,5
 // configure PA2,3,5 as SSI
 GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R
 & 0xFF0F00FF)
+ 0x00202200;

 GPIO_PORTA_AMSEL_R &= ~0x2C; // disable analog functionality on PA2,3,5
 SSI0_CR1_R &= ~SSI_CR1_SSE; // disable SSI
 SSI0_CR1_R &= ~SSI_CR1_MS; // master mode

 // configure for system clock/PLL baud clock source
 SSI0_CC_R = (SSI0_CC_R&~SSI_CC_CS_M)
 + SSI_CC_CS_SYSPLL;

 // clock divider for 8 MHz SSIClk (80 MHz PLL/24)
 // SysClk/(CPSDVSR*(1+SCR))
 // 80/(10*(1+0)) = 8 MHz (slower than 4 MHz)
 SSI0_CPSR_R = (SSI0_CPSR_R
 &~SSI_CPSR_CPSDVSR_M)
 + 10; // must be even number

 // SCR = 0 (8 Mbps data rate)
 // SPH = 0
 // SPO = 0
 SSI0_CR0_R &= ~(SSI_CR0_SCR_M
 | SSI_CR0_SPH
| SSI_CR0_SPO);

 // FRF = Freescale format
 SSI0_CR0_R = (SSI0_CR0_R
 &~SSI_CR0_FRF_M)
 + SSI_CR0_FRF_MOTO;

 // DSS = 8-bit data
 SSI0_CR0_R = (SSI0_CR0_R
 &~SSI_CR0_DSS_M)
 + SSI_CR0_DSS_8;
 // enable SSI
 SSI0_CR1_R |= SSI_CR1_SSE;

}
// ----------------------------------------------------------------------------
// ---------------- PORT B Initialization --------------------------------
// ----------------------------------------------------------------------------
//
// PB0 = LED3
void Port_B_Init(void)
{
 GPIO_PORTB_PCTL_R &= ~0x0000FFFF; // GPIO
 GPIO_PORTB_DIR_R &= 0x00; // make PB3-0 out
 GPIO_PORTB_AFSEL_R &= ~0x01; // regular port function
 GPIO_PORTB_DEN_R |= 0x01; // enable digital I/O on PB3-0
 GPIO_PORTB_AMSEL_R &= ~0x01; // disable analog functionality on PB3-0


 }
// ----------------------------------------------------------------------------
// ---------------- PORT C Initialization --------------------------------
// ----------------------------------------------------------------------------
//
// PC4 = LED2
// PC5 = LED1
// PC6 = Joystick 1 Horizontal
// PC7 = Joystick 1 Vertical
//
void Port_C_Init(void){

 SYSCTL_RCGCGPIO_R |= 0x04; // Activate clock for Port C
 while((SYSCTL_PRGPIO_R & 0x04) != 0x04){}; // Allow time for clock to start

 GPIO_PORTC_PCTL_R &= ~0xFFFF0000; // regular GPIO
 GPIO_PORTC_AMSEL_R &= ~0x30; // disable analog function on PC4,5
 GPIO_PORTC_AMSEL_R |= 0xC0; // enable analog function on PC6,7
 GPIO_PORTC_DIR_R &= ~0xC0; // inputs on PC7-PC6
 GPIO_PORTC_DIR_R |= 0x30; //outputs on PC5-PC4
 GPIO_PORTC_AFSEL_R &= ~0xF0; // regular port function
 GPIO_PORTC_PUR_R = 0x00; // disable pull-up on PC7-PC4
 GPIO_PORTC_PDR_R = 0x00;	// enable pull-down on PC4,5
 GPIO_PORTC_DEN_R |= 0x0; // enable digital port
 }

// ----------------------------------------------------------------------------
// ---------------- PORT D Initialization --------------------------------
// ----------------------------------------------------------------------------
//
// PD7 = U2TX
// PD6 = U2RX
// PD3 = SSI1_MOSI (to TLV5616)
// PD2 = AIN5 (Monitor DAC_OUT signal from TLV5616)
// PD1 = SSI1_FS/CS (to TLV5616)
// PD0 = SSI1_SCK (to TLV5616)

void Port_D_Init(void){
 
  SYSCTL_RCGCGPIO_R     |= 0x08;            // activate port D
  while((SYSCTL_PRGPIO_R & 0x08)==0){};     // allow time for clock to stabilize

  // ---------------  Initialize PB7 as U2TX, PB6 as U2RX  -----------------------  
    
  GPIO_PORTD_LOCK_R      = 0x4C4F434B;      // unlock REQUIRED for PD7
  GPIO_PORTD_CR_R       |= 0xC0;            // commit PD6, PD7
  
  GPIO_PORTD_AMSEL_R    &= ~0xC0;           // disable analog functionality on PD6, PD7
  GPIO_PORTD_AFSEL_R    |=  0xC0;           // enable alternate function on PD6, PD7
  GPIO_PORTD_DEN_R      |=  0xC0;           // enable digital on PD6, PD7 
                                            // (PD6 is U2RX, PD7 is U2TX)
  GPIO_PORTD_PCTL_R      =(GPIO_PORTD_PCTL_R  
                         & 0x00FFFFFF)
                         | 0x11000000;      // configure PD6, PD7 as UART
 
   // ---------------  Initialize PD2 as AIN5  ----------------------------------  
   
  GPIO_PORTD_DIR_R      &= ~0x04;           // make PD2 input
  GPIO_PORTD_AFSEL_R    |=  0x04;           // enable alternate function on PD2
  GPIO_PORTD_DEN_R      &= ~0x04;           // disable digital I/O on PD2
  GPIO_PORTD_AMSEL_R    |=  0x04;           // enable analog functionality on PD2

  // ---------------  Initialize PD3,1,0 as SSI1 MOSI, FS & SCK  ---------------- 
    
  GPIO_PORTD_AMSEL_R    &= ~0x0B;           // disable analog functionality on PD
  GPIO_PORTD_AFSEL_R    |=  0x0B;           // enable alt funct on PD3,1,0
  GPIO_PORTD_DEN_R      |=  0x0B;           // enable digital I/O on PD3,1,0
  GPIO_PORTD_PCTL_R      = (GPIO_PORTD_PCTL_R
                          & 0xFFFF0F00)
                          + 0x00002022;
}

// ----------------------------------------------------------------------------
// ---------------- PORT E Initialization --------------------------------
// ----------------------------------------------------------------------------
//

// PE1 = TX
// PE0 = RX
void Port_E_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x10; // activate port E
 while((SYSCTL_PRGPIO_R & 0x10)==0){};
// --------------- Initialize PE5 as U5TX, PE4 as U5RX -----------------------
 GPIO_PORTE_AFSEL_R |= 0x03; // enable alt funct on PE1-0
 GPIO_PORTE_DEN_R |= 0x03; // enable digital I/O on PE1-0
 // configure PE5-4 as UART
 GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R
 & 0xFFFFFF00)
+ 0x00000011;
 GPIO_PORTE_AMSEL_R &= ~0x03; // disable analog functionality on PE1-0

}
// ----------------------------------------------------------------------------
// ---------------- PORT F Initialization --------------------------------
// ----------------------------------------------------------------------------
//
void Port_F_Init(void){

 SYSCTL_RCGCGPIO_R |= 0x00000020; // activate clock for Port F
 while((SYSCTL_PRGPIO_R & 0x20)==0){}; // allow time for clock to stabilize

 GPIO_PORTF_LOCK_R = 0x4C4F434B; // unlock GPIO Port F
 GPIO_PORTF_CR_R = 0x1F; // allow changes to PF4-0

 GPIO_PORTF_AMSEL_R = 0x00; // disable analog on PF
 GPIO_PORTF_PCTL_R = 0x00000000; // PCTL GPIO on PF4-0
 GPIO_PORTF_DIR_R = 0x0E; // PF4,PF0 in, PF3-1 out
 GPIO_PORTF_AFSEL_R = 0x00; // disable alt funct on PF7-0
 GPIO_PORTF_PUR_R = 0x11; // enable pull-up on PF0 and PF4
 GPIO_PORTF_DEN_R = 0x1F; // enable digital I/O on PF4-0
}
