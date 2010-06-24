/*
 * Copyright 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include "mxc_ipudev_test.h"

#define MAX_PATH	128

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
	char *str;

	/* general */
	str = strstr(buf, "mode");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				test_handle->mode = strtol(str, NULL, 16);
				printf("mode\t\t= 0x%x\n", test_handle->mode);
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
				test_handle->input.width = strtol(str, NULL, 10);
				printf("in_width\t= %d\n", test_handle->input.width);
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
				test_handle->input.height = strtol(str, NULL, 10);
				printf("in_height\t= %d\n", test_handle->input.height);
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
				test_handle->input.fmt =
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
				test_handle->input.input_crop_win.pos.x =
					strtol(str, NULL, 10);
				printf("in_posx\t\t= %d\n",
					test_handle->input.input_crop_win.pos.x);
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
				test_handle->input.input_crop_win.pos.y =
					strtol(str, NULL, 10);
				printf("in_posy\t\t= %d\n",
					test_handle->input.input_crop_win.pos.y);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_win_w");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				test_handle->input.input_crop_win.win_w =
					strtol(str, NULL, 10);
				printf("in_win_w\t= %d\n",
					test_handle->input.input_crop_win.win_w);
			}
			return 0;
		}
	}

	str = strstr(buf, "in_win_h");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				test_handle->input.input_crop_win.win_h =
					strtol(str, NULL, 10);
				printf("in_win_h\t= %d\n",
					test_handle->input.input_crop_win.win_h);
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
				test_handle->input.motion_sel =
					strtol(str, NULL, 10);
				printf("motion_sel\t= %d\n",
					test_handle->input.motion_sel);
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
				test_handle->output.width = strtol(str, NULL, 10);
				printf("out_width\t= %d\n", test_handle->output.width);
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
				test_handle->output.height = strtol(str, NULL, 10);
				printf("out_height\t= %d\n", test_handle->output.height);
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
				test_handle->output.fmt =
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
				test_handle->output.rot = strtol(str, NULL, 10);
				printf("out_rot\t\t= %d\n", test_handle->output.rot);
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
				test_handle->output.show_to_fb = strtol(str, NULL, 10);
				printf("out_to_fb\t= %d\n", test_handle->output.show_to_fb);
			}
			return 0;
		}
	}

	str = strstr(buf, "out_fb_num");
	if (str != NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str++;
			if (*str != '\0') {
				test_handle->output.fb_disp.fb_num = strtol(str, NULL, 10);
				printf("out_fb_num\t= %d\n", test_handle->output.fb_disp.fb_num);
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
				test_handle->output.fb_disp.pos.x = strtol(str, NULL, 10);
				printf("out_posx\t= %d\n", test_handle->output.fb_disp.pos.x);
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
				test_handle->output.fb_disp.pos.y = strtol(str, NULL, 10);
				printf("out_posy\t= %d\n", test_handle->output.fb_disp.pos.y);
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

