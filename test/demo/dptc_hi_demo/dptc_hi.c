/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file test/demo/dptc_hi_demo/dptc_hi.c
 *
 * @brief Human Interface program for the Freescale Semiconductor MXC DPTC module.
 *
 * The DPTC driver Human Interface program \n
 * This program communicates with the DPTC (Dynamic Process Temperature
 * Compensation) daemon and allows the user to change parameters and control
 * the DPTC module. \n
 * This program allows the user to: \n
 *	1) Enable/Disable the DPTC module. \n
 *	2) Read the DPTC translation table loaded into the DPTC driver. \n
 *      3) Update the DPTC translation table in the DPTC driver. \n
 *	4) Change the frequency index used by the DPTC driver. \n
 *	5) Read the DPTC log buffer. \n
 *
 * @ingroup DPTC
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <unistd.h>
#include <termios.h>

#include <asm-arm/arch-mxc/pm_api.h>

/*!
 * number of log buffer entries to be displayed.
 */
#define LOG_NUM_OF_ROWS	10

/*!
 * This structure holds the DPTC log buffer entries read from the DPTC driver
 * after formatting them into text lines.
 */
typedef struct
{
	/*!
	 * Points to the position in the buffer the next formatted log
	 * entry will be written to.
	 */
	unsigned int	head;

	/*!
	 * A table that holds the DPTC log buffer entries read from the
	 * DPTC driver after formatting them to text lines.
	 */
	char		table[LOG_NUM_OF_ROWS][80];
} log_table_s;

/*!
 * Data structure that holds the DPTC log entries after formatting them into
 * text lines.
 */
log_table_s	log_table;

/*!
 * This function initializes the log table structure.
 */
void init_log_table(void)
{
	int i;

	/* Reset the head index to point to the beginning of the table */
	log_table.head = 0;

	/* Fill the log table entries text lines with blank values */
	for(i=0; i<LOG_NUM_OF_ROWS; i++) {
		sprintf(log_table.table[i], "    -----        -----\n");
	}
}

/*!
 * This function reads a single character from the terminal without echoing it
 * to the terminal.
 *
 * @return    the character read from the terminal.
 *
 */
char getch(void)
{
	int rv;
	char ch;
	/* force to stdin */
	int fd = 0;
	struct termios oldflags, newflags;

	tcdrain(fd);

	/*
	 * Reset the terminal to accept unbuffered input
	 */

	/*
	 * Get the current oldflags
	 */
	tcgetattr(fd, &oldflags);
	/* Make a copy of the flags so we can easily restore them */
	newflags = oldflags;
	/* Set raw input */
	newflags.c_cflag |= (CLOCAL | CREAD);
	newflags.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);


	/* Set the newflags */
	tcsetattr(fd, TCSANOW, &newflags);
	rv = read(fd, &ch, 1);

	/*
	 * Restore the oldflags -- This is important otherwise
	 * your terminal will no longer echo characters
	 */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &oldflags);
	return ch;
}

/*!
 * This function reads a single character from the terminal and echoes it
 * to the terminal.
 *
 * @return    the character read from the terminal.
 *
 */
char getche(void)
{
	int rv;
	char ch;
	int fd = 0; /* force to stdin */
	struct termios oldflags, newflags;

	tcdrain(fd);
	/*
	 * Reset the terminal to accept unbuffered input
	 */

	/*
	 * Get the current oldflags
	 */
	tcgetattr(fd, &oldflags);

	/* Make a copy of the flags so we can easily restore them */
	newflags = oldflags;

	/* Set raw input */
	newflags.c_cflag |= (CLOCAL | CREAD);
	newflags.c_lflag &= ~(ICANON | ECHOE | ISIG);


	/* Set the newflags */
	tcsetattr(fd, TCSANOW, &newflags);

	/* Read character from terminal */
	rv = read(fd, &ch, 1);

	/* Flush received characters */
	tcflush(fd, TCIFLUSH);

	/*
	 * Restore the oldflags
	 */
	tcsetattr(fd, TCSANOW, &oldflags);
	return ch;
}

/*!
 * This function check if a key was pressed on the terminal keyboard.
 *
 * @return    1 if a key was pressed else returns 0.
 *
 */
int kbhit(void)
{
	unsigned char ch;
	int nread;
	struct termios oldflags, newflags;

	tcgetattr(0, &oldflags);
	/* Make a copy of the flags so we can easily restore them */
	newflags = oldflags;

	/* Set raw input */
	newflags.c_cflag |= (CLOCAL | CREAD);
	newflags.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* Set mimimal characters read to 0 */
	newflags.c_cc[VMIN]=0;
	newflags.c_cc[VTIME] = 0;

	/* Set the newflags */
	tcsetattr(0, TCSANOW, &newflags);

	nread = read(0,&ch,1);

	/* Restore the oldflags */
	tcsetattr(0, TCSANOW, &oldflags);
	if (nread == 1)
		return 1;

	return 0;
}

/*!
 * This function sends an DPTC module enable command to the DPTC daemon and
 * waits for an ack message.
 *
 * @param     sock	the communication socket file descriptor number.
 *
 */
void enable_dptc(int dptc_fh)
{
        if(ioctl(dptc_fh,DPTC_IOCTENABLE) < 0)
		printf("command failed\n");
}

/*!
 * This function sends an DPTC module Disable command to the DPTC daemon and
 * waits for an ack message.
 *
 * @param     sock	the communication socket file descriptor number.
 *
 */
void disable_dptc(int dptc_fh)
{
        if(ioctl(dptc_fh,DPTC_IOCTDISABLE) < 0)
		printf("command failed\n");
}

/*!
 * This function checks the state of the DPTC module (Enabled/Disabled)
 * The function sends a GET_STATE command to the DPTC daemon and waits for an
 * ack message that contains the DPCT module state in the message parameter.
 *
 * @param     sock	the communication socket file descriptor number.
 *
 * @return    1 if the DPTC module is enabled, 0 if the DPTC is disabled and
 *	      -1 if an error occured while receiving the ack message.
 *
 */
int dptc_is_active(int dptc_fh)
{
        return ioctl(dptc_fh,DPTC_IOCGSTATE);
}

/*!
 * This function checks the value of the frequency index used by
 * the DPTC module.\n
 * The function sends a GET_FREQ command to the DPTC daemon and waits for an
 * ack message that contains the frequency index used in the message parameter.
 *
 * @param     sock	the communication socket file descriptor number.
 *
 * @return    the value of the frequency index used. returns -1 if an error
 *	      occured while receiving the ack message.
 *
 */
/*
int get_current_freq_index(int dptc_fh)
{
        return ioctl(dptc_fh,DPTC_IOCGCURRFREQ);
}
*/
/*!
 * This function sets the value of the frequency index used by
 * the DPTC module.\n
 * The function sends a SET_FREQ command to the DPTC daemon and waits
 * for an ack message. if an ack message is not received the function
 * prints an error message.
 *
 * @param     sock	   the communication socket file descriptor number.
 * @param     freq_index   new frequency index value to be set.
 *
 * @return    the value of the frequency index used. returns -1 if an error
 *	      occured while receiving the ack message.
 *
 */
/*
void set_freq_index(unsigned int freq_index,int dptc_fh)
{
	if (ioctl(dptc_fh,DPTC_IOCSCURRFREQ,freq_index) < 0)
		printf("command failed\n");
}
*/

void set_ref_circuits(unsigned int rc_status,int dptc_fh)
{
	if ((ioctl(dptc_fh,DPTC_IOCSENABLERC,(unsigned char)rc_status) < 0) ||
            (ioctl(dptc_fh,DPTC_IOCSDISABLERC,~(unsigned char)rc_status) < 0))
                printf("command failed\n");
}

void set_wp(int wp,int dptc_fh)
{
	if (ioctl(dptc_fh,DPTC_IOCSWP,wp) < 0)
                printf("command failed\n");
}

/*!
 * This function updates the DPTC translation table used by the DPTC driver.\n
 * The function sends a WRITE_TABLE command to the DPTC daemon and after that
 * a message that contains the new DPTC translation table.\n
 * The function waits for an ack message to be received from the DPTC daemon.
 *
 * @param     sock	the communication socket file descriptor number.
 * @param     buffer	pointer to the new DPTC translation table.
 * @param     size	size of new DPTC translation table.
 *
 * @return    TRUE if ack message was received from the DPTC daemon else
 *	      returns FALSE.
 *
 */
int send_table(int dptc_fh, void *buffer, int size)
{
        return ioctl(dptc_fh,PM_IOCSTABLE,buffer);
}

/*!
 * This function reads a new DPTC translation table from a file and
 * updates the DPTC driver to the new table through the DPTC daemon.\n
 * The function gets from the user the file name of the new translation table
 * opens and reads it and updates the driver by using the send_table function.
 *
 * @param     sock   the communication socket file descriptor number.
 *
 * @return    0 if update succeeded, else returns -1.
 *
 */
int write_table(int dptc_fh)
{
	FILE *table_file;
	char file_name[80];
	char in_buffer[4096];
	int count;
        char line[256];
	int ret_val = 0;

        count =0;

        memset(in_buffer,0,4096);

	/* Get table file name from user */
	printf("\nInput file name: ");
	scanf("%80s",file_name);

	/*
	 * Open table file for reading.
	 * If error occured while opening file print error message and return -1
	 */
	if ((table_file = fopen(file_name, "r")) < 0) {
		printf("Error opening file %s\n", file_name);
		return -1;
	}

        while (! feof(table_file)){
                fgets(line,256,table_file);

                count += strlen(line);

                if(count > 4096){
                        free(in_buffer);
                        ret_val = -1;
                        break;
                }

                strcat(in_buffer,line);
        }

        if (ret_val < 0){
                printf("Failed reading table file\n");
        }
        else if (send_table(dptc_fh, (void*)in_buffer, count) < 0) {
                /*
                 * Error updating the driver DPTC table
                 * (ack not received from the DPTC daemon)
                 */
                printf("Error updating DPTC table\n");
                ret_val = -1;
        }

	/* Close table file */
	fclose(table_file);

	return ret_val;
}

/*!
 * This function reads the DPTC translation table used by the DPTC driver via
 * the DPTC daemon and writes it to an output file.\n
 * The function sends a READ_TABLE command to the DPTC daemon and waits
 * for the returned data containing the DPTC taranslation table currently used
 * by the DPTC driver.\n
 *
 * @param     sock	the communication socket file descriptor number.
 *
 * @return    0 if table read succeeded, else returns -1.
 *
 */
int read_table(int dptc_fh)
{
	int table_file;
	char file_name[80];
	void *in_buffer;
	int ret_val = 0;

	/* Get output file name from user */
	printf("\nOutput file name: ");
	scanf("%80s",file_name);

        in_buffer = malloc(4096);

        /* Check if memory was allocated */
        if (in_buffer) {

                memset(in_buffer,0,4096);

                /* Wait and read message from daemon containing table data */
                if (ioctl(dptc_fh,PM_IOCGTABLE,in_buffer) >= 0) {

                        /* Open output file for writing */
                        if ((table_file = open(file_name, O_WRONLY | O_CREAT)) < 0) {
                                /* Unable to open output file */
                                printf("Error opening file %s\n", file_name);
                                ret_val = -1;
                        } else {
                                /* Write table data to output file */
                                write(table_file, in_buffer, strlen(in_buffer)+1);

                                /* Close output file */
                                close(table_file);
                        }
                } else {
                        /* Error receiving table data */
                        printf("Error receiving message\n");
                        ret_val = -1;
                }

                /* Free allocated memory */
                free(in_buffer);
        } else {
                /*
                 * Unable to allocate memory for table
                 * Print error message, close input file and return -1
                 */
                printf("Error allocating memory\n");
                ret_val = -1;
        }

	return ret_val;
}

/*!
 * This function reads a specified amount of log buffer entries from the DPTC
 * driver. The log is read via the DPTC daemon.
 *
 * @param     sock		the communication socket file descriptor number.
 * @param     num_of_entries	number of log entries to read.
 * @param     log_entries	pointer to the buffer where the log entries
 *				should be placed.
 *
 * @return    if read succeeded returns the number of bytes written to the
 *	      log_entries buffer, else returns -1.
 *
 */
int get_log_entries(int dptc_proc_fh, int num_of_entries, dptc_log_entry_s *log_entries)
{
        int read_size;

        read_size = num_of_entries * sizeof(dptc_log_entry_s);

	/* Wait for ack message from the daemon */
	if ((read_size = read(dptc_proc_fh, log_entries, read_size)) >= 0) {
                return read_size;
	} else {
		/* Ack message not received -> error */
		printf("Error reading data from dptc proc file\n");
		return(-1);
	}
}

/*!
 * This function displays the last LOG_NUM_OF_ROWS DPTC log entries as a
 * formatted text table.
 *
 * @param     sock		the communication socket file descriptor number.
 *
 */
void show_dptc_log(int dptc_proc_fh)
{
	unsigned int i,cur_index;
	dptc_log_entry_s log_entry;

	/* Display table until a key is hit in the terminal keyboard */
	while (!kbhit()) {
		/* Clear screen */
		system("clear");
		/*
		 * Read all the log entries from the DPTC driver.
		 * Entries are read one by one, formatted as a text line and added
		 * to the buffer to be displayed.
		 */
		while (get_log_entries(dptc_proc_fh, 1, &log_entry) > 0) {
			sprintf(log_table.table[log_table.head]," %8ld       %3d\n",log_entry.jiffies,log_entry.wp);
			log_table.head = (log_table.head + 1) % LOG_NUM_OF_ROWS;
		}

		/* Print log entries as a formatted table */
		printf("   Jiffies     WP index\n");
		for (i=1; i<=LOG_NUM_OF_ROWS; i++) {
			cur_index = (log_table.head - i + LOG_NUM_OF_ROWS) % LOG_NUM_OF_ROWS;
			printf("%s", log_table.table[cur_index]);
		}

		/* Sleep 5 seconds */
		sleep(5);
	}
}

void send_quit(int dptc_fh,int dptc_proc_fh)
{
        close(dptc_proc_fh);
        close(dptc_fh);
        exit(0);
}

/*!
 * This function receives the command entered by the user and processes it.
 *
 * @param     cmd	the command requested by the user.
 * @param     sock	the communication socket file descriptor number.
 *
 */
void process_cmd(char cmd,int dptc_fh,int dptc_proc_fh)
{
	unsigned int wp;
        unsigned int rc_status;

	switch (cmd) {
	/* Enable DPTC module command */
	case '1':
		enable_dptc(dptc_fh);
		break;
	/* Disable DPTC module command */
	case '2':
		disable_dptc(dptc_fh);
		break;
	/* Update DPTC driver translation table command */
	case '3':
		write_table(dptc_fh);
		break;
	/* Read DPTC driver translation table command */
	case '4':
		read_table(dptc_fh);
		break;
	/* Set reference circuits status */
	case '5':
                printf("\nRefercence circuits status: ");
		scanf("%x",&rc_status);
		set_ref_circuits(rc_status,dptc_fh);
		break;
	/* Show DPTC driver log buffer */
	case '6':
		show_dptc_log(dptc_proc_fh);
		break;
        /* Set working point */
        case '7':
                printf("\nNew working point: ");
		scanf("%d",&wp);
		set_wp(wp,dptc_fh);
		break;
	/* Quit */
	case '8':
		send_quit(dptc_fh,dptc_proc_fh);
		break;
	/* Invalid instruction */
	default:
		printf("Invalid instruction\n");
		break;
	}
}

/*!
 * This function performs the program human interface.
 */
void human_interface(int dptc_fh,int dptc_proc_fh)
{
	char cmd = 0;

	/* Continue until quit command is received */
	while (1) {
		/* Display DPTC driver status and available commands */
		system("clear");
		printf("DPTC module status:\n");
		printf("Enabled - %d\n\n",dptc_is_active(dptc_fh));

		printf("DPTC driver commands\n");
		printf("1) Enable DPTC\n");
		printf("2) Disable DPTC\n");
		printf("3) Update DPTC driver traslation table\n");
		printf("4) Read DPTC driver traslation table\n");
		//printf("5) Set new frequency index\n");
                printf("5) Set DPTC reference circuits\n");
		printf("6) Show DPTC log buffer\n");
                printf("7) Set DPTC working point\n");
		printf("8) Quit\n");
		printf("Choose command: \n");

		/* Get command from the user */
		cmd = getche();

		/* Process the requested command */
		process_cmd(cmd,dptc_fh,dptc_proc_fh);

		/* Wait until a key is pressed */
		printf("\npress any key to continue\n");
		while(!kbhit());
	}

}


/*!
 * This the main function of the porogram.
 *
 * @param     argc	number of arguments sent to the program.
 * @param     argv	array of strings containing the program arguments.
 *
 * @return    0 if program exited with no error. else returns -1.
 *
 */
int main(int argc, char *argv[])
{

        int dptc_fh,dptc_proc_fh;

	printf("DPTC Daemon Human Interface Program Ver 2.0\n");

#ifdef DPTC_KKP_DEBUG
        if (open("/dev/mxckpd",O_RDONLY) < 0) {
                printf("Error openning //dev//mxckpd");
                return -1;
        }
#endif

        if((dptc_fh = open("/dev/dptc",O_RDWR))<0){
                printf("Failed open /dev/dptc\n");
                return -1;
        }


        if((dptc_proc_fh = open("/proc/dptc",O_RDONLY))<0){
                 printf("Failed open /proc/dptc\n");
                return -1;
        }

        init_log_table();

        /* Start human interface */
        human_interface(dptc_fh,dptc_proc_fh);

	return 0;
}

