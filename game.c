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


struct game_status current_game = {false,0,0,63,79,false,3,true};

const uint8_t uart_flag_start = 212;
const uint16_t uart_flag_finish = 2120;

void current_game_state(void){
	ST7735_FillScreen(ST7735_WHITE);
}



void transmit_puck_info(void){
	PF2 ^= 0x4;
	UART_OutUDec(uart_flag_start);
	UART_OutUDec(current_game.game_over);
	UART_OutUDec(current_game.player_1_score);
	UART_OutUDec(current_game.player_2_score);
	UART_OutUDec(current_game.puck_x);
	UART_OutUDec(current_game.point_scored);
	UART_OutUDec(current_game.puck_direction);
	UART_OutUDec(current_game.stop_start);
	UART_OutUDec(uart_flag_finish);
	PF2 ^= 0x4;
}

void receive_puck_info(void){
	
	uint32_t input = UART_InChar();
	PF1 ^= 0x2;
	//ST7735_OutUDec(input);
	if(count == 0){
		//do nothing, received start flag
	}
	//got game over value
	else if(count == 1){
		current_game.game_over = input - '0';
	}
	else if(count == 2){
		current_game.player_1_score = input - '0';
	}
	else if(count == 3){
		current_game.player_2_score = input - '0';
	}
	else if(count == 4){
		current_game.puck_x = (int16_t)input;
	}
	else if(count == 5){
		current_game.point_scored = input - '0';
	}
	else if(count == 6){
		current_game.puck_direction = input - '0';
	}
	else if(count == 7){
		current_game.stop_start = input - '0';
	}
	else if(count == 8){
		//do nothing, got ending flag
		current_game.puck_y = -1;
		//ST7735_OutUDec(current_game.puck_x);
	}
	PF1 ^= 0x02;
	count = (count + 1) % 9;
}

void Timer2A_Handler(){
	TIMER2_ICR_R = TIMER_ICR_TATOCINT;
	receive_puck_info();
}

//will need to change bouncing logic later for 2 LCD's
//____________________________________________________
//____________________________________________________
void Timer1A_Handler()
{
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;
	//if the game is running
	if(current_game.stop_start){
		
		ST7735_DrawCircle(current_game.puck_x,current_game.puck_y,ST7735_WHITE);
		if(current_game.puck_y < 0){
			
			switch(current_game.puck_direction){
			//if direction is up
			case(0):
				current_game.puck_y--;
				if(current_game.puck_y <= -149){
					current_game.puck_direction = 4;
				}
				break;
			//if direction is up & left
			case(1):
				current_game.puck_y--;
				current_game.puck_x--;
				if(current_game.puck_y <= -149){
					current_game.puck_direction = 3;
				}
				else if(current_game.puck_x <= 0){
					current_game.puck_direction = 7;
				}
				break;
			//if direction is left
			case(2):
				current_game.puck_x--;
				if(current_game.puck_x <= 0){
					current_game.puck_direction = 6;
				}
				break;
			//if direction is down & left
			case(3):
				current_game.puck_y++;
				current_game.puck_x--;
				if(current_game.puck_y >= 0){
					transmit_puck_info();
				}
				else if(current_game.puck_x <= 0){
					current_game.puck_direction = 5;
				}
				break;
			//if direction is down
			case(4):
				current_game.puck_y++;
				if(current_game.puck_y >= 0){
					transmit_puck_info();
				}
				break;
			//if direction is down & right
			case(5):
				current_game.puck_y++;
				current_game.puck_x++;
				if(current_game.puck_y >= 0){
					transmit_puck_info();
				}
				else if(current_game.puck_x >= 117){
					current_game.puck_direction = 3;
				}
				break;
			//if direction is right
			case(6):
				current_game.puck_x++;
				if(current_game.puck_x >= 117){
					current_game.puck_direction = 2;
				}
				break;
			//if direction is up & right
			case(7):
				current_game.puck_y--;
				current_game.puck_x++;
				if(current_game.puck_y <= -149){
					current_game.puck_direction = 5;
				}
				else if(current_game.puck_x >= 117){
					current_game.puck_direction = 1;
				}
				break;
			}
			ST7735_DrawCircle(current_game.puck_x,abs(current_game.puck_y),ST7735_BLUE);
		}
		else {
			current_game_state();
		}
	}
}
