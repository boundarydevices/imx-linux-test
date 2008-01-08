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
 * Include Files
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/dma-mapping.h>
//#include <linux/devfs_fs_kernel.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>

#include <asm/arch/mxc_ipc.h>
//#include <asm/arch/ipctest.h>

#define DEBUG 1

#if DEBUG
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#define IOCTL_IPCTEST_EXEC_OPEN_CLOSE_TEST                      0x0
#define IOCTL_IPCTEST_EXEC_PACKET_DATA_LOOPBACK                 0x1
#define IOCTL_IPCTEST_EXEC_SHORT_MSG_LOOPBACK                   0x2
#define IOCTL_IPCTEST_EXEC_PACKET_DATA_WRITE_EX_CONT_LOOPBACK   0x5
#define IOCTL_IPCTEST_EXEC_PACKET_DATA_WRITE_EX_LINK_LOOPBACK   0x6
#define IOCTL_IPCTEST_EXEC_PACKET_DATA_WRITE_EX2   0x7
#define IOCTL_IPCTEST_EXEC_PACKET_DATA_READ_EX2  0x8
#define IOCTL_IPCTEST_EXEC_PACKET_DATA_READ_EX2_RAND 0x9
#define IOCTL_IPCTEST_EXEC_LOGGING_LOOPBACK                     0x4

#define IPCTEST_MAJOR                                   0
#define MAX_BUFFER_SIZE                                 512
#define CRC_POLYVAL                             	 0x8C
#define NUM_OF_BD_MAX					50

/*performance Measurement structs */

struct timeval tv_start, tv_end;
unsigned long total_time;
static unsigned long Bytes_Count;

struct ioctl_args {
	int iterations;
	unsigned short channel;
	unsigned short vchannel;
	unsigned short bytes;
	unsigned short message;
};

static int major_num = 0;
static struct class *ipc_tm_class;

static wait_queue_head_t write_queue, read_queue, notify_queue;
static bool read_done, write_done;

static void read_callback(HW_CTRL_IPC_READ_STATUS_T * status)
{
	do_gettimeofday(&tv_end);

	total_time = total_time + ((tv_end.tv_sec - tv_start.tv_sec) * 1000000);
	total_time += tv_end.tv_usec - tv_start.tv_usec;

	if (total_time) {
		DPRINTK("\n Total time to execute Read is %ld us \n",
			total_time);

	}

	DPRINTK("IPC: %d bytes read on channel nb %d\n", status->nb_bytes,
		status->channel->channel_nb);
	read_done = true;
	wake_up_interruptible(&read_queue);
}

static void write_callback(HW_CTRL_IPC_WRITE_STATUS_T * status)
{
	DPRINTK("IPC: %d bytes wrote on channel nb %d\n", status->nb_bytes,
		status->channel->channel_nb);
	write_done = true;
	wake_up_interruptible(&write_queue);
}

static void notify_callback(HW_CTRL_IPC_NOTIFY_STATUS_T * status)
{
	DPRINTK("IPC: Notify callback called from channel nb %d\n",
		status->channel->channel_nb);
	wake_up_interruptible(&notify_queue);
}

static void read_callback_new(HW_CTRL_IPC_READ_STATUS_T * status)
{
	DPRINTK("IPC: New Read callback invoked successfully\n");
	read_callback(status);
}

static void write_callback_new(HW_CTRL_IPC_WRITE_STATUS_T * status)
{
	DPRINTK("IPC: New Write callback invoked successfully\n");
	write_callback(status);
}

char crc_generator(char *ctrl_ptr, unsigned long size)
{

	unsigned char crc;
	unsigned char carac;
	unsigned int index;
	unsigned int j;
	index = 0;
	crc = 0;
	carac = 0;

	while (index < size) {
		carac = *(ctrl_ptr + index);
		for (j = 0; j < 8; j++) {
			if ((carac ^ crc) & 0x01) {
				crc = (crc >> 1) ^ CRC_POLYVAL;	/*Polynomialx8 +  x5 + x4 + 1 */
			} else {
				crc >>= 1;
			}
			carac >>= 1;
		}
		index++;

	}
	DPRINTK(" CRC generated on this Buffer is (hex) : %x\n", crc);
	return crc;

}

int check_data_integrity(char *buf1, char *buf2, int count)
{
	int result = 0;
	int i;

	for (i = 0; i < count; i++) {
		if (buf1[i] != buf2[i]) {
			printk("Corrupted data at %d wbuf = %d rbuf = %d\n",
			       i, buf1[i], buf2[i]);
			result = -1;
		}
	}
	return result;
}

int exec_open_close_test(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	HW_CTRL_IPC_CHANNEL_T *channel;
	int result;
	int i;

	channel = kmalloc(sizeof(HW_CTRL_IPC_CHANNEL_T), GFP_KERNEL);
	channel->channel_nb = ((struct ioctl_args *)arg)->channel;

	DPRINTK("IPC: opening and closing a SHORT MESSAGE IPC channel from "
		"kernel multiple times\n");

	config.index = channel->channel_nb;
	config.type = HW_CTRL_IPC_SHORT_MSG;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;
	kfree(channel);

	for (i = 0; i < 100; i++) {
		channel = hw_ctrl_ipc_open(&config);
		if (channel == 0) {
			DPRINTK("IPC: Unable to open virtual channel %d\n",
				channel->channel_nb);
			return -1;
		}

		result = hw_ctrl_ipc_close(channel);
		if (result != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC: Unable to close virtual channel %d\n",
				channel->channel_nb);
			return -1;
		}
	}

	DPRINTK("IPC: open_close test OK \n");

	return 0;
}

int exec_short_message_loopback(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	unsigned short channel;
	int result = 0, i;
	int iterations;
	char wbuf[4];
	char rbuf[4];
	int message;
	HW_CTRL_IPC_CHANNEL_T *vchannel = NULL;

	channel = ((struct ioctl_args *)arg)->channel;
	message = ((struct ioctl_args *)arg)->message;
	iterations = ((struct ioctl_args *)arg)->iterations;

	DPRINTK("IPC: about to send %d messages on channel %d\n", iterations,
		channel);

	config.index = channel;
	config.type = HW_CTRL_IPC_SHORT_MSG;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;

	vchannel = hw_ctrl_ipc_open(&config);
	if (vchannel == 0) {
		DPRINTK("IPC: Unable to open virtual channel %d\n",
			vchannel->channel_nb);
		return -1;
	}

	memset(wbuf, 0, 4);
	memset(rbuf, 0, 4);

	for (i = 0; i < 4; i++) {
		wbuf[i] = (char)i;
	}

	i = 0;
	while (i < iterations) {
		write_done = false;
		read_done = false;
		result = hw_ctrl_ipc_write(vchannel, wbuf, 4);
		if (result == HW_CTRL_IPC_STATUS_ERROR) {
			DPRINTK("IPC: Error on hw_ctrl_ipc_write function\n");
			break;
		}

		wait_event_interruptible(write_queue, write_done);

		result = hw_ctrl_ipc_read(vchannel, rbuf, 4);
		if (result != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC: Error on hw_ctrl_ipc_read function\n");
			break;
		}

		wait_event_interruptible(read_queue, read_done);

		DPRINTK("IPC: Received message # %d from channel %d\n", i,
			channel);

		if (check_data_integrity(wbuf, rbuf, 4) == -1) {
			break;
		}

		memset(rbuf, 0, 4);

		i++;
	}

	if (hw_ctrl_ipc_close(vchannel) != HW_CTRL_IPC_STATUS_OK) {
		DPRINTK("IPC: Error on hw_ctrl_ipc_close function\n");
		result = -1;
	}

	if (result == 0) {
		DPRINTK("TEST for Short Message channels OK\n");
	} else {
		DPRINTK("TEST for Short Message channels FAILED\n");
	}
	return result;
}

int exec_packet_data_loopback(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	int status = 0, i;
	int iterations;
	HW_CTRL_IPC_CHANNEL_T *vchannel = NULL;
	char *wbuf;
	char *rbuf;
	dma_addr_t wpaddr;
	dma_addr_t rpaddr;
	int bytes;

	bytes = ((struct ioctl_args *)arg)->bytes;
	iterations = ((struct ioctl_args *)arg)->iterations;

	wbuf = dma_alloc_coherent(NULL, bytes, &wpaddr, GFP_DMA);
	rbuf = dma_alloc_coherent(NULL, bytes, &rpaddr, GFP_DMA);

	DPRINTK("IPC: about to send %d bytes on channel 2\n", bytes);

	config.index = 0;
	config.type = HW_CTRL_IPC_PACKET_DATA;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;

	vchannel = hw_ctrl_ipc_open(&config);
	if (vchannel == 0) {
		DPRINTK("IPC: Unable to open virtual channel %d\n",
			vchannel->channel_nb);
		return -1;
	}

	memset(wbuf, 0, bytes);
	memset(rbuf, 0, bytes);

	for (i = 0; i < bytes; i++) {
		wbuf[i] = (char)i;
	}

	i = 0;
	while (i < iterations) {
		write_done = false;
		read_done = false;
		status =
		    hw_ctrl_ipc_write(vchannel, (unsigned char *)wpaddr, bytes);
		if (status == HW_CTRL_IPC_STATUS_ERROR) {
			DPRINTK("IPC: Error on hw_ctrl_ipc_write function\n");
			status = -1;
			break;
		}

		wait_event_interruptible(write_queue, write_done);

		status =
		    hw_ctrl_ipc_read(vchannel, (unsigned char *)rpaddr, bytes);
		if (status != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC: Error on hw_ctrl_ipc_read function\n");
			status = -1;
			break;
		}

		wait_event_interruptible(read_queue, read_done);

		DPRINTK("IPC: Received message # %d from channel 2\n", i);

		if (check_data_integrity(wbuf, rbuf, bytes) == -1) {
			DPRINTK("IPC: TEST FAILED on channel %d iteration %d\n",
				vchannel->channel_nb, i);
			status = -1;
			break;
		}

		memset(rbuf, 0, bytes);
		i++;
		/* Change callbacks for last iteration */
		if (i == (iterations - 1)) {
			hw_ctrl_ipc_ioctl(vchannel,
					  HW_CTRL_IPC_SET_READ_CALLBACK,
					  (void *)read_callback_new);
			hw_ctrl_ipc_ioctl(vchannel,
					  HW_CTRL_IPC_SET_WRITE_CALLBACK,
					  (void *)write_callback_new);
			hw_ctrl_ipc_ioctl(vchannel,
					  HW_CTRL_IPC_SET_MAX_CTRL_STRUCT_NB,
					  (void *)32);
		}
	}

	if (hw_ctrl_ipc_close(vchannel) != HW_CTRL_IPC_STATUS_OK) {
		DPRINTK("IPC: Error on hw_ctrl_ipc_close function\n");
		status = -1;
	}

	dma_free_coherent(NULL, bytes, wbuf, wpaddr);
	dma_free_coherent(NULL, bytes, rbuf, rpaddr);
	if (status == 0) {
		DPRINTK("TEST for Packet Data channel OK\n");
	} else {
		DPRINTK("TEST for Packet Data channel FAILED\n");
	}

	return status;
}

int exec_packet_data_write_ex_cont_loopback(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	int status = 0, i;
	int iterations;
	HW_CTRL_IPC_CHANNEL_T *vchannel = NULL;
	HW_CTRL_IPC_WRITE_PARAMS_T write_buf;
	char *wbuf;
	char *rbuf;
	dma_addr_t wpaddr;
	dma_addr_t rpaddr;
	int bytes;

	bytes = ((struct ioctl_args *)arg)->bytes;
	iterations = ((struct ioctl_args *)arg)->iterations;

	wbuf = dma_alloc_coherent(NULL, bytes, &wpaddr, GFP_DMA);
	rbuf = dma_alloc_coherent(NULL, bytes, &rpaddr, GFP_DMA);

	DPRINTK("IPC: about to send %d bytes on channel 2\n", bytes);

	config.index = 0;
	config.type = HW_CTRL_IPC_PACKET_DATA;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;

	vchannel = hw_ctrl_ipc_open(&config);
	if (vchannel == 0) {
		DPRINTK("IPC: Unable to open virtual channel %d\n",
			vchannel->channel_nb);
		return -1;
	}

	memset(wbuf, 0, bytes);
	memset(rbuf, 0, bytes);

	for (i = 0; i < bytes; i++) {
		wbuf[i] = (char)i;
	}

	write_buf.ipc_memory_read_mode = HW_CTRL_IPC_MODE_CONTIGUOUS;
	write_buf.read.cont_ptr = (HW_CTRL_IPC_CONTIGUOUS_T *)
	    kmalloc(sizeof(HW_CTRL_IPC_CONTIGUOUS_T), GFP_KERNEL);
	write_buf.read.cont_ptr->data_ptr = (unsigned char *)wpaddr;
	write_buf.read.cont_ptr->length = bytes;

	i = 0;
	while (i < iterations) {
		write_done = false;
		read_done = false;

		status = hw_ctrl_ipc_write_ex(vchannel, &write_buf);
		if (status == HW_CTRL_IPC_STATUS_ERROR) {
			DPRINTK
			    ("IPC:Error in hw_ctrl_ipc_write_ex function %d\n",
			     status);
			status = -1;
			break;
		}

		wait_event_interruptible(write_queue, write_done);

		status =
		    hw_ctrl_ipc_read(vchannel, (unsigned char *)rpaddr, bytes);
		if (status != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC:Error on hw_ctrl_ipc_read function\n");
			status = -1;
			break;
		}

		wait_event_interruptible(read_queue, read_done);

		DPRINTK("IPC: Received message # %d from channel 2\n", i);

		if (check_data_integrity(wbuf, rbuf, bytes) == -1) {
			DPRINTK("IPC: TEST FAILED on channel %d iteration %d\n",
				vchannel->channel_nb, i);
			status = -1;
			break;
		}

		memset(rbuf, 0, bytes);
		i++;
	}

	if (hw_ctrl_ipc_close(vchannel) != HW_CTRL_IPC_STATUS_OK) {
		DPRINTK("IPC: Error on hw_ctrl_ipc_close function\n");
		status = -1;
	}
	write_buf.read.cont_ptr->data_ptr = NULL;
	kfree(write_buf.read.cont_ptr);
	dma_free_coherent(NULL, bytes, wbuf, wpaddr);
	dma_free_coherent(NULL, bytes, rbuf, rpaddr);
	if (status == 0) {
		DPRINTK
		    ("TEST for Contiguous write_ex with Packet Data chnl OK\n");
	} else {
		DPRINTK
		    ("TEST for Contiguous write_ex with Packet Data chnl FAILED\n");

	}

	return status;
}

int exec_packet_data_write_ex2(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	int status = 0, i;
	int iterations;
	HW_CTRL_IPC_CHANNEL_T *vchannel = NULL;
	char *wbuf;
	char *rbuf;
	dma_addr_t wpaddr;
	dma_addr_t rpaddr;
	int bytes;
	HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T wbuf_BD[NUM_OF_BD_MAX];
	HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T rbuf_BD[NUM_OF_BD_MAX];

	bytes = ((struct ioctl_args *)arg)->bytes;
	iterations = ((struct ioctl_args *)arg)->iterations;

	wbuf = dma_alloc_coherent(NULL, (bytes), &wpaddr, GFP_DMA);
	rbuf = dma_alloc_coherent(NULL, (bytes), &rpaddr, GFP_DMA);

	DPRINTK("IPC: about to send %d bytes on channel 2\n", bytes);
	DPRINTK("IPC: number of iterations\n for %d ", iterations);

	config.index = 0;
	config.type = HW_CTRL_IPC_PACKET_DATA;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;

	vchannel = hw_ctrl_ipc_open(&config);
	if (vchannel == 0) {
		DPRINTK("IPC: Unable to open virtual channel %d\n",
			vchannel->channel_nb);
		return -1;
	}

	memset(wbuf, 0, (bytes));
	memset(rbuf, 0, (bytes));

	memset(wbuf_BD, 0,
	       NUM_OF_BD_MAX * sizeof(HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T));
	memset(rbuf_BD, 0,
	       NUM_OF_BD_MAX * sizeof(HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T));
	/*Create Buffer Descriptors */

	/* Populate the write buffer descriptor */

	wbuf_BD[0].data_ptr = (unsigned char *)wpaddr;
	wbuf_BD[0].comand = 0;
	wbuf_BD[0].length = 500;

	wbuf_BD[1].data_ptr = (unsigned char *)wpaddr + 500;
	wbuf_BD[1].comand = 0;
	wbuf_BD[1].length = 500;

	wbuf_BD[2].data_ptr = (unsigned char *)wpaddr + 1000;
	wbuf_BD[2].comand = 0x8000;
	wbuf_BD[2].length = 500;

	/* end of frame 1--start of frame 2 */

	wbuf_BD[3].data_ptr = (unsigned char *)wpaddr + 1500;
	wbuf_BD[3].comand = 0;
	wbuf_BD[3].length = 500;

	wbuf_BD[4].data_ptr = (unsigned char *)wpaddr + 2000;
	wbuf_BD[4].comand = 0;
	wbuf_BD[4].length = 500;

	wbuf_BD[5].data_ptr = (unsigned char *)wpaddr + 2500;
	wbuf_BD[5].comand = 0xC000;
	wbuf_BD[5].length = 500;

	/* Populate the read buffer descriptor */
	rbuf_BD[0].data_ptr = (unsigned char *)rpaddr;
	rbuf_BD[0].comand = 0;
	rbuf_BD[0].length = 500;

	rbuf_BD[1].data_ptr = (unsigned char *)rpaddr + 500;
	rbuf_BD[1].comand = 0;
	rbuf_BD[1].length = 500;

	rbuf_BD[2].data_ptr = (unsigned char *)rpaddr + 1000;
	rbuf_BD[2].comand = 0x0;
	rbuf_BD[2].length = 500;

	/* end of frame 1--start of frame 2 */

	rbuf_BD[3].data_ptr = (unsigned char *)rpaddr + 1500;
	rbuf_BD[3].comand = 0;
	rbuf_BD[3].length = 500;

	rbuf_BD[4].data_ptr = (unsigned char *)rpaddr + 2000;
	rbuf_BD[4].comand = 0;
	rbuf_BD[4].length = 500;

	rbuf_BD[5].data_ptr = (unsigned char *)rpaddr + 2500;
	rbuf_BD[5].comand = 0x4000;
	rbuf_BD[5].length = 500;

	for (i = 0; i < bytes; i++) {
		wbuf[i] = (char)i;
	}

	i = 0;
	while (i < iterations) {
		write_done = false;
		read_done = false;

		total_time = 0;
		do_gettimeofday(&tv_start);
		status =
		    hw_ctrl_ipc_write_ex2(vchannel,
					  (HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *)
					  wbuf_BD);
		if (status == HW_CTRL_IPC_STATUS_ERROR) {
			DPRINTK
			    ("IPC:Error in hw_ctrl_ipc_write_ex2 function %d\n",
			     status);
			status = -1;
			break;
		}

		wait_event_interruptible(write_queue, write_done);

		status =
		    hw_ctrl_ipc_read_ex2(vchannel,
					 (HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *)
					 rbuf_BD);
		if (status != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC:Error on hw_ctrl_ipc_read function\n");
			status = -1;
			break;
		}

		wait_event_interruptible(read_queue, read_done);

		do_gettimeofday(&tv_end);

		total_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000;
		total_time += tv_end.tv_usec - tv_start.tv_usec;

		if (total_time)
			DPRINTK("\nTotal Time  %ld us \n", total_time);

		if (check_data_integrity(wbuf, rbuf, bytes) == -1) {
			DPRINTK("IPC: TEST FAILED on channel %d iteration %d\n",
				vchannel->channel_nb, i);
			status = -1;
			break;
		}

		memset(rbuf, 0, bytes);

		rbuf_BD[0].comand = 0x0;
		rbuf_BD[1].comand = 0x0;
		rbuf_BD[2].comand = 0x0;
		rbuf_BD[3].comand = 0x0;
		rbuf_BD[4].comand = 0x0;
		rbuf_BD[5].comand = 0x4000;

		i++;
	}

	if (hw_ctrl_ipc_close(vchannel) != HW_CTRL_IPC_STATUS_OK) {
		DPRINTK("IPC: Error on hw_ctrl_ipc_close function\n");
		status = -1;
	}

	dma_free_coherent(NULL, bytes, wbuf, wpaddr);
	dma_free_coherent(NULL, bytes, rbuf, rpaddr);
	if (status == 0) {
		DPRINTK
		    ("TEST for Contiguous write_ex2 with Packet Data chnl OK\n");
	} else {
		DPRINTK
		    ("TEST for Contiguous write_ex2 with Packet Data chnl FAILED\n");

	}

	return status;
}

int exec_packet_data_write_ex_link_loopback(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	int status = 0, i;
	int iterations;
	HW_CTRL_IPC_CHANNEL_T *vchannel = NULL;
	HW_CTRL_IPC_WRITE_PARAMS_T write_buf;
	char *wbuf;
	char *rbuf;
	dma_addr_t wpaddr;
	dma_addr_t rpaddr;
	int bytes;

	bytes = ((struct ioctl_args *)arg)->bytes;
	iterations = ((struct ioctl_args *)arg)->iterations;

	wbuf = dma_alloc_coherent(NULL, bytes, &wpaddr, GFP_DMA);
	rbuf = dma_alloc_coherent(NULL, bytes, &rpaddr, GFP_DMA);

	DPRINTK("IPC: about to send %d bytes on channel 2\n", bytes);

	config.index = 0;
	config.type = HW_CTRL_IPC_PACKET_DATA;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;

	vchannel = hw_ctrl_ipc_open(&config);
	if (vchannel == 0) {
		DPRINTK("IPC: Unable to open virtual channel %d\n",
			vchannel->channel_nb);
		return -1;
	}

	memset(wbuf, 0, bytes);
	memset(rbuf, 0, bytes);

	for (i = 0; i < bytes; i++) {
		wbuf[i] = (char)i;
	}

	write_buf.ipc_memory_read_mode = HW_CTRL_IPC_MODE_LINKED_LIST;
	write_buf.read.list_ptr = (HW_CTRL_IPC_LINKED_LIST_T *)
	    kmalloc(sizeof(HW_CTRL_IPC_LINKED_LIST_T), GFP_KERNEL);
	write_buf.read.list_ptr->data_ptr = (unsigned char *)wpaddr;
	write_buf.read.list_ptr->length = bytes;
	write_buf.read.list_ptr->next = NULL;

	i = 0;
	while (i < iterations) {
		write_done = false;
		read_done = false;

		status = hw_ctrl_ipc_write_ex(vchannel, &write_buf);
		if (status == HW_CTRL_IPC_STATUS_ERROR) {
			DPRINTK("IPC:Error in hw_ctrl_ipc_write_ex function\n");
			status = -1;
			break;
		}

		wait_event_interruptible(write_queue, write_done);

		status =
		    hw_ctrl_ipc_read(vchannel, (unsigned char *)rpaddr, bytes);
		if (status != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC:Error on hw_ctrl_ipc_read function\n");
			status = -1;
			break;
		}

		wait_event_interruptible(read_queue, read_done);

		DPRINTK("IPC: Received message # %d from channel 2\n", i);

		if (check_data_integrity(wbuf, rbuf, bytes) == -1) {
			DPRINTK("IPC: TEST FAILED on channel %d iteration %d\n",
				vchannel->channel_nb, i);
			status = -1;
			break;
		}

		memset(rbuf, 0, bytes);
		i++;
	}

	if (hw_ctrl_ipc_close(vchannel) != HW_CTRL_IPC_STATUS_OK) {
		DPRINTK("IPC: Error on hw_ctrl_ipc_close function\n");
		status = -1;
	}
	write_buf.read.list_ptr->data_ptr = NULL;
	kfree(write_buf.read.list_ptr);
	dma_free_coherent(NULL, bytes, wbuf, wpaddr);
	dma_free_coherent(NULL, bytes, rbuf, rpaddr);
	if (status == 0) {
		DPRINTK("TEST for Linked write_ex with Packet Data chnl OK\n");
	} else {
		DPRINTK
		    ("TEST for Linked write_ex with Packet Data chnl FAILED\n");
	}

	return status;
}

int exec_packet_data_read_ex2(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	int status = 0, i;
	int iterations;
	HW_CTRL_IPC_CHANNEL_T *vchannel = NULL;
	char *write_buf;
	char *rbuf;
	dma_addr_t rpaddr, wpaddr;
	HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T rbuf2_BD[NUM_OF_BD_MAX];
	int bytes;

	bytes = ((struct ioctl_args *)arg)->bytes;
	iterations = ((struct ioctl_args *)arg)->iterations;

	rbuf = dma_alloc_coherent(NULL, bytes, &rpaddr, GFP_DMA);
	write_buf = dma_alloc_coherent(NULL, bytes, &wpaddr, GFP_DMA);

	DPRINTK("IPC: about to send %d bytes on channel 2\n", bytes);

	config.index = 0;
	config.type = HW_CTRL_IPC_PACKET_DATA;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;

	vchannel = hw_ctrl_ipc_open(&config);
	if (vchannel == 0) {
		DPRINTK("IPC:  Unable to open virtual channel %d\n",
			vchannel->channel_nb);
		return -1;
	}

	memset(rbuf, 0, bytes);
	memset(write_buf, 0, bytes);
	memset(rbuf2_BD, 0,
	       NUM_OF_BD_MAX * sizeof(HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T));

	/* Populate the read buffer descriptor */

	rbuf2_BD[0].data_ptr = (unsigned char *)rpaddr;
	rbuf2_BD[0].comand = 0;
	rbuf2_BD[0].length = 1500;

	rbuf2_BD[1].data_ptr = (unsigned char *)rpaddr + 1500;
	rbuf2_BD[1].comand = 0;
	rbuf2_BD[1].length = 1500;

	rbuf2_BD[2].data_ptr = (unsigned char *)rpaddr + 3000;
	rbuf2_BD[2].comand = 0x4000;
	rbuf2_BD[2].length = 1500;

	/*!Fill buffer with data pattern sent by BP for integrity checking */

	for (i = 0; i < bytes; i++) {
		write_buf[i] = (char)0x55;
	}

	/*calibrate timer */
	total_time = 0;
	do_gettimeofday(&tv_start);

	msleep_interruptible(1000);

	do_gettimeofday(&tv_end);

	total_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000;
	total_time += (tv_end.tv_usec - tv_start.tv_usec);

	if (total_time)
		DPRINTK("\nTotal Time  %ld us \n", total_time);

	/*! Initialize timer and start for performance measurements */
	total_time = 0;
	do_gettimeofday(&tv_start);

	i = 0;

	while (i < iterations) {
		write_done = false;
		read_done = false;

		total_time = 0;
		do_gettimeofday(&tv_start);

		status =
		    hw_ctrl_ipc_read_ex2(vchannel,
					 (HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *)
					 rbuf2_BD);
		if (status != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC:Error on hw_ctrl_ipc_read function\n");
			status = -1;
			break;
		}

		wait_event_interruptible(read_queue, read_done);

		DPRINTK("IPC: Received message # %d from channel 2\n", i);

		//Do a CRC check on the recieve buffer and compare to CRC generated by the DSP 
		//stored in the last byte of the each buffer recieved.

		if (check_data_integrity(write_buf, rbuf, bytes) == -1) {
			DPRINTK
			    ("IPC: TEST FAILED on channel %d iteration %d\n",
			     vchannel->channel_nb, i);
			status = -1;
			break;

		}

		memset(rbuf, 0, bytes);

		rbuf2_BD[0].comand = 0x0;
		rbuf2_BD[1].comand = 0x0;
		rbuf2_BD[2].comand = 0x4000;

		i++;
	}

	if (hw_ctrl_ipc_close(vchannel) != HW_CTRL_IPC_STATUS_OK) {
		DPRINTK("IPC: Error on hw_ctrl_ipc_close function\n");
		status = -1;
	}
	dma_free_coherent(NULL, bytes, rbuf, rpaddr);
	dma_free_coherent(NULL, bytes, write_buf, wpaddr);

	if (status == 0) {
		DPRINTK
		    ("TEST for Contiguous read_ex2 with Packet Data chnl OK\n");
	} else {
		DPRINTK
		    ("TEST for Contiguous read_ex2 with Packet Data chnl FAILED\n");

	}

	return status;
}

int exec_packet_data_read_ex2_rand(unsigned long arg)
{
	HW_CTRL_IPC_OPEN_T config;
	int status = 0, i, j;
	int iterations;
	HW_CTRL_IPC_CHANNEL_T *vchannel = NULL;
	char *rbuf;
	char *rbuf_anchor = NULL;
	dma_addr_t rpaddr;
	HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T rbuf2_BD[NUM_OF_BD_MAX];
	int bytes;
	unsigned long buf_offset = 0;
	char *temp2 = NULL;

	bytes = ((struct ioctl_args *)arg)->bytes;
	iterations = ((struct ioctl_args *)arg)->iterations;
	DPRINTK("Number of Iteration is: %x \n", iterations);
	rbuf = dma_alloc_coherent(NULL, bytes, &rpaddr, GFP_DMA);
	DPRINTK("IPC: about to send %d bytes on channel 2\n", bytes);

	config.index = 0;
	config.type = HW_CTRL_IPC_PACKET_DATA;
	config.read_callback = read_callback;
	config.write_callback = write_callback;
	config.notify_callback = notify_callback;

	vchannel = hw_ctrl_ipc_open(&config);
	if (vchannel == 0) {
		DPRINTK("IPC:  Unable to open virtual channel %d\n",
			vchannel->channel_nb);
		return -1;
	}

	/*calibrate timer */
	total_time = 0;
	do_gettimeofday(&tv_start);

	msleep_interruptible(1000);

	do_gettimeofday(&tv_end);

	total_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000;
	total_time += (tv_end.tv_usec - tv_start.tv_usec);

	if (total_time)
		DPRINTK("Calibrate timer: Total Time for 1 sec is: %ld us \n",
			total_time);

	/*! Initialization of Loop variables */
	i = 0;
	j = 0;

	/*!Store Read Buffer Pointer so that it can be reset on 2nd Iteration */
	rbuf_anchor = rbuf;

	/*!Iterative loop used in the execution of ReadEx2 */

	while (j < iterations) {
		write_done = false;
		read_done = false;

		memset(rbuf, 0, bytes);
		memset(rbuf2_BD, 0,
		       NUM_OF_BD_MAX *
		       sizeof(HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T));

		total_time = 0;
		do_gettimeofday(&tv_start);

		/* Populate the read buffer descriptor */

		i = 0;
		for (i = 0; i < 10; i++) {

			rbuf2_BD[i].data_ptr =
			    ((unsigned char *)rpaddr + (i * 1500));
			rbuf2_BD[i].comand = 0;
			rbuf2_BD[i].length = 1500;

		}
		rbuf2_BD[9].comand = 0x4000;	/* set the end bit */

		/*!Execute read_ex2 using the above intialized Data node descriptors */
		status =
		    hw_ctrl_ipc_read_ex2(vchannel,
					 (HW_CTRL_IPC_DATA_NODE_DESCRIPTOR_T *)
					 rbuf2_BD);
		if (status != HW_CTRL_IPC_STATUS_OK) {
			DPRINTK("IPC:Error on hw_ctrl_ipc_read function\n");
			status = -1;
			break;
		}
		/*! Intialize semaphore to wait for Read call back from IAPI */

		wait_event_interruptible(read_queue, read_done);

		DPRINTK("IPC: Received message # %d from channel 2\n", j);

		/*!
		 * Generate a CRC on the recieved buffers 
		 * and compare to CRC generated by the BP side which 
		 * is stored in the last byte of each Buffer recieved
		 *
		 */

		i = 0;

		Bytes_Count = 0;

		for (i = 0; i < 10; i++) {

			buf_offset = rbuf2_BD[i].length;

			Bytes_Count += rbuf2_BD[i].length;

			/*compute the location of the CRC value in each buffer and store in temp2 */
			temp2 = rbuf + buf_offset - 1;

			DPRINTK(" Buffer CRC in frame is: %x \n", *temp2);
			DPRINTK(" Buffer Length: %x \n", rbuf2_BD[i].length);
			DPRINTK(" Buffer Status %x \n", rbuf2_BD[i].comand);

			/*!Compute the CRC and Compare to the BP side CRC */

			if (crc_generator
			    (rbuf,
			     (unsigned long)(rbuf2_BD[i].length - 1)) !=
			    *temp2) {

				DPRINTK
				    ("IPC: TEST FAILED CRC Check on channel %d iteration %d\n",
				     vchannel->channel_nb, i);
				status = -1;
				break;
			}
			/*Check for end of frame break the loop and clear Done status bit  as required signal IAPI/scripts */
			if ((rbuf2_BD[i].comand & 0x4000) != 0) {
				/*!Reset Read buffer pointer */

				rbuf = rbuf_anchor;
				rbuf2_BD[i].comand = 0x0;
				break;
			}
			/*Advance read buffer pointer to the start of the next DND */
			rbuf += 1500;

			/*clear the done bit */
			rbuf2_BD[i].comand = 0x0;

		}

		DPRINTK("IPC: ByteCount for iteration:%d, is: %x\n\n", j,
			(unsigned int)Bytes_Count);
		rbuf2_BD[9].comand = 0x4000;
		memset(rbuf, 0, bytes);

		j++;
	}

	if (hw_ctrl_ipc_close(vchannel) != HW_CTRL_IPC_STATUS_OK) {
		DPRINTK("IPC: Error on hw_ctrl_ipc_close function\n");
		status = -1;
	}
	dma_free_coherent(NULL, bytes, rbuf, rpaddr);

	if (status == 0) {
		DPRINTK
		    ("TEST for Contiguous read_ex2 with Random Packet Data chnl OK\n");
	} else {
		DPRINTK
		    ("TEST for Contiguous read_ex2 with Random Packet Data chnl FAILED\n");

	}
	return status;
}

int exec_logging_loopback(unsigned long arg)
{
	return 0;
}

static int ipctest_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ipctest_close(struct inode *inode, struct file *filp)
{
	unsigned int minor;
	minor = MINOR(inode->i_rdev);
	return 0;
}

static int ipctest_ioctl(struct inode *inode,
			 struct file *file,
			 unsigned int action, unsigned long arg)
{
	int result = 0;

	DPRINTK("IPC: IOCTL to execute: %d\n", action);

	switch (action) {
	case IOCTL_IPCTEST_EXEC_OPEN_CLOSE_TEST:
		result = exec_open_close_test(arg);
		break;

	case IOCTL_IPCTEST_EXEC_PACKET_DATA_LOOPBACK:
		result = exec_packet_data_loopback(arg);
		break;

	case IOCTL_IPCTEST_EXEC_SHORT_MSG_LOOPBACK:
		result = exec_short_message_loopback(arg);
		break;

	case IOCTL_IPCTEST_EXEC_PACKET_DATA_WRITE_EX_CONT_LOOPBACK:
		result = exec_packet_data_write_ex_cont_loopback(arg);
		break;
	case IOCTL_IPCTEST_EXEC_PACKET_DATA_WRITE_EX2:
		result = exec_packet_data_write_ex2(arg);
		//DPRINTK("IPC: TEST FAILED exec_packet_data_write_ex2 result: %d \n", result);
		break;

	case IOCTL_IPCTEST_EXEC_PACKET_DATA_WRITE_EX_LINK_LOOPBACK:
		result = exec_packet_data_write_ex_link_loopback(arg);
		break;

	case IOCTL_IPCTEST_EXEC_LOGGING_LOOPBACK:
		result = exec_logging_loopback(arg);
		break;
	case IOCTL_IPCTEST_EXEC_PACKET_DATA_READ_EX2:
		result = exec_packet_data_read_ex2(arg);
		break;
	case IOCTL_IPCTEST_EXEC_PACKET_DATA_READ_EX2_RAND:
		result = exec_packet_data_read_ex2_rand(arg);
		break;

	default:
		DPRINTK("IPC: Unknown IOCTL: %d\n", action);
		return -1;
	}

	return result;
}

static struct file_operations ipctest_fops = {
	.owner = THIS_MODULE,
	//.read      = mxc_ipc_read,
	//.write     = mxc_ipc_write,
	.ioctl = ipctest_ioctl,
	.open = ipctest_open,
	.release = ipctest_close,
};

int __init ipctest_init_module(void)
{
	struct class_device *temp_class;
	int res = 0;

	res = register_chrdev(IPCTEST_MAJOR, "ipctest", &ipctest_fops);
	if (res >= 0) {
		DPRINTK("IPCTEST Driver Module Loaded\n");
	} else {
		DPRINTK("IPCTEST Driver Module is not Loaded successfully\n");
		return -1;
	}

	major_num = res;

	ipc_tm_class = class_create(THIS_MODULE, "ipctest");
	if (IS_ERR(ipc_tm_class)) {
		printk(KERN_ERR "Error creating ipctest class.\n");
		unregister_chrdev(major_num, "ipctest");
		class_device_destroy(ipc_tm_class, MKDEV(major_num, 0));
		return PTR_ERR(ipc_tm_class);
	}

	temp_class = class_device_create(ipc_tm_class, NULL,
					 MKDEV(major_num, 0), NULL, "ipctest");
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating ipctest class device.\n");
		class_device_destroy(ipc_tm_class, MKDEV(major_num, 0));
		class_destroy(ipc_tm_class);
		unregister_chrdev(major_num, "ipctest");
		return -1;
	}

	init_waitqueue_head(&write_queue);
	init_waitqueue_head(&read_queue);
	init_waitqueue_head(&notify_queue);
	return 0;
}

static void ipctest_cleanup_module(void)
{
	unregister_chrdev(major_num, "ipctest");
	class_device_destroy(ipc_tm_class, MKDEV(major_num, 0));
	class_destroy(ipc_tm_class);

	DPRINTK("IPCTEST Driver Module Unloaded\n");
}

module_init(ipctest_init_module);
module_exit(ipctest_cleanup_module);

MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("IPC driver");
MODULE_LICENSE("GPL");
