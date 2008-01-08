/************************************/
//DMA Test Driver
/************************************/
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<malloc.h>
#include <unistd.h>
#include<asm/page.h>
#include<sys/ioctl.h>


#include "mxc_udma_test.h"

#define DMA_BURST_LENGTH_4 4
#define DMA_BURST_LENGTH_8 8
#define DMA_BURST_LENGTH_16 16
#define DMA_BURST_LENGTH_32 32
#define DMA_BURST_LENGTH_64 0


int Test_1D_to_1D(void)
{
	unsigned long arg;
	int fd;
       fd = open("/tmp/dma", O_RDWR);
       if(fd < 0)
	{
		printf("can not open file dma.\n");
		return -1;
	}
	/*make sure that count set before the source mode*/
	arg = 0;
	ioctl(fd,DMA_IOC_RAM2RAM,arg);
	close(fd);
	return 0;
}


int Test_2D_to_2D(void)
{
	unsigned long arg;
       int fd;
       fd = open("/tmp/dma", O_RDWR);
       if(fd < 0)
	{
		printf("can not open file dma.\n");
		return -1;
	}
	/*make sure that count set before the source mode and after x/y/w*/
	arg = 0;
	ioctl(fd,DMA_IOC_RAM2D2RAM2D,arg);
	close(fd);
	return 0;
}




int Test_1D_to_2D(void)
{
	unsigned long arg;
       int fd;
       fd = open("/tmp/dma", O_RDWR);
       if(fd < 0)
	{
		printf("can not open file dma.\n");
		return -1;
	}
	/*make sure that count set before the source mode and after x/y/w*/
	arg = 0;
	ioctl(fd,DMA_IOC_RAM2RAM2D,arg);
	close(fd);
	return 0;
}




int Test_2D_to_1D(void)
{
	unsigned long arg;
       int fd;
       fd = open("/tmp/dma", O_RDWR);
       if(fd < 0)
	{
		printf("can not open file dma.\n");
		return -1;
	}
	/*make sure that count set before the source mode and after x/y/w*/
	arg = 0;
	ioctl(fd,DMA_IOC_RAM2D2RAM,arg);
	close(fd);
	return 0;
}

int Test_ChainBuffer(unsigned int iTransTime, int mode)
{
       unsigned long arg;
       int fd;
	if(iTransTime > 8 || iTransTime <= 0)
	{
               printf("Error transtime, it may between 1 to 8.\n");
		 return -1;
	}
       fd = open("/tmp/dma", O_RDWR);
       if(fd < 0)
	{
		printf("can not open file dma.\n");
		return -1;
	}
       /*this ioctl start testing the chainbuffer*/
	arg = iTransTime;
	if ( mode ) {
		ioctl(fd, DMA_IOC_HW_CHAINBUFFER,arg);	
	} else {
		ioctl(fd, DMA_IOC_SW_CHAINBUFFER, arg);
	}
	close(fd);
	return 0;
}

int main(int argc, char* argv[])
{
	int iInputChoice;
	if (argc<2)
	{
            printf("Function para needed. \n");        
            printf("0 test 1d-1d linear DMA. \n");        
            printf("1 test 2d-2d linear DMA. \n");       
	     printf("2 test 1d-2d linear DMA. \n");       
	     printf("3 test 2d-1d linear DMA. \n");       
	     printf("4 test DMA by your input. \n");      
	     printf("5 test Chain Buffer DMA. \n");       
		 
	     return -1;	
       }
	else   
	{
	       iInputChoice = atoi(argv[1]); 
              switch (iInputChoice)
	       {
	              case 0:
		  	  Test_1D_to_1D();
		  	  break;
		       case 1:
                          Test_2D_to_2D();
			     break;
                     case 2:
                          Test_1D_to_2D();
			     break;
			case 3:
                          Test_2D_to_1D();
			     break;
			case 4:
                           if(argc==3)
			          Test_ChainBuffer(atoi(argv[2]), 0);
				else		   
                             {
                                printf("Please Specify No. of buffers in para 2. \n");        
              	           return -1;	
                              }
				break;
			case 5:
                           if(argc==3)
			          Test_ChainBuffer(atoi(argv[2]), 1);
				else		   
                             {
                                printf("Please Specify No. of buffers in para 2. \n");        
              	           return -1;	
                              }
				break;
			default:
                          printf("Please enter choice between 0 to 6.\n");
			     break;
              }
	}
	return 0;		
}

