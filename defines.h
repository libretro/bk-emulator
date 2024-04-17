/*
 * This file is part of 'pdp', a PDP-11 simulator.
 *
 * For information contact:
 *
 *   Computer Science House
 *   Attn: Eric Edwards
 *   Box 861
 *   25 Andrews Memorial Drive
 *   Rochester, NY 14623
 *
 * Email:  mag@potter.csh.rit.edu
 * FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
 * 
 * Copyright 1994, Eric A. Edwards
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Eric A. Edwards makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * defines.h
 */


/*
 * Stuff to maintain compatibility across platforms.
 */

#ifndef DEFINES_INCLUDED
#define DEFINES_INCLUDED
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>		/* COMMENT for vax-bsd */
/* #include <sgtty.h> */
#include <ctype.h>
#include <fcntl.h>
#include <string.h>

/*#include <sys/select.h>	UNCOMMENT for AIX */


#if defined(sparc) || defined(m88k)	/* ADD AIX here */
#define SWAB
#endif


/*
 * Type definitions for PDP data types.
 */

typedef unsigned long c_addr;	/* core or BK Q-bus address (17 bit so far) */
typedef unsigned short l_addr;	/* logical address (16 bit) */
typedef unsigned short d_word;	/* data word (16 bit) */
typedef unsigned char d_byte;	/* data byte (8 bit) */
typedef unsigned char flag_t;	/* for boolean or small value flags */

void plug_joystick(void);
void plug_printer(void);
void plug_mouse(void);
void plug_covox(void);
void plug_synth(void);
void plug_bkplip(void);
void tty_keyevent(int c);
void fake_disk_io(void);
void fake_sector_io(void);
c_addr disas (c_addr a, char * dest);
void fake_tuneup_sequence(void);
void fake_array_with_tuneup(void);
void fake_read_strobe(void);
void fake_write_file(void);
void sound_discard(void);
void sound_semwait(void);
void platform_sound_init(void);
void sound_write_sample(short val);
void sound_flush(void);
void platform_sound_flush(void);

/*
 * PDP processor defines.
 */

#define R5	5	/* return register for MARK */
#define SP	6	/* stack pointer */
#define PC	7	/* program counter */


typedef struct _pdp_regs {
	d_word regs[8];		/* general registers */
	d_byte psw;		/* processor status byte (LSI-11) */
	d_word ir;		/* current instruction register */
	d_word ea_addr;		/* stored address for dest modifying insts */
	unsigned long total;	/* count of instructions executed */
	unsigned look_time;	/* when to handle things, saves time */
} pdp_regs;

extern unsigned int hasexit;

void timing(register pdp_regs *p);
int lc_word(c_addr addr, d_word *word);
int sc_word(c_addr addr, d_word word);
int load_src(register pdp_regs *p, d_word *data);
/*
 * Definitions for the memory map and memory operations.
 */


#define OK		0	/* memory and instruction results */
#define ODD_ADDRESS	1
#define BUS_ERROR	2
#define MMU_ERROR	3
#define CPU_ILLEGAL	4
#define CPU_HALT	5
#define CPU_WAIT	6
#define CPU_NOT_IMPL	7
#define CPU_TRAP	8
#define CPU_EMT		9
#define CPU_BPT		10
#define CPU_IOT		11
#define CPU_RTT		12
#define CPU_TURBO_FAIL	13

/*
 * Q-bus device addresses.
 */

#define BASIC           0120000
#define BASIC_SIZE      (24 * 512)      /* 24 Kbytes */
#define PORT_REG        0177714         /* printer, mouse, covox, ... */
#define PORT_SIZE       1
#define IO_REG          0177716         /* tape, speaker, memory */
#define IO_SIZE         1
#define TTY_REG         0177660
#define TTY_SIZE        3
#define LINE_REG        0176560
#define LINE_SIZE       4
#define TIMER_REG       0177706
#define TIMER_SIZE      3
#define DISK_REG	0177130
#define DISK_SIZE	2
#define SECRET_REG	0177700
#define SECRET_SIZE	3
#define TERAK_BOOT	0173000
#define TERAK_BSIZE	0200
#define TERAK_DISK_REG 0177000
#define TERAK_DISK_SIZE 2

#define PDP_READABLE_MEM_SIZE   (63 * 512)  /* 0 - 175777 */
#define PDP_FULL_MEM_SIZE       (64 * 512)  /* 0 - 177777 */

extern void line_init(void);
void bk_scr_init(void);
int boot_init(int force);
void timer_init(void);
void printer_init(void);
void covox_init(void);
void synth_init(void);
void mouse_init(void);
void tty_init(void);
void bkplip_init(void);
void io_init(void);
void disk_init(void);
void tdisk_init(void);
extern int boot_read(void), boot_write(c_addr, d_word), boot_bwrite(c_addr, d_byte);
extern int scr_write(int, c_addr, d_word), scr_switch(int, int);
extern int tty_read(c_addr addr, d_word *word);
extern int tty_write(c_addr, d_word), tty_bwrite(c_addr, d_byte);
extern int io_read(c_addr addr, d_word *word), io_write(c_addr, d_word), io_bwrite(c_addr, d_byte);
extern int disk_read(c_addr addr, d_word *word), disk_write(c_addr, d_word), disk_bwrite(c_addr, d_byte);
extern int tdisk_read(c_addr addr, d_word *word), tdisk_write(c_addr, d_word), tdisk_bwrite(c_addr, d_byte);
extern void disk_finish(void);
extern void tdisk_finish(void);
extern void io_read_start(void);
extern int timer_read(c_addr addr, d_word *word);
extern int timer_write(c_addr, d_word), timer_bwrite(c_addr, d_byte);
extern int line_read(c_addr addr, d_word *word), line_write(c_addr, d_word), line_bwrite(c_addr, d_byte);
extern int printer_read(c_addr addr, d_word *word), printer_write(c_addr, d_word), printer_bwrite(c_addr, d_byte);
extern int mouse_read(c_addr addr, d_word *word), mouse_write(c_addr, d_word), mouse_bwrite(c_addr, d_byte);
extern int covox_read(c_addr addr, d_word *word);
extern int covox_write(c_addr, d_word), covox_bwrite(c_addr, d_byte);
extern int synth_read(c_addr addr, d_word *word), synth_write(c_addr, d_word), synth_bwrite(c_addr, d_byte), synth_next(void);
extern int bkplip_read(c_addr addr, d_word *word), bkplip_write(c_addr, d_word), bkplip_bwrite(c_addr, d_byte);
extern int service(d_word vector);
unsigned short *get_vram_line (int bufno, int line);
void tape_read_start(void);
void tape_read_finish(void);
     
/*
 * Defines for the event handling system.
 */

#define NUM_PRI         2

/* Timer interrupt has higher priority */
#define TIMER_PRI	0
#define TTY_PRI		1

typedef struct _event {
	int (*handler)(d_word);		/* handler function */
	d_word info;			/* info or vector number */
	double when;			/* when to fire this event */
} event;


/*
 * Instruction Table for Fast Decode.
 */

typedef int (*_itab_t)(register pdp_regs *p);


/*
 * Global variables.
 */

struct bk_state {
	int ui_done;
	unsigned short _last_branch;
	pdp_regs _pdp; /* internal processor registers */
	int _TICK_RATE;

	unsigned short _tty_scroll;
	flag_t _key_pressed;
	flag_t _fake_disk;
	/* 1 if interrupt requested while WAIT was in effect */
	flag_t _in_wait_instr;
	unsigned io_sound_val;
	flag_t _io_stop_happened;
	int    io_tape_mode, io_tape_val, io_tape_bit;
	flag_t _nflag;		/* audio flag */
	flag_t _mouseflag;	/* mouse flag */
	flag_t _fullspeed;	/* do not slow down to real BK speed */
	flag_t _tapeflag;	/* Disable reading from tape */
	double _frame_delay;	/* Delay in ticks between video frames */
        double _half_frame_delay;

	flag_t _cflag, _bkmodel, _terak;
	long long _ticks_timer;
	long long _ticks, io_tape_ticks;     /* in main clock freq, integral */
	long long _tape_read_ticks, _tape_write_ticks;
	flag_t _timer_intr_enabled;
	volatile int _stop_it;	/* set when a SIGINT happens during execution */

	/* 
	 * BK-0011 has 8 8Kw RAM pages and 4 8 Kw ROM pages.
	 * RAM pages 1 and 7 are video RAM.
	 */
	d_word _ram[8][8192];
	d_word _rom[4][8192];
	d_word _system_rom[8192];
	unsigned char _umr[65536];

	/*
	 * Each bit corresponds to a Kword,
	 * the lowest 8 Kwords are RAM, the next 8 are screen memory,
	 * the rest is usually ROM.
	 */
	unsigned long _pdp_ram_map;
	unsigned long _pdp_mem_map;

	d_word _timer_count, _timer_setup, _timer_control;
	long long _ticks_start;
	unsigned int _timer_period;

	long long _framectr;
	long long _soundctr;

	unsigned _scan_line_duration;

	unsigned char _covox_val;
	unsigned int _covox_age;
	unsigned _io_sound_val;

	unsigned _io_max_sound_age;
	unsigned _io_sound_age;	/* in io_sound_pace's since last change */
	double _io_sound_pace;
	double _io_sound_count;
};

extern const char *printer_file;

extern const char *rompath10, *rompath12, *rompath16;
extern const char *const bos11rom, *const diskrom, *const bos11extrom;
extern const char *const basic11arom, *const basic11brom;

extern struct bk_state current_state;
extern 	flag_t traceflag;	/* print all instruction addresses */
extern 	FILE * tracefile;	/* trace goes here */
extern char * romdir;

extern unsigned scr_dirty;

extern const _itab_t itab[];

#define bkmodel current_state._bkmodel
#define stop_it current_state._stop_it
#define nflag current_state._nflag
#define io_stop_happened current_state._io_stop_happened
#define ticks current_state._ticks
#define in_wait_instr current_state._in_wait_instr
#define pdp current_state._pdp
#define timer_intr_enabled current_state._timer_intr_enabled
#define last_branch current_state._last_branch
#define ticks_timer current_state._ticks_timer
#define terak current_state._terak
#define TICK_RATE current_state._TICK_RATE
#define mouseflag current_state._mouseflag
#define fullspeed current_state._fullspeed
#define key_pressed current_state._key_pressed
#define fake_disk current_state._fake_disk
#define tapeflag current_state._tapeflag
#define frame_delay current_state._frame_delay
#define half_frame_delay current_state._half_frame_delay
#define cflag current_state._cflag
#define tty_scroll current_state._tty_scroll
#define ram current_state._ram
#define rom current_state._rom
#define system_rom current_state._system_rom
#define umr current_state._umr
#define pdp_ram_map current_state._pdp_ram_map
#define pdp_mem_map current_state._pdp_mem_map

#define timer_count current_state._timer_count
#define timer_setup current_state._timer_setup
#define timer_control current_state._timer_control
#define ticks_start current_state._ticks_start
#define timer_period current_state._timer_period
#define framectr current_state._framectr
#define soundctr current_state._soundctr
#define scan_line_duration current_state._scan_line_duration

#define covox_val current_state._covox_val
#define covox_age current_state._covox_age
#define io_sound_val current_state._io_sound_val
#define io_max_sound_age current_state._io_max_sound_age
#define io_sound_age current_state._io_sound_age
#define io_sound_pace current_state._io_sound_pace
#define io_sound_count current_state._io_sound_count
#define tape_read_ticks current_state._tape_read_ticks
#define tape_write_ticks current_state._tape_write_ticks

/*
 * Inline defines.
 */

/* For BK-0010 */

#define ll_word(p, a, w) lc_word(a, w)
#define sl_word(p, a, w) sc_word(a, w)

#define CC_N	010
#define CC_Z	04
#define CC_V	02
#define CC_C	01

#define CLR_CC_V()	p->psw &= ~CC_V
#define CLR_CC_C()	p->psw &= ~CC_C
#define CLR_CC_Z()	p->psw &= ~CC_Z
#define CLR_CC_N()	p->psw &= ~CC_N
#define CLR_CC_ALL()	p->psw &= ~(CC_V|CC_C|CC_Z|CC_N)

#define SET_CC_V()	p->psw |= CC_V
#define SET_CC_C()	p->psw |= CC_C
#define SET_CC_Z()	p->psw |= CC_Z
#define SET_CC_N()	p->psw |= CC_N

#define SRC_MODE	(( p->ir & 07000 ) >> 9 )
#define SRC_REG		(( p->ir & 0700 ) >> 6 )
#define DST_MODE	(( p->ir & 070 ) >> 3 )
#define DST_REG		( p->ir & 07 )

#define LSBIT	1		/*  least significant bit */

#define	MPI	0077777		/* most positive integer */
#define MNI	0100000		/* most negative integer */
#define NEG_1	0177777		/* negative one */
#define SIGN	0100000		/* sign bit */
#define CARRY   0200000		/* set if carry out */

#define	MPI_B	0177		/* most positive integer (byte) */
#define MNI_B	0200		/* most negative integer (byte) */
#define NEG_1_B	0377		/* negative one (byte) */
#define SIGN_B	0200		/* sign bit (byte) */
#define CARRY_B	0400		/* set if carry out (byte) */

#define LOW16( data )	(( data ) & 0177777 )	/* mask the lower 16 bits */
#define LOW8( data )	(( data ) & 0377 )	/* mask the lower 8 bits */

#define CHG_CC_N( d )	if ((d) & SIGN ) \
					SET_CC_N(); \
				else \
					CLR_CC_N()

#define CHGB_CC_N( d )	if ((d) & SIGN_B ) \
				SET_CC_N(); \
			else \
				CLR_CC_N()

#define CHG_CC_Z( d )	if ( d ) \
					CLR_CC_Z(); \
				else \
					SET_CC_Z()

#define CHGB_CC_Z( d )	if ( LOW8( d )) \
				CLR_CC_Z(); \
			else \
				SET_CC_Z()

#define CHG_CC_C( d )	if ((d) & CARRY ) \
					SET_CC_C(); \
				else \
					CLR_CC_C()

#define CHG_CC_IC( d )	if ((d) & CARRY ) \
					CLR_CC_C(); \
				else \
					SET_CC_C()

#define CHGB_CC_IC( d )	if ((d) & CARRY_B ) \
				CLR_CC_C(); \
			else \
				SET_CC_C()

#define CHG_CC_V( d1, d2, d3 )	\
				if ((( d1 & SIGN ) == ( d2 & SIGN )) \
				&& (( d1 & SIGN ) != ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_VC( d1, d2, d3 )	\
				if ((( d1 & SIGN ) != ( d2 & SIGN )) \
				&& (( d2 & SIGN ) == ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_VS( d1, d2, d3 )	\
				if ((( d1 & SIGN ) != ( d2 & SIGN )) \
				&& (( d1 & SIGN ) == ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHGB_CC_V( d1, d2, d3 )	\
				if ((( d1 & SIGN_B ) == ( d2 & SIGN_B )) \
				&& (( d1 & SIGN_B ) != ( d3 & SIGN_B ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHGB_CC_VC(d1,d2,d3)	\
				if ((( d1 & SIGN_B ) != ( d2 & SIGN_B )) \
				&& (( d2 & SIGN_B ) == ( d3 & SIGN_B ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_V_XOR_C_N()	\
				if ((( p->psw & CC_C ) && \
				   ( p->psw & CC_N )) \
				|| ((!( p->psw & CC_C )) && \
				   ( ! ( p->psw & CC_N )))) \
					CLR_CC_V(); \
				else \
					SET_CC_V()

int joystick_read(c_addr addr, d_word *word);
void joystick_init(void);
int joystick_write(c_addr addr, d_word word);
int joystick_bwrite(c_addr addr, d_byte byte);

typedef enum {
       nopD, rtcD, stepinD, stepoutD, readtsD, readD, writeD, delD
} disk_cmd;

/* Why bother, let's memory-map the files! */
typedef struct {
       unsigned int length;
       unsigned short * image;
       const unsigned short * ptr;
       unsigned char track;
       unsigned char side;
       unsigned char ro;
       unsigned char motor;
       unsigned char inprogress;
       unsigned char crc;
       unsigned char need_sidetrk;
       unsigned char need_sectsize;
       unsigned char cursec;
        disk_cmd cmd;
} disk_t;

extern unsigned long pending_interrupts;

void ev_register(unsigned priority, int (*handler)(d_word),
		 unsigned long delay,	/* in clock ticks */
		 d_word info);
void ev_fire( int priority );
char * state(pdp_regs * p);
void pagereg_bwrite(d_byte byte);
void serial_write(d_word w);
void tape_write(unsigned status, unsigned val);
int storeb_dst(register pdp_regs *p, d_byte data);
void pagereg_write(d_word word);
void sound_init(void);
int load_dst(register pdp_regs *p, d_word *data);
void tape_write(unsigned status, unsigned val);
int tape_read(void);
void tape_init(void);
d_word serial_read(void);
void scr_param_change(int pal, int buf);
int store_dst_2( register pdp_regs *p, d_word data);
int load_dst(register pdp_regs *p, d_word *data);
int store_dst(register pdp_regs *p, d_word data);
int loadb_dst(register pdp_regs *p, d_byte *data);
int brx(register pdp_regs *p, unsigned clear, unsigned set);
int pop(register pdp_regs *p, d_word *data);
int storeb_dst_2(register pdp_regs *p, d_byte data);
void q_reset(void);
int sl_byte(register pdp_regs *p, d_word laddr, d_byte byte);
int push(register pdp_regs *p, d_word data);
void addtocybuf(int val);
void mem_init(void);
void sim_init(void);
void tty_open(void);
void ev_init(void);
void scr_flush(void);
void maybe_scr_flush(void);
int run_cpu_until(register pdp_regs *p, long long max_ticks);
void load_and_run(FILE *f);
int ll_byte( register pdp_regs *p, d_word baddr, d_byte *byte );
int loadb_src( register pdp_regs *p, d_byte *data);
int load_ea( register pdp_regs *p, d_word *addr);
void scr_sync(void);
extern int breakpoint;
extern unsigned char change_req;
extern unsigned char param_change_line;
void scr_common_init(void);
extern unsigned char req_page[512], req_palette[512];
extern int cybuf[1024];
extern int cybufidx;
void ui_download(void);
void intr_hand(void);
d_word platform_joystick_get_state(void);
void platform_joystick_init(void);

enum joystick_state {
  JOYSTICK_BUTTON1 = 0x1,
  JOYSTICK_BUTTON2 = 0x2,
  JOYSTICK_BUTTON3 = 0x4,
  JOYSTICK_BUTTON4 = 0x8,
  JOYSTICK_RIGHT = 0x10,
  JOYSTICK_DOWN = 0x20,
  JOYSTICK_LEFT = 0x200,
  JOYSTICK_UP = 0x400
};

static inline enum joystick_state JOYSTICK_BUTTON(int idx) {
  return 1 << idx;
}

void platform_disk_init(disk_t *disks);

extern char * tape_prefix;
extern char * tape_suffix;

void load_and_run_bin(const void *data, size_t sz);
void *load_rom_file(const char * rompath, size_t *sz, size_t min_sz, size_t max_sz);
int load_file(FILE *f, int addr);

#endif
