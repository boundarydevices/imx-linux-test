
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mman.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>	// For interrupt handling

#include <asm/dma.h>
//#include <asm/mach/dma.h>
#include <asm/delay.h>
#include <asm/scatterlist.h>

#include <linux/device.h>
#include <asm/arch/dma.h>

#include <asm/arch/hardware.h>
#include "mxc_udma_testdriver.h"


#define _DMA_DEBUG_
#ifdef _DMA_DEBUG_
#define printkk	printk
#endif

static int gMajor; //major number of device
static struct class *dma_tm_class;
wait_queue_head_t q;

#define DMA_IRQ_BASE 32

int dma_open(struct inode * inode, struct file * filp);
ssize_t dma_read(struct file * filp, char __user  * buf, size_t count, loff_t * offset);
int dma_release(struct inode * inode, struct file * filp);
ssize_t dma_write(struct file * filp, const char __user  * buf, size_t count, loff_t * offset);
int dma_ioctl(struct inode * inode,	struct file *filp, unsigned int cmd, unsigned long arg);

extern void mxc_dump_dma_register(int channel);

char *g_pDmaSource_Ori,*g_pDmaDest_Ori;
char *g_pDmaSource,*g_pDmaDest;

static int g_sand=100;
struct scatterlist *sg;

//extern mxc_dma_channel_t g_mxc_dma_chan[];
static int g_opened = 0;
static volatile int g_dma_done = 0;
static int g_status = 0;

static void sand(int rand)
{
	g_sand=rand;
	return;
}


static char  rand(void)
{
	g_sand=g_sand*12357/1103;
	return g_sand;
}


void FillMem(char * p, int size, int rd)
{
	int i=0;
	printk("Fill 1d mem %8x\n",(unsigned int)p);

	sand(rd);
	for(i=0;i<size;i++)
		*p++=rand();
	//invalidate_dcache_range(p, p+size);
       return;
}



int  Verify(char *p, int size,int rd)
{
	int i=0,j=0;
	char x;
	//invalidate_dcache_range(p, p+size);
	sand(rd);
	for(i=0;i<size;i++)
	{
		x=rand();
		if(i==10)
			printk("Verify 1d mem %8x,x=%d,p[10]=%d\n",(unsigned int)p,(unsigned int)x,p[10]);
		if(x!=p[i])
		{
			j++;
			if(j<30)
			{
				printk("i = %d, Exp %d:%d\n",i,x,p[i]);

			}else
				return 1;
		}
	}

	if(j>0)
		return 1;
	else
		return 0;
}


void FillMem_2D(char * p, int x,int y,int w, int rd,int dir)
{
	int i,j;
	int DecOffset=0;
	if(dir)
		DecOffset=w-x;
	printk("Fill 2d mem %8x\n",(unsigned int)p);
	sand(rd);
       for(j=0;j<y;j++)
       	for(i=0;i<x;i++)
		{*(p+j*w+i+DecOffset)=rand();}
	return;
}


int  Verify_2D(char * p, int x,int y,int w, int rd,int dir)
{
	int i=0,j=0;
	int iCountError=0;
	char temp;
	int DecOffset=0;
	if(dir)
		DecOffset=w-x;
	sand(rd);
	for(j=0;j<y;j++)
		for(i=0;i<x;i++)
	{
	       /*take care of the loop sequence cause the verify rand*/
		temp=rand();
              if(i==1&&j==2)
			printk("Verify 2d mem %8x,x=%d,p[%d*%d+%d+%d]=%d\n",(unsigned int)p,temp,j,w,i,DecOffset,p[j*w+i+DecOffset]);

		if(temp!=p[j*w+i+DecOffset])
		{
			iCountError++;
			if(iCountError<30)
			{
				printk("i = %d; j =%d ; Exp %d:%d\n",i,j,temp,p[j*w+i+DecOffset]);
			}
			else
				return 1;
		}
	}
	if(iCountError>0)
		return 1;
	else
		return 0;
}


#define DMA_TRANS_SIZE  0x1000
#define DMA_INTERVAL_MEM 0x2000
#define DMA_SAND 13

static void dma_complete_fn(void * args, int error, unsigned int count)
{
	if(error != MXC_DMA_DONE) {
		printk("==========dma transfer is failure[%x]==========\n", error);
	} else {
		printk("==========dma transfer is success==========\n");
	}
	g_status = error;
	g_dma_done = 1;
}

static int __do_dma_test_ram2d2ram(void)
{
	dmach_t channel;
	mxc_dma_requestbuf_t buf;
	int count = 0;
	char * memory_base, * src, * dest;
	channel = mxc_dma_request(MXC_DMA_TEST_RAM2D2RAM, "dma test driver");
	if ( channel < 0 ) {
		printk("request dma fail n");
		return -ENODEV;
	}

	memory_base = __get_free_pages(GFP_KERNEL|GFP_DMA, 4);
	if ( memory_base == NULL ) {
		mxc_dma_free(channel);
		return -ENOMEM;
	}

	mxc_dma_callback_set(channel, dma_complete_fn, NULL);
	src = memory_base;
	dest = src+DMA_INTERVAL_MEM;
	FillMem_2D(src, 0x100, 0x10, 0x100, DMA_SAND, 0);
	memset(dest, 0, DMA_INTERVAL_MEM);
	buf.src_addr = virt_to_phys(src);
	buf.dst_addr = virt_to_phys(dest);
	buf.num_of_bytes = DMA_INTERVAL_MEM;
	mxc_dma_config(channel, &buf, 1, DMA_MODE_READ);
	mxc_dma_enable(channel);
	for(count =0; count<100; count++) {
		if(g_dma_done) {
			printk("dma transfer complete: error =%x\n", g_status);
			break;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(100*(HZ/1000));
		set_current_state(TASK_RUNNING);
	}
	if (count >= 100) {
		mxc_dump_dma_register(channel);
		printk("dma transfer timeout\n");
		mxc_dma_disable(channel);
	} else {
		if(!Verify(dest, 0x100*0x10, DMA_SAND)){
			printk("PASSED\n");
		} else {
			printk("VERIFY FAILURED\n");
		}
	}
	free_pages(memory_base, 4);
	printk("\n\n ===========check the status of DMA module \n");
//	mxc_dump_dma_register(channel);
	mxc_dma_free(channel);
}

static int __do_dma_test_ram2ram2d(void)
{
	dmach_t channel;
	mxc_dma_requestbuf_t buf;
	int count = 0;
	char * memory_base, * src, * dest;
	channel = mxc_dma_request(MXC_DMA_TEST_RAM2RAM2D, "dma test driver");
	if ( channel < 0 ) {
		printk("request dma fail n");
		return -ENODEV;
	}

	memory_base = __get_free_pages(GFP_KERNEL|GFP_DMA, 4);
	if ( memory_base == NULL ) {
		mxc_dma_free(channel);
		return -ENOMEM;
	}

	mxc_dma_callback_set(channel, dma_complete_fn, NULL);
	src = memory_base;
	dest = src+DMA_INTERVAL_MEM;
	FillMem(src, 0x80*0x10, DMA_SAND);
	memset(dest, 0, DMA_INTERVAL_MEM);
	buf.src_addr = virt_to_phys(src);
	buf.dst_addr = virt_to_phys(dest);
	buf.num_of_bytes = DMA_INTERVAL_MEM;
	mxc_dma_config(channel, &buf, 1, DMA_MODE_READ);
	mxc_dma_enable(channel);
	for(count =0; count<100; count++) {
		if(g_dma_done) {
			printk("dma transfer complete: error =%x\n", g_status);
			break;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(100*(HZ/1000));
		set_current_state(TASK_RUNNING);
	}
	if (count >= 100) {
		mxc_dump_dma_register(channel);
		printk("dma transfer timeout\n");
		mxc_dma_disable(channel);
	} else {
		if(!Verify_2D(dest, 0x80, 0x10, 0x100, DMA_SAND, 0)){
			printk("PASSED\n");
		} else {
			printk("VERIFY FAILURED\n");
		}
	}
	free_pages(memory_base, 4);
	printk("\n\n ===========check the status of DMA module \n");
//	mxc_dump_dma_register(channel);
	mxc_dma_free(channel);
}

static int __do_dma_test_ram2d2ram2d(void)
{
	dmach_t channel;
	mxc_dma_requestbuf_t buf;
	int count = 0;
	char * memory_base, * src, * dest;
	channel = mxc_dma_request(MXC_DMA_TEST_RAM2D2RAM2D, "dma test driver");
	if ( channel < 0 ) {
		printk("request dma fail n");
		return -ENODEV;
	}

	memory_base = __get_free_pages(GFP_KERNEL|GFP_DMA, 4);
	if ( memory_base == NULL ) {
		mxc_dma_free(channel);
		return -ENOMEM;
	}

	mxc_dma_callback_set(channel, dma_complete_fn, NULL);
	src = memory_base;
	dest = src+DMA_INTERVAL_MEM;
	FillMem_2D(src, 0x40, 0x10,0x80, DMA_SAND, 0);
	memset(dest, 0, DMA_INTERVAL_MEM);
	buf.src_addr = virt_to_phys(src);
	buf.dst_addr = virt_to_phys(dest);
	buf.num_of_bytes = DMA_INTERVAL_MEM;
	mxc_dma_config(channel, &buf, 1, DMA_MODE_READ);
	mxc_dma_enable(channel);
	for(count =0; count<100; count++) {
		if(g_dma_done) {
			printk("dma transfer complete: error =%x\n", g_status);
			break;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(100*(HZ/1000));
		set_current_state(TASK_RUNNING);
	}
	if (count >= 100) {
		mxc_dump_dma_register(channel);
		printk("dma transfer timeout\n");
		mxc_dma_disable(channel);
	} else {
		if(!Verify_2D(dest, 0x40, 0x10, 0x80, DMA_SAND, 0)){
			printk("PASSED\n");
		} else {
			printk("VERIFY FAILURED\n");
		}
	}
	free_pages(memory_base, 4);
	printk("\n\n ===========check the status of DMA module \n");
//	mxc_dump_dma_register(channel);
	mxc_dma_free(channel);
}

static int __do_dma_test_ram2ram(void)
{
	dmach_t channel;
	mxc_dma_requestbuf_t buf;
	int count = 0;
	char * memory_base, * src, * dest;
	channel = mxc_dma_request(MXC_DMA_TEST_RAM2RAM, "dma test driver");
	if ( channel < 0 ) {
		printk("request dma fail n");
		return -ENODEV;
	}

	memory_base = __get_free_pages(GFP_KERNEL|GFP_DMA, 4);
	if ( memory_base == NULL ) {
		mxc_dma_free(channel);
		return -ENOMEM;
	}

	mxc_dma_callback_set(channel, dma_complete_fn, NULL);
	src = memory_base;
	dest = src+DMA_INTERVAL_MEM;
	FillMem(src, DMA_INTERVAL_MEM, DMA_SAND);
	memset(dest, 0, DMA_INTERVAL_MEM);
	buf.src_addr = virt_to_phys(src);
	buf.dst_addr = virt_to_phys(dest);
	buf.num_of_bytes = DMA_INTERVAL_MEM;
	mxc_dma_config(channel, &buf, 1, DMA_MODE_READ);
	mxc_dma_enable(channel);
	for(count =0; count<100; count++) {
		if(g_dma_done) {
			printk("dma transfer complete: error =%x\n", g_status);
			break;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(100*(HZ/1000));
		set_current_state(TASK_RUNNING);
	}
	if (count >= 100) {
		mxc_dump_dma_register(channel);
		printk("dma transfer timeout\n");
		mxc_dma_disable(channel);
	} else {
		if(!Verify(dest, DMA_INTERVAL_MEM,DMA_SAND)){
			printk("PASSED\n");
		} else {
			printk("VERIFY FAILURED\n");
		}
	}
	free_pages(memory_base, 4);
	printk("\n\n ===========check the status of DMA module \n");
//	mxc_dump_dma_register(channel);
	mxc_dma_free(channel);
}


struct file_operations dma_fops = {
	open:		dma_open,
	release:	       dma_release,
	read:		dma_read,
	write:		dma_write,
	ioctl:		dma_ioctl,
};

int __init dma_init_module(void)
{
	struct class_device *temp_class;
	int error;
	int i;

	printk("register virtual dma driver.\n");

	/* register a character device */
	error = register_chrdev(0, "dma", &dma_fops);
	if (error < 0) {
		printk("dma driver can't get major number\n");
		return error;
	}
	gMajor = error;
	printk("dma major number = %d\n",gMajor);

	dma_tm_class = class_create(THIS_MODULE, "dma");
	if (IS_ERR(dma_tm_class)) {
		printk(KERN_ERR "Error creating dma test module class.\n");
		unregister_chrdev(gMajor, "dma");
		class_device_destroy(dma_tm_class, MKDEV(gMajor, 0));
		return PTR_ERR(dma_tm_class);
	}

	temp_class = class_device_create(dma_tm_class, NULL,
					     MKDEV(gMajor, 0), NULL,
					     "dma");
	if (IS_ERR(temp_class)) {
		printk(KERN_ERR "Error creating dma test module class device.\n");
		class_device_destroy(dma_tm_class, MKDEV(gMajor, 0));
		class_destroy(dma_tm_class);
		unregister_chrdev(gMajor, "dma");
		return -1;
	}

	init_waitqueue_head(&q);
	return 0;
}

mxc_dma_requestbuf_t *  dma_build_bufferlist(unsigned int iTransTime, char * base)
{
      	int i;
      	mxc_dma_requestbuf_t * buf;

      	buf =(mxc_dma_requestbuf_t *)kmalloc(iTransTime*sizeof(mxc_dma_requestbuf_t ), GFP_KERNEL);

	if ( buf == NULL ) return NULL;

       for(i=0;i<iTransTime;i++)
       {
        	buf[i].src_addr = virt_to_phys(base+(DMA_INTERVAL_MEM*i));
        	buf[i].dst_addr =virt_to_phys(base+(DMA_INTERVAL_MEM*i+DMA_TRANS_SIZE));
        	buf[i].num_of_bytes = DMA_TRANS_SIZE;
		printk("%d:: src=%x, dest=%x, length = %x\n",i,  buf[i].src_addr , buf[i].dst_addr , buf[i].num_of_bytes );
   	}
	return buf;
}


mxc_dma_requestbuf_t *  dma_build_speclist(unsigned int iTransTime, char * base)
{
      	int i;
      	mxc_dma_requestbuf_t * buf;
	int width, gap;

      	buf =(mxc_dma_requestbuf_t *)kmalloc(iTransTime*sizeof(mxc_dma_requestbuf_t ), GFP_KERNEL);

	if ( buf == NULL ) return NULL;

	width = (1024/iTransTime)*PAGE_SIZE;
	gap = width/2;

       for(i=0;i<iTransTime;i++)
       {
        	buf[i].src_addr = virt_to_phys(base+(width*i));
        	buf[i].dst_addr =virt_to_phys(base+(width*i+gap));
        	buf[i].num_of_bytes = gap;
		printk("%d:: src=%x, dest=%x, length = %x\n",i,  buf[i].src_addr , buf[i].dst_addr , buf[i].num_of_bytes );
   	}
	return buf;
}
/*This function test linear to linear increase Chainbuffer dma transfer*/
int Dma_Test_hw_ChainBuffer(unsigned int iTransTime)
{
	int i, count;
       	dmach_t channel;
	mxc_dma_requestbuf_t * buf;
	char * memory_base, * src, * dest;
	int width, gap;

	if(iTransTime>4)
	{
               printk("Too large transtime.\n");
		 return -EINVAL;
	}
	channel = mxc_dma_request(MXC_DMA_TEST_HW_CHAINING, "dma test driver");
	if ( channel < 0 ) {
		printk("request dma fail n");
		return -ENODEV;
	}

	memory_base = __get_free_pages(GFP_KERNEL|GFP_DMA, 10);
	if ( memory_base == NULL ) {
		mxc_dma_free(channel);
		return -ENOMEM;
	}

        buf = dma_build_speclist(iTransTime, memory_base);
	if (buf == NULL ) {
		printk("No enought memory \n");
		return -ENOMEM;
	}
	printk("The buffer list is %x\n", buf);

	mxc_dma_callback_set(channel, dma_complete_fn, NULL);

	width = (1024/iTransTime)*PAGE_SIZE;
	gap = width/2;
       /*initialize the source and dest memory*/
	for(i=0; i < iTransTime; i++)
        {
         src  = (char*)(memory_base+width*i);
	 dest  = (char*)(memory_base+(width*i + gap));

	  /*Only test linear memthod, so fill source with this function*/
         FillMem(src , gap, DMA_SAND+i);
	 memset(dest,0,gap);
       }

	mxc_dma_config(channel, buf, iTransTime, DMA_MODE_READ);
	mxc_dma_enable(channel);

	for(count =0; count<1000; count) {
		if(g_dma_done) {
			printk("dma transfer complete: error =%x\n", g_status);
			break;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(500*(HZ/1000));
		set_current_state(TASK_RUNNING);
	}
	if (count >= 1000) {
		mxc_dump_dma_register(channel);
		printk("dma transfer timeout\n");
		mxc_dma_disable(channel);
	}
	{
	/**************Begin of Set dma para******************************/
      		for(i=0;i<iTransTime;i++)
      		{
         		printk("Get chainbuffer %d verify\n",i);
			 dest  = (char*)(memory_base+(width*i + gap));
         		if(Verify(dest,gap,DMA_SAND+i)) {
				break;
			}
      		}
		if( i >= iTransTime ) printk("Passed!\n");
		else printk("Failure!\n");
	}

	kfree(buf);
	free_pages(memory_base, 10);
	printk("\n\n ===========check the status of DMA module \n");
//	mxc_dump_dma_register(channel);
	mxc_dma_free(channel);
	return 0;
}

/*This function test linear to linear increase Chainbuffer dma transfer*/
int Dma_Test_sw_ChainBuffer(unsigned int iTransTime)
{
	int i, count;
       	dmach_t channel;
	mxc_dma_requestbuf_t * buf;
	char * memory_base, * src, * dest;

	if(iTransTime>8)
	{
               printk("Too large transtime.\n");
		 return -EINVAL;
	}
	channel = mxc_dma_request(MXC_DMA_TEST_SW_CHAINING, "dma test driver");
	if ( channel < 0 ) {
		printk("request dma fail n");
		return -ENODEV;
	}

	memory_base = __get_free_pages(GFP_KERNEL|GFP_DMA, 4);
	if ( memory_base == NULL ) {
		mxc_dma_free(channel);
		return -ENOMEM;
	}

        buf = dma_build_bufferlist(iTransTime, memory_base);
	if (buf == NULL ) {
		printk("No enought memory \n");
		return -ENOMEM;
	}

	mxc_dma_callback_set(channel, dma_complete_fn, NULL);

       /*initialize the source and dest memory*/
	for(i=0; i < iTransTime; i++)
      {
         src  = (char*)(memory_base+DMA_INTERVAL_MEM*i);
	 dest  = (char*)(memory_base+(DMA_INTERVAL_MEM*i + DMA_TRANS_SIZE));

	  /*Only test linear memthod, so fill source with this function*/
         FillMem(src , DMA_TRANS_SIZE, DMA_SAND+i);
	 memset(dest,0,DMA_TRANS_SIZE);
      }

	mxc_dma_config(channel, buf, iTransTime, DMA_MODE_READ);
	mxc_dma_enable(channel);

	for(count =0; count<400; count++) {
		if(g_dma_done) {
			printk("dma transfer complete: error =%x\n", g_status);
			break;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(100*(HZ/1000));
		set_current_state(TASK_RUNNING);
	}
	if (count >= 400) {
		mxc_dump_dma_register(channel);
		printk("dma transfer timeout\n");
		mxc_dma_disable(channel);
	} else {
	/**************Begin of Set dma para******************************/
      		for(i=0;i<iTransTime;i++)
      		{
         		printk("Get chainbuffer %d verify\n",i);
         		dest = (char*)(memory_base+DMA_INTERVAL_MEM*i+DMA_TRANS_SIZE);
         		if(Verify(dest,DMA_TRANS_SIZE,DMA_SAND+i)) {
				break;
			}
      		}
		if( i >= iTransTime ) printk("Passed!\n");
		else printk("Failure!\n");
	}

	kfree(buf);
	free_pages(memory_base, 4);
	printk("\n\n ===========check the status of DMA module \n");
	//mxc_dump_dma_register(channel);
	mxc_dma_free(channel);
	return 0;
}
int dma_open(struct inode * inode, struct file * filp)
{
	if ( xchg(&g_opened, 1) ) return -EBUSY;
       return 0;
}

int dma_release(struct inode * inode, struct file * filp)
{
	xchg(&g_opened, 0);
	return 0;
}


ssize_t dma_read (struct file *filp, char __user * buf, size_t count, loff_t * offset)
{
	return 0;
}


ssize_t dma_write(struct file * filp, const char __user * buf, size_t count, loff_t * offset)
{
	return 0;
}


int dma_ioctl(struct inode * inode,	struct file *filp, unsigned int cmd, unsigned long arg)
{
       	switch(cmd)
	{
	case DMA_IOC_RAM2RAM:
		g_dma_done = 0;
		return  __do_dma_test_ram2ram();
	case DMA_IOC_RAM2D2RAM2D:
		g_dma_done = 0;
		return  __do_dma_test_ram2d2ram2d();
	case DMA_IOC_RAM2RAM2D:
		g_dma_done = 0;
		return  __do_dma_test_ram2ram2d();
	case DMA_IOC_RAM2D2RAM:
		g_dma_done = 0;
		return  __do_dma_test_ram2d2ram();
	case DMA_IOC_HW_CHAINBUFFER:
		g_dma_done = 0;
		return  Dma_Test_hw_ChainBuffer(arg);
	case DMA_IOC_SW_CHAINBUFFER:
		g_dma_done = 0;
		return  Dma_Test_sw_ChainBuffer(arg);
       	default:
        	printk("No such IO ctl\n");
		break;
	}
	return 0;
}

void __init dma_cleanup_module(void)
{
	//before exit, free all of the dma resources including irpt, memory and so on.
	class_device_destroy(dma_tm_class, MKDEV(gMajor, 0));
	class_destroy(dma_tm_class);
	unregister_chrdev(gMajor, "dma");

}

module_init(dma_init_module);
module_exit(dma_cleanup_module);

MODULE_LICENSE("GPL");

