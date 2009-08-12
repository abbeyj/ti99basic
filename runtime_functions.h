#ifndef RUNTIME_FUNCTIONS_H_INCLUDED
#define RUNTIME_FUNCTIONS_H_INCLUDED

#include "runtime_types.h"

extern void init_os(int argc, char **argv);

extern uint8_t hw_read(uint16_t addr);
extern void hw_write(uint16_t addr, uint8_t s);
extern void cruout(uint16_t addr, int v);
extern int cruin(uint16_t addr);
extern int GPL_interp_dispatch();

#endif
