/*
 * Copyright 2006-2009 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <endian.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <linux/types.h>
#include <linux/hdreg.h>
#include <linux/major.h>
#include <asm/byteorder.h>

#include "mxc_ata_test.h"

extern const char *minor_str[];

#define VERSION "v6.6"

#undef DO_FLUSHCACHE		/* under construction: force cache flush on -W0 */

#ifndef O_DIRECT
#define O_DIRECT	0200000	/* direct disk access, not easily obtained from headers */
#endif

#ifndef BLKGETSIZE64
#define BLKGETSIZE64		_IOR(0x12,114,size_t)
#endif

#define TIMING_BUF_MB		2
#define TIMING_BUF_BYTES	(TIMING_BUF_MB * 1024 * 1024)
#define BUFCACHE_FACTOR		2

char *progname;
static int verbose = 0, get_identity = 0, get_geom = 0;
static int flagcount = 0, do_flush = 0;
static int do_writing = 0, do_timings = 0;

#ifdef HDIO_SET_DMA
static unsigned long set_dma      = 0, get_dma      = 0, dma      = 0;
#endif
static unsigned long set_io32bit  = 0, get_io32bit  = 0, io32bit  = 0;
#ifdef HDIO_DRIVE_CMD
static unsigned long set_xfermode = 0, get_xfermode = 0;
static int xfermode_requested= 0;
#endif
static int get_IDentity = 0;

#ifdef HDIO_DRIVE_RESET
static int	perform_reset = 0;
#endif /* HDIO_DRIVE_RESET */

static int open_flags = O_RDWR|O_NONBLOCK;

// Historically, if there was no HDIO_OBSOLETE_IDENTITY, then
// then the HDIO_GET_IDENTITY only returned 142 bytes.
// Otherwise, HDIO_OBSOLETE_IDENTITY returns 142 bytes,
// and HDIO_GET_IDENTITY returns 512 bytes.  But the latest
// 2.5.xx kernels no longer define HDIO_OBSOLETE_IDENTITY
// (which they should, but they should just return -EINVAL).
//
// So.. we must now assume that HDIO_GET_IDENTITY returns 512 bytes.
// On a really old system, it will not, and we will be confused.
// Too bad, really.

const char *cfg_str[] =
{	"",	        " HardSect",   " SoftSect",  " NotMFM",
	" HdSw>15uSec", " SpinMotCtl", " Fixed",     " Removeable",
	" DTR<=5Mbs",   " DTR>5Mbs",   " DTR>10Mbs", " RotSpdTol>.5%",
	" dStbOff",     " TrkOff",     " FmtGapReq", " nonMagnetic"
};

const char *SlowMedFast[]	= {"slow", "medium", "fast", "eide", "ata"};
const char *BuffType[]	= {"unknown", "1Sect", "DualPort", "DualPortCache"};

#define YN(b)	(((b)==0)?"no":"yes")

static void dmpstr (const char *prefix, unsigned int i, const char *s[], unsigned int maxi)
{
	if (i > maxi)
		printf("%s%u", prefix, i);
	else
		printf("%s%s", prefix, s[i]);
}

static void dump_identity (const struct hd_driveid *id)
{
	int i;
	char pmodes[64] = {0,}, dmodes[128]={0,}, umodes[128]={0,};
	const unsigned short int *id_regs= (const void*) id;
	unsigned long capacity;

	printf("\n Model=%.40s, FwRev=%.8s, SerialNo=%.20s",
		id->model, id->fw_rev, id->serial_no);
	printf("\n Config={");
	for (i=0; i<=15; i++) {
		if (id->config & (1<<i))
			printf("%s", cfg_str[i]);
	}
	printf(" }\n");
	printf(" RawCHS=%u/%u/%u, TrkSize=%u, SectSize=%u, ECCbytes=%u\n",
		id->cyls, id->heads, id->sectors,
		id->track_bytes, id->sector_bytes, id->ecc_bytes);
	dmpstr(" BuffType=",id->buf_type,BuffType,3);
	printf(", BuffSize=%ukB, MaxMultSect=%u", id->buf_size/2, id->max_multsect);
	if (id->max_multsect) {
		printf(", MultSect=");
		if (!(id->multsect_valid&1))
			printf("?%u?", id->multsect);
		else if (id->multsect)
			printf("%u", id->multsect);
		else
			printf("off");
	}
	putchar('\n');
	if (id->tPIO <= 5) {
		strcat(pmodes, "pio0 ");
		if (id->tPIO >= 1) strcat(pmodes, "pio1 ");
		if (id->tPIO >= 2) strcat(pmodes, "pio2 ");
	}
	if (!(id->field_valid&1))
		printf(" (maybe):");
	capacity = (id->cur_capacity1 << 16) | id->cur_capacity0;
	printf(" CurCHS=%u/%u/%u, CurSects=%lu", id->cur_cyls, id->cur_heads, id->cur_sectors, capacity);
	printf(", LBA=%s", YN(id->capability&2));
	if (id->capability&2)
 		printf(", LBAsects=%u", id->lba_capacity);

	if (id->capability&1) {
		if (id->dma_1word | id->dma_mword) {
			if (id->dma_1word & 0x100)	strcat(dmodes,"*");
			if (id->dma_1word & 1)		strcat(dmodes,"sdma0 ");
			if (id->dma_1word & 0x200)	strcat(dmodes,"*");
			if (id->dma_1word & 2)		strcat(dmodes,"sdma1 ");
			if (id->dma_1word & 0x400)	strcat(dmodes,"*");
			if (id->dma_1word & 4)		strcat(dmodes,"sdma2 ");
			if (id->dma_1word & 0xf800)	strcat(dmodes,"*");
			if (id->dma_1word & 0xf8)	strcat(dmodes,"sdma? ");
			if (id->dma_mword & 0x100)	strcat(dmodes,"*");
			if (id->dma_mword & 1)		strcat(dmodes,"mdma0 ");
			if (id->dma_mword & 0x200)	strcat(dmodes,"*");
			if (id->dma_mword & 2)		strcat(dmodes,"mdma1 ");
			if (id->dma_mword & 0x400)	strcat(dmodes,"*");
			if (id->dma_mword & 4)		strcat(dmodes,"mdma2 ");
			if (id->dma_mword & 0xf800)	strcat(dmodes,"*");
			if (id->dma_mword & 0xf8)	strcat(dmodes,"mdma? ");
		}
	}
	printf("\n IORDY=");
	if (id->capability&8)
		printf((id->capability&4) ? "on/off" : "yes");
	else
		printf("no");
	if ((id->capability&8) || (id->field_valid&2)) {
		if (id->field_valid&2) {
			printf(", tPIO={min:%u,w/IORDY:%u}", id->eide_pio, id->eide_pio_iordy);
			if (id->eide_pio_modes & 1)	strcat(pmodes, "pio3 ");
			if (id->eide_pio_modes & 2)	strcat(pmodes, "pio4 ");
			if (id->eide_pio_modes &~3)	strcat(pmodes, "pio? ");
		}
		if (id->field_valid&4) {
			if (id->dma_ultra & 0x100)	strcat(umodes,"*");
			if (id->dma_ultra & 0x001)	strcat(umodes,"udma0 ");
			if (id->dma_ultra & 0x200)	strcat(umodes,"*");
			if (id->dma_ultra & 0x002)	strcat(umodes,"udma1 ");
			if (id->dma_ultra & 0x400)	strcat(umodes,"*");
			if (id->dma_ultra & 0x004)	strcat(umodes,"udma2 ");
#ifdef __NEW_HD_DRIVE_ID
			if (id->hw_config & 0x2000) {
#else /* !__NEW_HD_DRIVE_ID */
			if (id->word93 & 0x2000) {
#endif /* __NEW_HD_DRIVE_ID */
				if (id->dma_ultra & 0x0800)	strcat(umodes,"*");
				if (id->dma_ultra & 0x0008)	strcat(umodes,"udma3 ");
				if (id->dma_ultra & 0x1000)	strcat(umodes,"*");
				if (id->dma_ultra & 0x0010)	strcat(umodes,"udma4 ");
				if (id->dma_ultra & 0x2000)	strcat(umodes,"*");
				if (id->dma_ultra & 0x0020)	strcat(umodes,"udma5 ");
				if (id->dma_ultra & 0x4000)	strcat(umodes,"*");
				if (id->dma_ultra & 0x0040)	strcat(umodes,"udma6 ");
				if (id->dma_ultra & 0x8000)	strcat(umodes,"*");
				if (id->dma_ultra & 0x0080)	strcat(umodes,"udma7 ");
			}
		}
	}
	if ((id->capability&1) && (id->field_valid&2))
		printf(", tDMA={min:%u,rec:%u}", id->eide_dma_min, id->eide_dma_time);
	printf("\n PIO modes:  %s", pmodes);
	if (*dmodes)
		printf("\n DMA modes:  %s", dmodes);
	if (*umodes)
		printf("\n UDMA modes: %s", umodes);

	printf("\n AdvancedPM=%s",YN(id_regs[83]&8));
	if (id_regs[83] & 8) {
		if (!(id_regs[86]&8))
			printf(": disabled (255)");
		else if ((id_regs[91]&0xFF00)!=0x4000)
			printf(": unknown setting");
		else
			printf(": mode=0x%02X (%u)",id_regs[91]&0xFF,id_regs[91]&0xFF);
	}
	if (id_regs[82]&0x20)
		printf(" WriteCache=%s",(id_regs[85]&0x20) ? "enabled" : "disabled");
#ifdef __NEW_HD_DRIVE_ID
	if (id->minor_rev_num || id->major_rev_num) {
		printf("\n Drive conforms to: ");
		if (id->minor_rev_num <= 31)
			printf("%s: ", minor_str[id->minor_rev_num]);
		else
			printf("unknown: ");
		if (id->major_rev_num != 0x0000 &&  /* NOVAL_0 */
		    id->major_rev_num != 0xFFFF) {  /* NOVAL_1 */
			/* through ATA/ATAPI-7 is currently defined--
			 * increase this value as further specs are
			 * standardized (though we can guess safely to 15)
			 */
			for (i=0; i <= 7; i++) {
				if (id->major_rev_num & (1<<i))
					printf(" ATA/ATAPI-%u", i);
			}
		}
	}
#endif /* __NEW_HD_DRIVE_ID */
	printf("\n");
	printf("\n * signifies the current active mode\n");
	printf("\n");
}

void flush_buffer_cache (int fd)
{
	fsync (fd);				/* flush buffers */
	if (ioctl(fd, BLKFLSBUF, NULL))		/* do it again, big time */
		perror("BLKFLSBUF failed");
#ifdef HDIO_DRIVE_CMD
	if (ioctl(fd, HDIO_DRIVE_CMD, NULL) && errno != EINVAL)	/* await completion */
		perror("HDIO_DRIVE_CMD(null) (wait for flush complete) failed");
#endif
}

int read_big_block (int fd, char *buf)
{
	int i, rc;
	if ((rc = read(fd, buf, TIMING_BUF_BYTES)) != TIMING_BUF_BYTES) {
		if (rc) {
			if (rc == -1)
				perror("read() failed");
			else
				fprintf(stderr, "read(%u) returned %u bytes\n", TIMING_BUF_BYTES, rc);
		} else {
			fputs ("read() hit EOF - device too small\n", stderr);
		}
		return 1;
	}
	/* access all sectors of buf to ensure the read fully completed */
	for (i = 0; i < TIMING_BUF_BYTES; i += 512)
		buf[i] &= 1;
	return 0;
}

int write_big_block (int fd, char *buf)
{
	int i, rc;
	if ((rc = write(fd, buf, TIMING_BUF_BYTES)) != TIMING_BUF_BYTES) {
		if (rc) {
			if (rc == -1)
				perror("read() failed");
			else
				fprintf(stderr, "read(%u) returned %u bytes\n", TIMING_BUF_BYTES, rc);
		} else {
			fputs ("read() hit EOF - device too small\n", stderr);
		}
		return 1;
	}
	/* access all sectors of buf to ensure the read fully completed */
	for (i = 0; i < TIMING_BUF_BYTES; i += 512)
		buf[i] &= 1;
	return 0;
}

static int do_blkgetsize (int fd, unsigned long long *blksize64)
{
	int		rc;
	unsigned int	blksize32 = 0;

	if (0 == ioctl(fd, BLKGETSIZE64, blksize64)) {	// returns bytes
		*blksize64 /= 512;
		return 0;
	}
	rc = ioctl(fd, BLKGETSIZE, &blksize32);	// returns sectors
	if (rc)
		perror(" BLKGETSIZE failed");
	*blksize64 = blksize32;
	return rc;
}

void time_device (int fd)
{
	char *buf;
	double elapsed;
	struct itimerval e1, e2;
	int shmid;
	unsigned int max_iterations = 1024, total_MB, iterations;

	//
	// get device size
	//
	if (do_timings) {
		unsigned long long blksize;
		if (0 == do_blkgetsize(fd, &blksize))
			max_iterations = blksize / (2 * 1024) / TIMING_BUF_MB;
	}

	if ((shmid = shmget(IPC_PRIVATE, TIMING_BUF_BYTES, 0600)) == -1) {
		perror ("could not allocate sharedmem buf");
		return;
	}
	if (shmctl(shmid, SHM_LOCK, NULL) == -1) {
		perror ("could not lock sharedmem buf");
		(void) shmctl(shmid, IPC_RMID, NULL);
		return;
	}
	if ((buf = shmat(shmid, (char *) 0, 0)) == (char *) -1) {
		perror ("could not attach sharedmem buf");
		(void) shmctl(shmid, IPC_RMID, NULL);
		return;
	}
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
		perror ("shmctl(,IPC_RMID,) failed");

	/* Clear out the device request queues & give them time to complete */
	sync();
	sleep(3);

	printf(" Timing %s disk %s:  ", (open_flags & O_DIRECT) ? "O_DIRECT" : "buffered", do_writing?"writes":"reads");
	fflush(stdout);

	/*
	 * getitimer() is used rather than gettimeofday() because
	 * it is much more consistent (on my machine, at least).
	 */
	setitimer(ITIMER_REAL, &(struct itimerval){{1000,0},{1000,0}}, NULL);

	/* Now do the timings for real */
	iterations = 0;
	getitimer(ITIMER_REAL, &e1);
	do {
		++iterations;
		if (do_writing?write_big_block (fd, buf):read_big_block(fd, buf))
			goto quit;
		getitimer(ITIMER_REAL, &e2);
		elapsed = (e1.it_value.tv_sec - e2.it_value.tv_sec)
		 + ((e1.it_value.tv_usec - e2.it_value.tv_usec) / 1000000.0);
	} while (elapsed < 3.0 && iterations < max_iterations);

	total_MB = iterations * TIMING_BUF_MB;
	if ((total_MB / elapsed) > 1.0)  /* more than 1MB/s */
		printf("%3u MB in %5.2f seconds = %6.2f MB/sec\n",
			total_MB, elapsed, total_MB / elapsed);
	else
		printf("%3u MB in %5.2f seconds = %6.2f kB/sec\n",
			total_MB, elapsed, total_MB / elapsed * 1024);
quit:
	if (-1 == shmdt(buf))
		perror ("could not detach sharedmem buf");
}

static void on_off (unsigned int value)
{
	printf(value ? " (on)\n" : " (off)\n");
}

#ifdef HDIO_DRIVE_CMD
struct xfermode_entry {
	int val;
	const char *name;
};

static const struct xfermode_entry xfermode_table[] = {
	{ 8,    "pio0" },
	{ 9,    "pio1" },
	{ 10,   "pio2" },
	{ 11,   "pio3" },
	{ 12,   "pio4" },
	{ 13,   "pio5" },
	{ 14,   "pio6" },
	{ 15,   "pio7" },
	{ 16,   "sdma0" },
	{ 17,   "sdma1" },
	{ 18,   "sdma2" },
	{ 19,   "sdma3" },
	{ 20,   "sdma4" },
	{ 21,   "sdma5" },
	{ 22,   "sdma6" },
	{ 23,   "sdma7" },
	{ 32,   "mdma0" },
	{ 33,   "mdma1" },
	{ 34,   "mdma2" },
	{ 35,   "mdma3" },
	{ 36,   "mdma4" },
	{ 37,   "mdma5" },
	{ 38,   "mdma6" },
	{ 39,   "mdma7" },
	{ 64,   "udma0" },
	{ 65,   "udma1" },
	{ 66,   "udma2" },
	{ 67,   "udma3" },
	{ 68,   "udma4" },
	{ 69,   "udma5" },
	{ 70,   "udma6" },
	{ 71,   "udma7" },
	{ 0, NULL }
};

static int translate_xfermode(char * name)
{
	const struct xfermode_entry *tmp;
	char *endptr;
	int val = -1;

	for (tmp = xfermode_table; tmp->name != NULL; ++tmp) {
		if (!strcmp(name, tmp->name))
			return tmp->val;
	}
	val = strtol(name, &endptr, 10);
	if (*endptr == '\0')
		return val;
	return -1;
}

static void interpret_xfermode (unsigned int xfermode)
{
	printf(" (");
	switch(xfermode) {
		case 0:		printf("default PIO mode");
				break;
		case 1:		printf("default PIO mode, disable IORDY");
				break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:	printf("PIO flow control mode%u", xfermode-8);
				break;
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:	printf("singleword DMA mode%u", xfermode-16);
				break;
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:	printf("multiword DMA mode%u", xfermode-32);
				break;
		case 64:
		case 65:
		case 66:
		case 67:
		case 68:
		case 69:
		case 70:
		case 71:	printf("UltraDMA mode%u", xfermode-64);
				break;
		default:
				printf("unknown, probably not valid");
				break;
	}
	printf(")\n");
}
#endif /* HDIO_DRIVE_CMD */

#ifndef VXVM_MAJOR
#define VXVM_MAJOR 199
#endif

#ifndef CCISS_MAJOR
#define CCISS_MAJOR 104
#endif

void process_dev (char *devname)
{
	int fd;
	static long parm, multcount;

	fd = open (devname, open_flags);
	if (fd < 0) {
		perror(devname);
		exit(errno);
	}
	if (set_io32bit) {
		if (get_io32bit)
			printf(" setting 32-bit IO_support flag to %ld\n", io32bit);
		if (ioctl(fd, HDIO_SET_32BIT, io32bit))
			perror(" HDIO_SET_32BIT failed");
	}
#ifdef HDIO_SET_DMA
	if (set_dma) {
		if (get_dma) {
			printf(" setting using_dma to %ld", dma);
			on_off(dma);
		}
		if (ioctl(fd, HDIO_SET_DMA, dma))
			perror(" HDIO_SET_DMA failed");
	}
#endif
	if (set_xfermode) {
		unsigned char args[4] = {WIN_SETFEATURES,0,3,0};
		args[1] = xfermode_requested;
		if (get_xfermode) {
			printf(" setting xfermode to %d", xfermode_requested);
			interpret_xfermode(xfermode_requested);
		}
		if (ioctl(fd, HDIO_DRIVE_CMD, &args))
			perror(" HDIO_DRIVE_CMD(setxfermode) failed");
	}
	if (verbose || get_io32bit) {
		if (0 == ioctl(fd, HDIO_GET_32BIT, &parm)) {
			printf(" IO_support   =%3ld (", parm);
			switch (parm) {
				case 0:	printf("default ");
				case 2: printf("16-bit)\n");
					break;
				case 1:	printf("32-bit)\n");
					break;
				case 3:	printf("32-bit w/sync)\n");
					break;
				case 8:	printf("Request-Queue-Bypass)\n");
					break;
				default:printf("\?\?\?)\n");
			}
		}
	}
	if (verbose || get_geom) {
		unsigned long long blksize;
		static const char msg[] = " geometry     = %u/%u/%u, sectors = %lld, start = %ld\n";
// Note to self:  when compiled 32-bit (AMD,Mips64), the userspace version of this struct
// is going to be 32-bits smaller than the kernel representation.. random stack corruption!
		static struct hd_geometry g;
#ifdef HDIO_GETGEO_BIG
		static struct hd_big_geometry bg;
#endif

		if (0 == do_blkgetsize(fd, &blksize)) {
#ifdef HDIO_GETGEO_BIG
			if (!ioctl(fd, HDIO_GETGEO_BIG, &bg))
				printf(msg, bg.cylinders, bg.heads, bg.sectors, blksize, bg.start);
			else
#endif
			if (ioctl(fd, HDIO_GETGEO, &g))
				perror(" HDIO_GETGEO failed");
			else
				printf(msg, g.cylinders, g.heads, g.sectors, blksize, g.start);
		}
	}
#ifdef HDIO_DRIVE_RESET
	if (perform_reset) {
		if (ioctl(fd, HDIO_DRIVE_RESET, NULL))
			perror(" HDIO_DRIVE_RESET failed");
	}
#endif /* HDIO_DRIVE_RESET */
	if (get_identity) {
		static struct hd_driveid id;

		if (!ioctl(fd, HDIO_GET_IDENTITY, &id)) {
			if (multcount != -1) {
				id.multsect = multcount;
				id.multsect_valid |= 1;
			} else
				id.multsect_valid &= ~1;
			dump_identity(&id);
		} else if (errno == -ENOMSG)
			printf(" no identification info available\n");
		else
			perror(" HDIO_GET_IDENTITY failed");
	}
	if (get_IDentity) {
		__u16 *id;
		unsigned char args[4+512] = {WIN_IDENTIFY,0,0,1,}; // FIXME?
		unsigned i;
		if (ioctl(fd, HDIO_DRIVE_CMD, &args)) {
			args[0] = WIN_PIDENTIFY;
			if (ioctl(fd, HDIO_DRIVE_CMD, &args)) {
				perror(" HDIO_DRIVE_CMD(identify) failed");
				goto identify_abort;
			}
		}
		id = (__u16 *)&args[4];
		if (get_IDentity == 2) {
			for (i = 0; i < (256/8); ++i) {
				printf("%04x %04x %04x %04x %04x %04x %04x %04x\n", id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
				id += 8;
			}
		} else {
			for(i = 0; i < 0x100; ++i) {
				__le16_to_cpus(&id[i]);
			}
			identify((void *)id, NULL);
		}
identify_abort:	;
	}

	if (do_timings)
		time_device (fd);
	if (do_flush)
		flush_buffer_cache (fd);
	close (fd);
}

void usage_error (int out)
{
	FILE *desc;
	int ret;

	if (out == 0) {
		desc = stdout;
		ret = 0;
	} else {
		desc = stderr;
		ret = 1;
	}

	fprintf(desc,"\n%s - get/set hard disk parameters - version %s\n\n", progname, VERSION);
	fprintf(desc,"Usage:  %s  [options] [device] ..\n\n", progname);
	fprintf(desc,"Options:\n"
	" -c   get/set IDE 32-bit IO setting\n"
#ifdef HDIO_SET_DMA
	" -d   get/set using_dma flag\n"
#endif
	" -f   flush buffer cache for device on exit\n"
	" -g   display drive geometry\n"
	" -h   display terse usage information\n"
	" -i   display drive identification\n"
	" -I   detailed/current information directly from drive\n"
	" -t   perform device read timings\n"
	" -v   defaults; same as -mcudkrag for IDE drives\n"
	" -V   display program version and exit immediately\n"
#ifdef HDIO_DRIVE_CMD
#ifdef HDIO_DRIVE_RESET
	" -w   perform device reset (DANGEROUS)\n"
#endif
	" -X   set IDE xfer mode (DANGEROUS)\n"
#endif
	"\n");
	exit(ret);
}

#define GET_NUMBER(flag,num)	num = 0; \
				if (!*p && argc && isdigit(**argv)) \
					p = *argv++, --argc; \
				while (isdigit(*p)) { \
					flag = 1; \
					num = (num * 10) + (*p++ - '0'); \
				}

#define GET_RAW_NUMBER(num)	num = 0; \
				if (!*p && argc && isdigit(**argv)) \
					p = *argv++, --argc; \
				while (isdigit(*p)) { \
					num = (num * 10) + (*p++ - '0'); \
				}

#define GET_STRING(flag, num) tmpstr = name; \
				tmpstr[0] = '\0'; \
				if (!*p && argc && isalnum(**argv)) \
					p = *argv++, --argc; \
				while (isalnum(*p) && (tmpstr - name) < 31) { \
					tmpstr[0] = *p++; \
					tmpstr[1] = '\0'; \
					++tmpstr; \
				} \
				num = translate_xfermode(name); \
				if (num == -1) \
					flag = 0; \
				else \
					flag = 1;

#define GET_ASCII_PASSWORD(flag, pwd) tmpstr = pwd; \
				memset(&pwd,0,sizeof(pwd)); \
				if (!*p && argc && isgraph(**argv)) \
					p = *argv++, --argc; \
				while ((tmpstr - pwd) < 32) { \
					if (!isgraph(*p)) { \
						if (*p > 0) { \
							flag = 0; \
							printf("Abort: Password contains non-printable characters!"); \
						} else flag = 1; \
						break; \
					} else {\
						sscanf(p,"%c",(unsigned char *) &tmpstr[0]); \
						p = p+1; \
						++tmpstr; \
					} \
				}
#if 0
static int fromhex (unsigned char c)
{
	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	if (c >= '0' && c <= '9')
		return (c - '0');
	fprintf(stderr, "bad char: '%c' 0x%02x\n", c, c);
	exit(-1);
}
#endif
int main(int argc, char **argv)
{
	char c, *p ;
	char *tmpstr;
	char name[32];

	if  ((progname = (char *) strrchr(*argv, '/')) == NULL)
		progname = *argv;
	else
		progname++;
	++argv;

	if (!--argc)
		usage_error(1);

	open_flags |= O_DIRECT;

	while (argc--) {
		p = *argv++;
		if (*p == '-') {

			if (!*++p) usage_error(1);

			while ((c = *p++)) {
				++flagcount;
				switch (c) {
					case 'V':
						fprintf(stdout, "%s %s\n", progname, VERSION);
						exit(0);
						break;
					case 'v':
						verbose = 1;
						break;
					case 'I':
						get_IDentity = 1;
						break;
					case 'i':
						get_identity = 1;
						break;
					case 'g':
						get_geom = 1;
						break;
					case 'f':
						do_flush = 1;
						break;
#ifdef HDIO_SET_DMA
					case 'd':
						get_dma = 1;
						if (!*p && argc && isdigit(**argv))
							p = *argv++, --argc;
						if (*p >= '0' && *p <= '9') {
							set_dma = 1;
							dma = *p++ - '0';
						}
						break;
#endif
					case 'c':
						get_io32bit = 1;
						GET_NUMBER(set_io32bit,io32bit);
						break;
#ifdef HDIO_DRIVE_CMD
					case 'X':
						get_xfermode = 1;
						GET_STRING(set_xfermode,xfermode_requested);
						if (!set_xfermode)
							fprintf(stderr, "-X: missing value\n");
						break;
#endif /* HDIO_DRIVE_CMD */
#ifdef HDIO_DRIVE_RESET
					case 'w':
						perform_reset = 1;
						break;
#endif /* HDIO_DRIVE_RESET */
					case 't':
						do_timings = 1;
						do_flush = 1;
						GET_RAW_NUMBER(do_writing);
						break;
					default:
						usage_error(1);
				}
			}
			if (!argc)
				usage_error(1);
		} else {
			process_dev (p);
		}
	}
	exit (0);
}

#include "identify.c"
