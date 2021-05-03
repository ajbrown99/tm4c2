#include "../inc/LaunchPad.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "../inc/ST7735.h"
#include "../inc/ADCSWTrigger.h"
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
uint8_t xspeed = 1;
uint8_t yspeed = 1;
uint32_t data[2] = {0, 0};
uint8_t paddlex = 64;
uint8_t paddley = 135;
uint8_t prevpaddlex = 0;
uint8_t prevpaddley = 0;

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
	GPIO_PORTC_DATA_R &= ~0x30;
}

void receive_puck_info(void){
	
	uint32_t input = UART_InChar();
	if (currentSide == 1) currentSide = 2;
	else currentSide = 1;
	GPIO_PORTC_DATA_R |= 0x30;
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

void display_game(){
	ST7735_SetCursor(0,0);
	ST7735_OutUDec(current_game.player_2_score);
	ST7735_DrawFastVLine(34,130,30,ST7735_BLUE);
	ST7735_DrawFastVLine(94,130,30,ST7735_BLUE);
	ST7735_DrawFastHLine(34,130,60,ST7735_BLUE);
	ST7735_DrawCircle(prevpaddlex,prevpaddley,ST7735_WHITE);
	ST7735_DrawCircle(prev_puck_x,prev_puck_y,ST7735_WHITE);
	if (currentSide == 2)
	{
			ST7735_DrawCircle(current_game.puck_x,abs(current_game.puck_y),ST7735_BLUE);
	}
	ST7735_DrawCircle(paddlex,paddley,ST7735_BLACK);
	prevpaddlex = paddlex;
	prevpaddley = paddley;
	prev_puck_x = current_game.puck_x;
	prev_puck_y = abs(current_game.puck_y);
}

void Timer2A_Handler()
{
	TIMER2_ICR_R = TIMER_ICR_TATOCINT;
	//display_game();
	
}

int detectCollision()
{
	
	int absolute_puck_y = abs(current_game.puck_y);
	
	int rightPuck = current_game.puck_x + 5;	//right puck
	int leftPuck = current_game.puck_x - 5;	//left puck
	int bottomPuck = absolute_puck_y + 5;	//bottom puck
	int topPuck = absolute_puck_y - 5;	//top puck
	
	int rightPaddle = paddlex + 5;	//right paddle
	int leftPaddle = paddlex - 5;	//left paddle
	int bottomPaddle = paddley + 5;	//bottom paddle
	int topPaddle = paddley - 5;	//top paddle
	
	int topPaddleBottomPuckY = topPaddle - bottomPuck; //difference in y pos btwn top of paddle & bottom of puck
	int bottomPaddleTopPuckY = bottomPaddle - topPuck; //difference in y pos btwn bottom of paddle & top of puck
	
	int rightPaddleLeftPuckX = rightPaddle - leftPuck; //difference in x pos btwn right of paddle & left of puck
	int leftPaddleRightPuckX = leftPaddle - rightPuck; //difference in x pos btwn left of paddle & right of puck
	
	int x_difference = paddlex - current_game.puck_x;  //if x_difference < 0, then paddle is to the left of the puck
	int y_difference = paddley - absolute_puck_y;  //if y_difference < 0, then paddle is above the puck
	
	if (topPaddleBottomPuckY >= -4 && topPaddleBottomPuckY <= 4)
	{
		//check to see if the puck will now collide & go in the up right direction 
		if(x_difference >= -6 && x_difference <= -2){
			return 5;
		}
		//check to see if puck will now collide & go in the up direction
		else if(x_difference > -2 && x_difference < 2){
			return 4;
		}
		//check to see if puck will now collide & go in the up left direction
		else if(x_difference >= 2 && x_difference <= 6){
			return 3;
		}
		
		//if (topPaddleBottomPuckX <= 10) return 0;	//topPaddle hits bottomPuck
	}
	
	if (bottomPaddleTopPuckY >= -4 && bottomPaddleTopPuckY <= 4){
		
		//if (bottomPaddleTopPuckX <= 10) return 1; //bottom of paddle hits the top of the puck
		//check to see if the puck will now collide & go in the down right direction 
		if(x_difference >= -6 && x_difference <= -2){
			return 7;
		}
		//check to see if puck will now collide & go in the down direction
		else if(x_difference > -2 && x_difference < 2){
			return 0;
		}
		//check to see if puck will now collide & go in the down left direction
		else if(x_difference >= 2 && x_difference <= 6){
			return 1;
		}
	}
	
	if(rightPaddleLeftPuckX >= -4 && rightPaddleLeftPuckX <= 4){
		//puck will now go down right
		if(y_difference >= -6 && y_difference <= -2){
			return 7;
		}
		//puck will now go right
		else if(y_difference > -2 && y_difference < 2){
			return 6;
		}
		//puck will now go up right
		else if(y_difference >= 2 && y_difference <= 6){
			return 5;
		}
	}
	
	if(leftPaddleRightPuckX >= -4 && leftPaddleRightPuckX <= 4){
		//puck will now go down left
		if(y_difference >= -6 && y_difference <= -2){
			return 1;
		}
		//puck will now go left
		else if(y_difference > -2 && y_difference < 2){
			return 2;
		}
		//puck will now go up left
		else if(y_difference >= 2 && y_difference <= 6){
			return 3;
		}
	}
	
	return -1;
}

//will need to change bouncing logic later for 2 LCD's
//____________________________________________________
//____________________________________________________
void Timer1A_Handler()
{
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;
	int collisionValue = detectCollision();
	if (goalScored()) 
	{
		current_game.player_1_score++;
		current_game.puck_direction = 8;
		current_game.puck_x = 64;
		current_game.puck_y = -80;
		if (current_game.player_1_score == 1)
		{
			current_game.puck_direction = 4;
			current_game.puck_y = -1;
			current_game.game_over = true;
		}
	}
	if(collisionValue != -1){
		current_game.puck_direction = collisionValue;
	}
	ADC_In89(data);
	if (data[0] > 3072)
	{
		if (data[1] > 3072)				//Down right (3)
		{
			if (paddlex < 117) paddlex+=1;
			if (paddley < 149) paddley+=1;
		}
		else if (data[1] < 1028)	//Up right (1)
		{
			if (paddlex < 117) paddlex+=1;
			if (paddley > 30) 	 paddley-=1;
		}
		else											//Right (2)
		{
			if (paddlex < 117) paddlex+=1;
		}
	}
	else if (data[0] < 1028)
	{
		if (data[1] > 3072)				//Down left (5)
		{
			if (paddlex > 0)   paddlex-=1;
			if (paddley < 149) paddley+=1;
		}
		else if (data[1] < 1028)	//Up left (7)
		{
			if (paddlex > 0)   paddlex-=1;
			if (paddley > 30)  paddley-=1;
		}
		else
		{
			if (paddlex > 0)   paddlex-=1;							//Left (6)
		}
	}
	else
	{
		if (data[1] > 3072)				//Down (4)
		{
			if (paddley < 149) paddley+=1;
		} 
		else if (data[1] < 1028)	//Up (0)
		{
			if (paddley > 30)  paddley-=1;
		}
	}
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
			//current_game_state();
		}
	}
}

bool goalScored()
{
	if (current_game.puck_y <= -149)
	{
		if (current_game.puck_x <= 89 && current_game.puck_x >= 39)
		{
			return true;
		}
	}
	return false;
}
