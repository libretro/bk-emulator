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
 * boot.c - Boot Code
 */

#include "defines.h"
#include "intl.h"

/*
 * load_rom() - Read the contents of the ROM into the array. 
 * Used for BK-0010 style ROM - stores into the mapped memory.
 */

static int load_rom(unsigned start, const char * rompath, unsigned min_size, unsigned max_size) {
	int i;

	if (!rompath || !*rompath) return 1;

	size_t sz;
	unsigned char *romf = load_rom_file (rompath, &sz, min_size, max_size);

	if (romf == NULL)
		return 0;
	
	unsigned long saved_ram_map = pdp_ram_map;
	pdp_ram_map = ~0l;
	for (i = 0; i < sz/2; i++, start+=2) {
		sc_word(start, romf[2 * i] | romf[2 * i + 1]<<8);
	}
	free(romf);
        fprintf(stderr, _("Done.\n"));
	pdp_ram_map = saved_ram_map;

	return 1;
}

/*
 * Loads BK-0011 ROM into the givem ROM block from a given offset.
 */
static int load_rom11(d_word * rombuf, int byte_off, const char * rompath, int byte_size) {
	int i;

	if (!rompath || !*rompath) return 1;
	size_t sz;
	unsigned char *romf = load_rom_file (rompath, &sz, byte_size, byte_size);
	if (romf == NULL)
		return 0;

	rombuf += byte_off/2;
	for (i = 0; i < byte_size/2; i++, rombuf++) {
		*rombuf = romf[2 * i] | romf[2 * i + 1]<<8;
	}
	free(romf);
        fprintf(stderr, _("Done.\n"));

	return 1;
}

int
boot_init(int force)
{
	int ok = 1;
	static unsigned char boot_done = 0;
	if (force == 0 && boot_done) return 1;

	boot_done = 1;

	if (terak) {
		/* So far we only have Terak boot ROM */
		ok = ok && load_rom(0173000, "TERAK.ROM", 128, 128);
		return ok;
	}
	if (bkmodel != 0) {
		ok = ok && load_rom11(system_rom, 0, bos11rom, 8192);
		ok = ok && load_rom11(system_rom, 8192, diskrom, 4096);
		ok = ok && load_rom11(rom[0], 0, basic11arom, 16384);
		ok = ok && load_rom11(rom[1], 0, basic11brom, 8192);
		ok = ok && load_rom11(rom[1], 8192, bos11extrom, 8192);
		return ok;
	}

	/* Monitor must be exactly 8k */
	ok = ok && load_rom(0100000, rompath10, 8192, 8192);

	/* Basic or Focal ROM may be 24448 to 24576 bytes */
	ok = ok && load_rom(0120000, rompath12, 24448, 24576);

	/* Disk controller BIOS is exactly 4k */
	ok = ok && load_rom(0160000, rompath16, 4096, 4096);

	return ok;
}
