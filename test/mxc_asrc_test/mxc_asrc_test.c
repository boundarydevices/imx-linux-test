/*
 * Copyright 2008 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <asm/arch/mxc_asrc.h>

#define DMA_BUF_SIZE 4096
#define BUF_NUM 4

struct audio_info_s {
	int sample_rate;
	int channel;
	int data_len;
	int output_data_len;
	int output_sample_rate;
	unsigned short frame_bits;
	unsigned short blockalign;
};
struct audio_buf {
	char *start;
	unsigned int index;
	unsigned int length;
	unsigned int max_len;
};

static struct audio_buf input_buf[BUF_NUM];
static struct audio_buf output_buf[BUF_NUM];
static enum asrc_pair_index pair_index;
static char header[58];

void help_info(int ac, char *av[])
{
	printf("\n\n**************************************************\n");
	printf("* Test aplication for ASRC\n");
	printf("* Options : \n\n");
	printf("-to <output sample rate> <origin.wav> <converted.wav>\n");
	printf("**************************************************\n\n");
}

int configure_asrc_channel(int fd_asrc, struct audio_info_s *info)
{
	int err = 0;
	int i = 0;
	struct asrc_req req;
	struct asrc_config config;
	struct asrc_querybuf buf_info;

	req.chn_num = info->channel;
	if ((err = ioctl(fd_asrc, ASRC_REQ_PAIR, &req)) < 0) {
		return err;
	}

	config.pair = req.index;
	config.channel_num = req.chn_num;
	config.dma_buffer_size = DMA_BUF_SIZE;
	config.input_sample_rate = info->sample_rate;
	config.output_sample_rate = info->output_sample_rate;
	config.buffer_num = BUF_NUM;
	pair_index = req.index;
	if ((err = ioctl(fd_asrc, ASRC_CONIFG_PAIR, &config)) < 0)
		return err;
	for (i = 0; i < config.buffer_num; i++) {
		buf_info.buffer_index = i;
		if ((err = ioctl(fd_asrc, ASRC_QUERYBUF, &buf_info)) < 0)
			return err;
		input_buf[i].start =
		    mmap(NULL, buf_info.input_length, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fd_asrc, buf_info.input_offset);
		input_buf[i].max_len = buf_info.input_length;
		output_buf[i].start =
		    mmap(NULL, buf_info.output_length, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fd_asrc, buf_info.output_offset);
		output_buf[i].max_len = buf_info.output_length;
	};
	return 0;
}

int asrc_get_output_buffer_size(int input_buffer_size,
				int input_sample_rate, int output_sample_rate)
{
	int i = 0;
	int outbuffer_size = 0;
	int outsample = output_sample_rate;
	while (outsample >= input_sample_rate) {
		++i;
		outsample -= input_sample_rate;
	}
	outbuffer_size = i * input_buffer_size;
	i = 1;
	while (((input_buffer_size >> i) > 2) && (outsample != 0)) {
		if (((outsample << 1) - input_sample_rate) >= 0) {
			outsample = (outsample << 1) - input_sample_rate;
			outbuffer_size += (input_buffer_size >> i);
		} else {
			outsample = outsample << 1;
		}
		i++;
	}
	outbuffer_size = (outbuffer_size >> 3) << 3;
	return outbuffer_size;
}

int play_file(int fd_asrc, FILE * src, FILE * dst, struct audio_info_s *info)
{
	int err = 0;
	int i = 0;
	struct asrc_buffer inbuf, outbuf;
	char *p;
	int output_dma_size;
	int nleft, nwritten;

	output_dma_size =
	    asrc_get_output_buffer_size(DMA_BUF_SIZE, info->sample_rate,
					info->output_sample_rate);
	memset(input_buf[i].start, 0, DMA_BUF_SIZE);
	inbuf.length = DMA_BUF_SIZE;
	inbuf.index = i;
	if ((err = ioctl(fd_asrc, ASRC_Q_INBUF, &inbuf)) < 0)
		goto ERROR;
	outbuf.index = i;
	outbuf.length = output_dma_size;
	if ((err = ioctl(fd_asrc, ASRC_Q_OUTBUF, &outbuf)) < 0)
		goto ERROR;
	i++;
	while (i < BUF_NUM) {
		fread(input_buf[i].start, 1, DMA_BUF_SIZE, src);
		inbuf.length = DMA_BUF_SIZE;
		inbuf.index = i;
		info->data_len -= DMA_BUF_SIZE;
		if ((err = ioctl(fd_asrc, ASRC_Q_INBUF, &inbuf)) < 0)
			goto ERROR;
		outbuf.index = i;
		outbuf.length = output_dma_size;
		info->output_data_len -= output_dma_size;
		if ((err = ioctl(fd_asrc, ASRC_Q_OUTBUF, &outbuf)) < 0)
			goto ERROR;
		i++;
	}

	if ((err = ioctl(fd_asrc, ASRC_START_CONV, &pair_index)) < 0)
		goto ERROR;
	if ((err = ioctl(fd_asrc, ASRC_DQ_OUTBUF, &outbuf)) < 0)
		goto ERROR;
	if ((err = ioctl(fd_asrc, ASRC_Q_OUTBUF, &outbuf)) < 0)
		goto ERROR;
	if ((err = ioctl(fd_asrc, ASRC_DQ_INBUF, &inbuf)) < 0)
		goto ERROR;
	inbuf.length =
	    (info->data_len > DMA_BUF_SIZE) ? DMA_BUF_SIZE : info->data_len;
	if (info->data_len > 0) {
		fread(input_buf[inbuf.index].start, 1, inbuf.length, src);
		info->data_len -= inbuf.length;
	} else
		inbuf.length = DMA_BUF_SIZE;
	if ((err = ioctl(fd_asrc, ASRC_Q_INBUF, &inbuf)) < 0)
		goto ERROR;
	do {
		if ((err = ioctl(fd_asrc, ASRC_DQ_OUTBUF, &outbuf)) < 0)
			goto ERROR;
		outbuf.length =
		    (info->output_data_len >
		     output_dma_size) ? output_dma_size : info->output_data_len;
		nleft = outbuf.length;
		p = output_buf[outbuf.index].start;
		while (nleft > 0) {
			if ((nwritten = fwrite(p, 1, nleft, dst)) < 0) {
				perror("audio driver write error");
			}

			nleft -= nwritten;
			p += nwritten;
		}
		info->output_data_len -= outbuf.length;
		outbuf.length = output_dma_size;
		if ((err = ioctl(fd_asrc, ASRC_Q_OUTBUF, &outbuf)) < 0)
			goto ERROR;
		if ((err = ioctl(fd_asrc, ASRC_DQ_INBUF, &inbuf)) < 0)
			goto ERROR;
		inbuf.length =
		    (info->data_len >
		     DMA_BUF_SIZE) ? DMA_BUF_SIZE : info->data_len;
		if (info->data_len > 0) {
			fread(input_buf[inbuf.index].start, 1, inbuf.length,
			      src);
			info->data_len -= inbuf.length;
		} else
			inbuf.length = DMA_BUF_SIZE;
		if ((err = ioctl(fd_asrc, ASRC_Q_INBUF, &inbuf)) < 0)
			goto ERROR;
	} while (info->output_data_len > 0);

	err = ioctl(fd_asrc, ASRC_STOP_CONV, &pair_index);

      ERROR:
	return err;
}

void bitshift(FILE * src, FILE * dst, struct audio_info_s *info)
{

	unsigned int data;
	unsigned int zero;
	int nleft;
	int format_size;

	format_size = *(int *)&header[16];
	if (strncmp((char *)&header[20 + format_size], "fact", 4) == 0) {
		format_size += 12;
	}

	fseek(dst, 28 + format_size, SEEK_SET);

	if (info->frame_bits <= 16) {
		nleft = (info->data_len >> 1);
		do {
			fread(&data, 2, 1, src);
			zero = ((data << 8) & 0xFFFF00);
			fwrite(&zero, 4, 1, dst);
		} while (--nleft);
		info->data_len = info->data_len << 1;
		info->output_data_len = info->output_data_len << 1;
		info->frame_bits = 32;
		info->blockalign = 8;

		*(int *)&header[24 + format_size] = info->output_data_len;

		*(int *)&header[4] = info->output_data_len + 20 + format_size;

		*(int *)&header[28] =
		    info->output_sample_rate * info->channel *
		    (info->frame_bits / 8);

		*(unsigned short *)&header[32] = info->blockalign;
		*(unsigned short *)&header[34] = info->frame_bits;
	} else {
		nleft = (info->data_len >> 2);
		do {
			fread(&data, 4, 1, src);
			zero = ((data >> 8) & 0xFFFF00);
			fwrite(&zero, 4, 1, dst);
		} while (--nleft);
	}
	fseek(dst, 0, SEEK_SET);
	fwrite(header, 1, 28 + format_size, dst);
	fseek(dst, 28 + format_size, SEEK_SET);

}

void header_parser(FILE * src, struct audio_info_s *info)
{

	int format_size;

	fseek(src, 0, SEEK_SET);
	fread(header, 1, 58, src);

	fseek(src, 16, SEEK_SET);
	fread(&format_size, 4, 1, src);

	fseek(src, 22, SEEK_SET);
	fread(&info->channel, 2, 1, src);

	fseek(src, 24, SEEK_SET);
	fread(&info->sample_rate, 4, 1, src);

	fseek(src, 32, SEEK_SET);
	fread(&info->blockalign, 2, 1, src);

	fseek(src, 34, SEEK_SET);
	fread(&info->frame_bits, 2, 1, src);

	if (strncmp((char *)&header[20 + format_size], "fact", 4) == 0)
		format_size += 12;

	if (strncmp((char *)&header[20 + format_size], "data", 4) != 0)
		printf("header parser wrong\n");

	fseek(src, 24 + format_size, SEEK_SET);
	fread(&info->data_len, 4, 1, src);

	info->output_data_len =
	    asrc_get_output_buffer_size(info->data_len,
					info->sample_rate,
					info->output_sample_rate);
	*(int *)&header[24 + format_size] = info->output_data_len;

	*(int *)&header[4] = info->output_data_len + 20 + format_size;

	*(int *)&header[24] = info->output_sample_rate;

	*(int *)&header[28] =
	    info->output_sample_rate * info->channel * (info->frame_bits / 8);

	fseek(src, 28 + format_size, SEEK_SET);

	return;

}

void convert_data(FILE * raw, FILE * dst, struct audio_info_s *info)
{
	unsigned int data;
	unsigned int size;
	int format_size;

	format_size = *(int *)&header[16];
	if (strncmp((char *)&header[20 + format_size], "fact", 4) == 0) {
		format_size += 12;
	}
	size = *(int *)&header[24 + format_size];
	fseek(dst, 28 + format_size, SEEK_SET);
	fseek(raw, 0, SEEK_SET);
	do {
		fread(&data, 4, 1, raw);
		data = (data << 8) & 0xFFFF0000;
		fwrite(&data, 4, 1, dst);
		size -= 4;
	} while (size > 0);
}

int main(int ac, char *av[])
{
	FILE *fd_dst = NULL;
	FILE *fd_src = NULL;
	FILE *fd_raw = NULL;
	int fd_asrc;
	struct audio_info_s audio_info;
	int i = 0, err = 0;

	printf("Hi... \n");

	if (ac != 5) {
		help_info(ac, av);
		return 1;
	}
	memset(&audio_info, 0, sizeof(struct audio_info_s));
	fd_asrc = open("/dev/mxc_asrc", O_RDWR);
	if (fd_asrc < 0)
		printf("Unable to open device\n");

	for (i = 1; i < ac; i++) {
		if (strcmp(av[i], "-to") == 0)
			audio_info.output_sample_rate = atoi(av[++i]);
	}

	if ((fd_dst = fopen(av[ac - 1], "wb+")) <= 0) {
		goto err_dst_not_found;
	}

	if ((fd_src = fopen(av[ac - 2], "r")) <= 0) {
		goto err_src_not_found;
	}

	header_parser(fd_src, &audio_info);

	bitshift(fd_src, fd_dst, &audio_info);

	err = configure_asrc_channel(fd_asrc, &audio_info);

	if (err < 0)
		goto end_err;

	fd_raw = fopen("/tmp/raw.txt", "wb+");
	/* Config HW */
	err += play_file(fd_asrc, fd_dst, fd_raw, &audio_info);

	if (err < 0)
		goto end_err;

	convert_data(fd_raw, fd_dst, &audio_info);

	if (fd_raw != NULL)
		fclose(fd_raw);
	fclose(fd_src);
	fclose(fd_dst);
	close(fd_asrc);

	printf("All tests passed with success\n");
	return 0;

      end_err:
	fclose(fd_src);
	if (fd_raw != NULL)
		fclose(fd_raw);
      err_src_not_found:
	fclose(fd_dst);
      err_dst_not_found:
	ioctl(fd_asrc, ASRC_RELEASE_PAIR, &pair_index);
	close(fd_asrc);
	return err;
}
