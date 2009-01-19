/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All rights reserved.
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
#include <sys/ioctl.h>
#include <string.h>
#include "soundcard.h"
#include "audio_controls.h"
#define BUF_SIZE (1024 * 32)
void write_wav_header(FILE * fd_file, int *totalread, unsigned short *chan,
		      int *frequency, unsigned short *bits)
{
	char riff[] = { 'R', 'I', 'F', 'F' };
	char wave_fmt[] = { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' };
	char data[] = { 'd', 'a', 't', 'a' };
	int file_size = *totalread + 44;
	int byte_rate = *frequency * (*bits / 8);
	int temp;

	fseek(fd_file, 0, SEEK_SET);
	fwrite(riff, 4, 1, fd_file);	/* pos 0 */
	fwrite(&file_size, 4, 1, fd_file);	/* pos 4 */
	fwrite(wave_fmt, 8, 1, fd_file);	/* pos 8 */
	temp = 0x00000010;
	fwrite(&temp, 4, 1, fd_file);	/* pos 16 */
	temp = 0x0001;
	fwrite(&temp, 2, 1, fd_file);	/* pos 20 */
	fwrite(chan, 2, 1, fd_file);	/* pos 22 */
	fwrite(frequency, 4, 1, fd_file);	/* pos 24 */
	fwrite(&byte_rate, 4, 1, fd_file);	/* pos 28 */
	temp = 0x0004;
	fwrite(&temp, 2, 1, fd_file);	/* pos 32 */
	fwrite(bits, 2, 1, fd_file);	/* pos 34 */
	fwrite(data, 4, 1, fd_file);	/* pos 36 */
	fwrite(totalread, 4, 1, fd_file);	/* pos 40 */
}

int main(int ac, char *av[])
{
	unsigned short chan, bits;
	int remaining_bytes, bytes_read;
	int frequency, channels, bytes;
	int bytes_to_read, total_bytes;
	unsigned short buf[BUF_SIZE];
	int val, res, input_dev;
	int bytes_written;
	FILE * fd_file;
	int fd_audio;
        int format;

	printf("Hi... \n");

	if (ac != 6) {

#ifdef CONFIG_MX21ADS
		    printf
		    ("Records an audio file from /dev/sound/dsp (uncompressed data)\n");
#else
		    printf
		    ("Records an audio file from /dev/sound/dsp1 (uncompressed data)\n");
#endif
		printf
		    ("Usage: %s <soundfile.wav> <input_dev> <bytes> <frequency> <channels>\n",
		     av[0]);
		printf("soundfile.wav    : wav file to be created\n\n");
#ifdef CONFIG_MX21ADS
		printf
		    ("input_dev options: 1 (handphone, for mono/stereo recording)\n\n");

#else
		printf
		    ("input_dev options: 1 (handset, J4, for mono/stereo recording)\n");
		printf
		    ("                   2 (headset, J3, for mono recording)\n");
		printf
		    ("		   3 (line in, J7, for mono recording)\n\n");

#endif
		    printf("bytes            : Amount of bytes to record\n\n");
		printf("frequency options: 8000\n");

#ifdef CONFIG_MX21ADS
		    printf("                   44100\n\n");

#else
		    printf("                   16000\n\n");

#endif
		    printf("channels options : 1 (mono)\n");
		printf
		    ("                   2 (stereo, ONLY WORKS for input_dev = 1)\n\n");
		return 0;
	}
#ifdef CONFIG_MX21ADS
	input_dev = 1;
#else
	input_dev = atoi(av[2]);

#endif
	    bytes = atoi(av[3]);
	frequency = atoi(av[4]);
	channels = atoi(av[5]);

#ifdef CONFIG_MX21ADS
	    if ((fd_audio = open("/dev/sound/dsp", O_RDONLY)) < 0) {
		printf("Error opening /dev/sound/dsp");
		return 1;
	}
#else
	    if ((fd_audio = open("/dev/sound/dsp", O_RDONLY)) < 0) {
		printf("Error opening /dev/sound/dsp");
		return 1;
	}
#endif

	if ((fd_file = fopen(av[1], "w+")) <= 0) {
		printf("Error when creating the sample file");
		goto error1;
	}
	if ((channels != 2) && (channels != 1)) {
		printf("channels error: %d\n", channels);
		goto error2;
	}
	if ((channels == 2) && (input_dev != 1)) {
		printf("Stereo recording not supported for this device, \
				using mono instead\n");
		channels = 1;
	}

	printf("frequency = %d, bytes = %d, input_dev = %d\n", frequency,
	       bytes, input_dev);

#ifdef CONFIG_MX21ADS
	    if ((frequency != 44100) && (frequency != 8000)) {

#else
	    if ((frequency != 16000) && (frequency != 8000)) {

#endif
		    printf("frequency error: %d\n", frequency);
		goto error2;
	}

        format = AFMT_S16_LE;
        res = ioctl(fd_audio, SNDCTL_DSP_SETFMT, &format);
        if (res < 0) {
                printf("Sound format set error\n");
                goto error2;
        }
	res = ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &frequency);
	if (res < 0) {
		printf("SOUND_PCM_WRITE_RATE error\n");
		goto error2;
	}
	res = ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &channels);
	if (res < 0) {
		printf("SOUND_PCM_WRITE_CHANNELS error\n");
		goto error2;
	}

	/* set the intput src */
#ifdef CONFIG_MX21ADS
	val = SOUND_MASK_MIC;
#else
	switch (input_dev) {
	case 1:
		val = SOUND_MASK_PHONEIN;
		break;
	case 2:
		val = SOUND_MASK_MIC;
		break;
	case 3:
		val = SOUND_MASK_LINE;
		break;
	default:
		printf("Unknown input src, using the default one\n");
		val = SOUND_MASK_MIC;
	}

#endif
	    res = ioctl(fd_audio, SOUND_MIXER_WRITE_RECSRC, &val);
	if (res < 0) {
		printf("SOUND_MIXER_WRITE_RECSRC error\n");
		goto error2;
	}

	    /* write the WAV header, it will be filled in later on */
	    memset(buf, 0, BUF_SIZE);
	fwrite(buf, 44, 1, fd_file);
	bytes_to_read = remaining_bytes = bytes;
	while (remaining_bytes > 0) {

		    /* read samples */
		    if (bytes_to_read > BUF_SIZE) {
			bytes_to_read = BUF_SIZE;
		}
		bytes_read = read(fd_audio, buf, bytes_to_read);
		if (bytes_read < 0) {
			printf("Error reading audio data \n");
			break;
		}

		printf("%d bytes read from the device\n", bytes_read);
		remaining_bytes -= bytes_read;
		bytes_to_read = remaining_bytes;
		total_bytes += bytes_read;

		/* write the samples in file */
		    bytes_written = fwrite(buf, 1, bytes_read, fd_file);
		if (bytes_written < 0) {
			perror("/dev/audio\n");
		}
	}

	printf(" total bytes read: (%d)", total_bytes);

	    /* Fill in the wav header */
	    bits = 16;
	chan = channels;
	write_wav_header(fd_file, &total_bytes, &chan, &frequency, &bits);

	printf("Closing the device\n");
	return 0;
      error2:printf("Unsuccessful operation (2)\n");
	fclose(fd_file);
      error1:printf("Unsuccessful operation (1)\n");
	close(fd_audio);
	return 1;
}
