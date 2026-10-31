#include "/home/claude/nunofaria007/cm0102-bosman-edition/src/recomp_rt/win/crt_qsort.c"
Cpu cpu; Fpu fpu; uint8_t *g_mem; int rt_unwinding; uint32_t rt_call_target;
RtFn rt_lookup(uint32_t t) { (void)t; return 0; }
void rt_fatal(const char *f, ...) { (void)f; }
void setmem(uint8_t *p) { g_mem = p; }
int shim(int kind, uint32_t a, uint32_t b, int32_t *r) { return cmp_native(kind, a, b, r); }
int kind_of(uint32_t c) { return cmp_kind(c); }
