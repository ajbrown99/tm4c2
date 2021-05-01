//game header

void gameInit(void);
void movePuck(void);
void movePlayer(void);
void sendCoordinates(void);

void current_game_state(void);
void transmit_puck_info(void);
void receive_puck_info(void);

extern struct game_status current_game;
