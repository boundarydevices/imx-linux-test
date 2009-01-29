/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <fcntl.h>
#include <linux/input.h>
#include <asm/arch/imx_adc.h>

int main(void)
{
	int fd;
	struct t_adc_convert_param tmp;
	
	memset(&tmp, 0, sizeof tmp);
	fd = open("/dev/imx_adc", O_RDONLY);
	if (fd < 0)
	{
		printf("error open the device!\n");
		return 1;
	}
	
	while(1)
	{
		printf("----single channel adc0---\n");
		tmp.channel = GER_PURPOSE_ADC0;
		ioctl(fd, IMX_ADC_CONVERT , &tmp);
		printf("channel0 is %x\n", tmp.result[2]);
		
		printf("----multichannel----\n");
		ioctl(fd, IMX_ADC_CONVERT_MULTICHANNEL, &tmp);
		printf("channel0 is %x\n", tmp.result[2]);
		printf("channel1 is %x\n", tmp.result[3]);
		printf("channel2 is %x\n", tmp.result[4]);
		sleep(1);
	}
	close(fd);
	return 0;
}
