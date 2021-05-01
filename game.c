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


struct game_status {
	bool game_over;
	uint8_t player_1_score;
	uint8_t player_2_score;
	int16_t puck_x;
	int16_t puck_y;
	bool point_scored;
	uint8_t puck_direction; //0:up,1:up right,2:right,3:down right,4:down,5:down left,6:left,7:up left
	bool stop_start; //0:stop,1:start
};


struct game_status current_game = {false,0,0,63,79,false,3,true};

const uint8_t uart_flag_start = 212;
const uint16_t uart_flag_finish = 2120;

void current_game_state(void){
	ST7735_FillScreen(ST7735_WHITE);
}



void transmit_puck_info(void){
	UART_OutUDec(uart_flag_start);
	UART_OutUDec(current_game.game_over);
	UART_OutUDec(current_game.player_1_score);
	UART_OutUDec(current_game.player_2_score);
	UART_OutUDec(current_game.puck_x);
	UART_OutUDec(current_game.point_scored);
	UART_OutUDec(current_game.puck_direction);
	UART_OutUDec(current_game.stop_start);
	UART_OutUDec(uart_flag_finish);
}

void receive_puck_info(void){
	PF1 ^= 0x2;
	uint32_t input = UART_InUDec();
	ST7735_OutUDec(input);
	if(input == 212){
		
		bool transmission_over = false;
		uint8_t count = 0;
		while(transmission_over == false){
			input = UART_InUDec();
			if(count == 0){
				current_game.game_over = input;
			}
			else if(count == 1){
				current_game.player_1_score = input;
			}
			else if(count == 2){
				current_game.player_2_score = input;
			}
			else if(count == 3){
				current_game.puck_x = input;
			}
			else if(count == 4){
				current_game.point_scored = input;
			}
			else if(count == 5){
				current_game.puck_direction = input;
			}
			else if(count == 6){
				current_game.stop_start = input;
			}
			count++;
			if(input == 2120){
				transmission_over = true;
				current_game.puck_y = -1;
			}
		}
	}
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
