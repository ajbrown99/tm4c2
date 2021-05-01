#include <stdint.h>
uint32_t get_init_x_coord(void);
uint32_t get_init_y_coord(void);
void DAC_Init(uint16_t data);
void DAC_Out(uint16_t code);
void stopSound(void);
void playSong(void);
