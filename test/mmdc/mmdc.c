/*
 * Copyright (C) 2013-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


#include "mmdc.h"
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

const int AXI_BUS_WIDTH_IN_BYTE = 8;

#define MMDC_P0_IPS_BASE_ADDR 0x021B0000
#define MMDC_P1_IPS_BASE_ADDR 0x021B4000

/************************* Global Variables ***********************************/
pMMDC_t mmdc_p0 = (pMMDC_t)(MMDC_P0_IPS_BASE_ADDR);
pMMDC_t mmdc_p1 = (pMMDC_t)(MMDC_P1_IPS_BASE_ADDR);
int g_size = 4;
unsigned int system_rev = 0;
int g_quit = 0;
/**************************** Functions ***************************************/
static unsigned long getTickCount(void)
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL) != 0)
	return 0;
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/************************ Profiler Functions **********************************/

void start_mmdc_profiling(pMMDC_t mmdc)
{

	mmdc->madpcr0 = 0xA;		// Reset counters and clear Overflow bit
	msync(&(mmdc->madpcr0),4,MS_SYNC);

	mmdc->madpcr0 = 0x1;		// Enable counters
	msync(&(mmdc->madpcr0),4,MS_SYNC);

}

void stop_mmdc_profiling(pMMDC_t mmdc)
{
	mmdc->madpcr0 = 0x0;		// Disable counters
	msync(&(mmdc->madpcr0),4,MS_SYNC);

}

void pause_mmdc_profiling(pMMDC_t mmdc)
{
	mmdc->madpcr0 = 0x3;		// PRF_FRZ = 1
	msync(&(mmdc->madpcr0),4,MS_SYNC);
}

void resume_mmdc_profiling(pMMDC_t mmdc)
{
	mmdc->madpcr0 = 0x1;		// PRF_FRZ = 0
	msync(&(mmdc->madpcr0),4,MS_SYNC);
}
void load_mmdc_results(pMMDC_t mmdc)
{
	//printf("before : mmdc->madpcr0 0x%x\n",mmdc->madpcr0);
	mmdc->madpcr0 |= 0x4; //sets the PRF_FRZ bit to 1 in order to load the results into the registers
	//printf("after : mmdc->madpcr0 0x%x\n",mmdc->madpcr0);
	msync(&(mmdc->madpcr0),4,MS_SYNC);
}

void clear_mmdc_results(pMMDC_t mmdc)
{
	mmdc->madpcr0 = 0xA;            // Reset counters and clear Overflow bit
		msync(&(mmdc->madpcr0),4,MS_SYNC);
}

void get_mmdc_profiling_results(pMMDC_t mmdc, MMDC_PROFILE_RES_t *results)
{
	results->total_cycles 	= mmdc->madpsr0;
	results->busy_cycles 	= mmdc->madpsr1;
	results->read_accesses	= mmdc->madpsr2;
	results->write_accesses	= mmdc->madpsr3;
	results->read_bytes		= mmdc->madpsr4;
	results->write_bytes	= mmdc->madpsr5;
	if(results->read_bytes!=0 || results->write_bytes!=0)
	{
		results->utilization	= (int)(((float)results->read_bytes+(float)results->write_bytes)/((float)results->busy_cycles * 16) * 100);

		results->data_load  	= (int)((float)results->busy_cycles/(float)results->total_cycles * 100);
		results->access_utilization	= (int)(((float)results->read_bytes+(float)results->write_bytes)/((float)results->read_accesses + (float)results->write_accesses));
		if(mmdc->madpsr3)
			results->avg_write_burstsize = (int)mmdc->madpsr5 / mmdc->madpsr3;
		else
			results->avg_write_burstsize = 0;
		if(mmdc->madpsr2)
			results->avg_read_burstsize = (int)mmdc->madpsr4 / mmdc->madpsr2;
		else
			results->avg_read_burstsize = 0;
	}
	else
	{
		results->utilization	= 0;
		results->data_load  	= 0;
		results->access_utilization	= 0;
		results->avg_write_burstsize = 0;
		results->avg_read_burstsize = 0;
	}
}
void print_mmdc_profiling_results(MMDC_PROFILE_RES_t results, MMDC_RES_TYPE_t print_type, int time)
{
	if (print_type == RES_FULL)
	{
		printf("\nMMDC new Profiling results:\n");
		printf("***********************\n");
		printf("Measure time: %dms \n", time);
		printf("Total cycles count: %u\n",results.total_cycles);
		printf("Busy cycles count: %u\n",results.busy_cycles);
		printf("Read accesses count: %u\n",results.read_accesses);
		printf("Write accesses count: %u\n",results.write_accesses);
		printf("Read bytes count: %u\n",results.read_bytes);
		printf("Write bytes count: %u\n",results.write_bytes);
		printf("Avg. Read burst size: %u\n",results.avg_read_burstsize);
		printf("Avg. Write burst size: %u\n",results.avg_write_burstsize);

		printf("Read: ");

		printf("%0.2f MB/s / ", (float)results.read_bytes*1000/(1024*1024*time));

		printf(" Write: ");
		printf("%0.2f MB/s ", (float)results.write_bytes*1000/(1024*1024*(float)time));

		printf(" Total: ");
		printf("%0.2f MB/s ", (float)(results.write_bytes+results.read_bytes)*1000/(1024*1024*(float)time));

		printf("\r\n");
	}
	if((results.utilization > 100) || (results.data_load > 100))
	{
		printf("Warning:Counter overflow!!!Record time too long!!!\n");
	} else	{
		printf("Utilization: %u%%\n",results.utilization);
		printf("Overall Bus Load: %u%%\n",results.data_load);
		printf("Bytes Access: %u\n\n",results.access_utilization);
	}
}

static int get_system_rev(void)
{
	FILE *fp;
	char buf[2048];
	int nread;
	char *tmp, *rev;
	int ret = -1;

	fp = fopen("/proc/cpuinfo", "r");
	if (fp == NULL) {
		perror("/proc/cpuinfo\n");
		return ret;
	}

	nread = fread(buf, 1, sizeof(buf), fp);
	fclose(fp);
	if ((nread == 0) || (nread == sizeof(buf))) {
		printf("/proc/cpuinfo: %d\n",nread);
		nread = 0;
		ret = 0;
	}

	buf[nread] = '\0';

	tmp = strstr(buf, "Revision");
	if (tmp != NULL) {
		rev = index(tmp, ':');
		if (rev != NULL) {
			rev++;
			system_rev = strtoul(rev, NULL, 16);
			ret = 0;
		}
	}

	//cpuinfo is changed in 3.10.9 kernel, new way is used.
	if((ret == 0) && (system_rev ==0))
	{
		ret = -1;
		fp = fopen("/sys/devices/soc0/soc_id", "r");
		if (fp == NULL) {
			perror("/sys/devices/soc0/soc_id\n");
			return ret;
		}

		nread = fread(buf, 1, sizeof(buf), fp);
		fclose(fp);
		if ((nread == 0) || (nread == sizeof(buf))) {
			printf("/sys/devices/soc0/soc_id:%d\n",nread);
			return ret;
		}
		buf[nread] = '\0';
		if(strncmp(buf,"i.MX6Q",6)==0)
		{
			system_rev= 0x63000;
			ret =0;
			printf("i.MX6Q detected.\n");
		}else if(strncmp(buf,"i.MX6DL",7)==0){
			system_rev= 0x61000;
			ret = 0;
			printf("i.MX6DL detected.\n");
		}else if(strncmp(buf,"i.MX6SL",7)==0)
		{
			system_rev= 0x60000;
			ret = 0;
			printf("i.MX6SL detected.\n");
		}else if(strncmp(buf,"i.MX6SX",7)==0)
		{
			system_rev= 0x62000;
			ret = 0;
			printf("i.MX6SX detected.\n");
		}
	}
	return ret;
}

void signalhandler(int signal)
{
	printf("singal %d received, quit the application.\n", signal);
	g_quit = 1;
}
void help(void)
{
	printf("======================MMDC v1.3===========================\n");
	printf("Usage: mmdc [ARM:DSP1:DSP2:GPU2D:GPU2D1:GPU2D2:GPU3D:GPUVG:VPU:M4:PXP:SUM] [...]\n");
	printf("export MMDC_SLEEPTIME can be used to define profiling duration.1 by default means 1s\n");
	printf("export MMDC_LOOPCOUNT can be used to define profiling times. 1 by default. -1 means infinite loop.\n");
	printf("export MMDC_CUST_MADPCR1 can be used to customize madpcr1. Will ignore it if defined master\n");
	printf("Note1: More than 1 master can be inputed. They will be profiled one by one.\n");
	printf("Note2: MX6DL can't profile master GPU2D, GPU2D1 and GPU2D2 are used instead.\n");
}
int main(int argc, char **argv)
{
	int timeForSleep = 1;
	void *A;
	MMDC_PROFILE_RES_t results;
	int ulStartTime = 0;
	int i;
	int loopcount= 1;
	unsigned int customized_madpcr1= 0;
	char *p;

	p = getenv("MMDC_SLEEPTIME");
	if (p != 0)
	{
		timeForSleep = strtol(p, 0, 10);
		if(timeForSleep == 0)
			timeForSleep = 1;
	}
	p = getenv("MMDC_LOOPCOUNT");
	if (p != 0)
	{
		loopcount = strtol(p, 0, 10);
	}
	p = getenv("MMDC_CUST_MADPCR1");
	if (p != 0)
	{
		customized_madpcr1 = strtol(p, 0, 16);
	}

	int fd = open("/dev/mem", O_RDWR, 0);
		if (fd < 0)
	{
		printf("Could not open /dev/mem\n");
		return -1;
	}
	A = (ulong*) mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMDC_P0_IPS_BASE_ADDR);
		if (A == MAP_FAILED)
	{
		printf("Mapping failed mmdc_p0\n");
		close(fd);
		return -1;
	}


	if(get_system_rev()<0){
		printf("Fail to get system revision,parameter will be ignored \n");
		argc = 1;
	}
	g_quit = 0;
	for(i=0; !g_quit && i!=loopcount; i++)
	{
		if(argc >= 2)
		{
			int j;
			for(j=1; j<argc; j++)
			{
				if(strcmp(argv[j],"DSP1")==0){
					if(cpu_is_mx6sx()==1)
						((pMMDC_t)A)->madpcr1 = axi_lcd1_6sx;
					else if(cpu_is_mx6sl()==1)
						((pMMDC_t)A)->madpcr1 = axi_lcd1_6sl;
					else
						((pMMDC_t)A)->madpcr1 = axi_ipu1;
					printf("MMDC DSP1 \n");
				}else if((strcmp(argv[j], "DSP2")==0)&&(cpu_is_mx6q()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_ipu2_6q;
				  printf("MMDC DSP2 \n");
				}else if((strcmp(argv[j], "DSP2")==0)&&(cpu_is_mx6sx()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_lcd2_6sx;
				  printf("MMDC DSP2 \n");
				}else if((strcmp(argv[j], "M4")==0)&&(cpu_is_mx6sx()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_m4_6sx;
				  printf("MMDC M4 \n");
				}else if((strcmp(argv[j], "PXP")==0)&&(cpu_is_mx6sx()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_pxp_6sx;
				  printf("MMDC M4 \n");
				}else if((strcmp(argv[j], "GPU3D")==0)&&(cpu_is_mx6sx()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_gpu3d_6sx;
				  printf("MMDC GPU3D \n");
				}else if((strcmp(argv[j], "GPU3D")==0)&&(cpu_is_mx6dl()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_gpu3d_6dl;
				  printf("MMDC GPU3D \n");
				}else if((strcmp(argv[j], "GPU3D")==0)&&(cpu_is_mx6q()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_gpu3d_6q;
				  printf("MMDC GPU3D \n");
				}else if((strcmp(argv[j], "GPU2D1")==0)&&(cpu_is_mx6dl()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_gpu2d1_6dl;
				  printf("MMDC GPU2D1 \n");
				}else if((strcmp(argv[j], "GPU2D")==0)&&(cpu_is_mx6q()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_gpu2d_6q;
				  printf("MMDC GPU2D \n");
				}else if((strcmp(argv[j], "GPU2D2")==0)&&(cpu_is_mx6dl()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_gpu2d2_6dl;
				  printf("MMDC GPU2D2 \n");
				}else if((strcmp(argv[j], "GPU2D")==0)&&(cpu_is_mx6sl()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_gpu2d_6sl;
				  printf("MMDC GPU2D \n");
				}else if((strcmp(argv[j],"VPU")==0)&&(cpu_is_mx6dl()==1)){
				  ((pMMDC_t)A)->madpcr1 = axi_vpu_6dl;
				  printf("MMDC VPU \n");
				}else if((strcmp(argv[j],"VPU")==0)&&(cpu_is_mx6q()==1)){
				   ((pMMDC_t)A)->madpcr1 = axi_vpu_6q;
				   printf("MMDC VPU \n");
				}else if((strcmp(argv[j],"GPUVG")==0)&&(cpu_is_mx6q()==1)){
				   ((pMMDC_t)A)->madpcr1 = axi_openvg_6q;
				   printf("MMDC GPUVG \n");
				}else if((strcmp(argv[j],"GPUVG")==0)&&(cpu_is_mx6sl()==1)){
				   ((pMMDC_t)A)->madpcr1 = axi_openvg_6sl;
				   printf("MMDC GPUVG \n");
				}else if(strcmp(argv[j],"ARM")==0){
					if((cpu_is_mx6sx()==1))
						((pMMDC_t)A)->madpcr1 = axi_arm_6sx;
					else
						((pMMDC_t)A)->madpcr1 = axi_arm;
					printf("MMDC ARM \n");
				}else if(strcmp(argv[j], "SUM")==0){
				  ((pMMDC_t)A)->madpcr1 = axi_default;
				  printf("MMDC SUM \n");
				}else{
					printf("MMDC DOES NOT KNOW %s \n",argv[j]);
					help();
					close(fd);
					return 0;
				}
				msync(&(((pMMDC_t)A)->madpcr1),4,MS_SYNC);
				clear_mmdc_results((pMMDC_t)A);
				ulStartTime=getTickCount();
				start_mmdc_profiling((pMMDC_t)A);
				sleep(timeForSleep);
				load_mmdc_results((pMMDC_t)A);
				get_mmdc_profiling_results((pMMDC_t)A, &results);
				print_mmdc_profiling_results(results , RES_FULL,getTickCount()-ulStartTime);
				fflush(stdout);
				stop_mmdc_profiling((pMMDC_t)A);
			}
		}else {
			if(customized_madpcr1!= 0)
			{
				((pMMDC_t)A)->madpcr1 = customized_madpcr1;
				msync(&(((pMMDC_t)A)->madpcr1),4,MS_SYNC);
				printf("MMDC 0x%x \n",customized_madpcr1);
			}else{
				((pMMDC_t)A)->madpcr1 = axi_default;
				msync(&(((pMMDC_t)A)->madpcr1),4,MS_SYNC);
				printf("MMDC SUM \n");
			}
			clear_mmdc_results((pMMDC_t)A);
			ulStartTime=getTickCount();
			start_mmdc_profiling((pMMDC_t)A);
			sleep(timeForSleep); //leave it collect 1sec data;
			load_mmdc_results((pMMDC_t)A);
			get_mmdc_profiling_results((pMMDC_t)A, &results);
			print_mmdc_profiling_results(results , RES_FULL,getTickCount()-ulStartTime);
			fflush(stdout);
			stop_mmdc_profiling((pMMDC_t)A);
		}
	}
	close(fd);
	return 0;
}
