#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* For Hantro MPEG4 encoder */
#include "mp4encapi.h"

/* For linear memory allocation */
#include "memalloc.h"

#include <linux/videodev.h>
#define TEST_BUFFER_NUM 3

struct cmd_line
{
	int width;
	int height;
	int count;
	int fps;
	char output[64];
};

struct testbuffer {
	size_t offset;
	unsigned int length;
};


/* Globals */
u32 *outbuf = NULL;
u32 outbuf_size;
u32 outbuf_bus_address = 0;

MP4EncInst encoder;
int fd_v4l;

/* Kernel driver for linear memory allocation of shared memories */
const char *memdev = "/dev/memalloc";
int memdev_fd = -1;

struct cmd_line cmdl;
struct testbuffer buffers[TEST_BUFFER_NUM];

int start_capturing(void)
{
	unsigned int i;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	for (i = 0; i < TEST_BUFFER_NUM; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.index = i;
		if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0) {
			printf("VIDIOC_QUERYBUF error\n");
			return -1;
		}

		buffers[i].length = buf.length;
		buffers[i].offset = (size_t) buf.m.offset;
	}

	for (i = 0; i < TEST_BUFFER_NUM; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		buf.m.offset = buffers[i].offset;
		if (ioctl(fd_v4l, VIDIOC_QBUF, &buf) < 0) {
			printf("VIDIOC_QBUF error\n");
			return -1;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_v4l, VIDIOC_STREAMON, &type) < 0) {
		printf("VIDIOC_STREAMON error\n");
		return -1;
	}

	return 0;
}

void stop_capturing(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_v4l, VIDIOC_STREAMOFF, &type) < 0) {
		printf("VIDIOC_STREAMOFF failed\n");
	}
}

int v4l_capture_setup(void)
{
	char v4l_device[32] = "/dev/video0";
	struct v4l2_format fmt;
	struct v4l2_streamparm parm;
	struct v4l2_requestbuffers req;

	if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
		printf("Unable to open %s\n", v4l_device);
		return -1;
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.width = cmdl.width;
	fmt.fmt.pix.height = cmdl.height;
	fmt.fmt.pix.bytesperline = cmdl.width;
	fmt.fmt.pix.priv = 0;
	fmt.fmt.pix.sizeimage = 0;

	if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0) {
		printf("set format failed\n");
		close(fd_v4l);
		return -1;
	}

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = cmdl.fps;
	parm.parm.capture.capturemode = 0;

	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0) {
		printf("set frame rate failed\n");
		close(fd_v4l);
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.count = TEST_BUFFER_NUM;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (ioctl(fd_v4l, VIDIOC_REQBUFS, &req) < 0) {
		printf("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
		close(fd_v4l);
		return -1;
	}

	return 0;
}

int v4l_capture_and_encode_test(void)
{
	MP4EncRet ret;
	MP4EncIn encIn;
	MP4EncOut encOut;

	struct v4l2_buffer buf;
	int fd_mp4, next, last, retcode = 0;

	if (start_capturing() < 0) {
		printf("start capturing failed\n");
		return -1;
	}

	/* Step 3: Assign memory resources */
	encIn.pOutBuf = outbuf;
	encIn.outBufSize = outbuf_size;
	encIn.outBufBusAddress = outbuf_bus_address;

	/* The use of VP size buffer is optional */
	encIn.pVpSizes = NULL;

	printf("Output buffer %d bytes\n", encIn.outBufSize);

	/* Step 4: Start the stream */
	ret = MP4EncStrmStart(encoder, &encIn, &encOut);
	if (ret != ENC_OK) {
		printf("Failed to start stream. Error code: %8i\n", ret);
		stop_capturing();
		return -1;
	} else {
		fd_mp4 = open(cmdl.output, O_RDWR | O_CREAT);
		if (fd_mp4 < 0) {
			printf("Unable to open %s\n", cmdl.output);
			MP4EncStrmEnd(encoder, &encIn, &encOut);
			stop_capturing();
			return -1;
		}

		printf("Stream start header %u bytes\n", encOut.strmSize);
		write(fd_mp4, encIn.pOutBuf, encOut.strmSize);
	}

	/* Step 5: Encode the video frames */
	ret = ENC_VOP_READY;
	next = 0;
	last = cmdl.count;

	while ( (next < last) &&
		(ret == ENC_VOP_READY || ret == ENC_VOP_READY_VBV_FAIL ||
		 ret == ENC_OUTPUT_BUFFER_OVERFLOW ))
	{
		/* Select VOP type */
		if (next == 0) {
			encIn.timeIncr = 0;	/* Start time is 0 */
			encIn.vopType = INTRA_VOP; /* first frame has to be INTRA coded */
		} else {
			encIn.timeIncr = 1; 	/* time unit between frames */
			encIn.vopType = PREDICTED_VOP;
		}

		/* Read the video frame */
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(fd_v4l, VIDIOC_DQBUF, &buf) < 0) {
			printf("VIDIOC_DQBUF, Unable to read video frame\n");
			retcode = -1;
			break;
		}

		/* we use one linear buffer for the input image */
		encIn.busLuma = (u32)buffers[buf.index].offset;
		encIn.busChromaU = (u32)NULL;
		encIn.busChromaV = (u32)NULL;

		/* Loop until one VOP is encoded */
		do
		{
			ret = MP4EncStrmEncode(encoder, &encIn, &encOut);
			switch (ret)
			{
			case ENC_VOP_READY_VBV_FAIL:
				printf("Warning: VBV Overflow!!\n");
				/* Fall through */
			case ENC_VOP_READY:
				write(fd_mp4, encIn.pOutBuf, encOut.strmSize);
				break;
			case ENC_GOV_READY:
				write(fd_mp4, encIn.pOutBuf, encOut.strmSize);
				break;
			default:
				printf("FAILED. Error code: %i\n", ret);
				break;
			}
		} while (ret == ENC_GOV_READY);

		next++;

		if ((last - next) >= TEST_BUFFER_NUM) {
			if (ioctl(fd_v4l, VIDIOC_QBUF, &buf) < 0) {
				printf("VIDIOC_QBUF failed\n");
				retcode = -1;
				break;
			}
		} else {
			printf("buf.index %d	count = %d\n",
					buf.index, (last - next));
		}
	} /* End of main encoding loop */

	/* Step 6: End stream */
	ret = MP4EncStrmEnd(encoder, &encIn, &encOut);
	if (ret != ENC_OK) {
		printf("MP4EncStrmEnd failed\n");
		retcode = -1;
	} else {
		write(fd_mp4, encIn.pOutBuf, encOut.strmSize);
	}

	stop_capturing();
	close(fd_mp4);
	return retcode;
}

/*------------------------------------------------------------------------------

    FreeRes

------------------------------------------------------------------------------*/
void FreeRes(void)
{
    	if(outbuf != MAP_FAILED)
        	munmap(outbuf, outbuf_size);

    	if(outbuf_bus_address != 0)
        	ioctl(memdev_fd, MEMALLOC_IOCSFREEBUFFER, &outbuf_bus_address);

    	if(memdev_fd != -1)
        	close(memdev_fd);
}

/*------------------------------------------------------------------------------

    AllocRes

    OS dependent implementation for allocating the physical memories
    used by both SW and HW: output buffer.

    To access the memory HW uses the physical linear address (bus address)
    and SW uses virtual address (user address).

    In Linux the physical memories can only be allocated with sizes
    of power of two times the page size.

------------------------------------------------------------------------------*/
int AllocRes(void)
{
	memdev_fd = open(memdev, O_RDWR);
	if(memdev_fd == -1)
	{
		printf("Failed to open dev: %s\n", memdev);
		return (-1);
	}

	outbuf_size = outbuf_bus_address = 64 * sysconf(_SC_PAGESIZE);
	printf("Output buffer size:\t\t\t%d\n", outbuf_size);

	ioctl(memdev_fd, MEMALLOC_IOCXGETBUFFER, &outbuf_bus_address);
	printf("Output buffer bus address:\t\t0x%08x\n", outbuf_bus_address);

	outbuf = (u32 *) mmap(0, outbuf_size, PROT_READ | PROT_WRITE,
			MAP_SHARED, memdev_fd, outbuf_bus_address);
	printf("Output buffer user address:\t\t0x%08x\n", (u32) outbuf);
	if(outbuf == MAP_FAILED)
	{
		printf("Failed to alloc output bitstream buffer\n");
		FreeRes();
		return (-1);
	}

	return 0;
}

int main(int argc, char **argv)
{
	MP4EncApiVersion ver;
	MP4EncRet ret;
    	MP4EncCfg cfg;
	int retcode = 0;

	/* Command line parameters */
	if (argc < 6) {
		fprintf(stdout,
			"Usage: cam2mpeg4 [width] [height] [number of frames] [frame rate] [filename]\n");
		return -1;
	}

	cmdl.width = atoi(argv[1]);
	cmdl.height = atoi(argv[2]);
	cmdl.count = atoi(argv[3]);
	cmdl.fps = atoi(argv[4]);
	strncpy(cmdl.output, argv[5], 64);

	if (v4l_capture_setup() < 0)
		return -1;

	ver = MP4EncGetVersion();
	printf("API version %d.%d\n", ver.major, ver.minor);

	/* Allocate MPEG4 output stream buffer */
	if (AllocRes() != 0) {
		close(fd_v4l);
		return -1;
	}

	/* Step 1: Encoder Initialization */
    	cfg.frmRateDenom = 1;
    	cfg.frmRateNum = 30;
    	cfg.width = cmdl.width;
    	cfg.height = cmdl.height;
    	cfg.strmType = MPEG4_PLAIN_STRM;
    	cfg.profileAndLevel = MPEG4_ADV_SIMPLE_PROFILE_LEVEL_5;
    	if ((ret = MP4EncInit(&cfg, &encoder)) != ENC_OK)
    	{
        	printf("Failed to initialize the encoder. Error code: %8i\n", ret);
		retcode = -1;
		goto error;
    	}

	retcode = v4l_capture_and_encode_test();

	/* Free all resources */
	if ((ret = MP4EncRelease(encoder)) != ENC_OK)
    	{
        	printf("Failed to release the encoder. Error code: %8i\n", ret);
		retcode = -1;
    	}

error:
	FreeRes();
	close(fd_v4l);
	return retcode;
}

