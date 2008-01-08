/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <linux/types.h>

#include <asm/arch/mxc_pf.h>



/*!
*******************************************************************************
*   Function Declarations
*******************************************************************************
*/

static int pf_init(uint32_t width, uint32_t height, uint32_t stride);
static int pf_uninit(void);
static int pf_start(int qp_buf);

int fd_pf;
int fd_inbuf;
int fd_refbuf;
int fd_outbuf;
int fd_qpbuf;

int g_pf_mode = 4;
int g_width;
int g_height;
int g_frame_size;
int g_num_buffers;
char * g_out_file;
char * g_pf_buf[2];
char * g_pf_qp_buf[2];
int g_pf_qp_size;
int g_use_wait;

pf_buf g_buf_desc[2];

int process_cmdline(int argc, char **argv)
{
        int i;
        
        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-w") == 0) {
                        g_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-h") == 0) {
                        g_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-async") == 0) {
                        g_use_wait = 1;
                }
                else if (strcmp(argv[i], "-o") == 0) {
                        g_out_file = argv[++i];
                }
                else if (strcmp(argv[i], "-m") == 0) {
                        g_pf_mode = atoi(argv[++i]);
			if ((g_pf_mode < 1) || (g_pf_mode > 4))
				return 1;
                }
        }

        return 0;
}

int main (int argc, char *argv[])
{
	int ret = 0;
	int qp_buf = 0;
	char * ref_buf;
	int frame_cnt = 0;

	if ((argc < 8) || process_cmdline(argc, argv)) {
		printf("Usage: %s -w <width> -h <height> -m <mode> [-async] <yuv input file> <qp data file> [<h.264 bs data file>] <yuv output file>\n\n",
			argv[0]);
		return 1;
	}

	fd_refbuf = fopen(argv[--argc], "r");
	if (!fd_refbuf) {
		ret = 1;
		goto err0;
	}


	fd_qpbuf = fopen(argv[--argc], "r");
	if (!fd_qpbuf) {
		ret = 1;
		goto err1;
	}
	
	fd_inbuf = fopen(argv[--argc], "r");
	if (!fd_inbuf) {
		ret = 1;
		goto err2;
	}

	if (g_out_file) {
		fd_outbuf = fopen(g_out_file, "w");
		if (!fd_outbuf) {
			ret = 1;
			goto err3;
		}
	}
	
        printf("Calling PF init\n");
        pf_init(g_width, g_height, g_width);
	g_frame_size = (g_width * g_height * 3) / 2;

	ref_buf = malloc(g_frame_size);

	while(1) {
		if (fread(g_pf_buf[0], 1, g_frame_size, fd_inbuf) < g_frame_size)
			break;

		
		if (fread(g_pf_qp_buf[qp_buf], 1, g_pf_qp_size, fd_qpbuf) < g_pf_qp_size)
			break;

		if (pf_start(qp_buf) < 0) {
			printf("Error starting post-filter.\n");
			ret = 1;
			break;
		}
		fsync(fd_pf);

		if (fread(ref_buf, 1, g_frame_size, fd_refbuf) < g_frame_size)
			break;

		if (g_out_file)
			fwrite(g_pf_buf[(g_pf_mode == 4) ? 0 : 1], 1, g_frame_size, fd_outbuf);

	        if (memcmp(g_pf_buf[(g_pf_mode == 4) ? 0 : 1], ref_buf, g_frame_size)) {
	                printf("Mismatch in output buffer, frame # %d\n", frame_cnt);
			ret = 1;
			break;
	        }
		qp_buf = qp_buf ? 0 : 1;
		frame_cnt++;
		if ((frame_cnt % 10) == 0)
			printf("frames - %d\r", frame_cnt);
	}
	printf("frames - %d\n", frame_cnt);
	
        pf_uninit();

	if (fd_outbuf)
		fclose(fd_outbuf);
err3:
	fclose(fd_inbuf);
err2:
	fclose(fd_qpbuf);
err1:
	fclose(fd_refbuf);
err0:
        return ret;
}



/*!
*******************************************************************************
*    Function Definitions
*******************************************************************************
*/

static int pf_init(uint32_t width, uint32_t height, uint32_t stride)
{
        int i;
        int retval = 0;
        pf_init_params pf_init;
        pf_reqbufs_params pf_reqbufs;
        
        if ((fd_pf = open("/dev/mxc_ipu_pf", O_RDWR, 0)) < 0)
        {
                printf("Unable to open pf device\n");
                retval = -1;
                goto err0;
        }

        memset(&pf_init, 0, sizeof(pf_init));
        pf_init.pf_mode = g_pf_mode;
        pf_init.width = width;
        pf_init.height = height;
        pf_init.stride = stride;
        if (ioctl(fd_pf, PF_IOCTL_INIT, &pf_init) < 0)
        {
                printf("PF_IOCTL_INIT failed\n");
                retval = -1;
                goto err1;
        }
	g_pf_qp_size = pf_init.qp_size / 2;
        g_pf_qp_buf[0] = mmap(NULL, pf_init.qp_size, 
                         PROT_READ | PROT_WRITE, MAP_SHARED, 
                         fd_pf, pf_init.qp_paddr);
	g_pf_qp_buf[1] = g_pf_qp_buf[0] + g_pf_qp_size;
        if (g_pf_qp_buf[0] == NULL) {
                printf("v4l2_out test: mmap for qp buffer failed\n");
                retval = -1;
                goto err1;
        }
        
        pf_reqbufs.count = 2;
        pf_reqbufs.req_size = 0;
        if (ioctl(fd_pf, PF_IOCTL_REQBUFS, &pf_reqbufs) < 0)
        {
                printf("PF_IOCTL_REQBUFS failed\n");
                retval = -1;
                goto err1;
        }

        for (i = 0; i < pf_reqbufs.count; i++) {

                g_buf_desc[i].index = i;
                if (ioctl(fd_pf, PF_IOCTL_QUERYBUF, &g_buf_desc[i]) < 0)
                {
                        printf("PF_IOCTL_QUERYBUF failed\n");
                        retval = -1;
                        goto err1;
                }
                        
                g_pf_buf[i] = mmap (NULL, g_buf_desc[i].size, 
                                 PROT_READ | PROT_WRITE, MAP_SHARED, 
                                 fd_pf, g_buf_desc[i].offset);
                if (g_pf_buf[i] == NULL) {
                        printf("v4l2_out test: mmap for input buffer failed\n");
                        retval = -1;
                        goto err1;
                }
        }

        printf("PF driver initialized\n");
        return retval;
err1:
        close(fd_pf);
err0:
        return retval;

}

static int pf_uninit(void)
{
        pf_reqbufs_params pf_reqbufs;
        int retval = 0;

        printf("Closing PF driver");
        
        pf_reqbufs.count = 0;   // Zero deallocates buffers
        if (ioctl(fd_pf, PF_IOCTL_REQBUFS, &pf_reqbufs) < 0)
        {
                printf("PF_IOCTL_REQBUFS failed\n");
                retval = -1;
                goto err1;
        }

        if (ioctl(fd_pf, PF_IOCTL_UNINIT, NULL) < 0)
        {
                printf("PF uninit failed\n");
                retval = -1;
        }

        munmap(g_pf_buf[0], g_buf_desc[0].size);
        munmap(g_pf_buf[1], g_buf_desc[1].size);

        close(fd_pf);
        printf(" - Done\n");
err1:
        return retval;
}        

static int pf_start(int qp_buf)
{
        int retval = 0;
        pf_start_params pf_st;
        
        memset(&pf_st, 0, sizeof(pf_st));
        pf_st.wait = !g_use_wait;
        pf_st.in = g_buf_desc[0];
	if (g_pf_mode == 4) {
	        pf_st.out = g_buf_desc[0];
	} else {
	        pf_st.out = g_buf_desc[1];
	}
	pf_st.qp_buf = qp_buf;

        if ((retval = ioctl(fd_pf, PF_IOCTL_START, &pf_st)) < 0)
        {
                printf("PF start failed: %d\n", retval);
                goto err1;
        }
	
	if (g_use_wait) {
		if ((retval = ioctl(fd_pf, PF_IOCTL_WAIT, PF_WAIT_Y)) < 0)
		{
			printf("PF wait Y failed: %d\n", retval);
			goto err1;
		}

		if ((retval = ioctl(fd_pf, PF_IOCTL_WAIT, PF_WAIT_U | PF_WAIT_V)) < 0)
		{
			printf("PF wait U and V failed: %d\n", retval);
			goto err1;
		}
	}

err1:
        return retval;
}

