#include "../inc/tm4c123gh6pm.h"
#include "../inc/LaunchPad.h"
#include "../inc/CortexM.h"

void flashLED()
{
	GPIO_PORTC_DATA_R ^= 0x30;	//Flashing LEDs on PC4-5
	GPIO_PORTB_DATA_R ^= 0x01;	//Flashing LED on PB0
}