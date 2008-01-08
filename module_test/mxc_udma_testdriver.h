#ifndef DMA_CHR_DRIVER
#define DMA_CHR_DRIVER

#include <linux/ioctl.h>
#include <asm/arch/hardware.h>

// ioctl number
#define	DMA_IOC_MAGIC		'v' // stand for: Virtual dma driver

#define DMA_IOC_RAM2RAM 	_IO(DMA_IOC_MAGIC, 0)
#define DMA_IOC_RAM2D2RAM2D 	_IO(DMA_IOC_MAGIC, 1)
#define DMA_IOC_RAM2RAM2D 	_IO(DMA_IOC_MAGIC, 2)
#define DMA_IOC_RAM2D2RAM 	_IO(DMA_IOC_MAGIC, 3)
#define DMA_IOC_HW_CHAINBUFFER 	_IO(DMA_IOC_MAGIC, 4)
#define DMA_IOC_SW_CHAINBUFFER 	_IO(DMA_IOC_MAGIC, 5)

#endif

