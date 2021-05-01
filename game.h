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

extern struct game_status current_game;
