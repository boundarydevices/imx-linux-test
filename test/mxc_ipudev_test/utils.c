/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

/*!
 * @file utils.c
 *
 * @brief IPU device lib test utils implementation
 *
 * @ingroup IPU
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "mxc_ipudev_test.h"

#define MAX_PATH	128

#define deb_msg 1
#if deb_msg
#define deb_printf printf
#else
#define deb_printf
#endif

char * options = "p:d:t:c:l:i:o:O:s:f:h";

void util_help()
{
 printf("options: \n\r");
 printf("p:d:t:c:l:i:o:O:s:f:h \r\n");
 printf("p: priority\r\n");
 printf("d: tak id\r\n");
 printf("t: timeout\r\n");
 printf("c: frame count\r\n");
 printf("l: loop count\r\n");
 printf("i: input width,height,format,crop pos.x, pos.y,w,h,deinterlace.enable,deinterlace.motion\r\n");
 printf("o: overlay enable,w,h,format,crop x,y,width,height,alpha mode,alpha value,colorkey enable, color key value\r\n");
 printf("O: output width,height,format,rotation, crop pos.x,pos.y,w,h\r\n");
 printf("s: output to fb enable\r\n");
 printf("f: output file name\r\n");
}


char * skip_unwanted(char *ptr)
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

int parse_options(char *buf, ipu_test_handle_t *test_handle)
{
	struct ipu_task *t = &test_handle->task;
	char *str;

	/* general */
	str = strstr(buf, "priority");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->priority = strtol(str, NULL, 10);
				printf("priority\t= %d\n", t->priority);
			}
			return 0;
		}
	}

	str = strstr(buf, "task_id");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->task_id = strtol(str, NULL, 10);
				printf("task_id\t\t= %d\n", t->task_id);
			}
			return 0;
		}
	}

	str = strstr(buf, "timeout");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->timeout = strtol(str, NULL, 10);
				printf("timeout\t\t= %d\n", t->timeout);
			}
			return 0;
		}
	}

	str = strstr(buf, "fcount");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				test_handle->fcount = strtol(str, NULL, 10);
				printf("fcount\t\t= %d\n", test_handle->fcount);
			}
			return 0;
		}
	}

	str = strstr(buf, "loop_cnt");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				test_handle->loop_cnt = strtol(str, NULL, 10);
				printf("loop_cnt\t= %d\n", test_handle->loop_cnt);
			}
			return 0;
		}
	}

	/* input */
	str = strstr(buf, "in_width");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.width = strtol(str, NULL, 10);
				printf("in_width\t= %d\n", t->input.width);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_height");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.height = strtol(str, NULL, 10);
				printf("in_height\t= %d\n", t->input.height);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_fmt");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.format =
					v4l2_fourcc(str[0], str[1], str[2], str[3]);
				printf("in_fmt\t\t= %s\n", str);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_posx");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.crop.pos.x =
					strtol(str, NULL, 10);
				printf("in_posx\t\t= %d\n",
					t->input.crop.pos.x);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_posy");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.crop.pos.y =
					strtol(str, NULL, 10);
				printf("in_posy\t\t= %d\n",
					t->input.crop.pos.y);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_crop_w");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.crop.w =
					strtol(str, NULL, 10);
				printf("in_crop_w\t= %d\n",
					t->input.crop.w);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_crop_h");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.crop.h =
					strtol(str, NULL, 10);
				printf("in_crop_h\t= %d\n",
					t->input.crop.h);
			}
			return 0;
		}
	}

	str = strstr(buf, "deinterlace_en");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.deinterlace.enable =
					strtol(str, NULL, 10);
				printf("deinterlace_en\t= %d\n",
					t->input.deinterlace.enable);
			}
			return 0;
		}
	}

	str = strstr(buf, "motion_sel");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->input.deinterlace.motion =
					strtol(str, NULL, 10);
				printf("motion_sel\t= %d\n",
					t->input.deinterlace.motion);
			}
			return 0;
		}
	}

	/* overlay */
	str = strstr(buf, "overlay_en");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay_en = strtol(str, NULL, 10);
				printf("overlay_en\t= %d\n", t->overlay_en);
			}
			return 0;
		}
	}

	str = strstr(buf, "ov_width");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.width = strtol(str, NULL, 10);
				printf("ov_width\t= %d\n", t->overlay.width);
			}
			return 0;
		}
	}

	str = strstr(buf, "ov_height");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.height = strtol(str, NULL, 10);
				printf("ov_height\t= %d\n", t->overlay.height);
			}
			return 0;
		}
	}

	str = strstr(buf, "ov_fmt");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.format =
					v4l2_fourcc(str[0], str[1], str[2], str[3]);
				printf("ov_fmt\t\t= %s\n", str);
			}
			return 0;
		}
	}

	str = strstr(buf, "ov_posx");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.crop.pos.x =
					strtol(str, NULL, 10);
				printf("ov_posx\t\t= %d\n",
					t->overlay.crop.pos.x);
			}
			return 0;
		}
	}

	str = strstr(buf, "ov_posy");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.crop.pos.y =
					strtol(str, NULL, 10);
				printf("ov_posy\t\t= %d\n",
					t->overlay.crop.pos.y);
			}
			return 0;
		}
	}

	str = strstr(buf, "ov_crop_w");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.crop.w =
					strtol(str, NULL, 10);
				printf("ov_crop_w\t= %d\n",
					t->overlay.crop.w);
			}
			return 0;
		}
	}

	str = strstr(buf, "ov_crop_h");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.crop.h =
					strtol(str, NULL, 10);
				printf("ov_crop_h\t= %d\n",
					t->overlay.crop.h);
			}
			return 0;
		}
	}

	str = strstr(buf, "alpha_mode");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.alpha.mode =
					strtol(str, NULL, 10);
				printf("alpha_mode\t= %d\n",
					t->overlay.alpha.mode);
			}
			return 0;
		}
	}

	str = strstr(buf, "alpha_value");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.alpha.gvalue =
					strtol(str, NULL, 10);
				printf("alpha_value\t= %d\n",
					t->overlay.alpha.gvalue);
			}
			return 0;
		}
	}

	str = strstr(buf, "colorkey_en");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.colorkey.enable =
					strtol(str, NULL, 10);
				printf("colorkey_en\t= %d\n",
					t->overlay.colorkey.enable);
			}
			return 0;
		}
	}

	str = strstr(buf, "colorkey_value");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->overlay.colorkey.value =
					strtol(str, NULL, 16);
				printf("colorkey_value\t= 0x%x\n",
					t->overlay.colorkey.value);
			}
			return 0;
		}
	}

	/* output */
	str = strstr(buf, "out_width");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.width = strtol(str, NULL, 10);
				printf("out_width\t= %d\n", t->output.width);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_height");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.height = strtol(str, NULL, 10);
				printf("out_height\t= %d\n", t->output.height);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_fmt");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.format =
					v4l2_fourcc(str[0], str[1], str[2], str[3]);
				printf("out_fmt\t\t= %s\n", str);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_rot");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.rotate = strtol(str, NULL, 10);
				printf("out_rot\t\t= %d\n", t->output.rotate);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_posx");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.crop.pos.x = strtol(str, NULL, 10);
				printf("out_posx\t= %d\n", t->output.crop.pos.x);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_posy");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.crop.pos.y = strtol(str, NULL, 10);
				printf("out_posy\t= %d\n", t->output.crop.pos.y);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_crop_w");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.crop.w =
					strtol(str, NULL, 10);
				printf("out_crop_w\t= %d\n",
					t->output.crop.w);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_crop_h");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				t->output.crop.h =
					strtol(str, NULL, 10);
				printf("out_crop_h\t= %d\n",
					t->output.crop.h);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_to_fb");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				test_handle->show_to_fb = strtol(str, NULL, 10);
				printf("out_to_fb\t= %d\n", test_handle->show_to_fb);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_filename");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				memcpy(test_handle->outfile, str, strlen(str));
				printf("out_filename\t= %s\n", test_handle->outfile);
			}
			return 0;
		}
	}

	return 0;
}

int parse_config_file(char *file_name, ipu_test_handle_t *test_handle)
{
	FILE *fp;
	char line[128];
	char *ptr;

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		printf("Failed to open config file\n");
		return -1;
	}

	printf("\nGet config from config file %s:\n\n", file_name);

	while (fgets(line, MAX_PATH, fp) != NULL) {
		ptr = skip_unwanted(line);
		parse_options(ptr, test_handle);
	}

	printf("\n");

	fclose(fp);
	return 0;
}

int parse_cmd_input(int argc, char ** argv, ipu_test_handle_t *test_handle)
{
	char opt;
	char fourcc[5];
	struct ipu_task *t = &test_handle->task;

	printf("pass cmdline %d, %s\n", argc, argv[0]);

	/*default settings*/
	t->priority = 0;
	t->task_id = 0;
	t->timeout = 1000;
	test_handle->fcount = 50;
	test_handle->loop_cnt = 1;
	t->input.width = 320;
	t->input.height = 240;
	t->input.format =  v4l2_fourcc('I', '4','2', '0');
	t->input.crop.pos.x = 0;
	t->input.crop.pos.y = 0;
	t->input.crop.w = 0;
	t->input.crop.h = 0;
	t->input.deinterlace.enable = 0;
	t->input.deinterlace.motion = 0;

	t->overlay_en = 0;
	t->overlay.width = 320;
	t->overlay.height = 240;
	t->overlay.format = v4l2_fourcc('I', '4','2', '0');
	t->overlay.crop.pos.x = 0;
	t->overlay.crop.pos.y = 0;
	t->overlay.crop.w = 0;
	t->overlay.crop.h = 0;
	t->overlay.alpha.mode = 0;
	t->overlay.alpha.gvalue = 0;
	t->overlay.colorkey.enable = 0;
	t->overlay.colorkey.value = 0x555555;

	t->output.width = 1024;
	t->output.height = 768;
	t->output.format = v4l2_fourcc('U', 'Y','V', 'Y');
	t->output.rotate = 0;
	t->output.crop.pos.x = 0;
	t->output.crop.pos.y = 0;
	t->output.crop.w = 0;
	t->output.crop.h = 0;

	test_handle->show_to_fb = 1;
	memcpy(test_handle->outfile,"ipu0-1st-ovfb",13);

	while((opt = getopt(argc, argv, options)) > 0)
	{
		deb_printf("\nnew option : %c \n", opt);
		switch(opt)
		{
			case 'p':/*priority*/
				if(NULL == optarg)
					break;
				t->priority = strtol(optarg, NULL, 10);
				deb_printf("priority set %d \n",t->priority);
				break;
			case 'd': /*task id*/
				if(NULL == optarg)
					break;
				t->task_id = strtol(optarg, NULL, 10);
				deb_printf("task id set %d \n",t->task_id);
				break;
			case 't':
				if(NULL == optarg)
					break;
				t->timeout = strtol(optarg, NULL, 10);
			case 'c':
				if(NULL == optarg)
					break;
				test_handle->fcount = strtol(optarg, NULL, 10);
				deb_printf("frame count set %d \n",test_handle->fcount);
				break;
			case 'l':
				if(NULL == optarg)
					break;
				test_handle->loop_cnt = strtol(optarg, NULL, 10);
				deb_printf("loop count set %d \n",test_handle->loop_cnt);
				break;
			case 'i': /*input param*/
				if(NULL == optarg)
					break;
				memset(fourcc,0,sizeof(fourcc));
				sscanf(optarg,"%d,%d,%c%c%c%c,%d,%d,%d,%d,%d,%d",
						&(t->input.width),&(t->input.height),
						&(fourcc[0]),&(fourcc[1]), &(fourcc[2]), &(fourcc[3]),
						&(t->input.crop.pos.x),&(t->input.crop.pos.y),
						&(t->input.crop.w), &(t->input.crop.h),
						(int *)&(t->input.deinterlace.enable), (int *)&(t->input.deinterlace.motion));
				t->input.format = v4l2_fourcc(fourcc[0], fourcc[1],
						fourcc[2], fourcc[3]);
				deb_printf("input w=%d,h=%d,fucc=%s,cpx=%d,cpy=%d,cpw=%d,cph=%d,de=%d,dm=%d\n",
						t->input.width,t->input.height,
						fourcc,t->input.crop.pos.x,t->input.crop.pos.y,
						t->input.crop.w, t->input.crop.h,
						t->input.deinterlace.enable, t->input.deinterlace.motion);
				break;
			case 'o':/*overlay setting*/
				if(NULL == optarg)
					break;
				memset(fourcc,0,sizeof(fourcc));
				sscanf(optarg,"%d,%d,%d,%c%c%c%c,%d,%d,%d,%d,%d,%d,%d,%x",
						(int *)&(t->overlay_en),&(t->overlay.width),&(t->overlay.height),
						&(fourcc[0]),&(fourcc[1]), &(fourcc[2]), &(fourcc[3]),
						&(t->overlay.crop.pos.x),&(t->overlay.crop.pos.y),
						&(t->overlay.crop.w), &(t->overlay.crop.h),
						(int *)&(t->overlay.alpha.mode), (int *)&(t->overlay.alpha.gvalue),
						(int *)&(t->overlay.colorkey.enable),&(t->overlay.colorkey.value));
				t->overlay.format = v4l2_fourcc(fourcc[0], fourcc[1],
						fourcc[2], fourcc[3]);
				deb_printf("overlay en=%d,w=%d,h=%d,fourcc=%c%c%c%c,cpx=%d,\
						cpy=%d,cw=%d,ch=%d,am=%c,ag=%d,ce=%d,cv=%x\n",
						t->overlay_en, t->overlay.width,t->overlay.height,
						fourcc[0],fourcc[1], fourcc[2], fourcc[3],
						t->overlay.crop.pos.x,t->overlay.crop.pos.y,
						t->overlay.crop.w, t->overlay.crop.h,
						t->overlay.alpha.mode, t->overlay.alpha.gvalue,
						t->overlay.colorkey.enable,t->overlay.colorkey.value);
				break;
			case 'O':/*output setting*/
				memset(fourcc,0,sizeof(fourcc));
				if(NULL == optarg)
					break;
				sscanf(optarg,"%d,%d,%c%c%c%c,%d,%d,%d,%d,%d",
						&(t->output.width),&(t->output.height),
						&(fourcc[0]),&(fourcc[1]), &(fourcc[2]), &(fourcc[3]),
						(int *)&(t->output.rotate),&(t->output.crop.pos.x),
						&(t->output.crop.pos.y),&(t->output.crop.w),
						&(t->output.crop.h));
				t->output.format = v4l2_fourcc(fourcc[0],
						fourcc[1],fourcc[2], fourcc[3]);
				deb_printf(optarg,"%d,%d,%s,%d,%d,%d,%d,%d\n",
						t->output.width,t->output.height,
						fourcc,t->output.rotate,t->output.crop.pos.x,
						t->output.crop.pos.y,t->output.crop.w,
						t->output.crop.h);
				break;
			case 's':/*fb setting*/
				if(NULL == optarg)
					break;
				test_handle->show_to_fb = strtol(optarg, NULL, 10);
				deb_printf("show to fb %d\n", test_handle->show_to_fb);
				break;
			case 'f':/*output0 file name*/
				if(NULL == optarg)
					break;
				memset(test_handle->outfile,0,sizeof(test_handle->outfile));
				sscanf(optarg,"%s",test_handle->outfile);
				deb_printf("output file name %s \n",test_handle->outfile);
				break;
			case 'h':
				util_help();
				break;
			default:
				return 0;
		}
	}
	return 0;
}
