/*
 * Copyright (c) 2009 James Abbatiello, Michael Steil
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
extern unsigned char RAM[];
#include "runtime_regs.h"
#include "runtime_functions.h"
extern const unsigned char grom[0x6000];


typedef enum block_e
{
	BLOCK_NONE,
	BLOCK_ONE,
	BLOCK_LINE
} block_t;

static void console_set_block(block_t block);

int in_source_mode = 0;      /* BASIC source file specified on command line */
int source_mode_running = 0; /* BASIC source has been fed in and is now running */
FILE *fp_source = NULL;

void
init_os(int argc, char **argv) {
	WP = rd(0x0000); // WP = 0x83E0

	if (argc > 1)
	{
		int ch;
		in_source_mode = 1;
		fp_source = fopen(argv[1], "r");
		if (!fp_source)
		{
			perror("opening BASIC source file");
			exit(1);
		}
		if ((ch = getc(fp_source)) != '#')
			ungetc(ch, fp_source);
		else
		{
			while (ch != EOF && ch != '\x0D' && ch != '\x0A')
				ch = getc(fp_source);
		}
	}

	console_set_block(BLOCK_ONE);
}

static inline void warn(const char *fmt, ...)
{
#if 1
	va_list va;
	fprintf(stderr, "warning: ");
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
#endif
}

static uint16_t grom_addr = 0;
static int      grom_addr_read_lo = 0;   /* reading low byte? */
static int      grom_addr_write_lo = 0;  /* writing low byte? */

static inline uint8_t grom_addr_read()
{
	uint8_t ret;
	if (grom_addr_read_lo)
		ret = ((grom_addr + 1) >> 0) & 0xFF;
	else
		ret = ((grom_addr + 1) >> 8) & 0xFF;
	grom_addr_read_lo = !grom_addr_read_lo;
	return ret;
}

static inline void grom_addr_write(uint8_t s)
{
	if (grom_addr_write_lo)
		grom_addr = (grom_addr & 0xFF00) | (s << 0);
	else
		grom_addr = (grom_addr & 0x00FF) | (s << 8);
	grom_addr_write_lo = !grom_addr_write_lo;
}

static inline uint8_t grom_data_read()
{
	uint8_t ret;
	grom_addr_read_lo = 0;
	grom_addr_write_lo = 0;
	if (grom_addr < sizeof(grom))
		ret = grom[grom_addr];
	else
		ret = 0;
	grom_addr++;
	return ret;
}

static inline void grom_data_write(uint8_t s)
{
	grom_addr_read_lo = 0;
	grom_addr_write_lo = 0;
	warn("attempt to write to GROM[>%04X]", grom_addr);
	grom_addr++;
}

static uint8_t  vdp_ram[0x4000];
static uint8_t  vdp_reg[8];
static uint16_t vdp_addr = 0;
static uint8_t  vdp_data_read_latch = 0;
static uint8_t  vdp_cmd_write_latch = 0;
static int      vdp_cmd_write_2 = 0;
static uint16_t vdp_last_output_addr = 0xFFFF;
static int      suppress_output = 1;       /* suppress all output during reset routine */
static int      supress_next_newline = 0;  /* ignore next output iff it is a newline */

static inline void vdp_data_read_ahead()
{
	vdp_data_read_latch = vdp_ram[vdp_addr];
	vdp_addr = (vdp_addr + 1) & 0x3FFF;
}

static inline uint8_t vdp_data_read()
{
	uint8_t ret;
	vdp_cmd_write_2 = 0;
	ret = vdp_data_read_latch;
	vdp_data_read_ahead();
	return ret;
}

static inline int vdp_is_in_pattern_table(uint16_t addr)
{
	int base = (vdp_reg[2] & 0x0F) * 0x400;
	return (addr >= base && addr < (base + 0x300));
}

static inline int vdp_translate_char(uint8_t s)
{
	int ret = (vdp_reg[4] & 0x07) ? s : s - 0x60;
	if (ret < ' ')
		ret = '.';
	return ret;
}

/* if they skipped ahead, output a space to catch up */
static void vdp_output_catchup()
{
	if (vdp_last_output_addr == vdp_addr - 2)
		putchar(' ');
	vdp_last_output_addr = vdp_addr;
}

static inline void vdp_output_char(uint8_t s)
{
	if (grom_addr == 0x20CC && !in_source_mode) /* TI BASIC READY */
		suppress_output = 0;

	if (suppress_output)
		return;

	if (!vdp_is_in_pattern_table(vdp_addr))
		return;

	if (PC == 0x05A8) /* GPL ALL */
		return;

	if (grom_addr == 0x00EE || /* reset routine fill */

		grom_addr == 0x56D4 || /* scroll up one line */
		grom_addr == 0x56DB || /* new line lead in */
		grom_addr == 0x56DD || /* new line spaces */

		grom_addr == 0x2A83 || /* line input clear space */
		grom_addr == 0x2B84 || /* line input echo */

		grom_addr == 0x2FB7 || /* warning: zero character for subsequent write */
		grom_addr == 0x2FF3 || /* LIST: zero character for subsequent write */
		grom_addr == 0x42C6 || /* number: zero character for subsequent write */
		grom_addr == 0x4BEE || /* name: zero character for subsequent write */

		grom_addr == 0x44F9 || /* INPUT: clear line */
		grom_addr == 0x4506)   /* INPUT: line lead out */
		return;

	if (grom_addr == 0x56DF) /* new line lead out */
	{
		static int skip_new_line = 0;
		uint16_t top_sub_stack  = 0x8300 + RAM[0x8373];
		uint16_t grom_ret_addr  = RAM[top_sub_stack  ]<<8 | RAM[top_sub_stack+1];
		uint16_t grom_ret_addr2 = RAM[top_sub_stack-2]<<8 | RAM[top_sub_stack-1];

		/* line wrap */
		if (grom_ret_addr == 0x4C03 && 
			(grom_ret_addr2 == 0x2FEB || grom_ret_addr2 == 0x4C8A))
			return;

		/* beginning of "** DONE **" message */
		if (grom_ret_addr == 0x4E62 && source_mode_running)
			exit(0);

		/* there are two lead out characters and we only want one newline */
		if ((skip_new_line = !skip_new_line))
			return;
		if (!supress_next_newline)
			putchar('\n');
		vdp_last_output_addr = 0x02E1; /* 1 before start of bottom line of pattern table */
		supress_next_newline = 0;
		RAM[0x8379] = (uint8_t)clock(); /* update VDP interrupt counter for RANDOMIZE */
		return;
	}

	if (grom_addr == 0x21DD) /* line input prompt ">" */
	{
		if (source_mode_running)
			exit(1); /* probably an error during running the BASIC program */
		else if (in_source_mode)
			return;  /* don't print out prompts while feeding the source file in */
	}

	if (grom_addr == 0x2A5A) /* line input cursor */
	{
		/* don't output the cursor character but do catch up
		   so the host's line editor will start in the right place */
		vdp_output_catchup();
		return;
	}

	vdp_output_catchup();
	putchar(vdp_translate_char(s));
	supress_next_newline = 0;
}

static inline void vdp_data_write(uint8_t s)
{
	vdp_output_char(s);
	vdp_cmd_write_2 = 0;
	vdp_ram[vdp_addr] = s;
	vdp_data_read_ahead();
}

static inline void vdp_cmd_write(uint8_t s)
{
	if (vdp_cmd_write_2)
	{
		int s_lo = s & 0x3F;
		switch (s >> 6)
		{
		case 0: /* set address for read */
			vdp_addr = ((s_lo << 8) | vdp_cmd_write_latch);
			vdp_data_read_ahead();
			break;
		case 1: /* set address for write */
			vdp_addr = ((s_lo << 8) | vdp_cmd_write_latch);
			break;
		case 2: /* write to reg */
			if (s_lo < sizeof(vdp_reg))
				vdp_reg[s_lo] = vdp_cmd_write_latch;
			else
				warn("VDP reg %d = >%02X", s_lo, vdp_cmd_write_latch);
			break;
		case 3: /* undefined */
			warn("unknown VDP command 3: >%02X", s_lo);
			break;
		}
	}
	else
	{
		vdp_cmd_write_latch = s;
	}
	vdp_cmd_write_2 = !vdp_cmd_write_2;
}


extern uint8_t hw_read(uint16_t addr)
{
	if (addr == 0x8800)                  return vdp_data_read();
	if ((addr & 0xFFC3) == 0x9800)       return grom_data_read();
	if ((addr & 0xFFC3) == 0x9802)       return grom_addr_read();
	if (addr >= 0x8300 && addr < 0x8400) return RAM[addr];
	if (addr < 0x2000)                   return RAM[addr];
	if (addr != 0x4000 && /* Peripheral cards ROM */
		addr != 0x6000 && /* Cartridge ROM */
		addr != 0x8400 && /* Sound chip */
		addr != 0x9400)   /* Speech synth */
		warn("unhandled read [>%04X]", addr);
	return 0;
}

extern void hw_write(uint16_t addr, uint8_t s)
{
	if (addr == 0x8C00)                  { vdp_data_write(s); return; }
	if (addr == 0x8C02)                  { vdp_cmd_write(s); return; }
	if ((addr & 0xFFC3) == 0x9C00)       { grom_data_write(s); return; }
	if ((addr & 0xFFC3) == 0x9C02)       { grom_addr_write(s); return; }
	if (addr >= 0x8300 && addr < 0x8400) { RAM[addr] = s; return; }
	if (addr != 0x8400 && /* Sound chip */
		addr != 0x9400)   /* Speech synth */
		warn("unhandled write [>%04X] = >%02X", addr, s);
}

extern void cruout(uint16_t addr, int v)
{
	switch (addr)
	{
	case 0x0012:
	case 0x0013:
	case 0x0014:
		/* TODO select keyboard column */
		break;
	case 0x0015:
		/* TODO set alpha lock */
		break;
	case 0x0780:
		/* bug in reset routine, ignore */
		break;
	default:
		/* don't warn about scanning for peripherals */
		if (!(addr & 0x800))
			warn("cruout(>%04X, %d)", addr, v);
	}
}

extern int cruin(uint16_t addr)
{
	switch (addr)
	{
	case 0x0003:
	case 0x0004:
	case 0x0005:
	case 0x0006:
	case 0x0007:
	case 0x0008:
	case 0x0009:
	case 0x000A:
		return 1; /* TODO key not pressed */
	default:
		warn("cruin(>%04X)", addr);
		return 0;
	}
}

static int is_stdin_console = 0;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <conio.h>
#include <windows.h>

static DWORD mode_orig;

static void console_cleanup(void)
{
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode_orig);
}

static void console_set_block(block_t block)
{
	DWORD mode;

	if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode))
	{
		if (!is_stdin_console)
		{
			mode_orig = mode;
			is_stdin_console = 1;
			atexit(console_cleanup);
		}
		if (block == BLOCK_LINE)
			mode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
		else
			mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
	}
}

static int getch_internal(block_t block)
{
	int ret;
	char ch;

	if (is_stdin_console)
	{
		if (block == BLOCK_NONE)
		{
			ret = EOF;
			while (ret == EOF && kbhit())
			{
				ret = getch();
				if (ret == 0 || ret == 0xE0)
				{
					/* extended character, discard */
					getch();
					ret = EOF;
				}
			}
		}
		else if (block == BLOCK_ONE)
			ret = read(0, &ch, 1) ? ch : EOF;
		else if (block == BLOCK_LINE)
		{
			console_set_block(BLOCK_LINE);
			ret = getchar();
			console_set_block(BLOCK_ONE);
		}
	}
	else
	{
		if (block == BLOCK_NONE)
			ret = EOF;
		else
			ret = getchar();
	}

	return ret;
}

static void flush_console_input()
{
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}

#else
#include <errno.h>
#include <unistd.h>
#include <termios.h>

static struct termios mode_orig;

static void console_cleanup(void)
{
	tcsetattr(0, TCSANOW, &mode_orig);
}

static void console_set_block(block_t block)
{
	struct termios t;

	if (tcgetattr(0, &t) == 0)
	{
		if (!is_stdin_console)
		{
			mode_orig = t;
			is_stdin_console = 1;
			atexit(console_cleanup);
		}
		if (block == BLOCK_LINE)
			t.c_lflag |= ICANON | ECHO;
		else
			t.c_lflag &= ~(ICANON | ECHO);
		t.c_cc[VMIN] = block ? 1 : 0;
		t.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &t);
	}
}

static ssize_t read_unintr(int fildes, void *buf, size_t nbyte)
{
        ssize_t ret;
        do
        {
                ret = read(fildes, buf, nbyte);
        } while (ret == -1 && errno == EINTR);
        return ret;
}

static int getchar_unintr(void)
{
        int ret;
        do
        {
                ret = getchar();
        } while (ret == EOF && errno == EINTR);
        return ret;
}

static int getch_internal(block_t block)
{
	int ret = EOF;
	char ch;

	if (is_stdin_console)
	{
		if (block == BLOCK_NONE)
		{
			console_set_block(BLOCK_NONE);
			ret = read_unintr(0, &ch, 1) > 0 ? ch : EOF;
			console_set_block(BLOCK_ONE);
		}
		else if (block == BLOCK_ONE)
			ret = read_unintr(0, &ch, 1) > 0 ? ch : EOF;
		else if (block == BLOCK_LINE)
		{
			console_set_block(BLOCK_LINE);
			ret = getchar_unintr();
			console_set_block(BLOCK_ONE);
		}
	}
	else
	{
		if (block == BLOCK_NONE)
			ret = EOF;
		else
			ret = getchar_unintr();
	}

	return ret;
}

static void flush_console_input()
{
	tcflush(0, TCIFLUSH);
}

#endif

static int chrin(block_t block)
{
	static int stuff_run = -1;
	int ret;

	if (in_source_mode)
	{
		suppress_output = 0;
		if (fp_source != NULL)
		{
			ret = getc(fp_source);
			if (ret != EOF)
				return ret;
			fclose(fp_source);
			fp_source = NULL;
			stuff_run = 0;
		}
		if (stuff_run >= 0)
		{
			source_mode_running = 1;
			ret = "RUN\n"[stuff_run++];
			if (ret)
				return ret;
			stuff_run = -1;
		}
	}
	return getch_internal(block);
}

static inline void GPL_SCAN_NOKEY()
{
	RAM[0x8375] = 0xFF;
	RAM[0x837C] &= ~0x20; /* clear cnd bit */
}

static inline void GPL_SCAN_KEY(uint8_t key)
{
	RAM[0x8375] = key;
	RAM[0x837C] |= 0x20;  /* set cnd bit to indicate new key */
}


/* emulate GPL SCAN routine */
static inline void GPL_SCAN()
{
	int ch;
	int grom_call_addr = grom_addr - 1;
	int mode = RAM[0x8374];

	if (mode == 1 || mode == 2)
	{
		/* TODO joystick scanning */
		GPL_SCAN_NOKEY();
	}
	else if (mode == 0 || mode == 3 || mode == 4 || mode == 5)
	{
		int block;
		if (mode != 0)
		{
			RAM[0x8374] = 0;         /* mode 0 next time */
			RAM[0x83C6] = mode - 3;  /* remember mode */
		}

		/* TODO: pay attention to mode */

		switch (grom_call_addr)
		{
		case 0x01B1:	/* Boot: Press any key */
			GPL_SCAN_KEY(' ');
			return;
		case 0x02F9:	/* Boot menu: Press 1,2,... */
			GPL_SCAN_KEY('1');  /* 1 FOR TI BASIC */
			return;
		case 0x010B:	/* reset routine */
		case 0x21A0:	/* return from BASIC */
		case 0x4E37:	/* breakpoint */
			flush_console_input();
			GPL_SCAN_NOKEY();
			return;
		case 0x35E9:	/* Sound: abort if CLEAR pressed */
		case 0x47FC:	/* LIST: abort if CLEAR pressed */
		case 0x5789:	/* CALL KEY */
		default:
			block = 0;
			break;
		case 0x142D:	/* Close/Delete: Press Y,N */
		case 0x1528:	/* Save: Press E,C,R,Enter */
			block = 1;
			break;
		case 0x2A5C:	/* Line editor */
			block = 2;
			supress_next_newline = 1;
			break;
		}

		ch = chrin(block);
		if (block && ch == EOF)
			exit(0);

		if (ch != EOF)
		{
			if (ch == '\n')
				ch = 0x0D;
			GPL_SCAN_KEY(ch);
		}
		else
			GPL_SCAN_NOKEY();
	}
	else
	{
		/* unknown mode */
		GPL_SCAN_NOKEY();
	}
}

extern int GPL_interp_dispatch()
{
	switch (PC)
	{
	case 0x0024: /* reset / GPL EXIT / BASIC BYE */
		exit(0);
		break;
	case 0x02AE:
		GPL_SCAN();
		break;
	case 0x05D6: /* GPL I/O Sound */
		{
			uint16_t sndp = rd(rd(R3));
			switch (sndp)
			{
			case 0x0479: /* Accept beep */
			case 0x0484: /* Error beep */
				if (!suppress_output)
					putchar('\a'); /* TODO */
				break;
			default: 
				warn("sound: %04X", sndp);
			}
			break;
		}
	case 0x05EA: /* GPL I/O CRU Output */
		/*warn("ignoring GPL I/O CRU Output");*/
		break;
	default:
		return 0;
	}
	return 1;
}
