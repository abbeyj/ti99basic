#ifndef RUNTIME_REGS_H_INCLUDED
#define RUNTIME_REGS_H_INCLUDED

#include "runtime_types.h"

/* Internal Registers */
extern uint16_t PC;
extern uint16_t WP;

extern int ST_LGT; /* ST0 Logical Greater Than */
extern int ST_AGT; /* ST1 Arithmetic Greater Than */
extern int ST_EQ;  /* ST2 Equal */
extern int ST_C;   /* ST3 Carry */
extern int ST_O;   /* ST4 Overflow */
extern int ST_P;   /* ST5 Parity */
extern int ST_X;   /* ST6 XOP */
extern int ST_IM;  /* ST12-15 Interrupt Mask */

/* Workspace Registers */
#define R0   (WP + 2* 0)
#define R1   (WP + 2* 1)
#define R2   (WP + 2* 2)
#define R3   (WP + 2* 3)
#define R4   (WP + 2* 4)
#define R5   (WP + 2* 5)
#define R6   (WP + 2* 6)
#define R7   (WP + 2* 7)
#define R8   (WP + 2* 8)
#define R9   (WP + 2* 9)
#define R10  (WP + 2*10)
#define R11  (WP + 2*11)
#define R12  (WP + 2*12)
#define R13  (WP + 2*13)
#define R14  (WP + 2*14)
#define R15  (WP + 2*15)

/* TI BASIC readability hack */
#define R0L  (0x83E1 + 2* 0)
#define R1L  (0x83E1 + 2* 1)
#define R2L  (0x83E1 + 2* 2)
#define R3L  (0x83E1 + 2* 3)
#define R4L  (0x83E1 + 2* 4)
#define R5L  (0x83E1 + 2* 5)
#define R6L  (0x83E1 + 2* 6)
#define R7L  (0x83E1 + 2* 7)
#define R8L  (0x83E1 + 2* 8)
#define R9L  (0x83E1 + 2* 9)
#define R10L (0x83E1 + 2*10)
#define R11L (0x83E1 + 2*11)
#define R12L (0x83E1 + 2*12)
#define R13L (0x83E1 + 2*13)
#define R14L (0x83E1 + 2*14)
#define R15L (0x83E1 + 2*15)

#define rd(a)        (RAM[a]<<8 | RAM[(a)+1])
#define rdb(a)       RAM[a]
#define wr(a,v)      RAM[a] = ((v)>>8); RAM[(a)+1] = (uint8_t)(v)
#define wrb(a,v)     RAM[a] = (uint8_t)v

static inline uint16_t rd_inc(uint16_t w)
{
	uint16_t ret = rd(w);
	wr(w, ret + 1);
	return ret;
}

static inline uint16_t rd_inct(uint16_t w)
{
	uint16_t ret = rd(w);
	wr(w, ret + 2);
	return ret;
}

#endif