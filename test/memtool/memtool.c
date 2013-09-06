/*
 * Copyright (C) 2006-2012 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "memtools_register_info.h"

int g_size = 4;
unsigned long g_paddr;
int g_is_write;
uint32_t g_value = 0;
uint32_t g_count = 1;
int g_is_reg = 0;
char *g_module;
char *g_reg;
char *g_field;

int g_fmem;
int g_map_vaddr = 0;
int g_map_paddr = 0;

#define MAP_SIZE 0x1000
#define KERN_VER(a, b, c) (((a) << 16) + ((b) << 8) + (c))

extern const module_t mx6q[];
extern const module_t mx6dl[];
extern const module_t mx6sl[];

char g_buffer[4096];

void die(char *p)
{
	printf("%s\n", p);
	exit(-1);
}

int open_mem_file(void)
{
	if (!g_fmem)
		g_fmem = open("/dev/mem", O_RDWR | O_SYNC, 0);

	if (!g_fmem)
		die("Can't open file /dev/mem\n");
	return 0;
}

int map_address(int address)
{
	if ((address & (~(MAP_SIZE - 1))) == g_map_paddr)
		return 0;
	if (g_map_vaddr)
		munmap((void *)g_map_vaddr, MAP_SIZE);

	g_map_vaddr =
	    (int)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		      g_fmem, address & (~(MAP_SIZE - 1)));
	g_map_paddr = address & (~(MAP_SIZE - 1));

	return 0;
}

int readm(int address, int width)
{
	uint8_t *addr8;
	uint16_t *addr16;
	uint32_t *addr32;

	open_mem_file();
	map_address(address);

	addr32 = (uint32_t *) (g_map_vaddr + (address & (MAP_SIZE - 1)));
	addr16 = (uint16_t *) addr32;
	addr8 = (uint8_t *) addr16;

	switch (width) {
	case 1:
		return *addr8;
	case 2:
		return *addr16;
	case 4:
		return *addr32;
	default:
		die("Unknown size\n");
	}
	return 0;
}

int writem(int address, int width, int value)
{
	uint8_t *addr8;
	uint16_t *addr16;
	uint32_t *addr32;

	open_mem_file();
	map_address(address);
	addr32 = (uint32_t *) (g_map_vaddr + (address & (MAP_SIZE - 1)));
	addr16 = (uint16_t *) addr32;
	addr8 = (uint8_t *) addr16;

	switch (width) {
	case 1:
		*addr8 = value;
		break;
	case 2:
		*addr16 = value;
		break;
	case 4:
		*addr32 = value;
		break;
	default:
		die("Unknown size\n");
	}
	return 0;

}

unsigned int get_value(int value, int lsb, int msb)
{
	return (value >> lsb) & (0xFFFFFFFF >> (31 - (msb - lsb)));
}

void print_blank(int x)
{
	int i = 0;
	for (i = 0; i < x; i++)
		putchar(' ');
}

void parse_field(const module_t * mx, const reg_t * reg, char *field, int value)
{
	const field_t *f = reg->fields;
	char *str = NULL;
	int i = 0;

	if (field)
		str = strchr(field, '*');
	if (str != NULL)
		*str = 0;

	for (i = 0; i < reg->field_count; i++) {
		if (field == NULL || *field == 0
		    || strncmp(f->name, field, strlen(field)) == 0) {
			printf("     %s.%s.%s(%d..%d) \t:0x%x\n", mx->name,
			       reg->name, f->name, f->lsb, f->msb,
			       get_value(value, f->lsb, f->msb)
			    );
			printf("             %s\n", f->description);
		}
		f++;
	}
}

void parse_reg(const module_t * mx, const char *reg, char *field)
{
	const reg_t *sreg = mx->regs;
	char *str;

	int i = 0;

	str = strchr(reg, '*');
	if (str != NULL)
		*str = 0;	/*cut register name */

	for (i = 0; i < mx->reg_count; i++) {
		if (reg == NULL || *reg == 0 || strlen(reg) == 0 ||
		    strncmp(sreg->name, reg, strlen(reg)) == 0 || *reg == '-') {
			printf("  %s.%s Addr:0x%08X Value:0x%08X - %s\n",
			       mx->name, sreg->name,
			       mx->base_address + sreg->offset,
			       readm(mx->base_address + sreg->offset,
				     sreg->width),
			       sreg->description);
			if (!(reg && *reg == '-'))
				parse_field(mx, sreg, field,
					    readm(mx->base_address +
						  sreg->offset, sreg->width));
			printf("\n");
		}
		sreg++;
	}
}

void write_reg_mask(int addr, int width, int value, int lsb, int msb)
{
	int org;
	unsigned int mask;

	printf("write 0x%08X to Bit %d..%d of 0x%08X \n", value, lsb, msb,
	       addr);

	org = readm(addr, width);
	mask = 0xFFFFFFFF;
	mask >>= lsb;
	mask <<= lsb;

	mask <<= 31 - msb;
	mask >>= 31 - msb;

	org &= ~mask;
	value <<= lsb;
	org |= value & mask;

	writem(addr, width, org);
}

void write_reg(int addr, int width, int value)
{
	printf("write 0x%08X to 0x%08X\n", value, addr);
	writem(addr, width, value);
}

void parse_module(char *module, char *reg, char *field, int iswrite)
{
	const module_t *mx = NULL;
	char *str = NULL;
	int fd = 0;
	int n;
	char *rev;

	int kv, kv_major, kv_minor, kv_rel;
	int rev_major, rev_minor;
	struct utsname sys_name;

	if (uname(&sys_name) < 0)
		die("uname error");

	if (sscanf(sys_name.release, "%d.%d.%d", &kv_major, &kv_minor, &kv_rel) != 3)
		die("fail to get release version");

	kv = ((kv_major << 16) + (kv_minor << 8) + kv_rel);
	if (kv < KERN_VER(3, 10, 0)) {
		fd = open("/proc/cpuinfo", O_RDONLY);
		if (fd < 0)
			die("can't open file /proc/cpuinfo\n");

		n = read(fd, g_buffer, 4095);
		if ((rev = strstr(g_buffer, "Revision"))) {
			int r;
			rev = strstr(rev, ":");
			if (!rev)
				die("Unkown CPUInfo format\n");

			r = strtoul(rev + 2, NULL, 16);

			switch (r >> 12) {
			case 0x63:
				mx = mx6q;
				printf("SOC is mx6q\n\n");
				break;
			case 0x61:
				mx = mx6dl;
				printf("SOC is mx6dl\n\n");
				break;
			case 0x60:
				mx = mx6sl;
				printf("SOC is mx6sl\n\n");
				break;
			default:
				die("Unknown SOC\n\n");
			}
		} else
			die("Unknown SOC\n");
	} else {
		FILE *fp;
		char soc_name[255];

		fp = fopen("/sys/devices/soc0/soc_id", "r");
		if (fp == NULL) {
			perror("/sys/devices/soc0/soc_id");
			die("fail to get soc_id");
		}

		if (fscanf(fp, "%s", soc_name) != 1) {
			fclose(fp);
			die("fail to get soc_name");
		}
		fclose(fp);

		printf("SOC: %s\n", soc_name);

		if (!strcmp(soc_name, "i.MX6Q"))
			mx = mx6q;
		else if (!strcmp(soc_name, "i.MX6DL"))
			mx = mx6dl;
		else if (!strcmp(soc_name, "i.MX6SL"))
			mx = mx6sl;
		else
			die("Unknown SOC\n");	
	}

	if (iswrite) {
		while (mx) {
			if (strcmp(mx->name, module) == 0) {

				const reg_t *r = mx->regs;
				int i = 0;
				for (i = 0; i < mx->reg_count; i++) {
					if (strcmp(r->name, reg) == 0) {
						if (field == NULL || *field == 0) {
							if (r->is_writable)
								write_reg(mx->
									  base_address
									  +
									  r->
									  offset,
									  r->
									  width,
									  g_value);
							else
								printf
								    ("%s.%s is not writable register\n",
								     mx->name,
								     r->name);
							return;
						} else {
							const field_t *f = r->fields;
							int j;
							for (j = 0; j < r->field_count; j++) {
								if (strcmp(f->name, field) == 0) {
									if (f->is_writable)
										write_reg_mask
										    (mx->
										     base_address
										     +
										     r->
										     offset,
										     r->
										     width,
										     g_value,
										     f->
										     lsb,
										     f->
										     msb);
									else
										printf
										    ("%s.%s.%s is not writable\n",
										     mx->
										     name,
										     r->
										     name,
										     f->
										     name);
								}
								f++;

							}
							return;
						}
					}

					r++;

				}
				printf("Can't find register %s\n", reg);
				return;
			}
			mx++;
		}
		printf("Can't find module %s\n", module);
		return;
	}
	if (module == NULL || *module == 0 || (str = strchr(module, '*'))) {	/* list all modules */
		printf("   Module\t\tBase Address\n");
		if (str)
			*str = 0;	/*Cut module */

		while (mx->name) {
			if (str == NULL
			    || (strncmp(mx->name, module, strlen(module)) ==
				0)) {
				printf("  %s", mx->name);
				print_blank(20 - strlen(mx->name));
				printf("0x%08X\n", mx->base_address);
			}
			mx++;
		}
		return;
	}

	while (mx->name) {

		if (strcmp(mx->name, module) == 0) {
			printf("%s\t Addr:0x%x \n", mx->name, mx->base_address);
			parse_reg(mx, reg, field);
			break;
		}
		mx++;
	}
}

int parse_cmdline(int argc, char **argv)
{
	int cur_arg = 0;
	char *str;

	if (argc < 2)
		return -1;

	cur_arg++;
	if (strcmp(argv[cur_arg], "-8") == 0) {
		cur_arg++;
		g_size = 1;
	} else if (strcmp(argv[cur_arg], "-16") == 0) {
		cur_arg++;
		g_size = 2;
	} else if (strcmp(argv[cur_arg], "-32") == 0) {
		cur_arg++;
		g_size = 4;
	}
	if (cur_arg >= argc)
		return -1;

	if ((str = strchr(argv[cur_arg], '.'))) {
		/*extend memory command */
		char *equal = 0;
		g_is_reg = 1;
		*str = 0;
		str++;
		g_module = argv[cur_arg];
		g_reg = str;

		if ((g_field = strchr(str, '.'))) {
			*g_field = 0;
			g_field++;
		}
		if ((equal = strchr(g_field ? g_field : str, '='))) {
			g_is_write = 1;
			*equal = 0;
			equal++;
			g_value = strtoul(equal, NULL, 16);
			return 0;
		}
		return 0;
		//cur_arg++;
	}

	if (!g_is_reg) {
		g_paddr = strtoul(argv[cur_arg], NULL, 16);
		if (!g_paddr)
			return -1;
	}

	if ((str = strchr(argv[cur_arg], '='))) {
		g_is_write = 1;
		if (strlen(str) > 1) {
			str++;
			g_value = strtoul(str, NULL, 16);
			return 0;
		}
	}
	if (++cur_arg >= argc)
		return -1;

	if ((argv[cur_arg])[0] == '=') {
		g_is_write = 1;
		if (strlen(argv[cur_arg]) > 1) {
			(argv[cur_arg])++;
		} else {
			if (++cur_arg >= argc)
				return -1;
		}
		g_value = strtoul(argv[cur_arg], NULL, 16);
	} else {
		if (g_is_write)
			g_value = strtoul(argv[cur_arg], NULL, 16);
		else
			g_count = strtoul(argv[cur_arg], NULL, 16);
	}
	printf("E\n");
	return 0;
}

void read_mem(void *addr, uint32_t count, uint32_t size)
{
	int i;
	uint8_t *addr8 = addr;
	uint16_t *addr16 = addr;
	uint32_t *addr32 = addr;

	switch (size) {
	case 1:
		for (i = 0; i < count; i++) {
			if ((i % 16) == 0)
				printf("\n0x%08X: ", g_paddr);
			printf(" %02X", addr8[i]);
			g_paddr++;
		}
		break;
	case 2:
		for (i = 0; i < count; i++) {
			if ((i % 8) == 0)
				printf("\n0x%08X: ", g_paddr);
			printf(" %04X", addr16[i]);
			g_paddr += 2;
		}
		break;
	case 4:
		for (i = 0; i < count; i++) {
			if ((i % 4) == 0)
				printf("\n0x%08X: ", g_paddr);
			printf(" %08X", addr32[i]);
			g_paddr += 4;
		}
		break;
	}
	printf("\n\n");

}

void write_mem(void *addr, uint32_t value, uint32_t size)
{
	uint8_t *addr8 = addr;
	uint16_t *addr16 = addr;
	uint32_t *addr32 = addr;

	switch (size) {
	case 1:
		*addr8 = value;
		break;
	case 2:
		*addr16 = value;
		break;
	case 4:
		*addr32 = value;
		break;
	}
}

int main(int argc, char **argv)
{
	int fd;
	void *mem;
	void *aligned_vaddr;
	unsigned long aligned_paddr;
	uint32_t aligned_size;

	if (parse_cmdline(argc, argv)) {
		printf("Usage:\n\n"
		       "Read memory: memtool [-8 | -16 | -32] <phys addr> <count>\n"
		       "Write memory: memtool [-8 | -16 | -32] <phys addr>=<value>\n\n"
		       "List SOC module: memtool *. or memtool .\n"
		       "Read register:  memtool UART1.*\n"
		       "                memtool UART1.UMCR\n"
		       "                memtool UART1.UMCR.MDEN\n"
		       "		memtool UART1.-\n"
		       "Write register: memtool UART.UMCR=0x12\n"
		       "                memtool UART.UMCR.MDEN=0x1\n"
		       "Default access size is 32-bit.\n\nAddress, count and value are all in hex.\n");
		return 1;
	}

	if (g_is_reg) {

		parse_module(g_module, g_reg, g_field, g_is_write);
		return 0;
	}
	/* Align address to access size */
	g_paddr &= ~(g_size - 1);

	aligned_paddr = g_paddr & ~(4096 - 1);
	aligned_size = g_paddr - aligned_paddr + (g_count * g_size);
	aligned_size = (aligned_size + 4096 - 1) & ~(4096 - 1);

	if (g_is_write)
		printf("Writing %d-bit value 0x%X to address 0x%08X\n",
		       g_size * 8, g_value, g_paddr);
	else
		printf("Reading 0x%X count starting at address 0x%08X\n",
		       g_count, g_paddr);

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC, 0)) < 0)
		return 1;

	aligned_vaddr =
	    mmap(NULL, aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		 aligned_paddr);
	if (aligned_vaddr == NULL) {
		printf("Error mapping address\n");
		close(fd);
		return 1;
	}

	mem = (void *)((uint32_t) aligned_vaddr + (g_paddr - aligned_paddr));

	if (g_is_write) {
		write_mem(mem, g_value, g_size);
	} else {
		read_mem(mem, g_count, g_size);
	}

	munmap(aligned_vaddr, aligned_size);
	close(fd);

	if (g_map_vaddr)
		munmap((void *)g_map_vaddr, MAP_SIZE);
	if (g_fmem)
		close(g_fmem);

	return 0;
}
