/* $Id: mkfirm.c,v 1.1 2005/09/01 20:49:08 stefan Exp stefan $
 *
 * Original version by Petr Novak for SMC7004ABR rev.2
 * Modified to support SMC7004VWBR PN720.x version by BLFC from Openline ISP.
 *
 * Heavily modified to support many different devices and enhanced firmware
 * formats by Stefan Weil.
 *
 * This software is distributed under the GNU public license (GPL).
 */

#include <stdio.h>
#include <stdlib.h>	// exit
#include <string.h>
#include <unistd.h>	// getopt
#include <sys/types.h>
#include <fcntl.h>

typedef unsigned long uint32_t;

#if !defined(O_BINARY)
# define O_BINARY 0
#endif

/**********************************************************************
 *
 * The following are support routines for inflate.c
 *
 **********************************************************************/

static uint32_t crc_32_tab[256];

/*
 * Code to compute the CRC-32 table. Borrowed from 
 * gzip-1.0.3/makecrc.c.
 */

static void
makecrc(void)
{
/* Not copyrighted 1990 Mark Adler	*/

  unsigned long c;      /* crc shift register */
  unsigned long e;      /* polynomial exclusive-or pattern */
  int i;                /* counter for all possible eight bit values */
  int k;                /* byte being shifted into crc apparatus */

  /* terms of polynomial defining this crc (except x^32): */
  static const int p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

  /* Make exclusive-or pattern from polynomial */
  e = 0;
  for (i = 0; i < sizeof(p)/sizeof(int); i++)
    e |= 1L << (31 - p[i]);

  crc_32_tab[0] = 0;

  for (i = 1; i < 256; i++)
  {
    c = 0;
    for (k = i | 256; k != 1; k >>= 1)
    {
      c = c & 1 ? (c >> 1) ^ e : c >> 1;
      if (k & 1)
        c ^= e;
    }
    crc_32_tab[i] = c;
  }
}

unsigned long comp_crc(unsigned char *p, unsigned long len)
{
	unsigned long crc = 0xFFFFFFFFUL;

	while (len--) {
		crc = crc_32_tab[(crc ^ *p++) & 0xff] ^ (crc >> 8);
	}
	return crc ^ 0xFFFFFFFFUL;
}

typedef struct {
	const char magic[10];
	const char *description;
	size_t pfs_size;
	size_t soho_size;
} device_t;

#define KiB 1024
#define MiB (KiB * KiB)

/* Please note:
 * Not all entries in the following device table could be verified.
 * Unknown values are marked with BRN??? or 0 * KiB.
 */

static const device_t device[] = {
	// 3COM 11g?
	// BUFFALO BBR-4HG
	// A123456789 = AR4502GW = 154BAS???
	// http://hri.sourceforge.net/hw/smc7004abr/
	{"BRNABR", "SMC7004ABR V2", 576 * KiB, 192 * KiB},
	// http://hri.sourceforge.net/hw/smc7004vbr/
	{"BRN???", "SMC7004VBR", 0 * KiB, 0 * KiB},	
	// http://hri.sourceforge.net/SMC7004/
	{"BRNAW", "SMC7004VWBR", 0x30000, 0xbb800},
	// http://hri.sourceforge.net/hw/index.html
	{"BRN???", "SMC7004FW", 0 * KiB, 0 * KiB},	
	{"BRN???", "SMC7004WFW", 0 * KiB, 0 * KiB},	
	// http://hri.sourceforge.net/hw/smc2804wbr/
	{"BRN2804W", "SMC2804WBR V1", 0 * KiB, 0 * KiB},	
	// http://hri.sourceforge.net/hw/northq9100/
	{"BRN6104V2", "NorthQ9100", 0x30000, 0x90000},
	//{"BRN154BAS", "Sinus 154 DSL Basic SE", 0x30000, 0xbb800},
	{"BRN154BAS", "Sinus 154 DSL Basic SE", 832 * KiB, 896 * KiB},
	{"BRNDTBAS3", "Sinus 154 DSL Basic 3", 832 * KiB, 896 * KiB},
	{"BRN154DSL", "Sinus 154 DSL", 0 * KiB, 0 * KiB},
	{"BRN154KOM", "Sinus 154 DSL Komfort", 0 * KiB, 0 * KiB},
	{"", 0}
};

/* buffer must be large enough to contain pfs + soho + signature */
static unsigned char buffer[4 * MiB];

static char signature[10];

static const char *program_name;

static void usage(void)
{
	const device_t *dev;
	fprintf(stderr, "Usage: %s [-h|-?]\n", program_name);
	fprintf(stderr, "  or:  %s [-o <outfile>] [-l <blocksize>] -m <magic> <zipfile>\n", program_name);
	fprintf(stderr, "  or:  %s [-o <outfile>] [-l <blocksize>] -m <magic> <pfsfile> <sohofile>\n", program_name);
	fprintf(stderr, "<magic>:     any of the following\n");
	for (dev = device; ; dev++) {
		const char *magic = dev->magic;
		const char *description = dev->description;
		if (*magic == 0) break;
		fprintf(stderr, "\t%-10s%s\n", magic, description);
	}
	fprintf(stderr, "<zipfile>:   zipped file with code or user interface\n");
	fprintf(stderr, "<pfsfile>:   zipped file with user interface (pfs.img)\n");
	fprintf(stderr, "<sohofile>:  zipped file with code (soho.bin)\n");
	fprintf(stderr, "<outfile>:   write result into this file\n");
	fprintf(stderr, "<blocksize>: size of flash blocks (default: 65536)\n");
}

static unsigned char *write_data(const char *filename, unsigned char *buffer, unsigned long max_size)
{
	unsigned long *p;
	unsigned long crc;
	unsigned long len;
	int fd = open(filename, O_RDONLY|O_BINARY);
	if (fd < 0) {
		perror(filename);
		exit(1);
	}
	len = read(fd, buffer, max_size);
	close(fd);
	fprintf(stderr, "%s has %lu (0x%lx) bytes, %lu bytes left\n", filename, len, len, max_size - len);
	crc = comp_crc(buffer, len);
	max_size = ((len + 0x7fff) & 0xffff8000);
	fprintf(stderr, "%s uses %lu (0x%lx) bytes = %lu KiB, %lu bytes left\n",
			filename, max_size, max_size, max_size / 1024, max_size - len);
	if (strncmp((char *)buffer, "PK", 2) != 0) {
		fprintf(stderr, "%s is no zip file\n", filename);
	}
	p = (unsigned long *)(buffer + max_size - 3 * 4);
	*p++ = len;
	*p++ = 0x12345678;
	*p++ = crc;
	return (unsigned char *)p;
}

int main(int argc, char *argv[])
{
	unsigned char *p;
	int fd;
	
	const char *magic = 0;
	const char *pfs_file;
	const char *soho_file;
	const char *output_file = 0;

	const device_t *dev;
	size_t pfs_size;
	size_t soho_size;
	size_t total_size;
	size_t block_size = 65536;

	program_name = argv[0];

	for (;;) {
		int option = getopt(argc, argv, "?hmo");
		if (option == -1) {
			break;
		} else if (option == '?' || option == 'h') {
			usage();
			exit(0);
		} else if (option == 'l') {
			block_size = strtoul(argv[optind++], 0, 0);
		} else if (option == 'm') {
			magic = argv[optind++];
		} else if (option == 'o') {
			output_file = argv[optind++];
		} else {
			usage();
			exit(1);
		}
	}

	if (((optind + 1 != argc) && (optind + 2 != argc)) || magic == 0) {
		usage();
		exit(1);
	}

	pfs_file = argv[optind++];
	soho_file = argv[optind++];

	for (dev = device; ; dev++) {
		if (*dev->magic == 0) {
			usage();
			exit(2);
		}
		if (strcmp(magic, dev->magic) == 0) break;
	}

	pfs_size = dev->pfs_size;
	soho_size = dev->soho_size;
	total_size = pfs_size + soho_size + sizeof(signature);
	strncpy(signature, magic, 10);

	if (total_size > sizeof(buffer)) {
		fprintf(stderr, "buffer too small, needs %lu bytes, has %lu bytes\n",
			(unsigned long)total_size, (unsigned long)sizeof(buffer));
		exit(3);
	}

	makecrc();
	memset(buffer, 0xff, total_size);

	p = buffer;
	p = write_data(pfs_file, p, pfs_size);

	if (soho_file != 0) {
		p = write_data(soho_file, p, soho_size);
	}

	memcpy(p, signature, 10);

	total_size = (p - buffer) + sizeof(signature);
	
	if (output_file != 0) {
		fd = open(output_file, O_CREAT|O_WRONLY|O_TRUNC|O_BINARY, 0666);
		if (fd < 0) {
			perror(pfs_file);
			exit(1);
		}
		write(fd, buffer, total_size);
		close(fd);
	} else if (!isatty(1)) {
		write(1, buffer, total_size);
	}

	return 0;
}
