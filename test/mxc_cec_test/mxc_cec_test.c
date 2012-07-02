/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/**
 * HDMI CEC driver unit test file
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


#include "hdmi-cec.h"
static int ready_flay = 0;
int my_hdmi_cec_callback(unsigned char event_type, void *parg)
{
	hdmi_cec_message *msg;
	int i =0;
	if(HDMI_CEC_DEVICE_READY == event_type)
		ready_flay = 1;
	else if(HDMI_CEC_RECEIVE_MESSAGE == event_type)
	{
		msg = (hdmi_cec_message *)parg;
		if(0x9e == msg->opcode){
			printf("  receive CEC Version message!\n");
			if(0x4 == msg->operand[0])
				printf("  CEC Version is 1.3a!\n");
			else if(0x5 == msg->operand[0])
				printf("  CEC Version is 1.4 or 1.4a!\n");
			else
				printf("  Unknown CEC Version!\n");
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	printf("Start CEC testing! \n");
	hdmi_cec_init();
	hdmi_cec_open(Playback_Device,my_hdmi_cec_callback);
	while(0 == ready_flay){
		sleep(1);
		printf("sleep for ready! \n");
	}
	printf("  Try to get CEC Version !\n");
	hdmi_cec_send_message(0, 0x9F, 0,NULL);
	sleep(5);
	printf("---------------->  Power off TV  <-------------------------\n");
	hdmi_cec_send_message(0, 0x36, 0,NULL);
	sleep(1);
	hdmi_cec_close(Playback_Device);
	hdmi_cec_deinit();
	printf("end of test! \n");
	return 0;
}



