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

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pxp_test.h"

extern int quitflag;

static char* skip(char *ptr)
{
	switch (*ptr) {
	case    '\0':
	case    ' ':
	case    '\t':
	case    '\n':
		break;
	case    '\"':
		ptr++;
		while ((*ptr != '\"') && (*ptr != '\0') && (*ptr != '\n')) {
			ptr++;
		}
		if (*ptr != '\0') {
			*ptr++ = '\0';
		}
		break;
	default :
		while ((*ptr != ' ') && (*ptr != '\t')
			&& (*ptr != '\0') && (*ptr != '\n')) {
			ptr++;
		}
		if (*ptr != '\0') {
			*ptr++ = '\0';
		}
		break;
	}

	while ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n')) {
		ptr++;
	}

	return (ptr);
}

void get_arg(char *buf, int *argc, char *argv[])
{
	char *ptr;
	*argc = 0;

	while ( (*buf == ' ') || (*buf == '\t'))
		buf++;

	for (ptr = buf; *argc < 32; (*argc)++) {
		if (!*ptr)
			break;
		argv[*argc] = ptr + (*ptr == '\"');
		ptr = skip(ptr);
	}

	argv[*argc] = NULL;
}


int
open_files(struct cmd_line *cmd)
{
	if (cmd->dst_scheme == PATH_FILE) {
		cmd->dst_fd = open(cmd->output, O_CREAT | O_RDWR | O_TRUNC,
					S_IRWXU | S_IRWXG | S_IRWXO);
		if (cmd->dst_fd < 0) {
			perror("file open");

			return -1;
		}
		printf("Output file \"%s\" opened.\n", cmd->output);
	}

	return 0;
}

void close_files(struct cmd_line *cmd)
{
	if ((cmd->dst_fd > 0)) {
		close(cmd->dst_fd);
		cmd->dst_fd = -1;
	}
}

char* skip_unwanted(char *ptr)
{
	int i = 0;
	static char buf[MAX_PATH];
	while (*ptr != '\0') {
		if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') {
			ptr++;
			continue;
		}

		if (*ptr == '#')
			break;

		buf[i++] = *ptr;
		ptr++;
	}

	buf[i] = 0;
	return (buf);
}

