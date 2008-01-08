/* 
 * Memalloc, encoder memory allocation driver (kernel module)
 *
 * Copyright (C) 2005  Hantro Products Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
    
#ifndef _MEMALLOC_H_
#define _MEMALLOC_H_
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */
/*
 * Macros to help debugging
 */
#undef PDEBUG   /* undef it, just in case */
#ifdef MEMALLOC_DEBUG
#  ifdef __KERNEL__
    /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_INFO "memalloc: " fmt, ## args)
#  else
    /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)  /* not debugging: nothing */
#endif
/*
 * Ioctl definitions
 */
/* Use 'k' as magic number */
#define MEMALLOC_IOC_MAGIC  'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define MEMALLOC_IOCXGETBUFFER		_IOWR(MEMALLOC_IOC_MAGIC,  1, unsigned long)
#define MEMALLOC_IOCSFREEBUFFER		_IOW(MEMALLOC_IOC_MAGIC,  2, unsigned long)
#define MEMALLOC_IOCHARDRESET		_IO(MEMALLOC_IOC_MAGIC, 3)
#define MEMALLOC_IOC_MAXNR 3

#endif /* _MEMALLOC_H_ */
