/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file mxc_dcic_test.c
 *
 * @brief DCIC driver test application
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/mxcfb.h>
#include <linux/mxc_dcic.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#define TFAIL -1
#define TPASS 0

#define GET_BIT(d,i)	((d >> i) & (unsigned int)0x1)
#define SET_BIT(d,i,b)	(d | ((b) << i))

static int fd_fb0 = 0;
static int fd_dcic = 0;
static unsigned char *fb0;
static int g_fb0_size;

static unsigned int g_dcic_dev = 0;
static unsigned int g_show_data = 0x33;
static unsigned int g_disp_bus_width = 24;
static unsigned int g_start_x_offset=100;
static unsigned int g_start_y_offset=100;
static unsigned int g_end_x_offset=200;
static unsigned int g_end_y_offset=200;

unsigned int crc32_calc_single_24bit(unsigned int crc_in, unsigned int data_in)
{
	unsigned int C = crc_in;
	unsigned int D = data_in;
	unsigned int crc_out = 0;

	crc_out = SET_BIT(crc_out, 0, GET_BIT(D, 16) ^ GET_BIT(D, 12) ^ GET_BIT(D, 10) ^ GET_BIT(D, 9) ^ GET_BIT(D, 6) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 14)
	        ^ GET_BIT(C, 17) ^ GET_BIT(C, 18) ^ GET_BIT(C, 20) ^ GET_BIT(C, 24));
	crc_out = SET_BIT(crc_out, 1, GET_BIT(D, 17) ^ GET_BIT(D, 16) ^ GET_BIT(D, 13) ^ GET_BIT(D, 12) ^ GET_BIT(D, 11) ^ GET_BIT(D, 9) ^ GET_BIT(D, 7) ^ GET_BIT(D, 6)
			^ GET_BIT(D, 1) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 9) ^ GET_BIT(C, 14) ^ GET_BIT(C, 15) ^ GET_BIT(C, 17) ^ GET_BIT(C, 19) ^ GET_BIT(C, 20)
			^ GET_BIT(C, 21) ^ GET_BIT(C, 24) ^ GET_BIT(C, 25));
	crc_out = SET_BIT(crc_out, 2, GET_BIT(D, 18) ^ GET_BIT(D, 17) ^ GET_BIT(D, 16) ^ GET_BIT(D, 14) ^ GET_BIT(D, 13) ^ GET_BIT(D, 9) ^ GET_BIT(D, 8) ^ GET_BIT(D, 7)
			^ GET_BIT(D, 6) ^ GET_BIT(D, 2) ^ GET_BIT(D, 1) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 9) ^ GET_BIT(C, 10) ^ GET_BIT(C, 14) ^ GET_BIT(C, 15)
			^ GET_BIT(C, 16) ^ GET_BIT(C, 17) ^ GET_BIT(C, 21) ^ GET_BIT(C, 22) ^ GET_BIT(C, 24) ^ GET_BIT(C, 25) ^ GET_BIT(C, 26));
	crc_out = SET_BIT(crc_out, 3, GET_BIT(D, 19) ^ GET_BIT(D, 18) ^ GET_BIT(D, 17) ^ GET_BIT(D, 15) ^ GET_BIT(D, 14) ^ GET_BIT(D, 10) ^ GET_BIT(D, 9) ^ GET_BIT(D, 8)
			^ GET_BIT(D, 7) ^ GET_BIT(D, 3) ^ GET_BIT(D, 2) ^ GET_BIT(D, 1) ^ GET_BIT(C, 9) ^ GET_BIT(C, 10) ^ GET_BIT(C, 11) ^ GET_BIT(C, 15) ^ GET_BIT(C, 16)
			^ GET_BIT(C, 17) ^ GET_BIT(C, 18) ^ GET_BIT(C, 22) ^ GET_BIT(C, 23) ^ GET_BIT(C, 25) ^ GET_BIT(C, 26) ^ GET_BIT(C, 27));
	crc_out = SET_BIT(crc_out, 4, GET_BIT(D, 20) ^ GET_BIT(D, 19) ^ GET_BIT(D, 18) ^ GET_BIT(D, 15) ^ GET_BIT(D, 12) ^ GET_BIT(D, 11) ^ GET_BIT(D, 8) ^ GET_BIT(D, 6)
			^ GET_BIT(D, 4) ^ GET_BIT(D, 3) ^ GET_BIT(D, 2) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 10) ^ GET_BIT(C, 11) ^ GET_BIT(C, 12) ^ GET_BIT(C, 14)
			^ GET_BIT(C, 16) ^ GET_BIT(C, 19) ^ GET_BIT(C, 20) ^ GET_BIT(C, 23) ^ GET_BIT(C, 26) ^ GET_BIT(C, 27) ^ GET_BIT(C, 28));
	crc_out = SET_BIT(crc_out, 5, GET_BIT(D, 21) ^ GET_BIT(D, 20) ^ GET_BIT(D, 19) ^ GET_BIT(D, 13) ^ GET_BIT(D, 10) ^ GET_BIT(D, 7) ^ GET_BIT(D, 6) ^ GET_BIT(D, 5)
			^ GET_BIT(D, 4) ^ GET_BIT(D, 3) ^ GET_BIT(D, 1) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 9) ^ GET_BIT(C, 11) ^ GET_BIT(C, 12) ^ GET_BIT(C, 13)
			^ GET_BIT(C, 14) ^ GET_BIT(C, 15) ^ GET_BIT(C, 18) ^ GET_BIT(C, 21) ^ GET_BIT(C, 27) ^ GET_BIT(C, 28) ^ GET_BIT(C, 29));
	crc_out = SET_BIT(crc_out, 6, GET_BIT(D, 22) ^ GET_BIT(D, 21) ^ GET_BIT(D, 20) ^ GET_BIT(D, 14) ^ GET_BIT(D, 11) ^ GET_BIT(D, 8) ^ GET_BIT(D, 7) ^ GET_BIT(D, 6)
			^ GET_BIT(D, 5) ^ GET_BIT(D, 4) ^ GET_BIT(D, 2) ^ GET_BIT(D, 1) ^ GET_BIT(C, 9) ^ GET_BIT(C, 10) ^ GET_BIT(C, 12) ^ GET_BIT(C, 13) ^ GET_BIT(C, 14)
			^ GET_BIT(C, 15) ^ GET_BIT(C, 16) ^ GET_BIT(C, 19) ^ GET_BIT(C, 22) ^ GET_BIT(C, 28) ^ GET_BIT(C, 29) ^ GET_BIT(C, 30));
	crc_out = SET_BIT(crc_out, 7, GET_BIT(D, 23) ^ GET_BIT(D, 22) ^ GET_BIT(D, 21) ^ GET_BIT(D, 16) ^ GET_BIT(D, 15) ^ GET_BIT(D, 10) ^ GET_BIT(D, 8) ^ GET_BIT(D, 7)
			^ GET_BIT(D, 5) ^ GET_BIT(D, 3) ^ GET_BIT(D, 2) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 10) ^ GET_BIT(C, 11) ^ GET_BIT(C, 13) ^ GET_BIT(C, 15)
			^ GET_BIT(C, 16) ^ GET_BIT(C, 18) ^ GET_BIT(C, 23) ^ GET_BIT(C, 24) ^ GET_BIT(C, 29) ^ GET_BIT(C, 30) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 8, GET_BIT(D, 23) ^ GET_BIT(D, 22) ^ GET_BIT(D, 17) ^ GET_BIT(D, 12) ^ GET_BIT(D, 11) ^ GET_BIT(D, 10) ^ GET_BIT(D, 8) ^ GET_BIT(D, 4)
			^ GET_BIT(D, 3) ^ GET_BIT(D, 1) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 9) ^ GET_BIT(C, 11) ^ GET_BIT(C, 12) ^ GET_BIT(C, 16) ^ GET_BIT(C, 18)
			^ GET_BIT(C, 19) ^ GET_BIT(C, 20) ^ GET_BIT(C, 25) ^ GET_BIT(C, 30) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 9, GET_BIT(D, 23) ^ GET_BIT(D, 18) ^ GET_BIT(D, 13) ^ GET_BIT(D, 12) ^ GET_BIT(D, 11) ^ GET_BIT(D, 9) ^ GET_BIT(D, 5) ^ GET_BIT(D, 4)
			^ GET_BIT(D, 2) ^ GET_BIT(D, 1) ^ GET_BIT(C, 9) ^ GET_BIT(C, 10) ^ GET_BIT(C, 12) ^ GET_BIT(C, 13) ^ GET_BIT(C, 17) ^ GET_BIT(C, 19)
			^ GET_BIT(C, 20) ^ GET_BIT(C, 21) ^ GET_BIT(C, 26) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 10, GET_BIT(D, 19) ^ GET_BIT(D, 16) ^ GET_BIT(D, 14) ^ GET_BIT(D, 13) ^ GET_BIT(D, 9) ^ GET_BIT(D, 5) ^ GET_BIT(D, 3) ^ GET_BIT(D, 2)
			^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 10) ^ GET_BIT(C, 11) ^ GET_BIT(C, 13) ^ GET_BIT(C, 17) ^ GET_BIT(C, 21) ^ GET_BIT(C, 22)
			^ GET_BIT(C, 24) ^ GET_BIT(C, 27));
	crc_out = SET_BIT(crc_out, 11, GET_BIT(D, 20) ^ GET_BIT(D, 17) ^ GET_BIT(D, 16) ^ GET_BIT(D, 15) ^ GET_BIT(D, 14) ^ GET_BIT(D, 12) ^ GET_BIT(D, 9) ^ GET_BIT(D, 4)
			^ GET_BIT(D, 3) ^ GET_BIT(D, 1) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 9) ^ GET_BIT(C, 11) ^ GET_BIT(C, 12) ^ GET_BIT(C, 17) ^ GET_BIT(C, 20)
			^ GET_BIT(C, 22) ^ GET_BIT(C, 23) ^ GET_BIT(C, 24) ^ GET_BIT(C, 25) ^ GET_BIT(C, 28));
	crc_out = SET_BIT(crc_out, 12, GET_BIT(D, 21) ^ GET_BIT(D, 18) ^ GET_BIT(D, 17) ^ GET_BIT(D, 15) ^ GET_BIT(D, 13) ^ GET_BIT(D, 12) ^ GET_BIT(D, 9) ^ GET_BIT(D, 6)
			^ GET_BIT(D, 5) ^ GET_BIT(D, 4) ^ GET_BIT(D, 2) ^ GET_BIT(D, 1) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 9) ^ GET_BIT(C, 10) ^ GET_BIT(C, 12)
			^ GET_BIT(C, 13) ^ GET_BIT(C, 14) ^ GET_BIT(C, 17) ^ GET_BIT(C, 20) ^ GET_BIT(C, 21) ^ GET_BIT(C, 23) ^ GET_BIT(C, 25) ^ GET_BIT(C, 26)
			^ GET_BIT(C, 29));
	crc_out = SET_BIT(crc_out, 13, GET_BIT(D, 22) ^ GET_BIT(D, 19) ^ GET_BIT(D, 18) ^ GET_BIT(D, 16) ^ GET_BIT(D, 14) ^ GET_BIT(D, 13) ^ GET_BIT(D, 10) ^ GET_BIT(D, 7)
			^ GET_BIT(D, 6) ^ GET_BIT(D, 5) ^ GET_BIT(D, 3) ^ GET_BIT(D, 2) ^ GET_BIT(D, 1) ^ GET_BIT(C, 9) ^ GET_BIT(C, 10) ^ GET_BIT(C, 11) ^ GET_BIT(C, 13)
			^ GET_BIT(C, 14) ^ GET_BIT(C, 15) ^ GET_BIT(C, 18) ^ GET_BIT(C, 21) ^ GET_BIT(C, 22) ^ GET_BIT(C, 24) ^ GET_BIT(C, 26) ^ GET_BIT(C, 27)
			^ GET_BIT(C, 30));
	crc_out = SET_BIT(crc_out, 14, GET_BIT(D, 23) ^ GET_BIT(D, 20) ^ GET_BIT(D, 19) ^ GET_BIT(D, 17) ^ GET_BIT(D, 15) ^ GET_BIT(D, 14) ^ GET_BIT(D, 11) ^ GET_BIT(D, 8)
			^ GET_BIT(D, 7) ^ GET_BIT(D, 6) ^ GET_BIT(D, 4) ^ GET_BIT(D, 3) ^ GET_BIT(D, 2) ^ GET_BIT(C, 10) ^ GET_BIT(C, 11) ^ GET_BIT(C, 12) ^ GET_BIT(C, 14)
			^ GET_BIT(C, 15) ^ GET_BIT(C, 16) ^ GET_BIT(C, 19) ^ GET_BIT(C, 22) ^ GET_BIT(C, 23) ^ GET_BIT(C, 25) ^ GET_BIT(C, 27) ^ GET_BIT(C, 28)
			^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 15, GET_BIT(D, 21) ^ GET_BIT(D, 20) ^ GET_BIT(D, 18) ^ GET_BIT(D, 16) ^ GET_BIT(D, 15) ^ GET_BIT(D, 12) ^ GET_BIT(D, 9) ^ GET_BIT(D, 8)
			^ GET_BIT(D, 7) ^ GET_BIT(D, 5) ^ GET_BIT(D, 4) ^ GET_BIT(D, 3) ^ GET_BIT(C, 11) ^ GET_BIT(C, 12) ^ GET_BIT(C, 13) ^ GET_BIT(C, 15) ^ GET_BIT(C, 16)
			^ GET_BIT(C, 17) ^ GET_BIT(C, 20) ^ GET_BIT(C, 23) ^ GET_BIT(C, 24) ^ GET_BIT(C, 26) ^ GET_BIT(C, 28) ^ GET_BIT(C, 29));
	crc_out = SET_BIT(crc_out, 16, GET_BIT(D, 22) ^ GET_BIT(D, 21) ^ GET_BIT(D, 19) ^ GET_BIT(D, 17) ^ GET_BIT(D, 13) ^ GET_BIT(D, 12) ^ GET_BIT(D, 8) ^ GET_BIT(D, 5)
			^ GET_BIT(D, 4) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 12) ^ GET_BIT(C, 13) ^ GET_BIT(C, 16) ^ GET_BIT(C, 20) ^ GET_BIT(C, 21)
			^ GET_BIT(C, 25) ^ GET_BIT(C, 27) ^ GET_BIT(C, 29) ^ GET_BIT(C, 30));
	crc_out = SET_BIT(crc_out, 17, GET_BIT(D, 23) ^ GET_BIT(D, 22) ^ GET_BIT(D, 20) ^ GET_BIT(D, 18) ^ GET_BIT(D, 14) ^ GET_BIT(D, 13) ^ GET_BIT(D, 9) ^ GET_BIT(D, 6)
			^ GET_BIT(D, 5) ^ GET_BIT(D, 1) ^ GET_BIT(C, 9) ^ GET_BIT(C, 13) ^ GET_BIT(C, 14) ^ GET_BIT(C, 17) ^ GET_BIT(C, 21) ^ GET_BIT(C, 22)
			^ GET_BIT(C, 26) ^ GET_BIT(C, 28) ^ GET_BIT(C, 30) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 18, GET_BIT(D, 23) ^ GET_BIT(D, 21) ^ GET_BIT(D, 19) ^ GET_BIT(D, 15) ^ GET_BIT(D, 14) ^ GET_BIT(D, 10) ^ GET_BIT(D, 7) ^ GET_BIT(D, 6)
			^ GET_BIT(D, 2) ^ GET_BIT(C, 10) ^ GET_BIT(C, 14) ^ GET_BIT(C, 15) ^ GET_BIT(C, 18) ^ GET_BIT(C, 22) ^ GET_BIT(C, 23) ^ GET_BIT(C, 27)
			^ GET_BIT(C, 29) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 19, GET_BIT(D, 22) ^ GET_BIT(D, 20) ^ GET_BIT(D, 16) ^ GET_BIT(D, 15) ^ GET_BIT(D, 11) ^ GET_BIT(D, 8) ^ GET_BIT(D, 7) ^ GET_BIT(D, 3)
			^ GET_BIT(C, 11) ^ GET_BIT(C, 15) ^ GET_BIT(C, 16) ^ GET_BIT(C, 19) ^ GET_BIT(C, 23) ^ GET_BIT(C, 24) ^ GET_BIT(C, 28) ^ GET_BIT(C, 30));
	crc_out = SET_BIT(crc_out, 20, GET_BIT(D, 23) ^ GET_BIT(D, 21) ^ GET_BIT(D, 17) ^ GET_BIT(D, 16) ^ GET_BIT(D, 12) ^ GET_BIT(D, 9) ^ GET_BIT(D, 8) ^ GET_BIT(D, 4)
			^ GET_BIT(C, 12) ^ GET_BIT(C, 16) ^ GET_BIT(C, 17) ^ GET_BIT(C, 20) ^ GET_BIT(C, 24) ^ GET_BIT(C, 25) ^ GET_BIT(C, 29) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 21, GET_BIT(D, 22) ^ GET_BIT(D, 18) ^ GET_BIT(D, 17) ^ GET_BIT(D, 13) ^ GET_BIT(D, 10) ^ GET_BIT(D, 9) ^ GET_BIT(D, 5) ^ GET_BIT(C, 13)
			^ GET_BIT(C, 17) ^ GET_BIT(C, 18) ^ GET_BIT(C, 21) ^ GET_BIT(C, 25) ^ GET_BIT(C, 26) ^ GET_BIT(C, 30));
	crc_out = SET_BIT(crc_out, 22, GET_BIT(D, 23) ^ GET_BIT(D, 19) ^ GET_BIT(D, 18) ^ GET_BIT(D, 16) ^ GET_BIT(D, 14) ^ GET_BIT(D, 12) ^ GET_BIT(D, 11) ^ GET_BIT(D, 9)
			^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 17) ^ GET_BIT(C, 19) ^ GET_BIT(C, 20) ^ GET_BIT(C, 22) ^ GET_BIT(C, 24) ^ GET_BIT(C, 26)
			^ GET_BIT(C, 27) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 23, GET_BIT(D, 20) ^ GET_BIT(D, 19) ^ GET_BIT(D, 17) ^ GET_BIT(D, 16) ^ GET_BIT(D, 15) ^ GET_BIT(D, 13) ^ GET_BIT(D, 9) ^ GET_BIT(D, 6)
			^ GET_BIT(D, 1) ^ GET_BIT(D, 0) ^ GET_BIT(C, 8) ^ GET_BIT(C, 9) ^ GET_BIT(C, 14) ^ GET_BIT(C, 17) ^ GET_BIT(C, 21) ^ GET_BIT(C, 23) ^ GET_BIT(C, 24)
			^ GET_BIT(C, 25) ^ GET_BIT(C, 27) ^ GET_BIT(C, 28));
	crc_out = SET_BIT(crc_out, 24, GET_BIT(D, 21) ^ GET_BIT(D, 20) ^ GET_BIT(D, 18) ^ GET_BIT(D, 17) ^ GET_BIT(D, 16) ^ GET_BIT(D, 14) ^ GET_BIT(D, 10) ^ GET_BIT(D, 7)
			^ GET_BIT(D, 2) ^ GET_BIT(D, 1) ^ GET_BIT(C, 0) ^ GET_BIT(C, 9) ^ GET_BIT(C, 10) ^ GET_BIT(C, 15) ^ GET_BIT(C, 18) ^ GET_BIT(C, 22) ^ GET_BIT(C, 24)
			^ GET_BIT(C, 25) ^ GET_BIT(C, 26) ^ GET_BIT(C, 28) ^ GET_BIT(C, 29));
	crc_out = SET_BIT(crc_out, 25, GET_BIT(D, 22) ^ GET_BIT(D, 21) ^ GET_BIT(D, 19) ^ GET_BIT(D, 18) ^ GET_BIT(D, 17) ^ GET_BIT(D, 15) ^ GET_BIT(D, 11) ^ GET_BIT(D, 8)
			^ GET_BIT(D, 3) ^ GET_BIT(D, 2) ^ GET_BIT(C, 1) ^ GET_BIT(C, 10) ^ GET_BIT(C, 11) ^ GET_BIT(C, 16) ^ GET_BIT(C, 19) ^ GET_BIT(C, 23)
			^ GET_BIT(C, 25) ^ GET_BIT(C, 26) ^ GET_BIT(C, 27) ^ GET_BIT(C, 29) ^ GET_BIT(C, 30));
	crc_out = SET_BIT(crc_out, 26, GET_BIT(D, 23) ^ GET_BIT(D, 22) ^ GET_BIT(D, 20) ^ GET_BIT(D, 19) ^ GET_BIT(D, 18) ^ GET_BIT(D, 10) ^ GET_BIT(D, 6) ^ GET_BIT(D, 4)
			^ GET_BIT(D, 3) ^ GET_BIT(D, 0) ^ GET_BIT(C, 2) ^ GET_BIT(C, 8) ^ GET_BIT(C, 11) ^ GET_BIT(C, 12) ^ GET_BIT(C, 14) ^ GET_BIT(C, 18) ^ GET_BIT(C, 26)
			^ GET_BIT(C, 27) ^ GET_BIT(C, 28) ^ GET_BIT(C, 30) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 27, GET_BIT(D, 23) ^ GET_BIT(D, 21) ^ GET_BIT(D, 20) ^ GET_BIT(D, 19) ^ GET_BIT(D, 11) ^ GET_BIT(D, 7) ^ GET_BIT(D, 5) ^ GET_BIT(D, 4)
			^ GET_BIT(D, 1) ^ GET_BIT(C, 3) ^ GET_BIT(C, 9) ^ GET_BIT(C, 12) ^ GET_BIT(C, 13) ^ GET_BIT(C, 15) ^ GET_BIT(C, 19) ^ GET_BIT(C, 27)
			^ GET_BIT(C, 28) ^ GET_BIT(C, 29) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 28, GET_BIT(D, 22) ^ GET_BIT(D, 21) ^ GET_BIT(D, 20) ^ GET_BIT(D, 12) ^ GET_BIT(D, 8) ^ GET_BIT(D, 6) ^ GET_BIT(D, 5) ^ GET_BIT(D, 2)
			^ GET_BIT(C, 4) ^ GET_BIT(C, 10) ^ GET_BIT(C, 13) ^ GET_BIT(C, 14) ^ GET_BIT(C, 16) ^ GET_BIT(C, 20) ^ GET_BIT(C, 28) ^ GET_BIT(C, 29)
			^ GET_BIT(C, 30));
	crc_out = SET_BIT(crc_out, 29, GET_BIT(D, 23) ^ GET_BIT(D, 22) ^ GET_BIT(D, 21) ^ GET_BIT(D, 13) ^ GET_BIT(D, 9) ^ GET_BIT(D, 7) ^ GET_BIT(D, 6) ^ GET_BIT(D, 3)
			^ GET_BIT(C, 5) ^ GET_BIT(C, 11) ^ GET_BIT(C, 14) ^ GET_BIT(C, 15) ^ GET_BIT(C, 17) ^ GET_BIT(C, 21) ^ GET_BIT(C, 29) ^ GET_BIT(C, 30)
			^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 30, GET_BIT(D, 23) ^ GET_BIT(D, 22) ^ GET_BIT(D, 14) ^ GET_BIT(D, 10) ^ GET_BIT(D, 8) ^ GET_BIT(D, 7) ^ GET_BIT(D, 4) ^ GET_BIT(C, 6)
			^ GET_BIT(C, 12) ^ GET_BIT(C, 15) ^ GET_BIT(C, 16) ^ GET_BIT(C, 18) ^ GET_BIT(C, 22) ^ GET_BIT(C, 30) ^ GET_BIT(C, 31));
	crc_out = SET_BIT(crc_out, 31, GET_BIT(D, 23) ^ GET_BIT(D, 15) ^ GET_BIT(D, 11) ^ GET_BIT(D, 9) ^ GET_BIT(D, 8) ^ GET_BIT(D, 5) ^ GET_BIT(C, 7) ^ GET_BIT(C, 13)
			^ GET_BIT(C, 16) ^ GET_BIT(C, 17) ^ GET_BIT(C, 19) ^ GET_BIT(C, 23) ^ GET_BIT(C, 31));

	return crc_out;
}

/* bpp 24, bus width 24 */
unsigned int crc32_calc_24bit(unsigned int crc_init, unsigned int *data_in, unsigned int words_count)
{
	unsigned int crc_out = 0;
	unsigned int i;

	crc_out = crc32_calc_single_24bit(crc_init, (*data_in++));
	if (words_count > 1)
		for (i = 1; i < words_count; i++)
			crc_out = crc32_calc_single_24bit(crc_out, (*data_in++));

	return crc_out;
}

/* bpp 24, bus width 18 */
unsigned int crc32_calc_18of24bit(unsigned int crc_init, unsigned int *data_in, unsigned int words_count)
{
	unsigned int crc_out = 0;
	unsigned int i;
	unsigned int data18b;

	data18b = (*data_in++);
	data18b = ((data18b & 0xFC0000) >> 6) | ((data18b & 0xFC00) >> 4) | ((data18b & 0xFC) >> 2);
	crc_out = crc32_calc_single_24bit(crc_init, data18b);

	if (words_count > 1)
		for (i = 1; i < words_count; i++)
		{
			data18b = (*data_in++);
			data18b = ((data18b & 0xFC0000) >> 6) | ((data18b & 0xFC00) >> 4) | ((data18b & 0xFC) >> 2);
			crc_out = crc32_calc_single_24bit(crc_out, data18b);
		}

	return crc_out;
}

/* bpp 16, bus width 24 */
unsigned int crc32_calc_24of16bit(unsigned int crc_init, unsigned short *data_in, unsigned int words_count)
{
	unsigned int crc_out = 0;
	unsigned int i;
	unsigned int data16b;

	data16b = (*data_in++);
	data16b = ((data16b & 0xF800) << 8) | ((data16b & 0xE000) << 3) |
				((data16b & 0x7E0) << 5) | ((data16b & 0x600) >> 1) |
				((data16b & 0x1F) << 3) | ((data16b & 0x1C) >> 2);
	crc_out = crc32_calc_single_24bit(crc_init, data16b);

	if (words_count > 1)
		for (i = 1; i < words_count; i++)
		{
			data16b = (*data_in++);
			data16b = ((data16b & 0xF800) << 8) | ((data16b & 0xE000) << 3) |
						((data16b & 0x7E0) << 5) | ((data16b & 0x600) >> 1) |
						((data16b & 0x1F) << 3) | ((data16b & 0x1C) >> 2);
			crc_out = crc32_calc_single_24bit(crc_out, data16b);
		}

	return crc_out;
}

/* bpp=16, bus width 18 */
unsigned int crc32_calc_18of16bit(unsigned int crc_init, unsigned short *data_in, unsigned int words_count)
{
	unsigned int crc_out = 0;
	unsigned int i;
	unsigned int data16b;

	data16b = (*data_in++);
	data16b = ((data16b & 0xF800) << 2) | ((data16b & 0x8000) >> 3) |
				((data16b & 0x7E0) << 1) |
				((data16b & 0x1F) << 1) | ((data16b & 0x10) >> 4);
	crc_out = crc32_calc_single_24bit(crc_init, data16b);

	if (words_count > 1)
		for (i = 1; i < words_count; i++)
		{
			data16b = (*data_in++);
			data16b = ((data16b & 0xF800) << 2) | ((data16b & 0x8000) >> 3) |
						((data16b & 0x7E0) << 1) |
						((data16b & 0x1F) << 1) | ((data16b & 0x10) >> 4);
			crc_out = crc32_calc_single_24bit(crc_out, data16b);
		}

	return crc_out;
}

void dump_sreen_info(struct fb_var_screeninfo *fb_info)
{
	printf("xres=%d\n", fb_info->xres);
	printf("yres=%d\n", fb_info->yres);
	printf("xres_virtual=%d\n", fb_info->xres_virtual);
	printf("yres_virtual=%d\n", fb_info->yres_virtual);
	printf("xoffset=%d\n", fb_info->xoffset);
	printf("yoffset=%d\n", fb_info->yoffset);
	printf("bits_per_pixel=%d\n", fb_info->bits_per_pixel);
	printf("sync=%d\n", fb_info->sync);
}

void roi_fb_init(unsigned char *fb,
		struct fb_var_screeninfo * var, struct roi_params *roi)
{
	int y, i;
	int buf_size, line_len, offset;
	int bpp_bytes;
	unsigned char *pbuf;

	printf("Config ROI=%d\n", roi->roi_n);

	if (var->bits_per_pixel == 16)
		bpp_bytes = 2;
	else
		bpp_bytes = 4;

	/* alloc memory  */
	buf_size =	(roi->end_x - roi->start_x + 1) *
				(roi->end_y - roi->start_y + 1) * bpp_bytes;

	pbuf = malloc(buf_size);
	memset(pbuf, g_show_data, buf_size);

	/* sw calucate crc */
	if (var->bits_per_pixel == 16) {
		/* lcdif bpp 16 to bus width 24 */
		if (g_disp_bus_width == 24)
			roi->ref_sig = crc32_calc_24of16bit(0x0, (unsigned short *)pbuf, buf_size/sizeof(short));

		/* lcdif bpp=16, lvds bus width 18 */
		if (g_disp_bus_width == 18)
			roi->ref_sig = crc32_calc_18of16bit(0x0, (unsigned short *)pbuf, buf_size/sizeof(short));
	} else {
		/* lcdif bpp 24 to bus width 24 */
		if (g_disp_bus_width == 24)
			roi->ref_sig = crc32_calc_24bit(0x0, (unsigned int *)pbuf, buf_size/sizeof(int));

		/* lcdif bpp=24 lvds bus width 18  */
		if (g_disp_bus_width == 18)
			roi->ref_sig = crc32_calc_18of24bit(0x0, (unsigned int *)pbuf, buf_size/sizeof(int));
	}

	line_len = (roi->end_x - roi->start_x + 1) * bpp_bytes;

	/* cpoy to fb memory  */
	for (y = roi->start_y, i = 0; y <= roi->end_y; y++, i++) {
		offset = (y * var->xres_virtual + roi->start_x) * bpp_bytes;
		memcpy(fb + offset,	pbuf + i * line_len, line_len);
	}

	free(pbuf);
}

void roi_config(unsigned char *fb ,struct fb_var_screeninfo *fb_info)
{
	struct roi_params roi[3];
	int retval;
	int result;
	int i;

	printf("bpp=%d, bus_width=%d\n", fb_info->bits_per_pixel, g_disp_bus_width);

	/* ROI 0  */
	roi[0].roi_n = 1;
	roi[0].start_x = g_start_x_offset;
	roi[0].start_y = g_start_y_offset;
	roi[0].end_x = g_end_x_offset;
	roi[0].end_y = g_end_y_offset;
	roi[0].freeze = 0;

	roi_fb_init(fb, fb_info, &roi[0]);
	retval = ioctl(fd_dcic, DCIC_IOC_CONFIG_ROI, &roi[0]);

	/* ROI 1 */
	roi[1].roi_n = 3;
	roi[1].start_x = 250;
	roi[1].start_y = 250;
	roi[1].end_x = 360;
	roi[1].end_y = 360;
	roi[1].freeze = 0;

	roi_fb_init(fb, fb_info, &roi[1]);
	retval = ioctl(fd_dcic, DCIC_IOC_CONFIG_ROI, &roi[1]);

	/* ROI 2  */
	roi[2].roi_n = 5;
	roi[2].start_x = 400;
	roi[2].start_y = 400;
	roi[2].end_x = 450;
	roi[2].end_y = 450;
	roi[2].freeze = 0;

	roi_fb_init(fb, fb_info, &roi[2]);
	retval = ioctl(fd_dcic, DCIC_IOC_CONFIG_ROI, &roi[2]);

	/* get result  */
	retval = ioctl(fd_dcic, DCIC_IOC_GET_RESULT, &result);
	if (retval < 0) {
		printf("Get dcic check result failed\n");
		return;
	}

	if (result == 0)
		printf("All ROI CRC check success!\n");
	else
		for (i = 0; i < 16; i++) {
			if (result & ( 0x1 << i))
				printf("Error CRC Check ROI%d\n", i);
		}

	return;
}

void print_help(void)
{
	printf("DCIC Device Test\n"
		"Syntax: ./mxc_dcic_test -bw <display control bus width 18 or 24>\n"
		" -dev <dcic devi No: 0-DCIC0; 1-DCIC1>\n"
		" -sdata <CRC check rectangel content[0~255]>\n"
		" -sx <crc check start x offset>\n"
		" -sy <crc check start y offset>\n"
		" -ex <crc check end x offset>\n"
		" -ey <crc check end y offset>\n"
	);
}

int process_cmdline(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-dev") == 0) {
			g_dcic_dev = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-bw") == 0) {
			g_disp_bus_width = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-sdata") == 0) {
			g_show_data = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-sx") == 0) {
			g_start_x_offset = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-sy") == 0) {
			g_start_y_offset = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-ex") == 0) {
			g_end_x_offset = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-ey") == 0) {
			g_end_y_offset = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-help") == 0) {
			print_help();
			return -1;
		} else {
			print_help();
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int retval = TPASS;
	struct fb_var_screeninfo screen_info;

	if (process_cmdline(argc, argv) < 0) {
		return -1;
	}

	if ((fd_fb0 = open("/dev/fb0", O_RDWR, 0)) < 0) {
		printf("Unable to open /dev/fb0\n");
		retval = TFAIL;
		goto err0;
	}

	printf("Opened fb0\n");

	if (g_dcic_dev == 0) {
		if ((fd_dcic = open("/dev/mxc_dcic0", O_RDWR, 0)) < 0) {
			printf("Unable to open /dev/dcic0\n");
			retval = TFAIL;
			goto err1;
		}
		printf("open /dev/dcic0\n");
	} else {
		if ((fd_dcic = open("/dev/mxc_dcic1", O_RDWR, 0)) < 0) {
			printf("Unable to open /dev/dcic1\n");
			retval = TFAIL;
			goto err1;
		}
		printf("open /dev/dcic1\n");
	}

	/* unblank framebuffer */
	ioctl(fd_fb0, FBIOBLANK, FB_BLANK_UNBLANK);

	retval = ioctl(fd_fb0, FBIOGET_VSCREENINFO, &screen_info);
	if (retval < 0) {
		goto err2;
	}

	g_fb0_size = screen_info.xres * screen_info.yres_virtual * screen_info.bits_per_pixel / 8;

	/* Map the device to memory*/
	fb0 = (void *)mmap(0, g_fb0_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb0, 0);
	if ((int)fb0 <= 0) {
		printf("\nError: failed to map framebuffer device 0 to memory.\n");
		goto err2;
	}

	/* Config dcic  */
	retval = ioctl(fd_dcic, DCIC_IOC_CONFIG_DCIC, &screen_info.sync);

	roi_config(fb0 , &screen_info);

	munmap(fb0, g_fb0_size);
err2:
	close(fd_dcic);
err1:
	close(fd_fb0);
err0:
	return retval;
}
