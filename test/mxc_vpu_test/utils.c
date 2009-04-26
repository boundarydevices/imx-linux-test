/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vpu_test.h"

extern int quitflag;

/* Our custom header */
struct nethdr {
	int seqno;
	int iframe;
	int len;
};

int	/* write n bytes to a file descriptor */
fwriten(int fd, void *vptr, size_t n)
{
	int nleft;
	int nwrite;
	char  *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwrite = write(fd, ptr, nleft)) <= 0) {
			perror("fwrite: ");
			return (-1);			/* error */
		}

		nleft -= nwrite;
		ptr   += nwrite;
	}

	return (n);
} /* end fwriten */

int	/* Read n bytes from a file descriptor */
freadn(int fd, void *vptr, size_t n)
{
	int nleft = 0;
	int nread = 0;
	char  *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) <= 0) {
			if (nread == 0)
				return (n - nleft);

			perror("read");
			return (-1);			/* error EINTR XXX */
		}

		nleft -= nread;
		ptr   += nread;
	}

	return (nread);
}

/* Receive data from udp socket */
static int
udp_recv(struct cmd_line *cmd, int sd, char *buf, int n)
{
	int nleft, nread, nactual, nremain, ntotal = 0;
	char *ptr;
	struct nethdr *net_h;
	int hdrlen = sizeof(struct nethdr);
	fd_set rfds;
	struct timeval tv;

	if (cmd->nlen) {
		/* No more data to be received */
		if (cmd->nlen == -1) {
			return 0;
		}

		/* There was some data pending from the previous recvfrom.
		 * copy that data into the buffer
		 */
		if (cmd->nlen > n) {
			memcpy(buf, (cmd->nbuf + cmd->noffset), n);
			cmd->nlen -= n;
			cmd->noffset += n;
			return n;
		}

		memcpy(buf, (cmd->nbuf + cmd->noffset), cmd->nlen);
		ptr = buf + cmd->nlen;
		nleft = n - cmd->nlen;
		ntotal = cmd->nlen;
		cmd->nlen = 0;
	} else {
		ptr = buf;
		nleft = n;
	}

	while (nleft > 0) {
		tv.tv_sec = 0;
		tv.tv_usec = 3000;

		FD_ZERO(&rfds);
		FD_SET(sd, &rfds);

		nread = select(sd + 1, &rfds, NULL, NULL, &tv);
		if (nread < 0) {
			perror("select");
			return -1;
		}

		/* timeout */
		if (nread == 0) {
			if (quitflag) {
				n = ntotal;
				break;
			}

			/* wait for complete buffer to be full */
			if (cmd->complete) {
				continue;
			}

			if (ntotal == 0) {
			       return -EAGAIN;
			}

			n = ntotal;
			break;
		}

		nread = recvfrom(sd, cmd->nbuf, DEFAULT_PKT_SIZE, 0,
					NULL, NULL);
		if (nread < 0) {
			perror("recvfrom");
			return -1;
		}

		/* get our custom header */
		net_h = (struct nethdr *)cmd->nbuf;
		if (net_h->len == 0) {
			/* zero length data means no more data will be
			 * received */
			cmd->nlen = -1;
			return (n - nleft);
		}

		nactual = (nread - hdrlen);
		if (net_h->len != nactual) {
			warn_msg("length mismatch\n");
		}

		if (cmd->seq_no++ != net_h->seqno) {
			/* read till we get an I frame */
			if (net_h->iframe == 1) {
				cmd->seq_no = net_h->seqno + 1;
			} else {
				continue;
			}
		}

		/* check if there is space in user buffer to copy all the
		 * received data
		 */
		if (nactual > nleft) {
			nremain = nleft;
			cmd->nlen = nactual - nleft;
			cmd->noffset = (nleft + hdrlen);
		} else {
			nremain = nactual;
		}

		memcpy(ptr, (cmd->nbuf + hdrlen), nremain);
		ntotal += nremain;
		nleft -= nremain;
		ptr += nremain;
	}

	return (n);
}

/* send data to remote server */
static int
udp_send(struct cmd_line *cmd, int sd, char *buf, int n)
{
	int nwrite, hdrlen;
	struct nethdr net_h;
	struct sockaddr_in addr;

	bzero(&addr, sizeof(addr));
	hdrlen = sizeof(net_h);
	if ((n + hdrlen) > DEFAULT_PKT_SIZE) {
		err_msg("panic: increase default udp pkt size! %d\n", n);
		while (1);
	}

	if (n == 0) {
		net_h.seqno = -1;
		net_h.len = 0;
		memcpy(cmd->nbuf, (char *)&net_h, hdrlen);
	} else {
		net_h.seqno = cmd->seq_no++;
		net_h.iframe = cmd->iframe;
		net_h.len = n;
		memcpy(cmd->nbuf, (char *)&net_h, hdrlen);
		memcpy((cmd->nbuf + hdrlen), buf, n);
	}

	n += hdrlen;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(cmd->port);
	addr.sin_addr.s_addr = inet_addr(cmd->output);

	nwrite = sendto(sd, cmd->nbuf, n, 0, (struct sockaddr *)&addr,
				sizeof(addr));
	if (nwrite != n) {
		err_msg("sendto: error\n");
	}

	return nwrite;
}

int
vpu_read(struct cmd_line *cmd, char *buf, int n)
{
	int fd = cmd->src_fd;

	if (cmd->src_scheme == PATH_NET) {
		return udp_recv(cmd, fd, buf, n);
	}

	return freadn(fd, buf, n);
}

int
vpu_write(struct cmd_line *cmd, char *buf, int n)
{
	int fd = cmd->dst_fd;

	if (cmd->dst_scheme == PATH_NET) {
		return udp_send(cmd, fd, buf, n);
	}

	return fwriten(fd, buf, n);
}

static char*
skip(char *ptr)
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

void
get_arg(char *buf, int *argc, char *argv[])
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

static int
udp_open(struct cmd_line *cmd)
{
	int sd;
	struct sockaddr_in addr;

	cmd->nbuf = (char *)malloc(DEFAULT_PKT_SIZE);
	if (cmd->nbuf == NULL) {
		err_msg("failed to malloc udp buffer\n");
		return -1;
	}

	sd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		err_msg("failed to open udp socket\n");
		free(cmd->nbuf);
		cmd->nbuf = 0;
		return -1;
	}

	/* If server, then bind */
	if (cmd->src_scheme == PATH_NET) {
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(cmd->port);
		addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			err_msg("udp bind failed\n");
			close(sd);
			free(cmd->nbuf);
			cmd->nbuf = 0;
			return -1;
		}

	}

	return sd;
}

int
open_files(struct cmd_line *cmd)
{
	if (cmd->src_scheme == PATH_FILE) {
		cmd->src_fd = open(cmd->input, O_RDONLY, 0);
		if (cmd->src_fd < 0) {
			perror("file open");
			return -1;
		}
		info_msg("Input file \"%s\" opened.\n", cmd->input);
	} else if (cmd->src_scheme == PATH_NET) {
		/* open udp port for receive */
		cmd->src_fd = udp_open(cmd);
		if (cmd->src_fd < 0) {
			return -1;
		}

		info_msg("decoder listening on port %d\n", cmd->port);
	}

	if (cmd->dst_scheme == PATH_FILE) {
		cmd->dst_fd = open(cmd->output, O_CREAT | O_RDWR | O_TRUNC,
					S_IRWXU | S_IRWXG | S_IRWXO);
		if (cmd->dst_fd < 0) {
			perror("file open");

			if (cmd->src_scheme == PATH_FILE)
				close(cmd->src_fd);

			return -1;
		}
		info_msg("Output file \"%s\" opened.\n", cmd->output);
	} else if (cmd->dst_scheme == PATH_NET) {
		/* open udp port for send path */
		cmd->dst_fd = udp_open(cmd);
		if (cmd->dst_fd < 0) {
			if (cmd->src_scheme == PATH_NET)
				close(cmd->src_fd);
			return -1;
		}

		info_msg("encoder sending on port %d\n", cmd->port);
	}

	return 0;
}

void
close_files(struct cmd_line *cmd)
{
	if ((cmd->src_fd > 0)) {
		close(cmd->src_fd);
		cmd->src_fd = -1;
	}

	if ((cmd->dst_fd > 0)) {
		close(cmd->dst_fd);
		cmd->dst_fd = -1;
	}

	if (cmd->nbuf) {
		free(cmd->nbuf);
		cmd->nbuf = 0;
	}
}

int
check_params(struct cmd_line *cmd, int op)
{
	switch (cmd->format) {
	case STD_MPEG4:
		info_msg("Format: STD_MPEG4\n");
		switch (cmd->mp4Class) {
		case 0:
			info_msg("MPEG4 class: MPEG4\n");
			break;
		case 1:
			info_msg("MPEG4 class: DIVX5.0 or higher\n");
			break;
		case 2:
			info_msg("MPEG4 class: XVID\n");
			break;
		case 5:
			info_msg("MPEG4 class: DIVX4.0\n");
			break;
		default:
			err_msg("Unsupported MPEG4 Class!\n");
			break;
		}
		break;
	case STD_H263:
		info_msg("Format: STD_H263\n");
		break;
	case STD_AVC:
		info_msg("Format: STD_AVC\n");
		break;
	case STD_VC1:
		info_msg("Format: STD_VC1\n");
		break;
	case STD_MPEG2:
		info_msg("Format: STD_MPEG2\n");
		break;
	case STD_DIV3:
		info_msg("Format: STD_DIV3\n");
		break;
	case STD_RV:
		info_msg("Format: STD_RV\n");
		break;
	case STD_MJPG:
		info_msg("Format: STD_MJPG\n");
		break;
	default:
		err_msg("Unsupported Format!\n");
		break;
	}

	if (cmd->port == 0) {
		cmd->port = DEFAULT_PORT;
	}

	if (cmd->src_scheme != PATH_FILE && op == DECODE) {
		cmd->src_scheme = PATH_NET;
	}

	if (cmd->src_scheme == PATH_FILE && op == ENCODE) {
		if (cmd->width == 0 || cmd->height == 0) {
			warn_msg("Enter width and height for YUV file\n");
			return -1;
		}
	}

	if (cmd->src_scheme == PATH_V4L2 && op == ENCODE) {
		if (cmd->width == 0)
			cmd->width = 176; /* default */

		if (cmd->height == 0)
			cmd->height = 144;

		if (cmd->width % 16 != 0) {
			cmd->width -= cmd->width % 16;
			warn_msg("width not divisible by 16, adjusted %d\n",
					cmd->width);
		}

		if (cmd->height % 16 != 0) {
			cmd->height -= cmd->height % 16;
			warn_msg("height not divisible by 16, adjusted %d\n",
					cmd->height);
		}
	}

	if (cmd->dst_scheme != PATH_FILE && op == ENCODE) {
		if (cmd->dst_scheme != PATH_NET) {
			warn_msg("No output file specified, using default\n");
			cmd->dst_scheme = PATH_FILE;

			if (cmd->format == STD_MPEG4) {
				strncpy(cmd->output, "enc.mpeg4", 16);
			} else if (cmd->format == STD_H263) {
				strncpy(cmd->output, "enc.263", 16);
			} else {
				strncpy(cmd->output, "enc.264", 16);
			}
		}
	}

	if (cmd->rot_en) {
		if (cmd->rot_angle != 0 && cmd->rot_angle != 90 &&
			cmd->rot_angle != 180 && cmd->rot_angle != 270) {
			warn_msg("Invalid rotation angle. No rotation!\n");
			cmd->rot_en = 0;
			cmd->rot_angle = 0;
		}
	}

	if (cmd->mirror < MIRDIR_NONE || cmd->mirror > MIRDIR_HOR_VER) {
		warn_msg("Invalid mirror angle. Using 0\n");
		cmd->mirror = 0;
	}

	if (!(cmd->format == STD_MPEG4 || cmd->format == STD_H263 ||
	    cmd->format == STD_MPEG2 || cmd->format == STD_DIV3) &&
	    cmd->deblock_en) {
		warn_msg("Deblocking only for MPEG4 and MPEG2. Disabled!\n");
		cmd->deblock_en = 0;
	}

	return 0;
}

char*
skip_unwanted(char *ptr)
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

int parse_options(char *buf, struct cmd_line *cmd, int *mode)
{
	char *str;

	str = strstr(buf, "end");
	if (str != NULL) {
		return 100;
	}

	str = strstr(buf, "operation");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				*mode = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "input");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				strncpy(cmd->input, str, MAX_PATH);
				cmd->src_scheme = PATH_FILE;
			}
		}

		return 0;
	}

	str = strstr(buf, "output");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				strncpy(cmd->output, str, MAX_PATH);
				cmd->dst_scheme = PATH_FILE;
			}
		}

		return 0;
	}

	str = strstr(buf, "port");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->port = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "format");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->format = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "rotation");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->rot_en = 1;
				cmd->rot_angle = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "ipu_rot");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->ipu_rot_en = strtol(str, NULL, 10);
				if (cmd->ipu_rot_en == 1)
					cmd->rot_en = 0;
			}
		}

		return 0;
	}

        str = strstr(buf, "ip");
        if (str != NULL) {
                str = index(buf, '=');
                if (str != NULL) {
                        str++;
                        if (*str != '\0') {
                                strncpy(cmd->output, str, 64);
                                cmd->dst_scheme = PATH_NET;
                        }
                }

                return 0;
        }

	str = strstr(buf, "count");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->count = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "chromaInterleave");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->chromaInterleave = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "mp4Class");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->mp4Class = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "deblock");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->deblock_en = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "dering");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->dering_en = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "mirror");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->mirror = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "width");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->width = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "height");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->height = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "bitrate");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->bitrate = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "prescan");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->prescan = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	str = strstr(buf, "gop");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				cmd->gop = strtol(str, NULL, 10);
			}
		}

		return 0;
	}

	return 0;
}

void SaveQpReport(Uint32 *qpReportAddr, int picWidth, int picHeight,
		  int frameIdx, char *fileName)
{
	FILE *fp;
	int i, j;
	int MBx, MBy, MBxof4, MBxof1, MBxx;
	Uint32 *qpAddr;
	Uint32 qp;
	Uint8 lastQp[4];

	if (frameIdx == 0)
		fp = fopen(fileName, "wb");
	else
		fp = fopen(fileName, "a+b");

	if (!fp) {
		err_msg("Can't open %s in SaveQpReport\n", fileName);
		return;
	}

	MBx = picWidth / 16;
	MBxof1 = MBx % 4;
	MBxof4 = MBx - MBxof1;
	MBy = picHeight / 16;
	MBxx = (MBx + 3) / 4 * 4;
	for (i = 0; i < MBy; i++) {
		for (j = 0; j < MBxof4; j = j + 4) {
			dprintf(3, "qpReportAddr = %lx\n", (Uint32)qpReportAddr);
			qpAddr = (Uint32 *)((Uint32)qpReportAddr + j + MBxx * i);
			qp = *qpAddr;
			fprintf(fp, " %4d %4d %3d \n", frameIdx,
				MBx * i + j + 0, (Uint8) (qp >> 24));
			fprintf(fp, " %4d %4d %3d \n", frameIdx,
				MBx * i + j + 1, (Uint8) (qp >> 16));
			fprintf(fp, " %4d %4d %3d \n", frameIdx,
				MBx * i + j + 2, (Uint8) (qp >> 8));
			fprintf(fp, " %4d %4d %3d \n", frameIdx,
				MBx * i + j + 3, (Uint8) qp);
		}

		if (MBxof1 > 0) {
			qpAddr = (Uint32 *)((Uint32)qpReportAddr + MBxx * i + MBxof4);
			qp = *qpAddr;
			lastQp[0] = (Uint8) (qp >> 24);
			lastQp[1] = (Uint8) (qp >> 16);
			lastQp[2] = (Uint8) (qp >> 8);
			lastQp[3] = (Uint8) (qp);
		}

		for (j = 0; j < MBxof1; j++) {
			fprintf(fp, " %4d %4d %3d \n", frameIdx,
				MBx * i + j + MBxof4, lastQp[j]);
		}
	}

	fclose(fp);
}
