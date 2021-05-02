#include "../inc/LaunchPad.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "../inc/ST7735.h"
#include "game.h"
#include "UART.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))

uint32_t count = 0;


struct game_status current_game = {false,0,0,0,79,false,3,true};

const uint8_t uart_flag_start = 212;
const uint16_t uart_flag_finish = 2120;
uint16_t prev_puck_x = 0;
uint16_t prev_puck_y = 0;
uint8_t currentSide = 1;
uint8_t xspeed = 4;
uint8_t yspeed = 4;

void current_game_state(void){
	ST7735_FillScreen(ST7735_WHITE);
}



void transmit_puck_info(void){
	PF2 ^= 0x4;
	if (currentSide == 2) currentSide = 1;
	else currentSide = 2;
	UART_OutChar('a');
	UART_OutChar((char)current_game.game_over);
	UART_OutChar(current_game.player_1_score);
	UART_OutChar(current_game.player_2_score);
	UART_OutChar(current_game.puck_x);
	UART_OutChar((char)current_game.point_scored);
	UART_OutChar(current_game.puck_direction);
	UART_OutChar((char)current_game.stop_start);
	UART_OutChar('j');
	PF2 ^= 0x4;
}

void receive_puck_info(void){
	
	uint32_t input = UART_InChar();
	if (currentSide == 1) currentSide = 2;
	else currentSide = 1;
	PF1 ^= 0x2;
	//ST7735_OutUDec(input);
	if(count == 0){
		//do nothing, received start flag
	}
	//got game over value
	else if(count == 1){
		current_game.game_over = (bool)input;
	}
	else if(count == 2){
		current_game.player_1_score = input;
	}
	else if(count == 3){
		current_game.player_2_score = input;
	}
	else if(count == 4){
		current_game.puck_x = 117 - input;
	}
	else if(count == 5){
		current_game.point_scored = (bool)input;
	}
	else if(count == 6)
	{
		current_game.puck_direction = input;
	}
	else if(count == 7)
	{
		current_game.stop_start = (bool)input;
	}
	else if(count == 8)
	{
		//do nothing, got ending flag
		current_game.puck_y = -1;
	}
	PF1 ^= 0x02;
	count = (count + 1) % 9;
}

void Timer2A_Handler()
{
	ST7735_DrawCircle(prev_puck_x,prev_puck_y,ST7735_WHITE);
	if (currentSide == 2)
	{
			ST7735_DrawCircle(current_game.puck_x,abs(current_game.puck_y),ST7735_BLUE);
	}
	prev_puck_x = current_game.puck_x;
	prev_puck_y = abs(current_game.puck_y);
	TIMER2_ICR_R = TIMER_ICR_TATOCINT;
}

//will need to change bouncing logic later for 2 LCD's
//____________________________________________________
//____________________________________________________
void Timer1A_Handler()
{
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;
	//if the game is running
	if(current_game.stop_start){
		
		if(current_game.puck_y < 0){
			
			switch(current_game.puck_direction){
			//if direction is up
			case(0):
				current_game.puck_y -= yspeed;
				if(current_game.puck_y <= -149){
					current_game.puck_direction = 4;
				}
				break;
			//if direction is up & left
			case(1):
				current_game.puck_y -= yspeed;
				current_game.puck_x -= xspeed;
				if(current_game.puck_y <= -149){
					current_game.puck_direction = 3;
				}
				else if(current_game.puck_x <= 0){
					current_game.puck_direction = 7;
				}
				break;
			//if direction is left
			case(2):
				current_game.puck_x -= xspeed;
				if(current_game.puck_x <= 0){
					current_game.puck_direction = 6;
				}
				break;
			//if direction is down & left
			case(3):
				current_game.puck_y += yspeed;
				current_game.puck_x -= xspeed;
				if(current_game.puck_y >= 0){
					transmit_puck_info();
				}
				else if(current_game.puck_x <= 0){
					current_game.puck_direction = 5;
				}
				break;
			//if direction is down
			case(4):
				current_game.puck_y += yspeed;
				if(current_game.puck_y >= 0){
					transmit_puck_info();
				}
				break;
			//if direction is down & right
			case(5):
				current_game.puck_y += yspeed;
				current_game.puck_x += xspeed;
				if(current_game.puck_y >= 0){
					transmit_puck_info();
				}
				else if(current_game.puck_x >= 117){
					current_game.puck_direction = 3;
				}
				break;
			//if direction is right
			case(6):
				current_game.puck_x += xspeed;
				if(current_game.puck_x >= 117){
					current_game.puck_direction = 2;
				}
				break;
			//if direction is up & right
			case(7):
				current_game.puck_y -= yspeed;
				current_game.puck_x += xspeed;
				if(current_game.puck_y <= -149){
					current_game.puck_direction = 5;
				}
				else if(current_game.puck_x >= 117){
					current_game.puck_direction = 1;
				}
				break;
			}
		}
		else {
			current_game_state();
		}
	}
}
