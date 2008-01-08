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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "soundcard.h"
#include "audio_controls.h"

#define DEBUG

#ifdef DEBUG
#define DPRINTF(fmt, args...) 		printf("%s: " fmt, __FUNCTION__ , ## args)
#else				//DEBUG
#define DPRINTF(fmt, args...)
#endif				//DEBUG

#define BUF_SIZE 4096

/* Tries to set the volume for a given input device */
int set_input_volume(int fd_mixer, int cmd_read, int cmd_write, int vol)
{
	int ret, err = 0;
	int mem = (vol << 8) | vol;
	vol = mem;

	/*! if set volume fails, maybe bcse device not activated */
	ret = ioctl(fd_mixer, cmd_write, &mem);
	if ((ret < 0) || (vol != mem)) {
		err++;
		DPRINTF("Failed to set the volume on the selected device\n");
	}

	ret = ioctl(fd_mixer, cmd_read, &mem);
	if (ret < 0) {
		err++;
		DPRINTF("Failed to get the volume on the selected device\n");
	}

	if ((ret >= 0) && (vol != mem)) {
		err++;
		DPRINTF("Volume set: %04x, get: %04x\n", vol, mem);
	}

	return err;
}

/* Tries to set the volume for a given output device */
int set_output_volume(int fd_mixer, int cmd_read, int cmd_write, int vol)
{
	int ret, err = 0;
	int mem = (vol << 8) | vol;
	vol = mem;

	/*! if set volume fails, maybe bcse device not activated */
	ret = ioctl(fd_mixer, cmd_write, &mem);
	if (ret < 0) {
		err++;
		DPRINTF("Failed to set the volume on the selected device\n");
	}

	ret = ioctl(fd_mixer, cmd_read, &mem);
	if (ret < 0) {
		err++;
		DPRINTF("Failed to get the volume on the selected device\n");
	}

	if ((ret >= 0) && (vol != mem)) {
		err++;
		DPRINTF("Volume set: %04x, get: %04x\n", vol, mem);
	}

	return err;
}

int set_input_device(int fd_mixer, int device)
{
	int val = device;
	int ret;
	int err = 0;

	ret = ioctl(fd_mixer, SOUND_MIXER_WRITE_RECSRC, &val);
	if ((val != device) || (ret != 0)) {
		err++;
		DPRINTF("Failed to set the input device %x\n", device);
	}

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_RECSRC, &val);
	if ((val != device) || (ret != 0)) {
		err++;
		DPRINTF("Failed to get the input device %x\n", device);
	}

	/* check also if volume settings are OK */

	switch (device) {
	case SOUND_MASK_PHONEIN:

#ifndef CONFIG_MX21ADS
		err += set_input_volume(fd_mixer, SOUND_MIXER_READ_PHONEIN,
					SOUND_MIXER_WRITE_PHONEIN, 50);

		err += set_input_volume(fd_mixer, SOUND_MIXER_READ_PHONEIN,
					SOUND_MIXER_WRITE_PHONEIN, 75);
#endif
		break;

	case SOUND_MASK_LINE:
		err += set_input_volume(fd_mixer, SOUND_MIXER_READ_LINE,
					SOUND_MIXER_WRITE_LINE, 50);
		err += set_input_volume(fd_mixer, SOUND_MIXER_READ_LINE,
					SOUND_MIXER_WRITE_LINE, 75);
		break;

	case SOUND_MASK_MIC:
#ifndef CONFIG_MX21ADS
		err += set_input_volume(fd_mixer, SOUND_MIXER_READ_MIC,
					SOUND_MIXER_WRITE_MIC, 50);
		err += set_input_volume(fd_mixer, SOUND_MIXER_READ_MIC,
					SOUND_MIXER_WRITE_MIC, 75);
#endif
		break;

	default:
		err++;
		break;

	}

	return err;
}

int set_output_device(int fd_mixer, int device)
{
	int val = device;
	int ret;
	int err = 0;

	ret = ioctl(fd_mixer, SOUND_MIXER_WRITE_OUTSRC, &val);
	if ((val != device) || (ret != 0)) {
		err++;
		DPRINTF("Failed to set the output device\n");
	}

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_OUTSRC, &val);
	if ((val != device) || (ret != 0)) {
		err++;
		DPRINTF("Failed to get the output device\n");
	}

	/* check also if volume settings are OK */
	switch (device) {
	case SOUND_MASK_SPEAKER:
#ifndef CONFIG_MX21ADS
		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_SPEAKER,
					 SOUND_MIXER_WRITE_SPEAKER, 50);
		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_SPEAKER,
					 SOUND_MIXER_WRITE_SPEAKER, 75);
#endif
		break;

	case SOUND_MASK_VOLUME:

		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_VOLUME,
					 SOUND_MIXER_WRITE_VOLUME, 50);
		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_VOLUME,
					 SOUND_MIXER_WRITE_VOLUME, 75);
		break;

	case SOUND_MASK_PCM:
#ifndef CONFIG_MX21ADS
		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_PCM,
					 SOUND_MIXER_WRITE_PCM, 50);
		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_PCM,
					 SOUND_MIXER_WRITE_PCM, 75);
#endif
		break;

	case SOUND_MASK_PHONEOUT:
#ifndef CONFIG_MX21ADS
		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_PHONEOUT,
					 SOUND_MIXER_WRITE_PHONEOUT, 50);
		err += set_output_volume(fd_mixer, SOUND_MIXER_READ_PHONEOUT,
					 SOUND_MIXER_WRITE_PHONEOUT, 75);
#endif
		break;

	default:
		err++;
		break;

	}

	return err;
}

int set_bits(int fd_audio)
{
	int err = 0;
	int ret, val;

#ifdef CONFIG_MX21ADS
	val = AFMT_S16_LE;
#else
	val = AFMT_U8;
#endif
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_BITS, &val);
#ifdef CONFIG_MX21ADS
	if ((val != AFMT_S16_LE) || (ret != 0)) {
#else
	if ((val != AFMT_U8) || (ret != 0)) {
#endif
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_BITS (%08x, %08x)\n",
			val, ret);
		err++;
	}
#ifdef CONFIG_MX21ADS
	val = AFMT_U8;
#endif
	ret = ioctl(fd_audio, SOUND_PCM_READ_BITS, &val);
	if (val != AFMT_S16_LE) {
		DPRINTF("Error with IOCTL SOUND_PCM_READ_BITS\n");
		err++;
	}

	return err;
}

int set_channels(int fd_audio)
{
	int err = 0;
	int ret, val;

	val = 1;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &val);
	if ((val != 1) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_CHANNELS (%d)\n",
			val);
		err++;
	}

	val = 0;
	ret = ioctl(fd_audio, SOUND_PCM_READ_CHANNELS, &val);
	if ((val != 1) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_READ_CHANNELS (%d)\n", val);
		err++;
	}

	val = 2;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &val);
	if ((val != 2) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_CHANNELS (%d)\n",
			val);
		err++;
	}

	val = 0;
	ret = ioctl(fd_audio, SOUND_PCM_READ_CHANNELS, &val);
	if ((val != 2) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_READ_CHANNELS (%d)\n", val);
		err++;
	}

	val = 4;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_CHANNELS, &val);
	if ((val != 2) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_CHANNELS (%d)\n",
			val);
		err++;
	}

	val = 0;
	ret = ioctl(fd_audio, SOUND_PCM_READ_CHANNELS, &val);
	if ((val != 2) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_READ_CHANNELS (%d)\n", val);
		err++;
	}

	return err;
}

int set_speed(int fd_audio)
{
	int err = 0;
	int ret, val;

	val = 8000;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &val);
	if ((val != 8000) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_RATE %d %d\n", val,
			ret);
		err++;
	}
#ifndef CONFIG_MX21ADS
	val = 32000;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &val);
	if ((val != 32000) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_RATE %d %d\n", val,
			ret);
		err++;
	}
#endif

	val = 44100;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &val);
	if ((val != 44100) || (ret != 0)) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_RATE %d %d\n", val,
			ret);
		err++;
	}
#ifndef CONFIG_MX21ADS
	val = 4000;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &val);
	if (val != 8000) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_RATE %d %d\n", val,
			ret);
		err++;
	}
#endif

#ifndef CONFIG_MX21ADS
	val = 128000;
	ret = ioctl(fd_audio, SOUND_PCM_WRITE_RATE, &val);
	if (val != 96000) {
		DPRINTF("Error with IOCTL SOUND_PCM_WRITE_RATE %d %d\n", val,
			ret);
		err++;
	}
#endif

	return err;
}

int set_adder(int fd_audio)
{
	int err = 0;
	int ret, val;

	val = MC13783_AUDIO_ADDER_STEREO;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_ADDER, &val);
	if ((val != MC13783_AUDIO_ADDER_STEREO) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_ADDER\n");
		err++;
	}

	val = MC13783_AUDIO_ADDER_MONO_OPPOSITE;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_ADDER, &val);
	if ((val != MC13783_AUDIO_ADDER_MONO_OPPOSITE) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_ADDER\n");
		err++;
	}

	val = MC13783_AUDIO_ADDER_MONO;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_ADDER, &val);
	if ((val != MC13783_AUDIO_ADDER_MONO) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_ADDER\n");
		err++;
	}

	val = MC13783_AUDIO_ADDER_STEREO_OPPOSITE;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_ADDER, &val);
	if ((val != MC13783_AUDIO_ADDER_STEREO_OPPOSITE) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_ADDER\n");
		err++;
	}

	return err;
}

int set_balance(int fd_audio)
{
	int err = 0;
	int ret, val;

	val = 0;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_BALANCE, &val);
	if ((val != 0) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	val = -1;
	ret = ioctl(fd_audio, SNDCTL_MC13783_READ_OUT_BALANCE, &val);
	if ((val != 0) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	val = 100;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_BALANCE, &val);
	if ((val != 100) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	val = -1;
	ret = ioctl(fd_audio, SNDCTL_MC13783_READ_OUT_BALANCE, &val);
	if ((val != 100) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	val = 50;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_BALANCE, &val);
	if ((val != 50) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	val = -1;
	ret = ioctl(fd_audio, SNDCTL_MC13783_READ_OUT_BALANCE, &val);
	if ((val != 50) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	val = 73;
	ret = ioctl(fd_audio, SNDCTL_MC13783_WRITE_OUT_BALANCE, &val);
	if ((val != 73) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	val = -1;
	ret = ioctl(fd_audio, SNDCTL_MC13783_READ_OUT_BALANCE, &val);
	if ((val != 73) || (ret != 0)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUT_BALANCE\n");
		err++;
	}

	return err;
}

/* Modified code, because corresponding IOCTL obsolete */
int set_selector(int fd_audio, int val)
{
	int err = 0;
	int ret, mem;

	mem = val;
	ret = ioctl(fd_audio, SOUND_MIXER_WRITE_OUTSRC, &mem);
	if (!(mem & val)) {
		DPRINTF("Error with IOCTL SNDCTL_MC13783_WRITE_OUTSRC\n");
		err++;
	}

	return err;
}

int check_audio_ioctls(int fd_audio)
{
	int err = 0;

	err += set_bits(fd_audio);
	err += set_channels(fd_audio);
	err += set_speed(fd_audio);
#ifndef CONFIG_MX21ADS
	err += set_adder(fd_audio);
	err += set_balance(fd_audio);
#endif
	return err;
}

int check_mixer_ioctls(int fd_mixer)
{
	int nb_err, err = 0;
	int ret, val;

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_DEVMASK, &val);
	if (ret != 0) {
		DPRINTF
		    ("Returned Value Error with IOCTL SOUND_MIXER_READ_DEVMASK %d\n",
		     ret);
		err++;
	}
#ifdef CONFIG_MX21ADS
	if (val != (SOUND_MASK_VOLUME | SOUND_MASK_LINE | SOUND_MASK_MIC)) {
#else
	if (val != (SOUND_MASK_VOLUME | SOUND_MASK_PCM | SOUND_MASK_SPEAKER |
		    SOUND_MASK_PHONEOUT | SOUND_MASK_LINE |
		    SOUND_MASK_PHONEIN | SOUND_MASK_MIC)) {
#endif
		DPRINTF("Error with IOCTL SOUND_MIXER_READ_DEVMASK %x\n", val);
		err++;
	}

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_RECMASK, &val);
	if (ret != 0) {
		DPRINTF
		    ("Returned Value Error with IOCTL SOUND_MIXER_READ_RECMASK\n");
		err++;
	}
#ifdef CONFIG_MX21ADS
	if (val != (SOUND_MASK_LINE | SOUND_MASK_MIC)) {
#else
	if (val != (SOUND_MASK_LINE | SOUND_MASK_PHONEIN | SOUND_MASK_MIC)) {
#endif
		DPRINTF("Error with IOCTL SOUND_MIXER_READ_RECMASK %x\n", val);
		err++;
	}

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_OUTMASK, &val);
	if (ret != 0) {
		DPRINTF
		    ("Returned Value Error with IOCTL SOUND_MIXER_READ_OUTMASK\n");
		err++;
	}
#ifdef CONFIG_MX21ADS
	if (val != SOUND_MASK_VOLUME) {
#else
	if (val != (SOUND_MASK_VOLUME | SOUND_MASK_PCM | SOUND_MASK_SPEAKER
		    | SOUND_MASK_PHONEOUT)) {
#endif
		DPRINTF("Error with IOCTL SOUND_MIXER_READ_OUTMASK\n");
		err++;
	}

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_STEREODEVS, &val);
	if (ret != 0) {
		DPRINTF
		    ("Returned Value Error with IOCTL SOUND_MIXER_READ_STEREODEVS\n");
		err++;
	}
#ifdef CONFIG_MX21ADS
	if (val != (SOUND_MASK_VOLUME | SOUND_MASK_LINE)) {
#else
	if (val != (SOUND_MASK_VOLUME | SOUND_MASK_PHONEIN | SOUND_MASK_PCM)) {
#endif
		DPRINTF("Error with IOCTL SOUND_MIXER_READ_STEREODEVS\n");
		err++;
	}

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_CAPS, &val);
	if (ret != 0) {
		DPRINTF
		    ("Returned Value Error with IOCTL SOUND_MIXER_READ_CAPS\n");
		err++;
	}

	if (val != SOUND_CAP_EXCL_INPUT) {
		DPRINTF("Error with IOCTL SOUND_MIXER_READ_CAPS\n");
		err++;
	}

	ret = ioctl(fd_mixer, SOUND_MIXER_READ_RECSRC, &val);
	if (ret != 0) {
		DPRINTF
		    ("Returned Value Error with IOCTL SOUND_MIXER_READ_RECSRC\n");
		err++;
	}
#ifdef CONFIG_MX21ADS
	if ((val != SOUND_MASK_LINE) && (val != SOUND_MASK_MIC)) {
#else
	if ((val != SOUND_MASK_LINE) && (val != SOUND_MASK_PHONEIN) &&
	    (val != SOUND_MASK_MIC)) {
#endif
		DPRINTF("Error with IOCTL SOUND_MIXER_READ_RECSRC\n");
		err++;
	}

	/** Input source selection and volume control */
#ifndef CONFIG_MX21ADS
	nb_err = set_input_device(fd_mixer, SOUND_MASK_PHONEIN);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_RECSRC (SOUND_MASK_PHONEIN)\n");
		err += nb_err;
	}
#endif

	nb_err = set_input_device(fd_mixer, SOUND_MASK_LINE);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_RECSRC (SOUND_MASK_LINE)\n");
		err += nb_err;
	}

	nb_err = set_input_device(fd_mixer, SOUND_MASK_MIC);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_RECSRC (SOUND_MASK_MIC)\n");
		err += nb_err;
	}
#ifndef CONFIG_MX21ADS
	nb_err = set_input_device(fd_mixer, SOUND_MASK_PHONEIN);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_RECSRC (SOUND_MASK_PHONEIN)\n");
		err += nb_err;
	}
#endif

#ifndef CONFIG_MX21ADS
	nb_err = set_input_device(fd_mixer, SOUND_MASK_PHONEIN);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_RECSRC (SOUND_MASK_PHONEIN)\n");
		err += nb_err;
	}
#endif

	/** Output source selection and volume control */
	nb_err = set_output_device(fd_mixer, SOUND_MASK_VOLUME);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_OUTSRC (SOUND_MASK_VOLUME) \n");
		err += nb_err;
	}
#ifndef CONFIG_MX21ADS
	nb_err = set_output_device(fd_mixer, SOUND_MASK_PCM);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_OUTSRC (SOUND_MASK_PCM)\n");
		err += nb_err;
	}
#endif

#ifndef CONFIG_MX21ADS
	nb_err = set_output_device(fd_mixer, SOUND_MASK_SPEAKER);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_OUTSRC (SOUND_MASK_SPEAKER)\n");
		err += nb_err;
	}
#endif

#ifndef CONFIG_MX21ADS
	nb_err = set_output_device(fd_mixer, SOUND_MASK_PHONEOUT);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_OUTSRC (SOUND_MASK_PHONEOUT)\n");
		err += nb_err;
	}
#endif

	nb_err = set_output_device(fd_mixer, SOUND_MASK_VOLUME);
	if (nb_err) {
		DPRINTF
		    ("Error with IOCTL SOUND_MIXER_WRITE_OUTSRC (SOUND_MASK_VOLUME)\n");
		err += nb_err;
	}

	return err;
}

/*! Supposes that StDac is selected. Selects all output lines */
int setup_sound_driver(int fd_audio)
{
	int err = 0;

	err += set_selector(fd_audio, SOUND_MASK_VOLUME | SOUND_MASK_PCM |
			    SOUND_MASK_SPEAKER | SOUND_MASK_PHONEOUT);

	return err;
}

int main(int ac, char *av[])
{
	int fd_audio, fd_mixer;
	int err = 0;

	DPRINTF("Hi... \n");

	if (ac == 1) {
		DPRINTF
		    ("This program checks that IOCTLs are correctly implemented \n");
		DPRINTF(" --> main <no_arg>\n\n");
	}

	if ((fd_audio = open("/dev/sound/dsp1", O_WRONLY)) < 0) {
		DPRINTF("Error opening /dev/dsp\n");
		return 0;
	}
	if ((fd_mixer = open("/dev/sound/mixer", O_RDWR)) < 0) {
		DPRINTF("Error opening /dev/mixer\n");
		return 0;
	}

	if (setup_sound_driver(fd_audio)) {
		return 1;
	}

/*************************************** MIXER IOCTLs ****************************************/
	err += check_mixer_ioctls(fd_mixer);

/*************************************** AUDIO IOCTLs ****************************************/
	err += check_audio_ioctls(fd_audio);

/*************************************** END ****************************************/
	close(fd_audio);
	close(fd_mixer);

	if (!err) {
		printf("All tests passed with success\n");
	} else {
		printf("Encountered %d errors\n", err);
	}

	return 0;
}
