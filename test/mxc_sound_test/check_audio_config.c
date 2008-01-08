/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All rights reserved.
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
 *
 * Modification History
 *	2004 July, 1st, Creation: STDAC test
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include "include/linux/soundcard.h"

#include "dbmx31-ctrls.h"	/* header for usefull oss and mc13783 ioctl plus mask parameters */

#define BUF_SIZE 4096

#define STDAC				0
#define CODEC				1
#define STDAC_AND_CODEC	                2
#define ADDER				3

#define FALSE				0
#define TRUE				1

/* All parameters for the test, usefull when using threads, should be the own thread datas */
typedef struct {
	int device;
	int increment;
	char *testfilename;	/* test file name */
	FILE *fd_file;		/* test file pointer */

} test_parameters;

/* structure that groups all audio settings */
typedef struct {
	unsigned int volume;
	unsigned int balance;
	unsigned int adder;
	unsigned int channels;
	unsigned int sample_rate;
	unsigned int format;
	int active_output;
	int active_input;
	int codec_filter;

} audio_settings;

static const char *progname;	/* program name for error messages */
static audio_settings current_driver_settings;	/* Current settings of the audio HW */

void usage(void)
{
	fprintf(stderr, "usage: %s [switches] \n", progname);
	fprintf(stderr, "Switches (names may be abbreviated):\n");
	fprintf(stderr, "  -device n	Device to test\n");
	fprintf(stderr, "                                0 - STDAC\n");
	fprintf(stderr,
		"                                1 - CODEC (default)\n");
	fprintf(stderr,
		"                                2 - STDAC & CODEC (not simultanously)\n");
	fprintf(stderr, "                                3 - ADDER\n");
	fprintf(stderr,
		"  -inc		Select the increment of volume and balance (default 5)\n");
	fprintf(stderr,
		"  The number of increment impact of the length of the tests\n");
	fprintf(stderr, "  that are run between 0 and 100\n");
	fprintf(stderr,
		"  -file		Select the test file (default ringtone)\n");

	exit(EXIT_FAILURE);
}

int keymatch(char *arg, const char *keyword, int minchars)
{
	register int ca, ck;
	register int nmatched = 0;

	while ((ca = *arg++) != '\0') {
		if ((ck = *keyword++) == '\0')
			return FALSE;	/* arg longer than keyword, no good */
		if (isupper(ca))	/* force arg to lcase (assume ck is already) */
			ca = tolower(ca);
		if (ca != ck)
			return FALSE;	/* no good */
		nmatched++;	/* count matched characters */
	}
	/* reached end of argument; fail if it's too short for unique abbrev */
	if (nmatched < minchars)
		return FALSE;
	return TRUE;		/* A-OK */
}

int parse_switches(test_parameters * params, int argc, char **argv,
		   int last_file_arg_seen, int for_real)
{
	int argn;
	char *arg;

/* Set default test parameter values */
	params->testfilename = "ringout.wav";
	params->increment = 10;
	params->device = STDAC;

/* Scan command line options, adjust parameters */
	if (argc == 1) {
		usage();
		return 0;
	}

	for (argn = 1; argn < argc; argn++) {
		arg = argv[argn];
		if (*arg != '-') {
			/* Not a switch, must be a file name argument */
			if (argn <= last_file_arg_seen) {
				params->testfilename = NULL;	/* -outfile applies to just one input file */
				continue;	/* ignore this name if previously processed */
			}
			break;	/* else done parsing switches */
		}
		arg++;		/* advance past switch marker character */

		if (keymatch(arg, "-help", 4)) {
			usage();

		}

		if (keymatch(arg, "file", 4)) {
			/* Set the test file name. */
			if (++argn >= argc)	/* advance to next argument */
				usage();
			params->testfilename = argv[argn];	/* save it away for later use */

		}

		else if (keymatch(arg, "inc", 3)) {
			/* The increment to use for volume and balance test */
			if (++argn >= argc)	/* advance to next argument */
				usage();
			{
				int n;
				if (sscanf(argv[argn], "%d", &n) != 1)
					usage();

				if (n > 100)
					usage();
				else
					params->increment = n;

			}
		}

		else if (keymatch(arg, "device", 3)) {
			/* The device to test */
			if (++argn >= argc)	/* advance to next argument */
				usage();
			{
				int n;
				if (sscanf(argv[argn], "%d", &n) != 1)
					usage();

				if (n > 3)
					usage();
				else
					params->device = n;
			}
		}

		else {
			usage();	/* bogus switch */
		}
	}

	return argn;		/* return index of next arg (file name) */
}

void detect_enter(int time_out)
{
	int fd_console = 0;	/* 0 is the video input */
	fd_set fdset;
	struct timeval timeout;
	char c;

	FD_ZERO(&fdset);
	FD_SET(fd_console, &fdset);
	timeout.tv_sec = time_out;	/* set timeout !=0 => blocking select */
	timeout.tv_usec = 0;
	if (select(fd_console + 1, &fdset, 0, 0, &timeout) > 0) {
		do {
			read(fd_console, &c, 1);
		}
		while (c != 10);	// i.e. line-feed 
	}
}

unsigned short get_audio_bits(FILE * file)
{
	unsigned short ret;
	fseek(file, 34, SEEK_SET);
	fread(&ret, 2, 1, file);
	return ret;
}

unsigned short get_audio_channels(FILE * file)
{
	unsigned short ret;
	fseek(file, 22, SEEK_SET);
	fread(&ret, 2, 1, file);
	return ret;
}

int get_audio_frq(FILE * file)
{
	int ret;
	fseek(file, 24, SEEK_SET);
	fread(&ret, 4, 1, file);
	return ret;
}

int set_settings(int fd_audio, int fd_mixer, audio_settings settings_to_apply)
{
	int err = 0, ret = 0;

	/* for each settings, test if it has changed from the current configuration */

	/* Check if the format has been changed */
	if (settings_to_apply.format != current_driver_settings.format) {
		printf("\t Updating format to 0x%x\n",
		       settings_to_apply.format);
		current_driver_settings.format = settings_to_apply.format;
		ret =
		    ioctl(fd_audio, SNDCTL_DSP_SETFMT,
			  &settings_to_apply.format);
		if (settings_to_apply.format != current_driver_settings.format) {
			printf("Error with IOCTL SNDCTL_DSP_SETFMT\n");
			err++;
		}
	}

	/* Check if the sample_rate has been changed */
	if (settings_to_apply.sample_rate !=
	    current_driver_settings.sample_rate) {
		printf("\t Updating sample rate to %d\n",
		       settings_to_apply.sample_rate);
		current_driver_settings.sample_rate =
		    settings_to_apply.sample_rate;
		ret =
		    ioctl(fd_audio, SOUND_PCM_WRITE_RATE,
			  &settings_to_apply.sample_rate);
		if (settings_to_apply.sample_rate !=
		    current_driver_settings.sample_rate) {
			printf("Error with IOCTL SOUND_PCM_WRITE_RATE\n");
			err++;
		}
	}

	/* Check if the channels used has been changed */
	if (settings_to_apply.channels != current_driver_settings.channels) {
		printf("\t Updating channels to %d\n",
		       settings_to_apply.channels);
		current_driver_settings.channels = settings_to_apply.channels;
		ret =
		    ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS,
			  &settings_to_apply.channels);
		if (settings_to_apply.channels !=
		    current_driver_settings.channels) {
			printf("Error with IOCTL SOUND_PCM_WRITE_CHANNELS\n");
			err++;
		}
	}

	/* Check if the active output has been activated/disactivated */
	if (settings_to_apply.active_output !=
	    current_driver_settings.active_output) {
		printf("\t Updating active output to 0x%x\n",
		       settings_to_apply.active_output);
		current_driver_settings.active_output =
		    settings_to_apply.active_output;
		ret =
		    ioctl(fd_mixer, SOUND_MIXER_WRITE_OUTSRC,
			  &settings_to_apply.active_output);
		if (settings_to_apply.active_output !=
		    current_driver_settings.active_output) {
			printf("Error with IOCTL SOUND_MIXER_WRITE_OUTSRC\n");
			err++;
		}
	}

	/* Check if the volume has been changed */
	if (settings_to_apply.volume != current_driver_settings.volume) {
		printf("\t Updating volume to %d\n",
		       settings_to_apply.volume >> 8);
		current_driver_settings.volume = settings_to_apply.volume;
		/* for sure it is not useful to apply the volume to all connected device, just once is enought!!! */

		/* if the earpiece is active, change the appropriate volume */
		if (settings_to_apply.active_output & SOUND_MASK_VOLUME) {
			printf("Updating volume for headset\n");
			ret =
			    ioctl(fd_mixer, SOUND_MIXER_WRITE_VOLUME,
				  &settings_to_apply.volume);
			if (settings_to_apply.volume !=
			    current_driver_settings.volume) {
				printf
				    ("Error with IOCTL SOUND_MIXER_WRITE_VOLUME %d %d\n",
				     settings_to_apply.volume,
				     current_driver_settings.volume);
				err++;
			}
		}
		/* if the handsfree is active, change the appropriate volume */
		if (settings_to_apply.active_output & SOUND_MASK_SPEAKER) {
			ret =
			    ioctl(fd_mixer, SOUND_MIXER_WRITE_SPEAKER,
				  &settings_to_apply.volume);
			if (settings_to_apply.volume !=
			    current_driver_settings.volume) {
				printf
				    ("Error with IOCTL SOUND_MIXER_WRITE_SPEAKER\n");
				err++;
			}
		}
		/* if the headset is active, change the appropriate volume */
		if (settings_to_apply.active_output & SOUND_MASK_PCM) {
			ret =
			    ioctl(fd_mixer, SOUND_MIXER_WRITE_PCM,
				  &settings_to_apply.volume);
			if (settings_to_apply.volume !=
			    current_driver_settings.volume) {
				printf
				    ("Error with IOCTL SOUND_MIXER_WRITE_PCM\n");
				err++;
			}
		}
		/* if the lineout is active, change the appropriate volume */
		if (settings_to_apply.active_output & SOUND_MASK_PHONEOUT) {
			ret =
			    ioctl(fd_mixer, SOUND_MIXER_WRITE_PHONEOUT,
				  &settings_to_apply.volume);
			if (settings_to_apply.volume !=
			    current_driver_settings.volume) {
				printf
				    ("Error with IOCTL SOUND_MIXER_WRITE_PHONEOUT\n");
				err++;
			}
		}
	}

	/* Check if the balance has been changed */
	if (settings_to_apply.balance != current_driver_settings.balance) {
		printf("\t Updating balance to %d\n",
		       settings_to_apply.balance);
		current_driver_settings.balance = settings_to_apply.balance;
		ret =
		    ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_BALANCE,
			  &settings_to_apply.balance);
		if (settings_to_apply.balance !=
		    current_driver_settings.balance) {
			printf
			    ("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
			err++;
		}
	}

	/* Check if the codec filter configuration has been changed */
	if (settings_to_apply.codec_filter !=
	    current_driver_settings.codec_filter) {
		printf("\t Updating codec filter to 0x%x\n",
		       settings_to_apply.codec_filter);
		current_driver_settings.codec_filter =
		    settings_to_apply.codec_filter;
		ret =
		    ioctl(fd_audio, SNDCTL_MC13783_WRITE_CODEC_FILTER,
			  &settings_to_apply.codec_filter);
		if (settings_to_apply.codec_filter !=
		    current_driver_settings.codec_filter) {
			printf
			    ("Error with IOCTL SNDCTL_MC13783_WRITE_CODEC_FILTER\n");
			err++;
		}
	}

	printf("End of set settings\n");

	return err;
}

int get_settings(int fd_audio, int fd_mixer, audio_settings * setttings)
{
	int err = 0;
	audio_settings audio_tmp;
	int ret;

	printf("Default Audio configuration when opening the audio driver\n");

	/* Get the default balance */
	ret =
	    ioctl(fd_audio, SNDCTL_MC13783_READ_OUT_BALANCE,
		  &current_driver_settings.balance);
	printf("\t Balance: %d\n", current_driver_settings.balance);

	/* Get the default channels settings */
	ret =
	    ioctl(fd_audio, SOUND_PCM_READ_CHANNELS,
		  &current_driver_settings.channels);
	printf("\t Channels: %d\n", current_driver_settings.channels);

	/* Get the default sample rate */
	ret =
	    ioctl(fd_audio, SOUND_PCM_READ_RATE,
		  &current_driver_settings.sample_rate);
	printf("\t Sample rate: %d\n", current_driver_settings.sample_rate);

	/* Get the default active output */
	ret =
	    ioctl(fd_mixer, SOUND_MIXER_READ_OUTSRC,
		  &current_driver_settings.active_output);
	printf("\t Active Output: %d\n", current_driver_settings.active_output);

	/* Get the default active input */
	ret =
	    ioctl(fd_mixer, SOUND_MIXER_READ_RECSRC,
		  &current_driver_settings.active_input);
	printf("\t Active Input: %d\n", current_driver_settings.active_input);

	/* Get the default output volume */
	current_driver_settings.volume = 0;

	/* Get the default input gain */

	/* Get the mc13783 mono adder configuration */
	ret =
	    ioctl(fd_audio, SNDCTL_MC13783_READ_OUT_ADDER,
		  &current_driver_settings.adder);
	printf("\t Adder: %d\n", current_driver_settings.adder);

	/* Get the default codec filter configuration */
	ret =
	    ioctl(fd_audio, SNDCTL_MC13783_READ_CODEC_FILTER,
		  &current_driver_settings.codec_filter);
	printf("\t Codec filter: %d\n", current_driver_settings.codec_filter);

	/* Get the default format */
	ret =
	    ioctl(fd_audio, SOUND_PCM_READ_BITS,
		  &current_driver_settings.format);
	printf("\t Format: %d\n\n", current_driver_settings.format);

	*setttings = current_driver_settings;

	/* To be sure that the audio driver send the real HW value, apply them now! */
	audio_tmp = current_driver_settings;

	ret = ioctl(fd_audio, SOUND_PCM_WRITE_BITS, &audio_tmp.format);
	if (audio_tmp.format != current_driver_settings.format) {
		printf("Error with IOCTL SOUND_PCM_WRITE_BITS\n");
		err++;
	}

	ret = ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &audio_tmp.sample_rate);
	if (audio_tmp.sample_rate != current_driver_settings.sample_rate) {
		printf("Error with IOCTL SOUND_PCM_WRITE_RATE\n");
		err++;
	}

	ret = ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &audio_tmp.channels);
	if (audio_tmp.channels != current_driver_settings.channels) {
		printf("Error with IOCTL SOUND_PCM_WRITE_CHANNELS\n");
		err++;
	}

	ret =
	    ioctl(fd_mixer, SOUND_MIXER_WRITE_OUTSRC, &audio_tmp.active_output);
	if (audio_tmp.active_output != current_driver_settings.active_output) {
		printf("Error with IOCTL SOUND_MIXER_WRITE_OUTSRC\n");
		err++;
	}

	ret =
	    ioctl(fd_mixer, SOUND_MIXER_WRITE_RECSRC, &audio_tmp.active_input);
	if (audio_tmp.active_input != current_driver_settings.active_input) {
		printf("Error with IOCTL SOUND_MIXER_WRITE_RECSRC\n");
		err++;
	}

	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_ADDER, &audio_tmp.adder);
	if (audio_tmp.adder != current_driver_settings.adder) {
		printf("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_ADDER\n");
		err++;
	}

	ret =
	    ioctl(fd_audio, SNDCTL_MC13783_WRITE_CODEC_FILTER,
		  &audio_tmp.codec_filter);
	if (audio_tmp.codec_filter != current_driver_settings.codec_filter) {
		printf("Error with IOCTL SNDCTL_MC13783_WRITE_CODEC_FILTER\n");
		err++;
	}

	return err;
}

int set_audio_config(int fd_audio, int fd_mixer, FILE * file)
{
	int tmp, format, frequency, channels, ret = 0;
	audio_settings audio_set;

	ret += get_settings(fd_audio, fd_mixer, &audio_set);

	tmp = format = (int)get_audio_bits(file);
	audio_set.format = format;
	if (tmp != format)
		return -1;

	tmp = frequency = get_audio_frq(file);
	audio_set.sample_rate = frequency;
	if (tmp != frequency)
		return -1;

	tmp = channels = get_audio_channels(file);
	audio_set.channels = channels;
	if (tmp != channels)
		return -1;

	ret += set_settings(fd_audio, fd_mixer, audio_set);

	printf("Configuration : %d %d %d\n", format, frequency, channels);
	return ret;
}

int play_pouet(int fd_audio, FILE * file)
{
	int err = 0;
	int n, nleft, nwritten;
	char buf[BUF_SIZE], *p;

	// On passe l'entete WAV
	fseek(file, 44, SEEK_SET);

	// Lecture de l'échantillon
	while ((n = fread(buf, 1, sizeof(buf), file)) > 0) {
		nleft = n;
		p = buf;

		// On envoie l'échantillon...
		while (nleft) {
			if ((nwritten = write(fd_audio, p, nleft)) < 0) {
				perror("/dev/sound/dsp0 write error");
				err++;
			}
//                      else
//                              printf ("%d/%d written.\n", nwritten, nleft);

			nleft -= nwritten;
			p += nwritten;
		}
	}

	return err;
}

int volume_test(int fd_audio, int fd_mixer, audio_settings audio_config,
		test_parameters params)
{
	int vol = 0, err = 0;
	audio_config.volume = (vol << 8) | vol;

	while (vol <= 100) {
		err += set_settings(fd_audio, fd_mixer, audio_config);
		err += play_pouet(fd_audio, params.fd_file);
		vol += params.increment;
		audio_config.volume = (vol << 8) | vol;
		printf("\n Press enter to play @ new volume.....timeout 2s \n");
		detect_enter(2);
	}

	return err;
}

int balance_test(int fd_audio, int fd_mixer, audio_settings audio_config,
		 test_parameters params)
{
	int err = 0;
	audio_config.balance = 0;
	audio_config.volume = (100 << 8) | 100;

	while (audio_config.balance <= 100) {
		err += set_settings(fd_audio, fd_mixer, audio_config);
		err += play_pouet(fd_audio, params.fd_file);
		audio_config.balance += params.increment;
		printf
		    ("\n Press enter to play @ new balance.....timeout 2s \n");
		detect_enter(2);
	}

	return err;
}

int codec_filter_test(int fd_audio, int fd_mixer, audio_settings audio_config,
		      test_parameters params)
{
	int err = 0;

	printf("\n filter test in high pass in\n");
	audio_config.adder = MC13783_CODEC_FILTER_HIGH_PASS_IN;
	err += set_settings(fd_audio, fd_mixer, audio_config);
	err += play_pouet(fd_audio, params.fd_file);
	printf("\n Look at the signal on the scope and press enter\n");
	detect_enter(2);

	printf("\n filter test in high pass out \n");
	audio_config.adder = MC13783_CODEC_FILTER_HIGH_PASS_OUT;
	err += set_settings(fd_audio, fd_mixer, audio_config);
	err += play_pouet(fd_audio, params.fd_file);
	printf("\n Look at the signal on the scope and press enter\n");
	detect_enter(2);

	printf("\n filter test in fitler dithering \n");
	audio_config.adder = MC13783_CODEC_FILTER_DITHERING;
	err += set_settings(fd_audio, fd_mixer, audio_config);
	err += play_pouet(fd_audio, params.fd_file);
	printf("\n Look at the signal on the scope and press enter\n");
	detect_enter(2);

	return err;
}

int adder_test(int fd_audio, int fd_mixer, audio_settings audio_config,
	       test_parameters params)
{
	int err = 0;

	audio_config.volume = (25 << 8) | 25;
	audio_config.balance = 50;

	printf("\n Adder test in stereo mode \n");
	audio_config.adder = MC13783_AUDIO_ADDER_STEREO;
	err += set_settings(fd_audio, fd_mixer, audio_config);
	err += play_pouet(fd_audio, params.fd_file);
	printf("\n Look at the signal on the scope and press enter\n");
	detect_enter(2);

	printf("\n Adder test in stereo opposite mode \n");
	audio_config.adder = MC13783_AUDIO_ADDER_STEREO_OPPOSITE;
	err += set_settings(fd_audio, fd_mixer, audio_config);
	err += play_pouet(fd_audio, params.fd_file);
	printf("\n Look at the signal on the scope and press enter\n");
	detect_enter(2);

	printf("\n Adder test in mono mode \n");
	audio_config.adder = MC13783_AUDIO_ADDER_MONO;
	err += set_settings(fd_audio, fd_mixer, audio_config);
	err += play_pouet(fd_audio, params.fd_file);
	printf("\n Look at the signal on the scope and press enter\n");
	detect_enter(2);

	printf("\n Adder test in mono opposite mode \n");
	audio_config.adder = MC13783_AUDIO_ADDER_MONO_OPPOSITE;
	err += set_settings(fd_audio, fd_mixer, audio_config);
	err += play_pouet(fd_audio, params.fd_file);
	printf("\n Look at the signal on the scope and press enter\n");
	detect_enter(2);

	return err;
}

int volume_ioctl_test(int fd_audio, int fd_mixer, audio_settings audio_config,
		      test_parameters params)
{
	int err = 0, ret = 0, tmp = 0;

/*************************************** FIRST TEST *****************************************/
/* 2 outputs are active, set the volume on the 2 outputs at the same value */
	printf("Activate VOLUME and PHONEOUT\n");
	audio_config.active_output = SOUND_MASK_VOLUME | SOUND_MASK_PHONEOUT;
	audio_config.volume = (25 << 8) | 25;
	audio_config.balance = 50;
	printf("Volume set on both output to 25\n");
	set_settings(fd_audio, fd_mixer, audio_config);

/* Then, set the volume on 1 output to another value = 75 and read of the volume */
	audio_config.volume = 75;
	printf("Volume set on VOLUME @ 75\n");

	ret = ioctl(fd_mixer, SOUND_MIXER_WRITE_VOLUME, &audio_config.volume);
	if ((audio_config.volume & 0x00FF) != 75) {
		printf("Error with IOCTL SOUND_MIXER_WRITE_VOLUME\n");
		err++;
	}

/* on the other output to check of the volume has been updated on the 2 */
	ioctl(fd_mixer, SOUND_MIXER_READ_VOLUME, &tmp);
	printf("Volume read on VOLUME = %d\n", audio_config.volume);

/* on the other output to check of the volume has been updated on the 2 */
	ioctl(fd_mixer, SOUND_MIXER_READ_PHONEOUT, &tmp);
	printf("Volume read on PHONEOUT = %d\n", audio_config.volume);

	if (tmp != audio_config.volume)
		err++;

/************************************** SECOND TEST *****************************************/
/* Set the volume on an unactive output */
	printf("Activate VOLUME\n");
	audio_config.active_output = SOUND_MASK_VOLUME;
	audio_config.volume = (25 << 8) | 25;
	printf("Set volume @ 25 on VOLUME\n");
	set_settings(fd_audio, fd_mixer, audio_config);
	printf("Read the volume on PHONEOUT\n");
	ret = ioctl(fd_mixer, SOUND_MIXER_WRITE_PHONEOUT, &audio_config.volume);
	if (ret >= 0) {
		printf
		    ("The IOCTL should have failed since PHONEOUT were disabled\n");
		err++;
	} else
		printf("IOCTL failed since PHONEOUT were disabled\n");

	return err;

}

int main(int argc, char *argv[])
{
	int fd_audio, fd_mixer;
	int err = 0, err_1 = 0;
	audio_settings audio_config;
	test_parameters params;

	progname = argv[0];

	if (!parse_switches(&params, argc, argv, 0, FALSE))
		return 0;

/*************************************** TEST STDAC *****************************************/

	if ((params.device == STDAC) || (params.device == STDAC_AND_CODEC)) {
		printf("\n TEST SUITE on STDAC \n");

		if ((fd_audio = open("/dev/sound/dsp", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}

		if ((fd_mixer = open("/dev/sound/mixer", O_RDWR)) < 0) {
			printf("Error opening /dev/mixer");
			return 0;
		}

		/* Get the default audio settings to update the current settings structure */
		err += get_settings(fd_audio, fd_mixer, &audio_config);

		/* Set the settings (should not do anything) */
		err += set_settings(fd_audio, fd_mixer, audio_config);

		/* Open the test samples */
		if ((params.fd_file = fopen(params.testfilename, "r")) <= 0) {
			err_1++;
			goto _err_file_not_found;
		}

		/* Parse the header of the test file and update the audio configuration */
		if (set_audio_config(fd_audio, fd_mixer, params.fd_file) < 0) {
			printf("Bad format for file 1\n");
			err_1++;
			goto _err_fmt;
		}

		/* Get the modified config for playing the test file correctly */
		err += get_settings(fd_audio, fd_mixer, &audio_config);

	/**************************************** TEST IOCTL *****************************************/
		printf("\n TEST IOCTL\n");
		err_1 =
		    volume_ioctl_test(fd_audio, fd_mixer, audio_config, params);

		if (!err_1)
			printf("TEST IOCTL passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

		err += err_1;
		err_1 = 0;

	/*********************************** END OF TEST IOCTL ***************************************/

	/*************************************** TEST VOLUME ****************************************/

		/* Select the output device to activate */

		/* Earpiece: mono output */
		printf("\n TEST VOLUME on PHONEOUT\n");
		audio_config.active_output = SOUND_MASK_PHONEOUT;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Earpiece and handsfree: 2 mono outputs */
		printf("\n TEST VOLUME on PHONEOUT and HANDSFREE\n");
		audio_config.active_output =
		    SOUND_MASK_PHONEOUT | SOUND_MASK_SPEAKER;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Need to connect physically the headset to the platform */
		printf
		    ("\n To continue, please connect the headset and press enter\n");
		/* wait 100 seconds until the user connect the headset */
		/* if not done, the balance test will run on the active output */
		/* in mono mode and the user will just remark a volume ramp up */
		detect_enter(100);

		/* Headset and earpiece: 1 stereo and 1 mono outputs */
		printf("\n TEST VOLUME on HEADSET and EARPIECE\n");
		audio_config.active_output =
		    SOUND_MASK_VOLUME | SOUND_MASK_PHONEOUT;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Headset: stereo output */
		printf("\n TEST VOLUME on HEADSET\n");
		audio_config.active_output = SOUND_MASK_VOLUME;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Headset and line out: 2 stereo outputs */
		printf("\n TEST VOLUME on HEADSET and LINEOUT \n");
		audio_config.active_output = SOUND_MASK_PCM | SOUND_MASK_VOLUME;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		if (!err_1)
			printf("TEST VOLUME passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

		err += err_1;
		err_1 = 0;

	/********************************* END OF TEST VOLUME ***************************************/

	/************************************** TEST BALANCE ****************************************/

		/* Select the output device to activate */
		printf("\n TEST BALANCE on HEADSET\n");
		audio_config.active_output = SOUND_MASK_VOLUME;
		err_1 += balance_test(fd_audio, fd_mixer, audio_config, params);

		printf("\n TEST BALANCE on LINEOUT\n");
		audio_config.active_output = SOUND_MASK_PCM;
		err_1 += balance_test(fd_audio, fd_mixer, audio_config, params);

		if (!err_1)
			printf("TEST BALANCE passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

		err += err_1;
		err_1 = 0;

	/******************************* END OF TEST BALANCE ****************************************/

		fclose(params.fd_file);
		close(fd_audio);
		close(fd_mixer);

		if (!err)
			printf("All tests on STDAC passed with success\n");
		else
			printf("Encountered %d errors\n", err);

	}

/*************************************** END TEST STDAC**************************************/

	err += err_1;
	err_1 = 0;

/*************************************** TEST CODEC *****************************************/

	if ((params.device == CODEC) || (params.device == STDAC_AND_CODEC)) {
		printf("\n TEST SUITE on CODEC \n");

		if ((fd_audio = open("/dev/sound/dsp", O_RDWR)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}

		if ((fd_mixer = open("/dev/sound/mixer", O_RDWR)) < 0) {
			printf("Error opening /dev/mixer");
			return 0;
		}

		/* The test file played with the codec must be mono */
		/* Get the default audio settings to update the current settings structure */
		err += get_settings(fd_audio, fd_mixer, &audio_config);

		/* Set the settings (should not do anything) */
		err += set_settings(fd_audio, fd_mixer, audio_config);

		/* Open the test samples */
		if ((params.fd_file = fopen(params.testfilename, "r")) <= 0) {
			err_1++;
			goto _err_file_not_found;
		}

		/* Parse the header of the test file and update the audio configuration */
		if (set_audio_config(fd_audio, fd_mixer, params.fd_file) < 0) {
			printf("Bad format for file 1\n");
			err_1++;
			goto _err_fmt;
		}

	/*************************************** TEST VOLUME ****************************************/

		/* Select the output device to activate */

		/* Earpiece: mono output */
		printf("\n TEST VOLUME on PHONEOUT\n");
		audio_config.active_output = SOUND_MASK_PHONEOUT;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Earpiece and handsfree: 2 mono outputs */
		printf("\n TEST VOLUME on PHONEOUT and HANDSFREE\n");
		audio_config.active_output =
		    SOUND_MASK_PHONEOUT | SOUND_MASK_SPEAKER;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Need to connect physically the headset to the platform */
		printf
		    ("\n To continue, please connect the headset and press enter\n");
		/* wait 100 seconds until the user connect the headset */
		/* if not done, the balance test will run on the active output */
		/* in mono mode and the user will just remark a volume ramp up */
		detect_enter(100);

		/* Headset and earpiece: 1 stereo and 1 mono outputs */
		printf("\n TEST VOLUME on HEADSET and EARPIECE\n");
		audio_config.active_output =
		    SOUND_MASK_VOLUME | SOUND_MASK_PHONEOUT;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Headset: stereo output */
		printf("\n TEST VOLUME on HEADSET\n");
		audio_config.active_output = SOUND_MASK_VOLUME;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		/* Headset and line out: 2 stereo outputs */
		printf("\n TEST VOLUME on HEADSET and LINEOUT \n");
		audio_config.active_output = SOUND_MASK_PCM | SOUND_MASK_VOLUME;
		err_1 += volume_test(fd_audio, fd_mixer, audio_config, params);

		if (!err_1)
			printf("TEST VOLUME passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

		err += err_1;
		err_1 = 0;

	/********************************* END OF TEST VOLUME ***************************************/

	/************************************** TEST BALANCE ****************************************/

		/* Select the output device to activate */
		printf("\n TEST BALANCE on HEADSET\n");
		audio_config.active_output = SOUND_MASK_VOLUME;
		err_1 += balance_test(fd_audio, fd_mixer, audio_config, params);

		printf("\n TEST BALANCE on LINEOUT\n");
		audio_config.active_output = SOUND_MASK_PCM;
		err_1 += balance_test(fd_audio, fd_mixer, audio_config, params);

		if (!err_1)
			printf("TEST BALANCE passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

		err += err_1;
		err_1 = 0;

	/******************************* END OF TEST BALANCE ****************************************/

	/************************************** TEST FILTER ******************************************/
		printf("\n TEST FILTER on HEADSET\n");
		audio_config.active_output = SOUND_MASK_VOLUME;
		err_1 +=
		    codec_filter_test(fd_audio, fd_mixer, audio_config, params);

		if (!err_1)
			printf("TEST FILTER passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

		err += err_1;
		err_1 = 0;

	/******************************* END OF TEST FILTER ******************************************/

	/*************************************** TEST ADDER *****************************************/
		fclose(params.fd_file);
		close(fd_audio);
		close(fd_mixer);

		if (!err_1)
			printf("All tests on CODEC passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

	}

/*************************************** END TEST CODEC**************************************/

/*************************************** TEST ADDER *****************************************/
	if (params.device == ADDER) {
		printf("\n ADDER TEST on STDAC \n");

		if ((fd_audio = open("/dev/sound/dsp", O_WRONLY)) < 0) {
			printf("Error opening /dev/dsp");
			return 0;
		}

		if ((fd_mixer = open("/dev/sound/mixer", O_RDWR)) < 0) {
			printf("Error opening /dev/mixer");
			return 0;
		}

		/* This test needs a special stereo file with 2 differents channels */
		params.testfilename = "my_adder_test_file.wav";

		/* Get the default audio settings to update the current settings structure */
		err += get_settings(fd_audio, fd_mixer, &audio_config);

		/* Set the settings (should not do anything) */
		err += set_settings(fd_audio, fd_mixer, audio_config);

		/* Open the test samples */
		if ((params.fd_file = fopen(params.testfilename, "r")) <= 0) {
			err_1++;
			goto _err_file_not_found;
		}

		/* Parse the header of the test file and update the audio configuration */
		if (set_audio_config(fd_audio, fd_mixer, params.fd_file) < 0) {
			printf("Bad format for file 1\n");
			err_1++;
			goto _err_fmt;
		}

		audio_config.volume = (75 << 8) | 75;
		audio_config.balance = 50;

		/* Play on a mono output device */
		printf("\n TEST ADDER on PHONEOUT\n");
		audio_config.active_output = SOUND_MASK_PHONEOUT;
		err_1 += adder_test(fd_audio, fd_mixer, audio_config, params);

		/* Play on a stereo output device */
		printf("\n TEST ADDER on HEADSET\n");
		audio_config.active_output = SOUND_MASK_VOLUME;
		err_1 += adder_test(fd_audio, fd_mixer, audio_config, params);

		/* Play on a mono and stereo output devices */
		printf("\n TEST ADDER on HEADSET and PHONEOUT \n");
		audio_config.active_output = SOUND_MASK_PHONEOUT
		    || SOUND_MASK_VOLUME;
		err_1 += adder_test(fd_audio, fd_mixer, audio_config, params);

		fclose(params.fd_file);
		close(fd_audio);
		close(fd_mixer);

		if (!err_1)
			printf("TEST ADDER passed with success\n");
		else
			printf("Encountered %d errors\n", err_1);

		err += err_1;
		err_1 = 0;
	}
/********************************* END OF TEST ADDER *****************************************/

	if (!err)
		printf("All tests passed with success\n");
	else
		printf("Encountered %d errors\n", err);

	return 0;

      _err_fmt:
	close(fd_audio);

      _err_file_not_found:
	printf("Error opening the file %s \n", params.testfilename);
	close(fd_audio);

	return 0;
}
