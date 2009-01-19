
#define VOPS                    100
#define ASICRUNLENGTH           22
#define MEPG4_FILENAME             "./stream.mpeg4"
#define YUV_FILENAME               "./stream.yuv"

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <linux/videodev.h>
#include <string.h>
#include <malloc.h>

#include "basetype.h"
#include "MP4EncApi.h"
#define TEST_BUFFER_NUM 3

int g_frame_rate = 0;
int g_capture_count = 0;
int g_width = 0;
int g_height = 0;

struct testbuffer {
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

struct testbuffer buffers[TEST_BUFFER_NUM];

int start_capturing(int fd_v4l)
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
		buffers[i].start = mmap(NULL, buffers[i].length,
					PROT_READ | PROT_WRITE, MAP_SHARED,
					fd_v4l, buffers[i].offset);
		memset(buffers[i].start, 0xFF, buffers[i].length);
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

void stop_capturing(int fd_v4l)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_v4l, VIDIOC_STREAMOFF, &type) < 0) {
		printf("VIDIOC_STREAMOFF failed\n");
	}
}

int v4l_capture_setup(void)
{
	char v4l_device[100] = "/dev/video0";
	struct v4l2_format fmt;
	struct v4l2_control ctrl;
	struct v4l2_streamparm parm;
	int fd_v4l = 0;
	struct v4l2_requestbuffers req;

	if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
		printf("Unable to open %s\n", v4l_device);
		return -1;
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.width = g_width;
	fmt.fmt.pix.height = g_height;
	fmt.fmt.pix.bytesperline = g_width;
	fmt.fmt.pix.priv = 0;
	fmt.fmt.pix.sizeimage = 0;

	if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0) {
		printf("set format failed\n");
		return -1;
	}

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_frame_rate;
	parm.parm.capture.capturemode = 0;

	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0) {
		printf("set frame rate failed\n");
		return -1;
	}
	// Set rotation
	ctrl.id = V4L2_CID_PRIVATE_BASE + 0;
	ctrl.value = 0;
	if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0) {
		printf("set ctrl failed\n");
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.count = TEST_BUFFER_NUM;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (ioctl(fd_v4l, VIDIOC_REQBUFS, &req) < 0) {
		printf("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
		return -1;
	}

	return fd_v4l;
}

int v4l_capture_test(int fd_v4l, unsigned int *pStrm, void *encInst,
		      char *outfile_str)
{
	MP4API_EncInput encInp;
	MP4API_EncOutput encOut;
	int ret = 0;

	struct v4l2_buffer buf;
	struct v4l2_format fmt;
	int count = g_capture_count;
	int fid_mpeg4;

	fid_mpeg4 = open(outfile_str, O_RDWR | O_CREAT);
	if (fid_mpeg4 < 0) {
		printf("Unable to open %s\n", outfile_str);
		return -1;
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0) {
		printf("get format failed\n");
		close(fid_mpeg4);
		return -1;
	}

	printf("\t Width = %d", fmt.fmt.pix.width);
	printf("\t Height = %d", fmt.fmt.pix.height);
	printf("\t Image size = %d\n", fmt.fmt.pix.sizeimage);
	printf("\t pixelformat = %d\n", fmt.fmt.pix.pixelformat);

	/* Start stream */
	encInp.pu32_Picture = (unsigned int *)buffers[0].offset;
	encInp.u32_TimeIncrement = 0;
	encInp.pu32_OutputBuffer = pStrm;
	encInp.u32_OutBufSize = 1.5 * 384 * 1024 / 8 * VOPS;

	/* 1st = intra */
	encInp.VopCodingType = MP4API_ENC_VOP_CODING_TYPE_INTRA;
	ret = MP4API_EncoderStartStream(encInst, &encInp, &encOut);
	if (ret) {
		ret = -1;
		goto exitpoint;
	}

	encInp.u32_OutBufSize -= encOut.u32_Size;

	printf("Strm started (%d bytes)\n", encOut.u32_Size);
	write(fid_mpeg4, (u8 *) encInp.pu32_OutputBuffer, encOut.u32_Size);

	pStrm += encOut.u32_Size;

	if (start_capturing(fd_v4l) < 0) {
		printf("start capturing failed\n");
		ret = -1;
		goto exitpoint;
	}

	while (count-- > 0) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(fd_v4l, VIDIOC_DQBUF, &buf) < 0) {
			printf("VIDIOC_DQBUF failed.\n");
			ret = -1;
			break;
		}

		encInp.pu32_Picture = (unsigned int *)buffers[buf.index].offset;

		ret = MP4API_Encode(encInst, &encInp, &encOut);
		if (encOut.Code != MP4API_ENC_PICTURE_READY) {
			printf("VOP Encoded failed (%d bytes)\n",
			       encOut.u32_Size);
			ret = -1;
			break;
		}

		write(fid_mpeg4, (u8 *) encInp.pu32_OutputBuffer,
		      encOut.u32_Size);
		encInp.u32_OutBufSize -= encOut.u32_Size;
		encInp.VopCodingType = MP4API_ENC_VOP_CODING_TYPE_INTER;
		encInp.u32_TimeIncrement = 1;

		if (count >= TEST_BUFFER_NUM) {
			if (ioctl(fd_v4l, VIDIOC_QBUF, &buf) < 0) {
				printf("VIDIOC_QBUF failed\n");
				ret = -1;
				break;
			}
		} else
			printf("buf.index %d count = %d\n", buf.index, count);

	}

exitpoint:
	MP4API_EncoderEndStream(encInst, &encInp, &encOut);
	encInp.u32_OutBufSize -= encOut.u32_Size;

	printf("Strm ended (%d bytes)\n", encOut.u32_Size);
	write(fid_mpeg4, (u8 *) encInp.pu32_OutputBuffer, encOut.u32_Size);

	stop_capturing(fd_v4l);
	close(fid_mpeg4);
	return ret;
}

int main(int argc, char **argv)
{
	void *encInst = NULL;
	unsigned int *pStrm = NULL;
	int ret = 0;
	int fd_v4l;

	g_width = atoi(argv[1]);
	g_height = atoi(argv[2]);
	g_capture_count = atoi(argv[3]);
	g_frame_rate = atoi(argv[4]);

	fd_v4l = v4l_capture_setup();
	if (fd_v4l < 0)
		return -1;

	/* allocate mpeg4 stream buffer */
	pStrm = malloc(1.5 * 384 * 1024 / 8 * sizeof(unsigned char));
	if (pStrm == NULL) {
		printf("Failed to malloc\n");
		close(fd_v4l);
		return -1;
	}

	/* init enc */
	encInst = MP4API_EncoderInit(MP4API_ENC_SCHEME_CIF_30FPS_384KB_VP);
	if (encInst == NULL) {
		ret = -1;
		goto error1;
	}

	/* config enc */
	/* Disable video packets */
	{
		unsigned int ret;
		MP4API_EncParam_ErrorTools params;

		params.u8_VideoPacketsEnable = 0;	/* disabled */
		params.u8_DataPartitionEnable = 0;	/* disabled */
		params.u8_RVlcEnable = 0;	/* disabled */

		printf("Changing VP Enable   = %d\n",
		       params.u8_VideoPacketsEnable);
		printf("Changing DP Enable   = %d\n",
		       params.u8_DataPartitionEnable);
		printf("Changing RVLC Enable = %d\n", params.u8_RVlcEnable);
		ret = MP4API_EncoderConfig(encInst, &params,
					 MP4API_ENC_PID_ERROR_TOOLS);
		if (ret) {
			ret = -1;
			goto error2;
		}
	}

	/* Set interrupt interval */
	{
		unsigned int ret;
		u16 param = ASICRUNLENGTH;

		printf("Changing HW interrupt interval to %d\n", param);
		ret = MP4API_EncoderConfig(encInst,
					   &param,
					   MP4API_ENC_PID_HW_INTERRUPT_INTERVAL);
		if (ret) {
			ret = -1;
			goto error2;
		}
	}

	/* Disable CIR */
	{
		MP4API_EncParam_CyclicIntraRefresh params;

		params.u32_CirEnable = 0;
		params.u32_IntraPerVop = 0;

		printf("Changing CIR Enable = %d\n", params.u32_CirEnable);
		printf("Changing Intra/VOP  = %d\n", params.u32_IntraPerVop);
		ret = MP4API_EncoderConfig(encInst,
					   &params,
					   MP4API_ENC_PID_CYCLIC_INTRA_REFRESH);
		if (ret) {
			ret = -1;
			goto error2;
		}
	}

	{
		MP4API_EncParam_PictureSize pic_param;
		pic_param.u16_Height = g_height;
		pic_param.u16_Width = g_width;
		ret = MP4API_EncoderConfig(encInst,
					   &pic_param,
					   MP4API_ENC_PID_PICTURE_SIZE);
		if (ret) {
			ret = -1;
			goto error2;
		}
	}

	{
		u16 param = g_frame_rate;
		ret = MP4API_EncoderConfig(encInst,
					   &param,
					   MP4API_ENC_PID_TIME_RESOLUTION);
		if (ret) {
			ret = -1;
			goto error2;
		}
	}

	ret = v4l_capture_test(fd_v4l, pStrm, encInst, argv[argc - 1]);

error2:
	MP4API_EncoderShutdown(encInst);
error1:
	close(fd_v4l);
	free(pStrm);
	return (ret);
}

