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

