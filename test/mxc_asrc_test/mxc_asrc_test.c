/*
 * Copyright 2008-2012 Freescale Semiconductor, Inc. All rights reserved.
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
#include <linux/mxc_asrc.h>
#include <pthread.h>

#define DMA_BUF_SIZE 4096

/*
 * From 38 kernel, asrc driver only supports one pair of buffer
 * convertion per time
 */
#define BUF_NUM 2

struct audio_info_s {
	int sample_rate;
	int channel;
	int input_data_len;
	int output_data_len;
	int input_used;
	int output_used;
	int output_sample_rate;
	int output_dma_size;
	unsigned short frame_bits;
	unsigned short blockalign;
};
struct audio_buf {
	char *start;
	unsigned int index;
	unsigned int length;
	unsigned int max_len;
};

#define WAVE_HEAD_SIZE 44 + 14
static struct audio_buf input_buf[BUF_NUM];
static struct audio_buf output_buf[BUF_NUM];
static enum asrc_pair_index pair_index;
static char header[WAVE_HEAD_SIZE];

static int *input_buffer;
static int *output_buffer;

static enum asrc_inclk inclk;
static enum asrc_outclk outclk;

unsigned int convert_flag;
int fd_asrc;

static pthread_t input_thread;
static pthread_t output_thread;


void *asrc_input_thread(void *info);
void *asrc_output_thread(void *info);

void help_info(int ac, char *av[])
{
	printf("\n\n**************************************************\n");
	printf("* Test aplication for ASRC\n");
	printf("* Options : \n\n");
	printf("-to <output sample rate> <origin.wav> <converted.wav>\n");
	printf("<input clock source> <output clock source>\n");
	printf("input clock source types are:\n\n");
	printf("0  --  INCLK_NONE\n");
	printf("1  --  INCLK_ESAI_RX\n");
	printf("2  --  INCLK_SSI1_RX\n");
	printf("3  --  INCLK_SSI2_RX\n");
	printf("4  --  INCLK_SPDIF_RX\n");
	printf("5  --  INCLK_MLB_CLK\n");
	printf("6  --  INCLK_ESAI_TX\n");
	printf("7  --  INCLK_SSI1_TX\n");
	printf("8  --  INCLK_SSI2_TX\n");
	printf("9  --  INCLK_SPDIF_TX\n");
	printf("10 --  INCLK_ASRCK1_CLK\n");
	printf("default option for output clock source is 0\n");
	printf("output clock source types are:\n\n");
	printf("0  --  OUTCLK_NONE\n");
	printf("1  --  OUTCLK_ESAI_TX\n");
	printf("2  --  OUTCLK_SSI1_TX\n");
	printf("3  --  OUTCLK_SSI2_TX\n");
	printf("4  --  OUTCLK_SPDIF_TX\n");
	printf("5  --  OUTCLK_MLB_CLK\n");
	printf("6  --  OUTCLK_ESAI_RX\n");
	printf("7  --  OUTCLK_SSI1_RX\n");
	printf("8  --  OUTCLK_SSI2_RX\n");
	printf("9  --  OUTCLK_SPDIF_RX\n");
	printf("10 --  OUTCLK_ASRCK1_CLK\n");
	printf("default option for output clock source is 10\n");
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
		printf("Req ASRC pair FAILED\n");
		return err;
	}
	if (req.index == 0)
		printf("Pair A requested\n");
	else if (req.index == 1)
		printf("Pair B requested\n");
	else if (req.index == 2)
		printf("Pair C requested\n");

	config.pair = req.index;
	config.channel_num = req.chn_num;
	config.dma_buffer_size = DMA_BUF_SIZE;
	config.input_sample_rate = info->sample_rate;
	config.output_sample_rate = info->output_sample_rate;
	config.buffer_num = BUF_NUM;
	config.word_width = 32;
	config.inclk = inclk;
	config.outclk = outclk;
	pair_index = req.index;
	if ((err = ioctl(fd_asrc, ASRC_CONFIG_PAIR, &config)) < 0)
		return err;

	for (i = 0; i < config.buffer_num; i++) {
		buf_info.buffer_index = i;
		if ((err = ioctl(fd_asrc, ASRC_QUERYBUF, &buf_info)) < 0)
			return err;
		input_buf[i].start =
		    mmap(NULL, buf_info.input_length, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fd_asrc, buf_info.input_offset);
		if(input_buf[i].start == MAP_FAILED) {
			printf("mmap error!\r\n");
			return -1;
		}
		input_buf[i].max_len = buf_info.input_length;
		output_buf[i].start =
		    mmap(NULL, buf_info.output_length, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fd_asrc, buf_info.output_offset);
		if(output_buf[i].start == MAP_FAILED) {
			printf("mmap error!\r\n");
			return -1;
		}

		output_buf[i].max_len = buf_info.output_length;
	}
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

int play_file(int fd_asrc, struct audio_info_s *info)
{
	int err = 0;
	int i = 0;
	int y = 0;
	struct asrc_buffer inbuf, outbuf;
	char *p;
	char *input_p;
	char *output_p;
	struct asrc_status_flags flags;
	int flush_done = 0;

	info->output_dma_size =
	       	asrc_get_output_buffer_size(DMA_BUF_SIZE,
					info->sample_rate,
					info->output_sample_rate);
	info->output_dma_size = DMA_BUF_SIZE;

	flags.index = pair_index;
	input_p = (char *)input_buffer;
	output_p = (char *)output_buffer;

	for (i = 0;i < BUF_NUM; i++) {
		input_p = (char *)input_buffer + info->input_used;
		memcpy(input_buf[i].start, input_p, DMA_BUF_SIZE);
		info->input_used += DMA_BUF_SIZE;
		inbuf.length = DMA_BUF_SIZE;
		inbuf.index = i;
		if ((err = ioctl(fd_asrc, ASRC_Q_INBUF, &inbuf)) < 0)
			goto error;
		outbuf.index = i;
		outbuf.length = info->output_dma_size;
		if ((err = ioctl(fd_asrc, ASRC_Q_OUTBUF, &outbuf)) < 0)
			goto error;
	}
	convert_flag = 1;
	if ((err = ioctl(fd_asrc, ASRC_START_CONV, &pair_index)) < 0)
		goto error;

	pthread_create(&output_thread, NULL, asrc_output_thread,(void *)info);
	pthread_create(&input_thread, NULL, asrc_input_thread,(void *)info);
	pthread_join(input_thread, NULL);
	pthread_join(output_thread, NULL);

	err = ioctl(fd_asrc, ASRC_STOP_CONV, &pair_index);
error:
	return err;
}

void bitshift(FILE * src, struct audio_info_s *info)
{

	unsigned int data;
	unsigned int zero;
	int nleft;
	int format_size;
	int i = 0;
	format_size = *(int *)&header[16];

	if (info->frame_bits <= 16) {
		nleft = (info->input_data_len >> 1);
		input_buffer = (int *)malloc(sizeof(int) * nleft);
		if (input_buffer == NULL) {
			printf("allocate input buffer error\n");
		}
		do {
			fread(&data, 2, 1, src);
			zero = ((data << 8) & 0xFFFF00);
			input_buffer[i++] = zero;
		} while (--nleft);

		info->input_data_len = info->input_data_len << 1;
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
		nleft = (info->input_data_len >> 2);
		input_buffer = (int *)malloc(sizeof(int) * nleft);
		if (input_buffer == NULL) {
			printf("allocate input buffer error\n");
		}
		do {
			fread(&data, 4, 1, src);
			zero = ((data >> 8) & 0xFFFF00);
			input_buffer[i++] = zero;
		} while (--nleft);
	}
	output_buffer = (int *)malloc(info->output_data_len);
	if (output_buffer == NULL) {
		printf("output buffer allocate error\n");
	}
}

int header_parser(FILE * src, struct audio_info_s *info)
{

	int format_size;
	char chunk_id[4];
	int chunk_size;

	/* check the "RIFF" chunk */
	fseek(src, 0, SEEK_SET);
	fread(chunk_id, 4, 1, src);
	while (strncmp(chunk_id, "RIFF", 4) != 0){
		fread(&chunk_size, 4, 1, src);
		fseek(src, chunk_size, SEEK_CUR);
		if(fread(chunk_id, 4, 1, src) == 0) {
			printf("Wrong wave file format \n");
			return -1;
		}
	}
	fseek(src, -4, SEEK_CUR);
	fread(&header[0], 1, 12, src);

	/* check the "fmt " chunk */
	fread(chunk_id, 4, 1, src);
	while (strncmp(chunk_id, "fmt ", 4) != 0){
		fread(&chunk_size, 4, 1, src);
		fseek(src, chunk_size, SEEK_CUR);
		if(fread(chunk_id, 4, 1, src) == 0) {
			printf("Wrong wave file format \n");
			return -1;
		}
	}
	/* fmt chunk size */
	fread(&format_size, 4, 1, src);

	fseek(src, -8, SEEK_CUR);
	fread(&header[12], 1, format_size + 8, src);

	/* AudioFormat(2) */

	/* NumChannel(2) */
	info->channel = *(short *)&header[12 + 8 + 2];

	/* SampleRate(4) */
	info->sample_rate = *(int *)&header[12 + 8 + 2 + 2];

	/* ByteRate(4) */

	/* BlockAlign(2) */
	info->blockalign = *(short *)&header[12 + 8 + 2 + 2 + 4 + 4];

	/* BitsPerSample(2) */
	info->frame_bits = *(short *)&header[12 + 8 + 2 + 2 + 4 + 4 + 2];


	/* check the "data" chunk */
	fread(chunk_id, 4, 1, src);
	while (strncmp(chunk_id, "data", 4) != 0) {
		fread(&chunk_size, 4, 1, src);
		/* seek to next chunk if it is not "data" chunk */
		fseek(src, chunk_size, SEEK_CUR);
		if(fread(chunk_id, 4, 1, src) == 0) {
		    printf("No data chunk found \nWrong wave file format \n");
		    return -1;
		}
	}
	/* wave data length */
	fread(&info->input_data_len, 4, 1, src);
	fseek(src, -8, SEEK_CUR);
	fread(&header[format_size + 20], 1, 8, src);

	info->output_data_len =
	    asrc_get_output_buffer_size(info->input_data_len,
					info->sample_rate,
					info->output_sample_rate);

	*(int *)&header[24 + format_size] = info->output_data_len;

	*(int *)&header[4] = info->output_data_len + 20 + format_size;

	*(int *)&header[24] = info->output_sample_rate;

	*(int *)&header[28] =
	    info->output_sample_rate * info->channel * (info->frame_bits / 8);

	return 1;

}

void convert_data(FILE * dst, struct audio_info_s *info)
{
	unsigned int data;
	unsigned int size;
	int format_size;
	int i = 0;

	format_size = *(int *)&header[16];

	*(int *)&header[24 + format_size] = info->output_used;
	*(int *)&header[4] = info->output_used + 20 + format_size;
	size = *(int *)&header[24 + format_size];
	while (i < (format_size + 28)) {
		fwrite(&header[i], 1, 1, dst);
		i++;
	}
	i = 0;
	do {
		data = output_buffer[i++];
		data = (data << 8) & 0xFFFF0000;
		fwrite(&data, 4, 1, dst);
		size -= 4;
	} while (size > 0);
}

int main(int ac, char *av[])
{
	FILE *fd_dst = NULL;
	FILE *fd_src = NULL;
	struct audio_info_s audio_info;
	int i = 0, err = 0;

	inclk = INCLK_NONE;
	outclk = OUTCLK_ASRCK1_CLK;
	convert_flag = 0;
	printf("Hi... \n");

	if (ac < 5) {
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

	if (ac > 5)
	{
		i = atoi(av[5]);
		switch (i)
		{
			case 0:
			    inclk = INCLK_NONE;
			    printf("inclk : INCLK_NONE\n");
			    break;
			case 1:
			    inclk = INCLK_ESAI_RX;
			    printf("inclk : INCLK_ESAI_RX\n");
			    break;
			case 2:
			    inclk = INCLK_SSI1_RX;
			    printf("inclk : INCLK_SSI1_RX\n");
			    break;
			case 3:
			    inclk = INCLK_SSI2_RX;
			    printf("inclk : INCLK_SSI2_RX\n");
			    break;
			case 4:
			    inclk = INCLK_SPDIF_RX;
			    printf("inclk : INCLK_SPDIF_RX\n");
			    break;
			case 5:
			    inclk = INCLK_MLB_CLK;
			    printf("inclk : INCLK_MLB_CLK\n");
			    break;
			case 6:
			    inclk = INCLK_ESAI_TX;
			    printf("inclk : INCLK_ESAI_TX\n");
			    break;
			case 7:
			    inclk = INCLK_SSI1_TX;
			    printf("inclk : INCLK_SSI1_TX\n");
			    break;
			case 8:
			    inclk = INCLK_SSI2_TX;
			    printf("inclk : INCLK_SSI2_TX\n");
			    break;
			case 9:
			    inclk = INCLK_SPDIF_TX;
			    printf("inclk : INCLK_SPDIF_TX\n");
			    break;
			case 10:
			    inclk = INCLK_ASRCK1_CLK;
			    printf("inclk : INCLK_ASRCK1_CLK\n");
			    break;
			default:
			    printf("Incorrect clock source\n");
			    return;
		}

		i = atoi(av[6]);
		switch (i)
		{
			case 0:
			    outclk = OUTCLK_NONE;
			    printf("outclk : OUTCLK_NONE\n");
			    break;
			case 1:
			    outclk = OUTCLK_ESAI_TX;
			    printf("outclk : OUTCLK_ESAI_TX\n");
			    break;
			case 2:
			    outclk = OUTCLK_SSI1_TX;
			    printf("outclk : OUTCLK_SSI1_TX\n");
			    break;
			case 3:
			    outclk = OUTCLK_SSI2_TX;
			    printf("outclk : OUTCLK_SSI2_TX\n");
			    break;
			case 4:
			    outclk = OUTCLK_SPDIF_TX;
			    printf("outclk : OUTCLK_SPDIF_TX\n");
			    break;
			case 5:
			    outclk = OUTCLK_MLB_CLK;
			    printf("outclk : OUTCLK_MLB_CLK\n");
			    break;
			case 6:
			    outclk = OUTCLK_ESAI_RX;
			    printf("outclk : OUTCLK_ESAI_RX\n");
			    break;
			case 7:
			    outclk = OUTCLK_SSI1_RX;
			    printf("outclk : OUTCLK_SSI1_RX\n");
			    break;
			case 8:
			    outclk = OUTCLK_SSI2_RX;
			    printf("outclk : OUTCLK_SSI2_RX\n");
			    break;
			case 9:
			    outclk = OUTCLK_SPDIF_RX;
			    printf("outclk : OUTCLK_SPDIF_RX\n");
			    break;
			case 10:
			    outclk = OUTCLK_ASRCK1_CLK;
			    printf("outclk : OUTCLK_ASRCK1_CLK\n");
			    break;
			default:
			    printf("Incorrect clock source\n");
			    return ;
		}
	}

	if ((fd_dst = fopen(av[5 - 1], "wb+")) <= 0) {
		goto err_dst_not_found;
	}

	if ((fd_src = fopen(av[5 - 2], "r")) <= 0) {
		goto err_src_not_found;
	}

	if ((header_parser(fd_src, &audio_info)) <= 0) {
		goto end_err;
	}

	bitshift(fd_src, &audio_info);

	err = configure_asrc_channel(fd_asrc, &audio_info);

	if (err < 0)
		goto end_err;

	/* Config HW */
	err += play_file(fd_asrc, &audio_info);

	if (err < 0)
		goto end_err;
	convert_data(fd_dst, &audio_info);

	fclose(fd_src);
	fclose(fd_dst);
	close(fd_asrc);

	free(input_buffer);
	free(output_buffer);
	printf("All tests passed with success\n");
	return 0;

      end_err:
	fclose(fd_src);
      err_src_not_found:
	fclose(fd_dst);
      err_dst_not_found:
	ioctl(fd_asrc, ASRC_RELEASE_PAIR, &pair_index);
	close(fd_asrc);
	return err;
}

void *asrc_input_thread(void *info)
{
	int err;
	struct audio_info_s *pinfo = (struct audio_info_s *)info;
	struct asrc_buffer inbuf;
	unsigned int len;
	char *input_p;

	while (convert_flag) {
		if ((err = ioctl(fd_asrc, ASRC_DQ_INBUF, &inbuf)) < 0) {
			printf("ASRC_DQ_INBUF error!\r\n");
			goto exit;
		}
		input_p = (char *)input_buffer + pinfo->input_used;
		len = pinfo->input_data_len - pinfo->input_used;

		if(len >= DMA_BUF_SIZE) {
			memcpy(input_buf[inbuf.index].start, input_p,
					DMA_BUF_SIZE);
			pinfo->input_used += DMA_BUF_SIZE;
		} else {
			memcpy(input_buf[inbuf.index].start, input_p, len);
			pinfo->input_used += len;
		}
		if ((err = ioctl(fd_asrc, ASRC_Q_INBUF, &inbuf)) < 0) {
			printf("ASRC_Q_INBUF error!\r\n");
			goto exit;
		}
	}
exit:
	return NULL;
}

void *asrc_output_thread(void *info)
{
	int err;
	struct audio_info_s *pinfo = (struct audio_info_s *)info;
	struct asrc_buffer outbuf,inbuf;
	char *output_p, *input_p;
	unsigned int len;
	int output_dma_si = pinfo->output_dma_size;

	while(convert_flag) {
		if ((err = ioctl(fd_asrc, ASRC_DQ_OUTBUF, &outbuf)) < 0) {
			printf("ASRC_DQ_OUTBUF error!\r\n");
			goto exit;
		}
		output_p = (char *)output_buffer + pinfo->output_used;
		len = pinfo->output_data_len - pinfo->output_used;
		if(len >= output_dma_si) {
			memcpy(output_p,output_buf[outbuf.index].start,
				       	output_dma_si);
			pinfo->output_used += output_dma_si;
		} else {
			memcpy(output_p, output_buf[outbuf.index].start, len);
			pinfo->output_used += len;
		}
		if(pinfo->output_used == pinfo->output_data_len) {
			convert_flag = 0;
			break;
		}
		if ((err = ioctl(fd_asrc, ASRC_Q_OUTBUF, &outbuf)) < 0) {
			printf("ASRC_Q_OUTBUF error!\r\n");
			goto exit;
		}

	}
exit:
	return NULL;
}
