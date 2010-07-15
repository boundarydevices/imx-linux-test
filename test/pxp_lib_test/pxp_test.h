/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _PXP_TEST_H
#define _PXP_TEST_H

#define PATH_FILE	1

#define MAX_PATH	128
struct cmd_line {
	char output[MAX_PATH];  /* Output file name */
	int dst_scheme;
	int dst_fd;
	int rot_angle;
	int hflip;
	int vflip;
	int left;
	int top;
	int pixel_inversion;
};

void get_arg(char *buf, int *argc, char *argv[]);
int open_files(struct cmd_line *cmd);
void close_files(struct cmd_line *cmd);
char*skip_unwanted(char *ptr);

#endif
