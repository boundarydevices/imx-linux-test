/*
 * Copyright 2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/* Some prototypes for extern functions. */

#include <linux/types.h>	/* for __u16 */

#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)	/* if not using GCC, turn off the __attribute__
				   compiler-advice feature. */
#endif

/* identify() is the only extern function used across two source files.  The
   others, though, were declared in hdparm.c with global scope; since other
   functions in that file have static (file) scope, I assume the difference is
   intentional. */
extern void identify (__u16 *id_supplied, const char *devname);

extern void usage_error(int out)    __attribute__((noreturn));
extern int main(int argc, char **argv) __attribute__((noreturn));
extern void flush_buffer_cache (int fd);
extern int read_big_block (int fd, char *buf);
extern int write_big_block (int fd, char *buf);
extern void time_device (int fd);
extern void no_scsi (void);
extern void no_xt (void);
extern void process_dev (char *devname);

