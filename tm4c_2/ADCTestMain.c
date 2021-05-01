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

//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
// TExaSdisplay logic analyzer shows 5 bits 0,0,PF4,PF3,PF2,PF1,PF0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
	
uint16_t ADCPosition = 0;
uint16_t ADCMeasurements[1000] = {0};
uint32_t ADCTimes[1000] = {0};
uint32_t PMF[4096];
uint32_t smallestDiff = 0xFFFFFFFF;
uint32_t largestDiff  = 0x00000000;
uint32_t jitter = 0;
uint32_t minVal = 0;
uint32_t maxVal = 0;


void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|GPIO_PORTF_DATA_R; // sends at 10kHz
}
// measures analog voltage on PD3
void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}


uint32_t ADCvalue;

void Timer3A_Init(uint32_t period, uint32_t priority){
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|(priority<<29); // priority  
// interrupts enabled in the main program after all devices initialized
// vector number 51, interrupt number 35
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER3A
}

void Timer3A_Handler(void){
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER3A timeout
}


// ------------ DAC_Init --------------------------------------
//
// Initialize TLV5616 12-bit DAC
// assumes bus clock is 80 MHz
// inputs: initial voltage output (0 to 4095)
// outputs: none
//
void DAC_Init(uint16_t data){
 SYSCTL_RCGCSSI_R |= 0x02; // activate SSI1
 while((SYSCTL_RCGCSSI_R & 0x0002) == 0){}; // ready?

 SSI1_CR1_R = 0x00000000; // disable SSI, master mode
 SSI1_CPSR_R = 0x08; // 80MHz/8 = 10 MHz SSIClk (should work upto 20 MHz)
 SSI1_CR0_R &= ~(0x0000FFF0); // SCR = 0, SPH = 0, SPO = 1 FreescaleMode
 SSI1_CR0_R += 0x40; // SPO = 1
 SSI1_CR0_R |= 0x0F; // DSS = 16-bit data
 SSI1_DR_R = data; // load 'data' into transmit FIFO
 SSI1_CR1_R |= 0x00000002; // enable SSI
}
// ------------------- DAC_Out -------------------------------------
//
// Send data to TLV5616 12-bit DAC
// inputs: voltage output (0 to 4095)
// outputs: return parameter from SSI (not meaningful)
//
void DAC_Out(uint16_t code){
 while((SSI1_SR_R & 0x00000002)==0){}; // wait until room in FIFO
 SSI1_DR_R = code; // data out
 while((SSI1_SR_R & 0x00000004)==0){}; // wait until response
 }

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

//centerValue >= 64, centerValue <= 4032
void graphResults()
{
	uint32_t accumulation = 0;					
	ST7735_PlotClear(0, 400);
	for (int i = 0; i < 4096; i++)		//graph 128 bars, each accumulated from 32 samples
	{
		accumulation += PMF[i];
		if ((i + 1) % 32 == 0)
		{
			ST7735_PlotBar(accumulation);
			ST7735_PlotNext();
			accumulation = 0;
		}
	}
}
 
// This debug function initializes Timer0A to request interrupts
// at a 100 Hz frequency.  It is similar to FreqMeasure.c.
void Timer0A_Init100HzInt(void){
  volatile uint32_t delay;
  DisableInterrupts();
  // **** general initialization ****
  SYSCTL_RCGCTIMER_R |= 0x01;      // activate timer0
  delay = SYSCTL_RCGCTIMER_R;      // allow time to finish activating
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = 0;                // configure for 32-bit timer mode
  // **** timer0A initialization ****
                                   // configure for periodic mode
  TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
  TIMER0_TAILR_R = 799999;         // start value for 100 Hz interrupts
  TIMER0_IMR_R |= TIMER_IMR_TATOIM;// enable timeout (rollover) interrupt
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// clear timer0A timeout flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 32-b, periodic, interrupts
  // **** interrupt initialization ****
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R = 1<<19;              // enable interrupt 19 in NVIC
}
void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;    // acknowledge timer0A timeout
  PF2 ^= 0x04;                   // profile
  ADCvalue = ADC0_InSeq3();
	if (ADCPosition < 1000)					//section for filling the array
	{
		ADCMeasurements[ADCPosition] = ADCvalue;	//set current measurement
		ADCTimes[ADCPosition] = TIMER1_TAR_R;
		ADCPosition++;
	}
	else if (ADCPosition == 1000)		//when array is full perform computations
	{
		PMF[ADCMeasurements[0]]++;
		maxVal = 0;
			
		for (int i = 1; i < 1000; i++)					//jitter measurement
		{
			PMF[ADCMeasurements[i]]++;						//increment location in pmf
			if ((ADCTimes[i - 1] - ADCTimes[i]) < smallestDiff)
			{
				smallestDiff = ADCTimes[i - 1] - ADCTimes[i];
			}
			if ((ADCTimes[i - 1] - ADCTimes[i]) > largestDiff)
			{
				largestDiff = ADCTimes[i - 1] - ADCTimes[i];
			}
			jitter = largestDiff - smallestDiff;
		}
		
		for (int i = 0; i < 4096; i++)						//pmf max calculation
		{
			if (PMF[i] > maxVal)
			{
				maxVal = PMF[i];
			}
		}
		
		ADCPosition++;
		graphResults();
	}
}

void Timer1_Init(void){ 
 volatile uint32_t delay;
 SYSCTL_RCGCTIMER_R |= 0x02; // 0) activate TIMER1
 delay = SYSCTL_RCGCTIMER_R; // allow time to finish activating
 TIMER1_CTL_R = 0x00000000; // 1) disable TIMER1A during setup
 TIMER1_CFG_R = 0x00000000; // 2) configure for 32-bit mode
 TIMER1_TAMR_R = 0x00000002; // 3) configure for periodic mode, down-count
 TIMER1_TAILR_R = 0xFFFFFFFF; // 4) reload value
 TIMER1_TAPR_R = 0; // 5) bus clock resolution
 TIMER1_CTL_R = 0x00000001; // 10) enable TIMER1A
}

int main(void){
// pick one of the following three lines, all three set PLL to 80 MHz
  //PLL_Init(Bus80MHz);              // 1) call to have no TExaS debugging
  //TExaS_SetTask(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  TExaS_SetTask(&ScopeTask);       // or 3) call to activate analog scope PD3
  
  LaunchPad_Init();    	// activate port F
	Output_Init();
	Timer1_Init();
	Port_D_Init();
	DAC_Init(0);
	Timer3A_Init(7999, 1);
	ADC0_InitSWTriggerSeq3(5);      // allow time to finish activating
  ADC0_SAC_R = 0x6;
	Timer0A_Init100HzInt();            // set up Timer0A for 100 Hz interrupts
	PF2 = 0;                           // turn off LED
  EnableInterrupts();
  while(1){
    PF1 ^= 0x02;  // toggles when running in main
  }
}


