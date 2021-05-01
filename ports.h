#include <stdint.h>
void Unified_Port_Init(void);
void Port_A_Init(void);	//ST7735
void Port_B_Init(void);	//Unused
void Port_C_Init(void);	//Inputs
void Port_D_Init(void);	//DAC
void Port_E_Init(void);	//UART
void Port_F_Init(void);	//Unused
void Timer0A_Init(uint32_t period, uint32_t priority);	//Oneshot for debouncing
void Timer1A_Init(uint32_t period, uint32_t priority);
void Timer2A_Init(uint32_t period, uint32_t priority);
void Timer3A_Init(uint32_t period, uint32_t priority);	//Sound
