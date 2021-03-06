// UART.c
// Runs on LM4F120/TM4C123
// Use UART0 to implement bidirectional data transfer to and from a
// computer running PuTTy. Interrupts and software FIFOs are used.
// Daniel Valvano
// September 20, 2018
// Modified by EE345L students Charlie Gough && Matt Hawk
// Modified by EE345M students Agustinus Darmawan && Mingjie Qiu

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2018
   Program 5.11 Section 5.6, Program 3.10

 Copyright 2018 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

#include "UART.h"
#include "game.h"

#define NVIC_EN0_INT5           0x00000020  // Interrupt 5 enable

#define UART_FR_RXFF            0x00000040  // UART Receive FIFO Full
#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART_CTL_UARTEN         0x00000001  // UART Enable
#define UART_IFLS_RX1_8         0x00000000  // RX FIFO >= 1/8 full
#define UART_IFLS_TX1_8         0x00000000  // TX FIFO <= 1/8 full
#define UART_IM_RTIM            0x00000040  // UART Receive Time-Out Interrupt
                                            // Mask
#define UART_IM_TXIM            0x00000020  // UART Transmit Interrupt Mask
#define UART_IM_RXIM            0x00000010  // UART Receive Interrupt Mask
#define UART_RIS_RTRIS          0x00000040  // UART Receive Time-Out Raw
                                            // Interrupt Status
#define UART_RIS_TXRIS          0x00000020  // UART Transmit Raw Interrupt
                                            // Status
#define UART_RIS_RXRIS          0x00000010  // UART Receive Raw Interrupt
                                            // Status
#define UART_ICR_RTIC           0x00000040  // Receive Time-Out Interrupt Clear
#define UART_ICR_TXIC           0x00000020  // Transmit Interrupt Clear
#define UART_ICR_RXIC           0x00000010  // Receive Interrupt Clear



void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure

// Two-index implementation of the receive FIFO
// can hold 0 to RXFIFOSIZE elements
#define RXFIFOSIZE 64    // must be a power of 2
uint32_t volatile RxPutI; // put next
uint32_t volatile RxGetI; // get next
char static RxFifo[RXFIFOSIZE];

// initialize index FIFO
void RxFifo_Init(void){ long sr;
  sr = StartCritical(); // make atomic
  RxPutI = RxGetI = 0;  // Empty
  EndCritical(sr);
}
// add element to end of index FIFO
// return FIFOSUCCESS if successful
int RxFifo_Put(char data){
  if((RxPutI-RxGetI) & ~(RXFIFOSIZE-1)){
    return(FIFOFAIL); // Failed, fifo full
  }
  RxFifo[RxPutI&(RXFIFOSIZE-1)] = data; // put
  RxPutI++;  // Success, update
  return(FIFOSUCCESS);
}
// remove element from front of index FIFO
// return FIFOSUCCESS if successful
int RxFifo_Get(char *datapt){
  if(RxPutI == RxGetI ){
    return(FIFOFAIL); // Empty if RxPutI=RxGetI
  }
  *datapt = RxFifo[RxGetI&(RXFIFOSIZE-1)];
  RxGetI++;  // Success, update
  return(FIFOSUCCESS);
}
// number of elements in index FIFO
// 0 to RXFIFOSIZE-1
uint32_t RxFifo_Size(void){
 return ((uint32_t)(RxPutI-RxGetI));
}

// Two-index implementation of the transmit FIFO
// can hold 0 to TXFIFOSIZE elements
#define TXFIFOSIZE 64    // must be a power of 2
uint32_t volatile TxPutI; // put next
uint32_t volatile TxGetI; // get next
char static TxFifo[TXFIFOSIZE];

// initialize index FIFO
void TxFifo_Init(void){ long sr;
  sr = StartCritical(); // make atomic
  TxPutI = TxGetI = 0;  // Empty
  EndCritical(sr);
}
// add element to end of index FIFO
// return FIFOSUCCESS if successful
int TxFifo_Put(char data){
  if((TxPutI-TxGetI) & ~(TXFIFOSIZE-1)){
    return(FIFOFAIL); // Failed, fifo full
  }
  TxFifo[TxPutI&(TXFIFOSIZE-1)] = data; // put
  TxPutI++;  // Success, update
  return(FIFOSUCCESS);
}
// remove element from front of index FIFO
// return FIFOSUCCESS if successful
int TxFifo_Get(char *datapt){
  if(TxPutI == TxGetI ){
    return(FIFOFAIL); // Empty if TxPutI=TxGetI
  }
  *datapt = TxFifo[TxGetI&(TXFIFOSIZE-1)];
  TxGetI++;  // Success, update
  return(FIFOSUCCESS);
}
// number of elements in index FIFO
// 0 to TXFIFOSIZE-1
uint32_t TxFifo_Size(void){
 return ((uint32_t)(TxPutI-TxGetI));
}

// Initialize UART0
// Baud rate is 115200 bits/sec
// assuming 80 MHz bus clock
void UART_Init()
{
  SYSCTL_RCGCUART_R |= 0x80;            // activate UART7
  SYSCTL_RCGCGPIO_R |= 0x10;            // activate port E
  RxFifo_Init();                        // initialize empty FIFOs
  TxFifo_Init();
  UART7_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART7_IBRD_R = 43;                    // IBRD = int(80,000,000 / (16 * 115,200)) = int(43.403)
  UART7_FBRD_R = 26;                    // FBRD = round(0.403 * 64) = 25
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART7_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
  UART7_IFLS_R &= ~0x3F;                // clear TX and RX interrupt FIFO level fields
                                        // configure interrupt for TX FIFO <= 1/8 full
                                        // configure interrupt for RX FIFO >= 1/8 full
  UART7_IFLS_R += (UART_IFLS_TX1_8|UART_IFLS_RX1_8);
                                        // enable TX and RX FIFO interrupts and RX time-out interrupt
  UART7_IM_R |= (UART_IM_RXIM|UART_IM_TXIM|UART_IM_RTIM);
  UART7_CTL_R |= 0x01;                 // enable UART
  GPIO_PORTE_AFSEL_R |= 0x03;           // enable alt funct on PE1-0
  GPIO_PORTE_DEN_R |= 0x03;             // enable digital I/O on PE1-0
                                        // configure PE1-0 as UART
  GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R&0xFFFFFF00)+0x00000011;
  GPIO_PORTE_AMSEL_R = 0;               // disable analog functionality on PE
                                        // UART7=priority 2
  NVIC_PRI15_R = (NVIC_PRI15_R&0x00FFFFFF)|0x20000000; // bits 29-31
  NVIC_EN1_R = 0x80000000;           // enable interrupt 63 in NVIC
  //EnableInterrupts();
}
// copy from hardware RX FIFO to software RX FIFO
// stop when hardware RX FIFO is empty or software RX FIFO is full
void static copyHardwareToSoftware(void){
  char letter;
  while(((UART7_FR_R&UART_FR_RXFE) == 0) && (RxFifo_Size() < (RXFIFOSIZE - 1))){
    letter = UART7_DR_R;
    RxFifo_Put(letter);
  }
}
// copy from software TX FIFO to hardware TX FIFO
// stop when software TX FIFO is empty or hardware TX FIFO is full
void static copySoftwareToHardware(void){
  char letter;
  while(((UART7_FR_R&UART_FR_TXFF) == 0) && (TxFifo_Size() > 0)){
    TxFifo_Get(&letter);
    UART7_DR_R = letter;
  }
}
// input ASCII character from UART
// spin if RxFifo is empty
#include "../inc/ST7735.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
extern struct game_status current_game;
extern uint16_t prev_puck_x;
extern uint16_t prev_puck_y;
extern uint8_t prevpaddlex;
extern uint8_t prevpaddley;
extern uint8_t paddlex;
extern uint8_t paddley;
extern uint8_t currentSide;
uint32_t counter = 0;
uint8_t drawBG = 1;
char UART_InChar(void){
  char letter;
  while(RxFifo_Get(&letter) == FIFOFAIL)
	{
		if (counter == 50000)
		{
			counter = 0;
			DisableInterrupts();
			if (current_game.game_over == true)
			{
				if (drawBG == true)
				{
					drawBG = 0;
					ST7735_FillScreen(ST7735_RED);
				}
				ST7735_DrawCharS(30, 60, 'G', ST7735_WHITE, ST7735_WHITE, 3);
				for (int delay = 0; delay < 5000000; delay++);
				ST7735_DrawCharS(50, 60, 'A', ST7735_WHITE, ST7735_WHITE, 3);
				for (int delay = 0; delay < 5000000; delay++);
				ST7735_DrawCharS(70, 60, 'M', ST7735_WHITE, ST7735_WHITE, 3);
				for (int delay = 0; delay < 5000000; delay++);
				ST7735_DrawCharS(90, 60, 'E', ST7735_WHITE, ST7735_WHITE, 3);
				for (int delay = 0; delay < 5000000; delay++);
				ST7735_DrawCharS(30, 90, 'O', ST7735_WHITE, ST7735_WHITE, 3);
				for (int delay = 0; delay < 5000000; delay++);
				ST7735_DrawCharS(50, 90, 'V', ST7735_WHITE, ST7735_WHITE, 3);
				for (int delay = 0; delay < 5000000; delay++);
				ST7735_DrawCharS(70, 90, 'E', ST7735_WHITE, ST7735_WHITE, 3);
				for (int delay = 0; delay < 5000000; delay++);
				ST7735_DrawCharS(90, 90, 'R', ST7735_WHITE, ST7735_WHITE, 3);
			}
			else display_game();
			EnableInterrupts();
		}
		else counter++;
	}
  return(letter);
}
// input ASCII character from UART
// return 0 if RxFifo is empty
char UART_InCharNonBlock(void){
  char letter;
  if(RxFifo_Get(&letter) == FIFOFAIL){
    return 0; // no data
  }
  return(letter);
}
// output ASCII character to UART
// spin if TxFifo is full
void UART_OutChar(char data){
  while(TxFifo_Put(data) == FIFOFAIL){};
  UART7_IM_R &= ~UART_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART7_IM_R |= UART_IM_TXIM;           // enable TX FIFO interrupt
}
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART receiver has timed out
void UART7_Handler(void){
  if(UART7_RIS_R&UART_RIS_TXRIS){       // hardware TX FIFO <= 2 items
    UART7_ICR_R = UART_ICR_TXIC;        // acknowledge TX FIFO
    // copy from software TX FIFO to hardware TX FIFO
    copySoftwareToHardware();
    if(TxFifo_Size() == 0){             // software TX FIFO is empty
      UART7_IM_R &= ~UART_IM_TXIM;      // disable TX FIFO interrupt
    }
  }
  if(UART7_RIS_R&UART_RIS_RXRIS){       // hardware RX FIFO >= 2 items
    UART7_ICR_R = UART_ICR_RXIC;        // acknowledge RX FIFO
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
  if(UART7_RIS_R&UART_RIS_RTRIS){       // receiver timed out
    UART7_ICR_R = UART_ICR_RTIC;        // acknowledge receiver time out
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
}

//------------UART_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART_OutString(char *pt){
  while(*pt){
    UART_OutChar(*pt);
    pt++;
  }
}

//------------UART_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
uint32_t UART_InUDec(void){
uint32_t number=0, length=0;
char character;
  character = UART_InChar();
  while(character != CR){ // accepts until <enter> is typed
// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART_OutChar(character);
    }
// If the input is a backspace, then the return number is
// changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  return number;
}

//-----------------------UART_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART_OutUDec(uint32_t n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART_OutUDec(n/10);
    n = n%10;
  }
  UART_OutChar(n+'0'); /* n is between 0 and 9 */
}

//---------------------UART_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
uint32_t UART_InUHex(void){
uint32_t number=0, digit, length=0;
char character;
  character = UART_InChar();
  while(character != CR){
    digit = 0x10; // assume bad
    if((character>='0') && (character<='9')){
      digit = character-'0';
    }
    else if((character>='A') && (character<='F')){
      digit = (character-'A')+0xA;
    }
    else if((character>='a') && (character<='f')){
      digit = (character-'a')+0xA;
    }
// If the character is not 0-9 or A-F, it is ignored and not echoed
    if(digit <= 0xF){
      number = number*0x10+digit;
      length++;
      UART_OutChar(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if((character==BS) && length){
      number /= 0x10;
      length--;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  return number;
}

//--------------------------UART_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART_OutUHex(uint32_t number){
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
  if(number >= 0x10){
    UART_OutUHex(number/0x10);
    UART_OutUHex(number%0x10);
  }
  else{
    if(number < 0xA){
      UART_OutChar(number+'0');
     }
    else{
      UART_OutChar((number-0x0A)+'A');
    }
  }
}

//------------UART_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART_InString(char *bufPt, uint16_t max) {
int length=0;
char character;
  character = UART_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        UART_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  *bufPt = 0;
}
