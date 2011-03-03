/*
 * Copyright (c) 2009-2010 Atheros Communications Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>

#include "hciattach.h"

#define FALSE				0
#define TRUE				1

/* The maximum number of bytes possible in a patch entry */
#define MAX_PATCH_SIZE			20000

/* Maximum HCI packets that will be formed from the Patch file */
#define MAX_NUM_PATCH_ENTRY		((MAX_PATCH_SIZE/MAX_BYTE_LENGTH) + 1)

#define DEV_REGISTER			0x4FFC

#ifdef ANDROID_OS
#define FW_PATH                         "/system/lib/firmware/ar3k"
#else
#define FW_PATH				"/lib/firmware/ar3k/"
#endif

#define PS_ASIC_FILE			"PS_ASIC.pst"
#define PS_FPGA_FILE			"PS_FPGA.pst"
#define PATCH_FILE			"RamPatch.txt"
#define BDADDR_FILE			"ar3kbdaddr.pst"

#define HCI_CMD_HEADER_LEN		7

/* PS command types */
#define PS_RESET			2
#define PS_WRITE			1
#define WRITE_PATCH			8
#define PS_VERIFY_CRC			9
#define ENABLE_PATCH			11

/* PS configuration entry time */
#define PS_TYPE_HEX			0
#define PS_TYPE_DEC			1

#define PS_RESET_PARAM_LEN		6
#define PS_RESET_CMD_LEN		(PS_RESET_PARAM_LEN +\
					 HCI_CMD_HEADER_LEN)

#define NUM_WAKEUP_RETRY		10


#define RAM_PS_REGION			(1<<0)
#define RAM_PATCH_REGION		(1<<1)

#define RAMPS_MAX_PS_TAGS_PER_FILE	50
#define PS_MAX_LEN			500
#define LINE_SIZE_MAX			(PS_MAX_LEN * 2)

#define BYTES_OF_PS_DATA_PER_LINE	16
#define MAX_BYTE_LENGTH                 244

#define skip_space(str)	while (*(str) == (' ')) ((str)++)

#define IS_BETWEEN(x, lower, upper)	(((lower) <= (x)) && ((x) <= (upper)))

#define tohexval(c)	(isdigit(c) ? ((c) - '0') :	\
			(IS_BETWEEN((c), 'A', 'Z') ?	\
			((c) - 'A' + 10) : ((c) - 'a' + 10)))

#define stringtohex(str)	(((uint8_t)(tohexval((str)[0]) << 4)) |\
				((uint8_t)tohexval((str)[1])))

#define set_pst_format(pst, type, array_val)	((pst)->data_type = (type),\
						(pst)->is_array = (array_val))

struct ps_tag_entry {
	uint32_t		tag_id;
	uint32_t		tag_len;
	uint8_t			*tag_data;
};

struct ps_ram_patch {
	int16_t			Len;
	uint8_t			*Data;
};
struct ps_data_format {
	unsigned char		data_type;
	unsigned char		is_array;
};

struct ps_cmd_packet {
	uint8_t			*Hcipacket;
	int			packetLen;
};

struct st_read_status {
	unsigned		section;
	unsigned		line_count;
	unsigned		char_cnt;
	unsigned		byte_count;
};

struct ps_tag_entry		ps_tag_entry[RAMPS_MAX_PS_TAGS_PER_FILE];
struct ps_ram_patch		ram_patch[MAX_NUM_PATCH_ENTRY];

static void load_hci_header(uint8_t *hci_ps_cmd,
			    uint8_t opcode,
			    int length,
			    int index)
{
	hci_ps_cmd[0] = 0x0B;
	hci_ps_cmd[1] = 0xFC;
	hci_ps_cmd[2] = length + 4;
	hci_ps_cmd[3] = opcode;
	hci_ps_cmd[4] = (index & 0xFF);
	hci_ps_cmd[5] = ((index >> 8) & 0xFF);
	hci_ps_cmd[6] = length;
}

static int ath_create_ps_command(uint8_t opcode,
				 uint32_t param_1,
				 struct ps_cmd_packet *ps_patch_packet,
				 uint32_t *index)
{
	uint8_t *hci_ps_cmd;
	int i;

	switch (opcode) {
	case WRITE_PATCH:

		ATH_INFO("WRITE_PATCH");

		for (i = 0; i < param_1; i++) {

			/* Allocate command buffer */
			hci_ps_cmd = (uint8_t *) malloc(ram_patch[i].Len +
							HCI_CMD_HEADER_LEN);

			if (!hci_ps_cmd)
				return -ENOMEM;

			/* Update commands to buffer */
			load_hci_header(hci_ps_cmd,
					opcode,
					ram_patch[i].Len,
					i);
			memcpy(&hci_ps_cmd[HCI_CMD_HEADER_LEN],
			       ram_patch[i].Data,
			       ram_patch[i].Len);

			ps_patch_packet[*index].Hcipacket = hci_ps_cmd;
			ps_patch_packet[*index].packetLen = ram_patch[i].Len +
							    HCI_CMD_HEADER_LEN;

			(*index)++;
		}
		break;

	case ENABLE_PATCH:

		ATH_INFO("ENABLE_PATCH");

		hci_ps_cmd = (uint8_t *) malloc(HCI_CMD_HEADER_LEN);

		if (!hci_ps_cmd)
			return -ENOMEM;

		load_hci_header(hci_ps_cmd, opcode, 0, 0x00);
		ps_patch_packet[*index].Hcipacket = hci_ps_cmd;
		ps_patch_packet[*index].packetLen = HCI_CMD_HEADER_LEN;

		(*index)++;

		break;

	case PS_RESET:

		ATH_INFO("PS_RESET");

		hci_ps_cmd = (uint8_t *) malloc(PS_RESET_CMD_LEN);

		if (!hci_ps_cmd)
			return -ENOMEM;

		load_hci_header(hci_ps_cmd, opcode, PS_RESET_PARAM_LEN, 0x00);
		hci_ps_cmd[7] = 0x00;
		hci_ps_cmd[PS_RESET_CMD_LEN - 2] = (param_1 & 0xFF);
		hci_ps_cmd[PS_RESET_CMD_LEN - 1] = ((param_1 >> 8) & 0xFF);

		ps_patch_packet[*index].Hcipacket = hci_ps_cmd;
		ps_patch_packet[*index].packetLen = PS_RESET_CMD_LEN;

		(*index)++;

		break;

	case PS_WRITE:

		ATH_INFO("PS_WRITE");

		for (i = 0; i < param_1; i++) {
			hci_ps_cmd =
			    (uint8_t *) malloc(ps_tag_entry[i].tag_len +
					       HCI_CMD_HEADER_LEN);
			if (!hci_ps_cmd)
				return -ENOMEM;

			load_hci_header(hci_ps_cmd,
					opcode,
					ps_tag_entry[i].tag_len,
					ps_tag_entry[i].tag_id);

			memcpy(&hci_ps_cmd[HCI_CMD_HEADER_LEN],
			       ps_tag_entry[i].tag_data,
			       ps_tag_entry[i].tag_len);

			ps_patch_packet[*index].Hcipacket = hci_ps_cmd;

			ps_patch_packet[*index].packetLen =
			ps_tag_entry[i].tag_len + HCI_CMD_HEADER_LEN;

			(*index)++;
		}
		break;

	default:
		break;
	}

	return 0;
}

static int get_ps_type(char *line,
		       int eol_index,
		       unsigned char *type,
		       unsigned char *sub_type)
{

	switch (eol_index) {
	case 1:
		return 0;
	break;

	case 2:
		(*type) = toupper(line[1]);
	break;

	case 3:
		if (line[2] == ':')
			(*type) = toupper(line[1]);
		else if (line[1] == ':')
			(*sub_type) = toupper(line[2]);
		else
			return -1;

	break;

	case 4:
		if (line[2] != ':')
			return -1;
		(*type) = toupper(line[1]);
		(*sub_type) = toupper(line[3]);
	break;

	case -1:
		return -1;
	break;
	}
	return 0;
}

static int get_input_data_format(char *line, struct ps_data_format *pst_format)
{
	unsigned char type, sub_type;
	int eol_index, sep_index;
	int i;

	type = '\0';
	sub_type = '\0';
	eol_index = -1;
	sep_index = -1;

	/* The default values */
	set_pst_format(pst_format, PS_TYPE_HEX, TRUE);

	if (line[0] != '[') {

		set_pst_format(pst_format, PS_TYPE_HEX, TRUE);
		return 0;
	}

	for (i = 1; i < 5; i++) {
		if (line[i] == ']') {
			eol_index = i;
			break;
		}
	}

	if (get_ps_type(line, eol_index, &type, &sub_type) < 0)
		return -1;

	/* By default Hex array type is assumed */
	if (type == '\0' && sub_type == '\0')
		set_pst_format(pst_format, PS_TYPE_HEX, TRUE);

	/* Check is data type is of array */
	if (type == 'A' || sub_type == 'A')
		pst_format->is_array = TRUE;

	if (type == 'S' || sub_type == 'S')
		pst_format->is_array = FALSE;

	switch (type) {
	case 'D':
	case 'B':
		pst_format->data_type = PS_TYPE_DEC;
	break;

	default:
		pst_format->data_type = PS_TYPE_HEX;
	break;
	}

	line += (eol_index + 1);

	return 0;

}

static unsigned int read_data_in_section(char *line,
					 struct ps_data_format format_info)
{
	char *token_ptr = line;

	if (token_ptr[0] == '[') {

		while (token_ptr[0] != ']' && token_ptr[0] != '\0')
			token_ptr++;

		if (token_ptr[0] == '\0')
			return 0x0FFF;

		token_ptr++;
	}

	if (format_info.data_type == PS_TYPE_HEX) {

		if (format_info.is_array == TRUE)
			return 0x0FFF;
		else
			return strtol(token_ptr, NULL, 16);
	} else
		return 0x0FFF;

	return 0x0FFF;
}
static int ath_parse_file(FILE *stream)
{
	char *buffer;
	char *line;
	uint8_t tag_cnt;
	int16_t byte_count;
	uint32_t pos;
	int read_count;
	int num_ps_entry;
	struct ps_data_format stps_data_format;
	struct st_read_status read_status = {
		0, 0, 0, 0
	};

	pos = 0;
	buffer = NULL;
	tag_cnt = 0;
	byte_count = 0;

	if (!stream) {
		perror("Could not open config file  .\n");
		return -1;
	}

	buffer = malloc(LINE_SIZE_MAX + 1);

	if (!buffer)
		return -ENOMEM;

	do {
		line = fgets(buffer, LINE_SIZE_MAX, stream);

		if (!line)
			break;

		skip_space(line);

		if ((line[0] == '/') && (line[1] == '/'))
			continue;

		if ((line[0] == '#')) {

			if (read_status.section != 0) {
				perror("error\n");

				if (buffer != NULL)
					free(buffer);

				return -1;

			} else {
				read_status.section = 1;
				continue;
			}
		}

		if ((line[0] == '/') && (line[1] == '*')) {

			read_status.section = 0;

			continue;
		}

		if (read_status.section == 1) {
			skip_space(line);

			if (get_input_data_format(
			    line, &stps_data_format) < 0) {

				if (buffer != NULL)
					free(buffer);
				return -1;
			}
			ps_tag_entry[tag_cnt].tag_id =
			    read_data_in_section(line, stps_data_format);
			read_status.section = 2;

		} else if (read_status.section == 2) {

			if (get_input_data_format(
			    line, &stps_data_format) < 0) {

				if (buffer != NULL)
					free(buffer);
				return -1;
			}

			byte_count =
			    read_data_in_section(line, stps_data_format);

			read_status.section = 2;
			if (byte_count > LINE_SIZE_MAX / 2) {
				if (buffer != NULL)
					free(buffer);

				return -1;
			}

			ps_tag_entry[tag_cnt].tag_len = byte_count;
			ps_tag_entry[tag_cnt].tag_data = (uint8_t *)
							 malloc(byte_count);

			read_status.section = 3;
			read_status.line_count = 0;

		} else if (read_status.section == 3) {

			if (read_status.line_count == 0) {
				if (get_input_data_format(
				    line, &stps_data_format) < 0) {
					if (buffer != NULL)
						free(buffer);
					return -1;
				}
			}

			skip_space(line);
			read_status.char_cnt = 0;

			if (line[read_status.char_cnt] == '[') {

				while (line[read_status.char_cnt] != ']' &&
				       line[read_status.char_cnt] != '\0')
					read_status.char_cnt++;

				if (line[read_status.char_cnt] == ']')
					read_status.char_cnt++;
				else
					read_status.char_cnt = 0;

			}

			read_count = (byte_count > BYTES_OF_PS_DATA_PER_LINE)
			    ? BYTES_OF_PS_DATA_PER_LINE : byte_count;

			if (stps_data_format.data_type == PS_TYPE_HEX &&
			    stps_data_format.is_array == TRUE) {

				while (read_count > 0) {

					ps_tag_entry[tag_cnt].tag_data
					[read_status.byte_count] =
					stringtohex(
					&line[read_status.char_cnt]);

					ps_tag_entry[tag_cnt].tag_data
					[read_status.byte_count + 1] =
					stringtohex(
					&line[read_status.char_cnt + 3]);

					read_status.char_cnt += 6;
					read_status.byte_count += 2;
					read_count -= 2;

				}

				if (byte_count > BYTES_OF_PS_DATA_PER_LINE)
					byte_count -=
						BYTES_OF_PS_DATA_PER_LINE;
				else
					byte_count = 0;
			}

			read_status.line_count++;

			if (byte_count == 0) {
				read_status.section = 0;
				read_status.char_cnt = 0;
				read_status.line_count = 0;
				read_status.byte_count = 0;
			} else
				read_status.char_cnt = 0;

			if ((read_status.section == 0) &&
			    (++tag_cnt == RAMPS_MAX_PS_TAGS_PER_FILE)) {
				if (buffer != NULL)
					free(buffer);
				return -1;
			}

		}

	} while (line);

	num_ps_entry = tag_cnt;

	if (tag_cnt > RAMPS_MAX_PS_TAGS_PER_FILE) {
		if (buffer != NULL)
			free(buffer);
		return -1;
	}

	if (buffer != NULL)
		free(buffer);

	return num_ps_entry;
}

static int parse_patch_file(FILE *stream)
{
	char byte[3];
	char line[MAX_BYTE_LENGTH + 1];
	int byte_cnt, byte_cnt_org;
	int patch_index = 0;
	int i, k;
	int data;
	int patch_count = 0;

	byte[2] = '\0';

	while (fgets(line, MAX_BYTE_LENGTH, stream)) {
		if (strlen(line) <= 1 || !isxdigit(line[0]))
			continue;
		else
			break;
	}


	byte_cnt = strtol(line, NULL, 16);
	byte_cnt_org = byte_cnt;

	while (byte_cnt > MAX_BYTE_LENGTH) {

		/* Handle case when the number of patch buffer is
		 * more than the 20K */
		if (MAX_NUM_PATCH_ENTRY == patch_count) {
			for (i = 0; i < patch_count; i++)
				free(ram_patch[i].Data);
			return -ENOMEM;
		}
		ram_patch[patch_count].Len = MAX_BYTE_LENGTH;
		ram_patch[patch_count].Data =
		    (uint8_t *) malloc(MAX_BYTE_LENGTH);

		if (!ram_patch[patch_count].Data)
			return -ENOMEM;

		patch_count++;
		byte_cnt = byte_cnt - MAX_BYTE_LENGTH;
	}

	ram_patch[patch_count].Len = (byte_cnt & 0xFF);

	if (byte_cnt != 0) {
		ram_patch[patch_count].Data = (uint8_t *) malloc(byte_cnt);

		if (!ram_patch[patch_count].Data)
			return -ENOMEM;
		patch_count++;
	}

	while (byte_cnt_org > MAX_BYTE_LENGTH) {

		k = 0;
		for (i = 0; i < MAX_BYTE_LENGTH * 2; i += 2) {
			if (!fgets(byte, 3, stream))
				return -1;
			data = strtoul(&byte[0], NULL, 16);
			ram_patch[patch_index].Data[k] = (data & 0xFF);

			k++;
		}

		patch_index++;

		byte_cnt_org = byte_cnt_org - MAX_BYTE_LENGTH;
	}

	if (patch_index == 0)
		patch_index++;

	for (k = 0; k < byte_cnt_org; k++) {

		if (!fgets(byte, 3, stream))
			return -1;

		data = strtoul(byte, NULL, 16);
		ram_patch[patch_index].Data[k] = (data & 0xFF);
	}

	return patch_count;
}

static int ath_parse_ps(FILE *stream, int *total_tag_len)
{
	int num_ps_tags;
	int i;
	unsigned char bdaddr_present = 0;


	if (NULL != stream)
		num_ps_tags = ath_parse_file(stream);

	if (num_ps_tags < 0)
		return -1;

	if (num_ps_tags == 0)
		*total_tag_len = 10;
	else {

		for (i = 0; i < num_ps_tags; i++) {

			if (ps_tag_entry[i].tag_id == 1)
				bdaddr_present = 1;
			if (ps_tag_entry[i].tag_len % 2 == 1)
				*total_tag_len = *total_tag_len
					+ ps_tag_entry[i].tag_len + 1;
			else
				*total_tag_len =
				*total_tag_len + ps_tag_entry[i].tag_len;

		}
	}
	if (num_ps_tags > 0 && !bdaddr_present)
		*total_tag_len = *total_tag_len + 10;

	*total_tag_len = *total_tag_len + 10 + (num_ps_tags * 4);

	return num_ps_tags;
}

static int ath_create_cmd_list(struct ps_cmd_packet **hci_packet_list,
			       uint32_t *num_packets,
			       int tag_count,
			       int patch_count,
			       int total_tag_len)
{
	uint8_t count;
	uint32_t num_cmd_entry = 0;

	*num_packets = 0;


	if (patch_count || tag_count) {

		/* PS Reset Packet  + Patch List + PS List */
		num_cmd_entry += (1 + patch_count + tag_count);
		if (patch_count > 0)
			num_cmd_entry++;	/* Patch Enable Command */

		(*hci_packet_list) =
		    malloc(sizeof(struct ps_cmd_packet) * num_cmd_entry);

		if (!(*hci_packet_list))
			return	-ENOMEM;

		if (patch_count > 0) {

			if (ath_create_ps_command(WRITE_PATCH, patch_count,
			    *hci_packet_list, num_packets) < 0)
				return -1;
			if (ath_create_ps_command(ENABLE_PATCH, 0,
			    *hci_packet_list, num_packets) < 0)
				return -1;

		}

		if (ath_create_ps_command(PS_RESET, total_tag_len,
		    *hci_packet_list, num_packets) < 0)
			return -1;

		if (tag_count > 0)
			ath_create_ps_command(PS_WRITE, tag_count,
					      *hci_packet_list, num_packets);
	}

	for (count = 0; count < patch_count; count++)
		free(ram_patch[patch_count].Data);

	for (count = 0; count < tag_count; count++)
		free(ps_tag_entry[count].tag_data);

	return *num_packets;
}

static int ath_free_command_list(struct ps_cmd_packet **hci_packet_list,
			  uint32_t num_packets)
{
	int i;

	if (*hci_packet_list == NULL)
		return -1;

	for (i = 0; i < num_packets; i++)
		free((*hci_packet_list)[i].Hcipacket);

	free(*hci_packet_list);

	return 0;
}

/*
 *  This API is used to send the HCI command to controller and return
 *  with a HCI Command Complete event.
 */
static int send_hci_cmd_wait_event(int dev,
			    uint8_t *hci_command,
			    int cmd_length,
			    uint8_t **event, uint8_t **buffer_to_free)
{
	int r;
	uint8_t *hci_event;
	uint8_t pkt_type = 0x01;

	if (cmd_length == 0)
		return -1;

	if (write(dev, &pkt_type, 1) != 1)
		return -1;

	if (write(dev, (unsigned char *)hci_command, cmd_length) != cmd_length)
		return -1;

	hci_event = (uint8_t *) malloc(100);

	if (!hci_event)
		return -ENOMEM;

	r = read_hci_event(dev, (unsigned char *)hci_event, 100);

	if (r > 0) {
		*event = hci_event;
		*buffer_to_free = hci_event;
	} else {

		/* Did not get an event from controller. return error */
		free(hci_event);
		*buffer_to_free = NULL;
		return -1;
	}

	return 0;
}

static int read_ps_event(uint8_t *data)
{

	if (data[5] == 0xFC && data[6] == 0x00) {
		switch (data[4]) {
		case 0x0B:/* CRC Check */
		case 0x0C:/* Change Baudrate */
		case 0x04:/* Enable sleep */
			return 0;
		break;
		default:
			return -1;
		break;
		}
	}

	return -1;
}

static int get_ps_file_name(int devtype, int rom_version, char *path)
{
	char *filename;
	int status = 0;

	if (devtype == 0xdeadc0de) {
		filename = PS_ASIC_FILE;
		status = 1;
	} else {
		filename = PS_FPGA_FILE;
		status = 0;
	}

	sprintf(path, "%s%x/%s", FW_PATH, rom_version, filename);

	return status;
}

static int get_patch_file_name(int dev_type, int rom_version,
			       int build_version, char *path)
{

	if ((dev_type != 0) &&
	    (dev_type != 0xdeadc0de) &&
	    (rom_version == 0x99999999) &&
	    (build_version == 1))
		path[0] = '\0';
	 else
		sprintf(path, "%s%x/%s", FW_PATH, rom_version, PATCH_FILE);

	return 0;
}
static int get_ar3k_crc(int dev, int tag_count, int patch_count)
{
	uint8_t hci_cmd[7];
	uint8_t *event;
	uint8_t *buffer_to_free = NULL;
	int retval = 1;
	int crc = 0;


	if (patch_count > 0)
		crc |= RAM_PATCH_REGION;
	if (tag_count > 0)
		crc |= RAM_PS_REGION;

	load_hci_header(hci_cmd, PS_VERIFY_CRC, 0, crc);

	if (send_hci_cmd_wait_event(dev, hci_cmd,
				    sizeof(hci_cmd), &event,
				    &buffer_to_free) == 0) {
		if (read_ps_event(event) == 0)
			retval = -1;

		if (buffer_to_free != NULL)
			free(buffer_to_free);
	}

	return retval;
}
static int get_device_type(int dev, uint32_t *code)
{
	uint8_t hci_cmd[] = {
		0x05, 0xfc, 0x05, 0x00, 0x00, 0x00, 0x00, 0x04
	};
	uint8_t *event;
	uint8_t *buffer_to_free = NULL;
	uint32_t reg;

	int result = -1;
	*code = 0;

	hci_cmd[3] = (uint8_t) (DEV_REGISTER & 0xFF);
	hci_cmd[4] = (uint8_t) ((DEV_REGISTER >> 8) & 0xFF);
	hci_cmd[5] = (uint8_t) ((DEV_REGISTER >> 16) & 0xFF);
	hci_cmd[6] = (uint8_t) ((DEV_REGISTER >> 24) & 0xFF);

	if (send_hci_cmd_wait_event(dev, hci_cmd,
				    sizeof(hci_cmd), &event,
				    &buffer_to_free) == 0) {
		if (event[5] == 0xFC && event[6] == 0x00) {

			switch (event[4]) {
			case 0x05:
				reg = event[10];
				reg = ((reg << 8) | event[9]);
				reg = ((reg << 8) | event[8]);
				reg = ((reg << 8) | event[7]);
				*code = reg;
				result = 0;

				break;

			case 0x06:
				break;
			}
		}
	}

	if (buffer_to_free != NULL)
		free(buffer_to_free);

	return result;
}

static int read_ar3k_version(int pConfig, int *rom_version, int *build_version)
{
	uint8_t hci_cmd[] = {0x1E, 0xfc, 0x00};
	uint8_t *event;
	uint8_t *buffer_to_free = NULL;
	int result = -1;

	if (send_hci_cmd_wait_event(pConfig,
				    hci_cmd,
				    sizeof(hci_cmd),
				    &event,
				    &buffer_to_free) == 0) {
		if (event[5] == 0xFC && event[6] == 0x00 && event[4] == 0x1E) {
			(*rom_version) = event[10];
			(*rom_version) = (((*rom_version) << 8) | event[9]);
			(*rom_version) = (((*rom_version) << 8) | event[8]);
			(*rom_version) = (((*rom_version) << 8) | event[7]);

			(*build_version) = event[14];
			(*build_version) = (((*build_version) << 8) |
					   event[13]);
			(*build_version) = (((*build_version) << 8) |
					   event[12]);
			(*build_version) = (((*build_version) << 8) |
					   event[11]);

			result = 1;

		}
		if (buffer_to_free != NULL)
			free(buffer_to_free);
	}



	return result;
}

static int str2bdaddr(char *str_bdaddr, char *bdaddr)
{
	char bdbyte[3];
	char *str_byte = str_bdaddr;
	int i, j;
	unsigned char colon_present = 0;

	if (strstr(str_bdaddr, ":") != NULL)
		colon_present = 1;

	bdbyte[2] = '\0';

	for (i = 0, j = 5; i < 6; i++, j--) {

		bdbyte[0] = str_byte[0];
		bdbyte[1] = str_byte[1];
		bdaddr[j] = strtol(bdbyte, NULL, 16);

		if (colon_present == 1)
			str_byte += 3;
		else
			str_byte += 2;
	}
	return 0;
}

static int write_bdaddr(int pConfig, char *bdaddr)
{
	uint8_t bdaddr_cmd[] = { 0x0B, 0xFC, 0x0A, 0x01, 0x01,
		0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	uint8_t *event;
	uint8_t *buffer_to_free = NULL;
	int result = -1;

	str2bdaddr(bdaddr, (char *)&bdaddr_cmd[7]);

	if (send_hci_cmd_wait_event(pConfig,
				    bdaddr_cmd,
				    sizeof(bdaddr_cmd),
				    &event,
				    &buffer_to_free) == 0) {

		if (event[5] == 0xFC && event[6] == 0x00) {
			if (event[4] == 0x0B)
				result = 0;
		}

	} else
		perror("Write failed\n");

	if (buffer_to_free != NULL)
		free(buffer_to_free);

	return result;
}

int ath_ps_download(int hdev)
{
	int i;
	int status = 0;
	int tag_count = 0;
	int patch_count = 0;
	int total_tag_len = 0;
	int rom_version = 0, build_version = 0;

	struct ps_cmd_packet *hci_cmd_list;	/* List storing the commands */
	uint32_t num_cmds;
	uint8_t *event;
	uint8_t *buffer_to_free;
	uint32_t dev_type;

	char patch_file_name[PATH_MAX];
	char ps_file_name[PATH_MAX];
	char bdaddr_file_name[PATH_MAX];

	FILE *stream;
	char bdaddr[21];

	hci_cmd_list = NULL;

	ATH_INFO("");

	do {
		/*
		 * Verfiy firmware version. depending on it select the PS
		 * config file to download.
		 */

		if (get_device_type(hdev, &dev_type) == -1) {
			status = -1;
			break;
		}
		ATH_INFO("get_device_type");

		if (read_ar3k_version(hdev,
				      &rom_version,
				      &build_version) == -1) {
			status = -1;
			break;
		}
		ATH_INFO("read_ar3k_version");

		ATH_INFO("get_ps_file_name");
		get_ps_file_name(dev_type, rom_version, ps_file_name);

		ATH_INFO("get_patch_file_name");
		get_patch_file_name(dev_type, rom_version, build_version,
				    patch_file_name);

		/* Read the PS file to a dynamically allocated buffer */
		stream = fopen(ps_file_name, "r");
		if (stream == NULL) {
			perror("firmware file open error\n");
			status = -1;
			break;
		}
		ATH_INFO("fopen(ps_file_name)");

		tag_count = ath_parse_ps(stream, &total_tag_len);

		fclose(stream);

		if (tag_count == -1) {
			status = -1;
			break;
		}
		ATH_INFO("ath_parse_ps");

		/*
		 * It is not necessary that Patch file be available,
		 * continue with PS Operations if.
		 * failed.
		 */
		if (patch_file_name[0] == '\0')
			status = 0;

		stream = fopen(patch_file_name, "r");
		if (stream  == NULL)
			status = 0;
		else {
			/* parse and store the Patch file contents to
			 * a global variables
			 */
			ATH_INFO("fopen(patch_file_name)");

			patch_count = parse_patch_file(stream);

			fclose(stream);

			if (patch_count < 0) {
				status = -1;
				break;
			}
			ATH_INFO("parse_patch_file");
		}

		/*
		 * Send the CRC packet,
		 * Continue with the PS operations
		 * only if the CRC check failed
		 */
		if (get_ar3k_crc(hdev, tag_count, patch_count) < 0) {
			status = 0;
			break;
		}
		ATH_INFO("get_ar3k_crc");

		/* Create an HCI command list
		 * from the parsed PS and patch information
		 */
		ATH_INFO("ath_create_cmd_list");
		ath_create_cmd_list(&hci_cmd_list,
				    &num_cmds,
				    tag_count,
				    patch_count,
				    total_tag_len);

		for (i = 0; i < num_cmds; i++) {

			if (send_hci_cmd_wait_event
			    (hdev, hci_cmd_list[i].Hcipacket,
			     hci_cmd_list[i].packetLen, &event,
			     &buffer_to_free) == 0) {

				if (read_ps_event(event) < 0) {

					/* Exit if the status is not success */
					if (buffer_to_free != NULL)
						free(buffer_to_free);

					status = -1;
					break;
				}
				if (buffer_to_free != NULL)
					free(buffer_to_free);
			} else {
				status = 0;
				break;
			}
		}

		/* Read the PS file to a dynamically allocated buffer */
		sprintf(bdaddr_file_name, "%s%x/%s",
			FW_PATH,
			rom_version,
			BDADDR_FILE);

		stream = fopen(bdaddr_file_name, "r");

		if (stream == NULL) {
			status = 0;
			break;
		}
		ATH_INFO("fopen(bdaddr_file_name)");

		if (fgets(bdaddr, 20, stream) != NULL)
			status = write_bdaddr(hdev, bdaddr);

		ATH_INFO("write_bdaddr");

		fclose(stream);

	} while (FALSE);

	if (hci_cmd_list != NULL)
		ath_free_command_list(&hci_cmd_list, num_cmds);

	return status;
}
