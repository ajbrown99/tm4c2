//game header
#include <stdint.h>
#include <stdbool.h>
void gameInit(void);
void movePuck(void);
void movePlayer(void);
void sendCoordinates(void);

void current_game_state(void);
void transmit_puck_info(void);
void receive_puck_info(void);
int detectCollision(void);

struct game_status {
	bool game_over;
	char player_1_score;
	char player_2_score;
	char puck_x;
	int puck_y;
	bool point_scored;
	char puck_direction; //0:up,1:up right,2:right,3:down right,4:down,5:down left,6:left,7:up left
	bool stop_start; //0:stop,1:start
};

extern struct game_status current_game;
