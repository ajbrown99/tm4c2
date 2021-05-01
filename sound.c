#include "../inc/LaunchPad.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "sound.h"
#include "ports.h"
#include "../inc/ST7735.h"

// ------------ DAC_Init --------------------------------------
//
// Initialize TLV5616 12-bit DAC
// assumes bus clock is 80 MHz
// inputs: initial voltage output (0 to 4095)
// outputs: none
//

const uint32_t init_x_coord = 30;
const uint32_t init_y_coord = 30;
uint8_t up_down_direction = 0; //0 -> down, 1 -> up
uint32_t curr_x_coord = init_x_coord;
uint32_t curr_y_coord = init_y_coord;

#define C 	261
#define Cs 	277
#define D 	293
#define Ds 	311
#define E		329
#define F		349
#define Fs	369
#define G 	392
#define Gs	415
#define A 	440
#define As	466
#define R 	1

volatile uint8_t noteIndex = 0;
volatile uint32_t secondCounter = 0;

const uint16_t song[] =
{
	C, R, C, R, G, R, G, R, A, R, A, R, G, G, G, G,	
	F, R, F, R, E, R, E, R, D, R, D, R, C, C, C, C,
	G, R, G, R, F, R, F, R, E, R, E, R, D, D, D, D,
	G, R, G, R, F, R, F, R, E, R, E, R, D, D, D, D,
	C, R, C, R, G, R, G, R, A, R, A, R, G, G, G, G,	
	F, R, F, R, E, R, E, R, D, R, D, R, C, C, C, C,
};

const uint8_t songLength = 95;

const uint16_t sine[] =
{
	2048,2448,2832,3186,3496,3751,3940,4057,
	4096,4057,3940,3751,3496,3186,2832,2448,
	2048,1648,1264,910,600,345,156,39,
	0,39,156,345,600,910,1264,1648,
};

volatile uint8_t sampleIndex = 0;

uint32_t get_init_x_coord(){
	return init_x_coord;
}

uint32_t get_init_y_coord(){
	return init_y_coord;
}

void DAC_Init(uint16_t data)
{
	SYSCTL_RCGCSSI_R |= 0x02; 									// activate SSI1
	while((SYSCTL_RCGCSSI_R & 0x0002) == 0){}; 	// ready?

	SSI1_CR1_R = 0x00000000; 										// Disable SSI, master mode
	SSI1_CPSR_R = 0x01; 												// 80MHz/1 = 80 MHz SSIClk (should work upto 80 MHz)
	SSI1_CR0_R &= ~(0x0000FFF0); 								// SCR = 0, SPH = 0, SPO = 1 FreescaleMode
	SSI1_CR0_R += 0x40; 												// SPO = 1
	SSI1_CR0_R |= 0x0F; 												// DSS = 16-bit data
	SSI1_DR_R = data; 													// Load 'data' into transmit FIFO
	SSI1_CR1_R |= 0x00000002; 									// Enable SSI
}

// ------------------- DAC_Out -------------------------------------
//
// Send data to TLV5616 12-bit DAC
// inputs: voltage output (0 to 4095)
// outputs: return parameter from SSI (not meaningful)
//
void DAC_Out(uint16_t code)
{
	while((SSI1_SR_R & 0x00000002) == 0){}; // wait until room in FIFO
	SSI1_DR_R = code; 											// data out
	while((SSI1_SR_R & 0x00000004) == 0 ){}; // wait until response
}

void stopSound()
{
	TIMER3_CTL_R = 0x00000000; 
}

void playSong()
{
	Timer3A_Init(80000000/song[noteIndex]/32, 3);
}

void Timer3A_Handler()
{
	if (secondCounter == (8 * song[noteIndex])) 	//1/4 second per note
	{
		secondCounter = 0;
		if (noteIndex < songLength)									//check if song over
		{
			noteIndex++;
			playSong();
		}
		else
		{
			noteIndex = 0;
			secondCounter = 0;
			stopSound();
		}
	}
	else
	{
		secondCounter++;
	}
	TIMER3_ICR_R = TIMER_ICR_TATOCINT;
	DAC_Out(sine[sampleIndex]);
	sampleIndex = (sampleIndex + 1) % 32;
}


