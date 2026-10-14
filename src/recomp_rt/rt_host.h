/* Host side of the runtime: setup, the heap and the tables that the recompiler writes. */
#ifndef RECOMP_RT_HOST_H
#define RECOMP_RT_HOST_H

#include <stdint.h>

/* The same layout as the Unicorn run of tests/recomp/reference.py. */
#define RT_STACK_BASE 0x20000000u
#define RT_STACK_SIZE 0x100000u
#define RT_STACK_TOP (RT_STACK_BASE + RT_STACK_SIZE - 0x1000u)
#define RT_RETURN_ADDR 0x40000000u
#define RT_HEAP_BASE 0x70000000u
#define RT_HEAP_SIZE 0x40000000u

typedef struct { uint32_t iat, stub; } RtIat;
extern const RtIat rt_iat[];
extern const uint32_t rt_iat_size;

void rt_init(const char *exe_path);
uint32_t rt_image_base(void);
uint32_t rt_image_size(void);

/* Bump heap: never reuses memory, rounds blocks to 16 bytes, keeps each block's size for realloc. */
uint32_t rt_alloc(uint32_t size);
uint32_t rt_alloc_str(const char *s);
uint32_t rt_block_size(uint32_t addr);
void rt_set_block_size(uint32_t addr, uint32_t size);
uint32_t rt_heap_top(void);

/* The folder of the game data, for the data path replacement. */
extern const char *rt_data_dir;
/* How often each replacement ran. */
void rt_count(const char *name);
void rt_print_counts(void);

#endif
