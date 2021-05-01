#include "../inc/LaunchPad.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "ports.h"

/* Rearm Port C button interrupts */
void rearmButtonsC()
{
	/* Port C (Lab Board Switches) */
	GPIO_PORTC_IS_R &= ~0xF0;						// PC4-7 are edge-sensitive
	GPIO_PORTC_IBE_R &= ~0xF0;					// PC4-7 are single edge
	GPIO_PORTC_IEV_R |= 0xF0;						// PC4-7 are rising edge
	GPIO_PORTC_ICR_R = 0xF0;						// Clear all flags
	GPIO_PORTC_IM_R |= 0xF0;						// Arm interrupts on PC4-7
}

/* Disarm Port C button interrupts */
void disarmButtonsC()
{
	GPIO_PORTC_IM_R &= ~0xF0;						// Disarm all PC Switches
}

void initButtons()
{
	/* Port C (Lab Board Switches) */
	GPIO_PORTC_IS_R &= ~0x30;						// PC4-5 are edge-sensitive
	GPIO_PORTC_IBE_R &= ~0x30;					// PC4-5 are single edge
	GPIO_PORTC_IEV_R |= 0x30;						// PC4-5 are rising edge
	GPIO_PORTC_ICR_R = 0x30;						// Clear all flags
	GPIO_PORTC_IM_R |= 0x30;						// Arm interrupts on PC4-5
	
	/* Priority Configuration */
	NVIC_PRI0_R |= (0x1 << 23);					// Give interrupt 2 (C) priority 2
	NVIC_EN0_R = (1 << 2);							// Enable interrupts 2 (C)
}

void GPIOPortC(void)
{
	uint32_t state = GPIO_PORTC_MIS_R;	// Save state
	
	/* Wait before rearming to debounce */
	Timer0A_Init(160000, 3);
}

/* Oneshot for debounce */
void Timer0A_Handler()
{
	TIMER0_IMR_R = 0x00000000;
	rearmButtonsC();
}
