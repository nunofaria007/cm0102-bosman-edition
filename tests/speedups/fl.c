#include "rt.h"
uint8_t *g_mem; Cpu cpu;
void setmem(uint8_t *p){ g_mem = p; }
void run(uint32_t esp, uint32_t ebp, uint32_t *r){ rt_player_init_distance(esp, ebp, &r[0], &r[1], &r[2], &r[3], &r[4]); }
