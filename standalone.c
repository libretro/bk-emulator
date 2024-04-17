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

#include "defines.h"
#include "conf.h"
#include <SDL/SDL.h>
#include "intl.h"

unsigned int refreshtime = 0;
static int hasgame = 0;

const char * floppyA = "A.img";
const char * floppyB = "B.img";
const char * floppyC = "C.img";
const char * floppyD = "D.img";

char init_path[BUFSIZ];
char game_path[512];
static const char *const focal10rom = "FOCAL10.ROM";
static const char *const basic10rom = "BASIC10.ROM"; 

/*
 * Command line flags and variables.
 */

flag_t aflag;		/* autoboot flag */
flag_t covoxflag;	/* covox flag */
flag_t synthflag;	/* AY-3-8910 flag */
flag_t plipflag;	/* PLIP flag */
flag_t turboflag;	/* "Turbo" mode with doubled clock speed */
flag_t joystickflag;

long long ticks_screen = 0;
unsigned int hasexit = 0;

/*
 * main()
 */

int
main( argc, argv )
int argc;
char **argv;
{
	/* Gettext stuff */
 	setlocale (LC_ALL, "");
 	bindtextdomain ("bk", "/usr/share/locale");
 	textdomain ("bk");
	init_config();

	aflag = 1;		/* auto boot */
	nflag = 1;		/* enable sound */
	hasgame = 0;
	/* nothing is connected to the port by default, use ~/.bkrc */
	
	if ( args( argc, argv ) < 0 ) {
		fprintf( stderr, _("Usage: %s [options]\n"), argv[0] );
		fprintf( stderr, _("   -0        BK-0010\n") );
		fprintf( stderr, _("   -1        BK-0010.01\n") );
		fprintf( stderr, _("   -2        BK-0010.01 + FDD\n") );
		fprintf( stderr, _("   -3        BK-0011M + FDD\n") );
		fprintf( stderr, _("   -K        Terak 8510/a\n") );
		fprintf( stderr, _("   -A<file>  A: floppy image file or device (instead of %s)\n"), floppyA );
		fprintf( stderr, _("   -B<file>  B: floppy image file or device (instead of %s)\n"), floppyB );
		fprintf( stderr, _("   -C<file>  C: floppy image file or device (instead of %s)\n"), floppyC );
		fprintf( stderr, _("   -D<file>  D: floppy image file or device (instead of %s)\n"), floppyD );
		fprintf( stderr, _("   -a        Do not boot automatically\n") );
		fprintf( stderr, _("   -c        Color mode\n") );
		fprintf( stderr, _("   -n        Disable sound \n") );
		fprintf( stderr, _("   -v        Enable Covox\n") );
		fprintf( stderr, _("   -y        Enable AY-3-8910\n") );
		fprintf( stderr, _("   -m        Enable mouse\n") );
                fprintf( stderr, _("   -j        Enable joystick\n") );
		fprintf( stderr, _("   -S        Full speed mode\n") );
		fprintf( stderr, _("   -s        \"TURBO\" mode (Real overclocked BK)\n") );
		fprintf( stderr, _("   -R<file>  Specify an alternative ROM file @ 120000.\n") );
		fprintf( stderr, _("   -r<file>  Specify an alternative ROM file @ 160000.\n") );
		fprintf( stderr, _("   -T        Disable reading from tape\n") );
		fprintf( stderr, _("   -t        Trace mode, -t<file> - to file\n") );
		fprintf( stderr, _("   -l<path>  Enable printer and specify output pathname\n") );
		fprintf( stderr, _("\n\
The default ROM files are stored in\n\
%s or the directory specified\n\
by the environment variable BK_PATH.\n"), romdir );
		fprintf( stderr, _("\nExamples:.\n") );
		fprintf( stderr, _("   'bk -R./BK.ROM' - Use custom ROM\n") );
		fprintf( stderr, _("   'bk -a -n -f'   - Developer's mode\n") );
		fprintf( stderr, _("   'bk -c'         - Gaming mode\n\n") );
		exit( -1 );
	}

	atexit(SDL_Quit);
	atexit(disk_finish);

	/* Set ROM configuration */

	if (getenv("BK_PATH"))
	{
		romdir = getenv("BK_PATH");
	}

	switch( bkmodel ) {
	case 0: /* BK0010 */
		rompath10 = monitor10rom;
		rompath12 = focal10rom;
		rompath16 = 0;
		TICK_RATE = 3000000;
		break;
	case 1: /* BK0010.01 */
		rompath10 = monitor10rom;
		rompath12 = basic10rom;
		rompath16 = 0;
		TICK_RATE = 3000000;
		break;
	case 2: /* BK0010.01+FDD */
		rompath10 = monitor10rom;
		rompath12 = 0;
		rompath16 = diskrom;
		TICK_RATE = 3000000;
		break;
	case 3:	/* BK-0011M */
	case 9: /* Terak 8510/a */
		rompath10 = rompath12 = rompath16 = 0;
		TICK_RATE = 4000000;
		break;
	case 4: /* Slow BK-0011M */
		rompath10 = rompath12 = rompath16 = 0;
		TICK_RATE = 3000000;
		break;
	default: /* Unknown ROM configuration */
		fprintf( stderr, _("Unknown BK model. Bailing out.\n"), argv[0] );
		exit( -1 );
	}
	
	/* Turn on the "TURBO" mode */
	if ( turboflag ) {
	    TICK_RATE = (TICK_RATE * 2); 
	}

	printf( _("Initializing SDL.\n") );

        Uint32 sdlflags = SDL_INIT_VIDEO|SDL_INIT_TIMER;
        if (joystickflag)
          sdlflags |= SDL_INIT_JOYSTICK;

	if((SDL_Init(sdlflags)==-1)) {
		printf( _("Could not initialize SDL: %s.\n"), SDL_GetError());
		exit(-1);
	}

	fprintf(stderr, _("Welcome to \"Elektronika BK\" emulator!\n\n") );
	showemuhelp(); /* print a short emulator help message */
	showbkhelp(); /* print a short help message */

	printf( _("SDL initialized.\n") );

	/* Convert BK model to 0010/0011 flag */
	fake_disk &= bkmodel >= 2;
	terak = bkmodel == 9;
	bkmodel = bkmodel >= 3;
	if (joystickflag)
	  joystick_init();
	tty_open();             /* initialize the tty stuff */
	ev_init();		/* initialize the event system */
	sim_init();		/* ...the simulated cpu */
	mem_init();		/* ...main memory */
	bk_scr_init();		/* video display */
	boot_init(false);		/* ROM blocks */
	if (terak) {
		// setup_terak();
	} else {
	if (mouseflag)
		plug_mouse();
        if (joystickflag)
		plug_joystick();
	if (printer_file)
		plug_printer();
	if (covoxflag)
		plug_covox();
	if (synthflag)
		plug_synth();
	if (plipflag)
		plug_bkplip();
	}
	q_reset();             /* ...any devices */

	/* Starting frame rate */ 
	frame_delay = TICK_RATE/25;
	half_frame_delay = TICK_RATE/50;

	if (terak) {
		pdp.regs[PC] = 0173000;
	} else {
		lc_word(0177716, &pdp.regs[PC]);
		pdp.regs[PC] &= 0177400;
	}
	if (init_path[0]) {
		tracefile = fopen(init_path, "w");
	}
	
	/*if ( aflag ) 
	{
		run( 1 );
		ui();
	} else {
		ui();
	}*/
	while(!hasexit)
	{
		run( 1 );			/* go for it */	
		if (hasgame == 1)
		{
		  	FILE *f = fopen(game_path, "r");
			hasgame = 0;
			if (!f) {
			  perror("Error opening file");
			  continue;
			}
			load_and_run(f);
		}
		else if (hasexit == 0)
		{
			ui();	
		}
	}
	
	disk_finish();
	Quit_SDL();

	return 0;		/* get out of here */
}


/*
 * args()
 */

args( argc, argv )
int argc;
char **argv;
{
	char *farg;
	char **narg;

	narg = argv;
	while ( --argc ) {
		narg++;
		farg = *narg;
		if ( *farg++ == '-' ) {
			switch( *farg ) {
			 case '0': case '1': case '2': case '3': case '4':
				bkmodel = *farg - '0';
				break;
			 case 'K':
				bkmodel = 9;
				// Terak has no sound yet, turn sound off
				nflag = 0;
				break;
			 case 'A':
				floppyA = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'B':
				floppyB = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'C':
				floppyC = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'D':
				floppyD = *++farg ? farg : (argc--,*++narg);
				break;
			case 'a':
				aflag = 0;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'n':
				nflag = 0;
				break;
			case 'v':
				covoxflag = 1;
				break;
			case 'y':
				synthflag = 1;
				break;
			case 'm':
				mouseflag = *(farg+1) ? *(farg+1)-'0' : 1;
				break;
                        case 'j':
				joystickflag = *(farg+1) ? *(farg+1)-'0' : 1;
				break;
			case 'p':
				plipflag = 1;
				break;
			case 'S':
				fullspeed = 1;
				break;
			case 's':
				turboflag = 1;
				break;
			case 'R':
				rompath12 = *++farg ? farg : (argc--,*++narg);
				break;
			case 'r':
				rompath16 = *++farg ? farg : (argc--,*++narg);
				break;
			case 'T':
				tapeflag = 1;
				break;
			case 't':
				traceflag = 1;
				if (*++farg)
					strcpy(init_path, farg);
				break;
			case 'g':
				hasgame = 1;
				if (*++farg)
					strcpy(game_path, farg);
				break;
			case 'l':
				printer_file = *++farg ? farg : (argc--,*++narg);;
				break;
			default:
				return -1;
				/*NOTREACHED*/
				break;
			}
		} else {
			return -1;
		}
	}
	return 0;
}

/*
 * run() - Run instructions (either just one or until something bad
 * happens).  Lots of nasty stuff to set up the terminal and the
 * execution timing.
 */

int
run( int flag )
{
	register pdp_regs *p = &pdp;	/* pointer to global struct */
	struct timeval start_time;	/* system time of simulation start */
	struct timeval stop_time;	/* system time of simulation end */
	double expired, speed;		/* for statistics information */

	/*
	 * Set up the terminal cbreak i/o and start running.
	 */

	gettimeofday( &start_time, 0 );
	run_2( p, flag );
	gettimeofday( &stop_time, 0 );

	if (!flag)
		return;

	/*
	 * Compute execution statistics and print it.
	 */

	expired = ((double) stop_time.tv_sec) +
		(((double) stop_time.tv_usec) / 1000000.0 );
	expired -= ((double) start_time.tv_sec) +
		(((double) start_time.tv_usec) / 1000000.0 );
	if ( expired != 0.0 )
		speed = (((double)p->total) / expired );
	else
		speed = 0.0;
	fprintf( stderr, _("Instructions executed: %d\n"), p->total );
	fprintf( stderr, _("Simulation rate: %.5g instructions per second\n"),
		speed );
	fprintf( stderr, _("BK speed: %.5g instructions per second\n"),
		(double) p->total * TICK_RATE / ticks );
	p->total = 0;
}

int
run_2( p, flag )
register pdp_regs *p;
int flag;
{
	/*
	 * Clear execution stop flag and install SIGINT handler.
	 */

	stop_it = 0;
	signal( SIGINT, intr_hand );

	double timing_delta = ticks - SDL_GetTicks() * (TICK_RATE/1000.0);
	c_addr startpc = p->regs[PC];

	/*
	 * Run until told to stop.
	 */

	do 
	{
		flag = run_cpu_until(p, ticks_screen);

		/* Some games require the BASIC ROM to be loaded
		 * in memory before they are playable so i had to make
		 * this little hack. */
		if (hasgame == 1) {
		  refreshtime++;
		  if (refreshtime > 2) return 0;
		}

		if (ticks >= ticks_screen) {
		    maybe_scr_flush();
		    tty_recv();
		    ticks_screen += frame_delay;
		    /* In simulated speed, if we're more than 10 ms
		     * ahead, slow down. Avoid rounding the delay up
		     * by SDL. If the sound is on, sound buffering
		     * provides synchronization.
		     */
		    if (!fullspeed && !nflag) {
		    	double cur_delta =
				ticks - SDL_GetTicks() * (TICK_RATE/1000.0);
			if (cur_delta - timing_delta > TICK_RATE/100) {
				int msec = (cur_delta - timing_delta) / (TICK_RATE/1000);
				SDL_Delay((msec / 10 * 10));
			}
		    }
		}

		if (hasexit == 1) {
			flag = 0;
		}
	} while( flag );

	signal( SIGINT, SIG_DFL );
}

void showemuhelp()
{
    fprintf(stderr, _("Emulator window hotkeys:\n\n"));
    fprintf(stderr, _(" ScrollLock - Toggle video mode (B/W, Color)\n"));
    fprintf(stderr, _(" Left Super+F11 - Reset emulated machine\n"));
    fprintf(stderr, _(" F12 - Load a file into BK memory\n\n"));
}

void showbkhelp()
{
char *monitor10help = _("BK0010 MONITOR (the OS) commands:\n\n\
 'A' to 'K'  - Quit MONITOR.\n\
 'M'         - Read from tape. Press 'Enter' and type in the filename of\n\
               the desired .bin snapshot. Wait until the data loads into\n\
               the memory or use F12 instead.\n\
 'S'         - Start execution. You can specify an address to start from.\n");

char *monitor11help = _("BK0011M BOS commands:\n\n\
 'B'         - Boot from any bootable floppy.\n\
 'xxxB'      - Boot from the floppy drive xxx.\n\
 'L'         - Load file from tape\n\
 'xxxxxxL'   - Load file to address xxxxxx.\n\
 'M' or '0M' - Turn the tape-recoder on.\n\
 'xM'        - Turn the tape-recoder off.\n\
 'G'         - Run currently loaded program.\n\
 'xxxxxxG'   - Run from address xxxxxx.\n\
 'P'         - Continue after the STOP key press or HALT.\n\
 'Step'      - Execute a single instruction and return to MONITOR.\n\
 'Backspace' - Delete last digit (digits only).\n\
 'xxxxxx/'   - Open word xxxxxx (octal) in memory for editing.\n\
 'xxxxxx\\'   - Open byte xxxxxx (octal) in memory for editing.\n\
 'Rx'        - Open system register x for editing.\n\
 'Enter'     - Close opened memory cell and accept changes.\n\
 'Up'        - Move to the next memory cell and accept changes.\n\
 'Down'      - Move to the previous memory cell and accept changes\n\
 'Left'      - Jump to address <address>+<word>+2 (\"67\" addressing).\n\
 'Right'     - Jump to address <address>+<byte>*2+2 (assembler 'BR' jump)\n\
 '@'         - Close and jump to the address stored in the current memory cell.\n\
 'N;MC'      - Map memory page N (octal) to address range M (octal).\n");

    switch( bkmodel ) { /* Make the hints model-specific */
    case 0: /* BK0010 */
	fprintf(stderr, monitor10help);
        fprintf(stderr, _(" 'T' - Run built-in tests.\n\n"));
        fprintf(stderr, _("Type 'P M' to quit FOCAL and get the MONITOR prompt.\n"));
        fprintf(stderr, _("Type 'P T' to enter the test mode. 1-5 selects a test.\n\n"));
    break;
    case 1: /* BK0010.01 */
	fprintf(stderr, monitor10help);
        fprintf(stderr, _("\nType 'MO' to quit BASIC VILNIUS 1986 and get the MONITOR prompt.\n\n"));
    break;
    case 2: /* BK0010.01+FDD */
	fprintf(stderr, monitor10help);
        fprintf(stderr, _("\nType 'S160000' to boot from floppy A:.\n"));
        fprintf(stderr, _("The BASIC ROM is disabled.\n\n"));
    break;
    case 3: /* BK0011M+FDD */
	fprintf(stderr, monitor11help);
        fprintf(stderr, _("\nBK-0011M boots automatically from the first floppy drive available.\n\n"));
    break;
    case 4: /* BK0011M */
	fprintf(stderr, monitor11help);
    break;
    }
}


/* Pretty much had to rewrite it for portability rofl. - Gameblabla 
 * This does not seem to handle writes to the file.
 * */
static void disk_open(disk_t * pdt, const char * name) 
{
	FILE* fp;
	int result;
	
	/* First, we check if the file exists. */
	fp = fopen(name, "rb");
	if (!fp)
	{
		/* It doesn't so let's exit right away. */
		perror(name);
		return;
	}
	else
		fclose(fp);
	
	fp = fopen(name, "r+b");
	if (!fp)
	{
		/* Open file as Read-only */
		fp = fopen(name, "rb");
		if (!fp)
		{
			perror(name);
			return;
		}
		pdt->ro = 1;
	}
	
	/* Determine size of file*/
	fseek(fp , 0 , SEEK_END );
	pdt->length = ftell (fp);
	fseek(fp , 0 , SEEK_SET );
	
	if (pdt->length == -1) perror("seek");
	if (pdt->length % 64) 
	{
		fprintf(stderr, _("%s is not an integer number of blocks: %d bytes\n"), name, pdt->length);
		fclose(fp);
		return;
	}
	
	pdt->image = malloc(pdt->length);
	if (pdt->image == NULL)
	{
		fprintf(stderr, _("Unable to malloc. Out of memory ?\n"));
		fclose(fp);
		perror(name);
	}
	
	result = fread (pdt->image, sizeof(unsigned char), pdt->length, fp);
	if (fp) fclose(fp);
	
	if (pdt->ro) 
	{
		fprintf(stderr, _("%s will be read only\n"), name);
	}
}

void platform_disk_init(disk_t *disks) {
        disk_open(&disks[0], floppyA);	
        disk_open(&disks[1], floppyB);	
        disk_open(&disks[2], floppyC);	
        disk_open(&disks[3], floppyD);	
}

void *load_rom_file(const char * rompath, size_t *sz, size_t min_sz, size_t max_sz)
{
	FILE * romf;

	char *path = malloc(strlen(romdir)+strlen(rompath)+2);

	if (!path) { fprintf(stderr, _("No memory\n")); exit(1); }

	/* If rompath is a real path, do not apply romdir to it */
	if (*romdir && !strchr(rompath, '/'))
		sprintf(path, "%s/%s", romdir, rompath);
	else
		strcpy(path, rompath);

	fprintf(stderr, _("Loading %s..."), path);

	romf = fopen(path, "r");
	if (!romf) {
		fprintf(stderr, _("Couldn't open file.\n"));
		exit(1);
	}

	char *ret = malloc (max_sz);
	int c, i;

	while ((c = fgetc(romf)) >= 0)
		ret[i++] = c;

	fclose(romf);
	free(path);

	if (i < min_sz) {
		fprintf(stderr, _("Incomplete or damaged file.\n"));
		exit(1);
	}

	*sz = i;

	return ret;
}
