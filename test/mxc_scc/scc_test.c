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

/* +FHDR-----------------------------------------------------------------------
 * FILE NAME      : scc_test.c
 * DEPARTMENT     : Security Technology Center (STC), NCSG
 * AUTHOR         : Ron Harvey (r66892)
 * ----------------------------------------------------------------------------
 * REVIEW(S) :
 * ----------------------------------------------------------------------------
 * RELEASE HISTORY
 * VERSION DATE       AUTHOR       DESCRIPTION
 * 0.0.1   2004-12-02 R. Harvey    Initial version
 * 0.0.8   2005-03-08 R. Harvey    Implementation complete
 * ----------------------------------------------------------------------------
 * KEYWORDS : SCC, SMN, SCM, Security, Linux driver
 * ----------------------------------------------------------------------------
 * PURPOSE: Provide a program to test the SCC (Security Controller) driver
 * ----------------------------------------------------------------------------
 * REUSE ISSUES
 * Version 2 of the SCC block will look a lot different
 * -FHDR-----------------------------------------------------------------------
 */

/**
 * @file scc_test.c
 * @brief This is a test program for the SCC driver.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdarg.h>

#include <inttypes.h>
#include "scc_test.h"


/* test routines */
void display_configuration(int device);
int check_safe_registers(int device);
int dump_registers(int device);
int run_aic_tests(int scc_fd);
int run_cipher_tests(int device);
void run_plaintext_cipher_text_tests(int device);
void run_timer_tests(int device, uint32_t);
void run_zeroize_tests(int device);
void set_software_alarm(int device);
void run_mmap_tests(int device);
int run_wrap_tests(int device);

/* utility print functions */
void print_ram_data (uint32_t *ram, uint32_t address, int count);
void print_smn_status_register(uint32_t);
void print_scm_control_register(uint32_t);
void print_scm_status_register(uint32_t);
void print_scc_error_status_register(uint32_t);
char *get_smn_state_name(const uint8_t);
void print_smn_timer_control_register(const uint32_t);
void print_smn_command_register(const uint32_t);
void print_scc_debug_detector_register(uint32_t);
void print_scc_return_code(const scc_return_t);
void dump_cipher_control_block(scc_encrypt_decrypt*);
int do_slot_function(int device, char* arg);

/* utility register access functions */
int write_scc_register(int device, uint32_t address, uint32_t value);
int read_scc_register(int device, uint32_t address, uint32_t *value);


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

int debug = FALSE;


/** Secret which is going to be encrypted and decrtyped.  See also
 *  #init_plaintext */
uint8_t plaintext[4096] = {0xca, 0xbb, 0xad, 0xba,
                           0xde, 0xad, 0xbe, 0xef,
                           0xdb, 0xea, 0x11, 0xbe,
                           'A', 'B', 'C', 'D',
                           0x73, 0x1c, 0xab, 0xad,
                           0xab, 0xad, 0xac, 0x24
};


/** Set up full */
void
init_plaintext()
{
    int i;

    /* Starting after precompiled values, fill up the rest */
    for (i = 24; i < sizeof(plaintext); i++) {
        plaintext[i] = i%256;
    }
}


/** Normal number of bytes to encrypt */
#define DEFAULT_PLAINTEXT_LENGTH 24


/** */
int plaintext_length = DEFAULT_PLAINTEXT_LENGTH;


/** */
int byte_offset = 0;


/** Normal number of bytes of padding to add to ciphertext storage */
#define DEFAULT_PADDING_ALLOWANCE 10

int encrypt_padding_allowance = DEFAULT_PADDING_ALLOWANCE;

int decrypt_padding_allowance = 0;

/** */
scc_verify_t check_mode = SCC_VERIFY_MODE_NONE;


/** */
scc_crypto_mode_t crypto_mode = SCC_ECB_MODE;

/** */
int encrypt_only = FALSE;

/** */
int inject_crc_error = FALSE;

/** */
int superbrief_read_register = FALSE;

/* miscellaneous utility functions */
scc_configuration_access *get_scc_configuration(int);


/*****************************************************************************
 *  main
 ****************************************************************************/
int
main(int arg_count,             /* # command-line arguments */
     char*arg_list[])            /* pointers to command-line arguments */
{ 
    /* Declare and initialize variables */
    int scc_fd;                 /* The SCC device */
    char *scc_device_path = "/dev/scc_test";
    int argument_switch;
    int test_switch;
    uint32_t timer_value = 0x5f0000;
    char *test_to_run = "Cres"; /* default list of tests to be run in order */
    int test_status = 0;

#if 0
    int one = 1;                /* little-endian test vector */
#endif
    
    init_plaintext();

#if 0
    if (*(char *)&one == 1) {
        printf("This CPU is little-endian\n");
    }
    else {
        printf("This CPU is big-endian\n");
    }
#endif

    /* Open up the SCC device */
    /* Yes, I know, there is a command option to change the device path. */
    scc_fd = open(scc_device_path, O_RDWR);
    if (scc_fd < 0) {
        perror("Cannot open SCC device");
        exit (1);
    }

    /* Process command line arguments - until we come up empty */
    while ( (argument_switch = getopt(arg_count, arg_list, "K:L:MP:R:S:T:W:"))
            != EOF ) {
        switch (argument_switch) {
        case 'K':               /* Key slot functions */
            test_status |= do_slot_function(scc_fd, optarg);
            test_to_run = "";
            break;
        case 'L':               /* List tests to run */
            test_to_run = optarg;
            break;
        case 'P':               /* set Path of SCC device */
            scc_device_path = optarg;
            break;
        case 'R':               /* Read a specific register */
            {
                unsigned int offset;
                uint32_t value;
                int err;

                /* get the offset */
                if (sscanf(optarg, "%x", &offset) != 1) {
                    fprintf(stderr, "improper use of -R %s\n", optarg);
                }
                else {
                    err = read_scc_register(scc_fd, offset, &value);
                    if (!err) {
                        if (superbrief_read_register) {
                            printf("%08x\n", value);
                        }
                        else {
                            printf("0x%05x: %08x\n", offset, value);
                        }
                    }
                    else {
                        test_status = 1;
                        perror("Reading register: ");
                    }
                }
            }
            test_to_run = "";
            break;
        case 'M':               /* test mmap() of SCC registers */
            run_mmap_tests(scc_fd);
            test_to_run = "";
            break;
        case 'S':               /* (re)set a switch */
            switch (*(optarg+1)) {
            case 'c':
                if (*optarg == '+') {
                    if (strlen(optarg) > 2) {
                        if (sscanf(optarg+2, "%x", &inject_crc_error) != 1) {
                            fprintf(stderr, "improper hex value with -S+c\n");
                            inject_crc_error = 0x1959;
                        }
                    }
                    else {      /* arbitrary change */
                        inject_crc_error = 0x1959;
                    }
                }
                else {
                    inject_crc_error = 0;
                }
                break;
            case 'D':
                if (*optarg == '+') {
                    debug = TRUE;
                }
                else {
                    debug = FALSE;
                }
                break;
            case 'e':
                if (*optarg == '+') {
                    encrypt_only = TRUE;
                }
                else {
                    encrypt_only = FALSE;
                }
                break;
            case 'l':
                if (*optarg == '+') {
                    plaintext_length = atoi(optarg+2);
                    if (plaintext_length > sizeof(plaintext)) {
                        plaintext_length = sizeof(plaintext);
                        fprintf(stderr,
                                "plaintext size too large. Using %ld\n",
                                (long)plaintext_length);
                    }
                }
                else {
                    plaintext_length = DEFAULT_PLAINTEXT_LENGTH;
                }
                break;
            case 'm':           /* crypto mode */
                if (*optarg == '+') {
                    crypto_mode = SCC_CBC_MODE;
                }
                else {
                    crypto_mode = SCC_ECB_MODE;
                }
                break;
            case 'o':           /* byte offset */
                if (*optarg == '+') {
                    byte_offset = atoi(optarg+2);
                }
                else {
                    byte_offset = 0;
                }
                break;
            case 'p':           /* ciphertext padding allowance */
                if (*optarg == '+') {
                    encrypt_padding_allowance = atoi(optarg+2);
                }
                else {
                    decrypt_padding_allowance = atoi(optarg+2);
                }
                break;
            case 'Q':           /* superbrief read register (quiet) */
                if (*optarg == '+') {
                    superbrief_read_register = TRUE;
                }
                else {
                    superbrief_read_register = FALSE;
                }
                break;
            case 'v':           /* verification */
                if (*optarg == '+') {
                    check_mode = SCC_VERIFY_MODE_CCITT_CRC;
                }
                else {
                    check_mode = SCC_VERIFY_MODE_NONE;
                }
                break;
            default:
                fprintf(stderr, "unknown switch %c\n", *(optarg+1));
            }
            break;
        case 'T':               /* change Timer Initial Value */
            timer_value = 44;   /* code this up! */
            break;
        case 'W':               /* write a specific register */
            {
                unsigned int offset;
                unsigned int value;
                int err;

                if (sscanf(optarg, "%x:%x", &offset, &value) != 2) {
                    fprintf(stderr, "improper use of -W (%s)\n", optarg);
                }
                else {
                    err = write_scc_register(scc_fd, offset, value);
                    if (!err) {
                        err = read_scc_register(scc_fd, offset, &value);
                        if (!err) {
                            printf("0x%05x => %08x\n", offset, value);
                        }
                        else {
                            perror("Reading register after write: ");
                        }
                    }
                    else {
                        test_status = 1;
                        perror("Writing register: ");
                    }
                }
            }
            test_to_run = "";
            break;
        default:
            fprintf(stderr, "Usage: scc_test [-P sccpath] [-T timerIV]"
                    " [-Roff] [Woff:hex_value]\n"
                    " [-Kaction:arguments] where action can be\n"
                    "    a(allocate slot) with args length:hex_owner\n"
                    "    d(deallocate slot) with args slot:hex_owner\n"
                    "    l(load key value) with args slot:hex_owner:color:"
                    "hex_value, (color is r/b)\n"
                    "    u(unload key value) with args slot:hex_owner\n"
                    " [-L tests] where tests can be a(alarm)\n"
                    "    C(config)\n"
                    "    e(encrypt/decrypt)\n"
                    "    r(print Registers)\n"
                    "    s(print Safe registers)\n"
                    "    w(key-wrapping tests)\n"
                    "    z(zeroize memories)\n"
                    " [-Sopt] where options can be\n"
                    "    c(corrupt ciphertext before decrypting)\n"
                    "    D(print debug messages)\n"
                    "    l(set Length of plaintext for cipher test)\n"
                    "    m(+set/-reset CBC mode)\n"
                    "    o(change byte offset of ciphertext&recovered"
                    " plaintext\n"
                    "    p(change padding allowance of ciphertext buffer\n"
                    "    v(turn +on/-off cipher verification mode\n");
            exit(1);
            break;
        }
    }


    /* loop through all of the requested tests, running each in turn */
    while ((test_switch = *test_to_run++) != 0) {
        printf("\n");

        switch (test_switch) {
        case 'a':
            set_software_alarm(scc_fd);
            break;
        case 'c':
            run_aic_tests(scc_fd);
            break;
        case 'C':
            display_configuration(scc_fd);
            break;
        case 'e':
            test_status |= run_cipher_tests(scc_fd);
            break;
        case 'p':
            run_plaintext_cipher_text_tests(scc_fd);
            break;
        case 'r':
            test_status |= dump_registers(scc_fd);
            break;
        case 's':               /* the always available ones */
            test_status |= check_safe_registers(scc_fd);
            break;
        case 't':
            run_timer_tests(scc_fd, timer_value);
            break;
        case 'w':
            test_status |= run_wrap_tests(scc_fd);
            break;
        case 'z':
            run_zeroize_tests(scc_fd);
            break;
        default:
            fprintf(stderr, "Test switch %c unknown\n", test_switch);
        }
        
    }

    close(scc_fd);

    exit(test_status != 0);

}  /* End of Main */


void
display_configuration(int scc_fd) {
    scc_configuration_access *config;
    
    config = get_scc_configuration(scc_fd);
    if (config == NULL) {
        perror("\nCannot display SCC Configuration");
    }
    else {
        printf("SCM version: %d, SMN version: %d, driver version %d.%d\n",
               config->scm_version, config->smn_version,
               config->driver_major_version, config->driver_minor_version);
        printf("SCM block size is %d bytes, Black has %d blocks, "
               "Red has %d blocks\n", config->block_size,
               config->black_ram_size, config->red_ram_size);
    }
}



/* Print values and verify access of all 'always available' registers */
int
check_safe_registers(int scc_fd) {
    int status;
    uint32_t value;
    int had_error = 0;

    status = read_scc_register(scc_fd, SCM_STATUS, &value);
    if (!status) {
        printf("(%04X) SCM Status                     (0x%08x): ",
               SCM_STATUS, value);
        print_scm_status_register(value);
    }
    else {
        printf("(Could not get SCM Status Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_ERROR_STATUS, &value);
    if (!status) {
        printf("(%04X) SCM Error Register             (0x%08x):",
               SCM_ERROR_STATUS, value);
        print_scc_error_status_register(value);
    }
    else {
        printf("(Could not get SCM Error Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_INTERRUPT_CTRL, &value);
    if (!status) {
        printf("(%04X) SCM Interrupt Control Register: 0x%08x\n",
               SCM_INTERRUPT_CTRL, value);
    }
    else {
        printf("(Could not get SCM Interrupt Control Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_CONFIGURATION, &value);
    if (!status) {
        printf("(%04X) SCM Configuration Register:     0x%08x\n",
               SCM_CONFIGURATION, value);
    }
    else {
        printf("(Could not get SCM Configuration Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_STATUS, &value);
    if (!status) {
        printf("(%04X) SMN Status Register            (0x%08x): ",
               SMN_STATUS, value);
        print_smn_status_register(value);
    }
    else {
        printf("(Could not get SMN Status Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_COMMAND, &value);
    if (!status) {
        printf("(%04X) SMN Command Register:          (0x%08x):",
               SMN_COMMAND, value);
        print_smn_command_register(value);
    }
    else {
        printf("(Could not get SMN Command Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_DEBUG_DETECT_STAT, &value);
    if (!status) {
        printf("(%04X) SMN Debug Detected Register:   (0x%08x):",
               SMN_DEBUG_DETECT_STAT, value);
        print_scc_debug_detector_register(value);
    }
    else {
        printf("(Could not get SMN Debug Detected Register)\n");
        had_error++;
    }

    return (had_error != 0);
}


int
dump_registers(int scc_fd)
{
    int status;
    uint32_t value;
    int had_error = 0;

    status = read_scc_register(scc_fd, SCM_RED_START, &value);
    if (!status)
        printf("(%04X) SCM Red Start Register:         0x%08x\n",
               SCM_RED_START, value);
    else {
        had_error++;
        printf("(Could not get SCM Red Start Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_BLACK_START, &value);
    if (!status)
        printf("(%04X) SCM Black Start Register:       0x%08x\n",
               SCM_BLACK_START, value);
    else {
        had_error++;
        printf("(Could not get SCM Black Start Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_LENGTH, &value);
    if (!status)
        printf("(%04X) SCM Length Register:            0x%08x\n",
               SCM_LENGTH, value);
    else {
        had_error++;
        printf("(Could not get SCM Length Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_CONTROL, &value);
    if (!status) {
        printf("(%04X) SCM Control Register           (0x%08x): ",
               SCM_CONTROL, value);
        print_scm_control_register(value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Control Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_STATUS, &value);
    if (!status) {
        printf("(%04X) SCM Status                     (0x%08x): ",
               SCM_STATUS, value);
        print_scm_status_register(value);
    }
    else {
        printf("(Could not get SCM Status Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_ERROR_STATUS, &value);
    if (!status) {
        printf("(%04X) SCM Error Register             (0x%08x):",
               SCM_ERROR_STATUS, value);
        print_scc_error_status_register(value);
    }
    else {
        printf("(Could not get SCM Error Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_INTERRUPT_CTRL, &value);
    if (!status) {
        printf("(%04X) SCM Interrupt Control Register: 0x%08x\n",
               SCM_INTERRUPT_CTRL, value);
    }
    else {
        printf("(Could not get SCM Interrupt Control Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_CONFIGURATION, &value);
    if (!status) {
        printf("(%04X) SCM Configuration Register:     0x%08x\n",
               SCM_CONFIGURATION, value);
    }
    else {
        printf("(Could not get SCM Configuration Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_INIT_VECTOR_0, &value);
    if (!status) {
        printf("(%04X) SCM Init Vector 0 Register:     0x%08x\n",
               SCM_INIT_VECTOR_0, value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Init Vector 0 Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_INIT_VECTOR_1, &value);
    if (!status) {
        printf("(%04X) SCM Init Vector 1 Register:     0x%08x\n",
               SCM_INIT_VECTOR_1, value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Init Vector 1 Register)\n");
    }

    printf("\n");               /* Visually divide the halves */

    status = read_scc_register(scc_fd, SMN_STATUS, &value);
    if (!status) {
        printf("(%04X) SMN Status Register            (0x%08x): ",
               SMN_STATUS, value);
        print_smn_status_register(value);
    }
    else {
        printf("(Could not get SMN Status Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_COMMAND, &value);
    if (!status) {
        printf("(%04X) SMN Command Register:          (0x%08x):",
               SMN_COMMAND, value);
        print_smn_command_register(value);
    }
    else {
        printf("(Could not get SMN Command Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_SEQUENCE_START, &value);
    if (!status) {
        printf("(%04X) SMN Sequence Start Register:    0x%08x\n",
               SMN_SEQUENCE_START, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Sequence Start Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_SEQUENCE_END, &value);
    if (!status) {
        printf("(%04X) SMN Sequence End Register:      0x%08x\n",
               SMN_SEQUENCE_END, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Sequence IV Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_SEQUENCE_CHECK, &value);
    if (!status) {
        printf("(%04X) SMN Sequence Check Register:    0x%08x\n",
               SMN_SEQUENCE_CHECK, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Sequence Check Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_BIT_COUNT, &value);
    if (!status) {
        printf("(%04X) SMN Bit Count Register:         0x%08x\n",
               SMN_BIT_COUNT, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Bit Count Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_BITBANK_INC_SIZE, &value);
    if (!status) {
        printf("(%04X) SMN Bit Bank Inc Size Register: 0x%08x\n",
               SMN_BITBANK_INC_SIZE, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Bit Bank Inc Size Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_COMPARE_SIZE, &value);
    if (!status) {
        printf("(%04X) SMN Compare  Size Register:     0x%08x\n",
               SMN_COMPARE_SIZE, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Compare Size Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_PLAINTEXT_CHECK, &value);
    if (!status) {
        printf("(%04X) SMN Plaintext  Check Register:  0x%08x\n",
               SMN_PLAINTEXT_CHECK, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Plaintext Check Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_CIPHERTEXT_CHECK, &value);
    if (!status) {
        printf("(%04X) SMN Ciphertext  Check Register: 0x%08x\n",
               SMN_CIPHERTEXT_CHECK, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Ciphertext Check Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_TIMER_IV, &value);
    if (!status) {
        printf("(%04X) SMN Timer IV Register:          0x%08x\n",
               SMN_TIMER_IV, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Timer IV Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_TIMER_CONTROL, &value);
    if (!status) {
        printf("(%04X) SMN Timer Control Register     (0x%08x): ",
               SMN_TIMER_CONTROL, value);
        print_smn_timer_control_register(value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Timer IV Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_DEBUG_DETECT_STAT, &value);
    if (!status) {
        printf("(%04X) SMN Debug Detected Register:   (0x%08x):",
               SMN_DEBUG_DETECT_STAT, value);
        print_scc_debug_detector_register(value);
    }
    else {
        printf("(Could not get SMN Debug Detected Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_TIMER, &value);
    if (!status) {
        printf("(%04X) SMN Timer Register:            (0x%08x)\n",
               SMN_TIMER, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Timer Register)\n");
    }

    return (had_error != 0);
}


/*Encryption and Decryption Tests*/
int
run_cipher_tests(int scc_fd)
{
    scc_encrypt_decrypt cipher_control;
    uint32_t value;             /* value of various registers  */
    int status;
    uint8_t* ciphertext = NULL;
    uint8_t* new_plaintext = NULL;
    int had_error = 0;

    ciphertext = malloc(sizeof(plaintext)) + byte_offset;
    new_plaintext = malloc(sizeof(plaintext) + byte_offset);

    cipher_control.data_in = plaintext;
    cipher_control.data_in_length = plaintext_length;
    cipher_control.data_out = ciphertext;
    /* sufficient for block and padding */
    cipher_control.data_out_length = plaintext_length +
        encrypt_padding_allowance;

    cipher_control.init_vector[0] = 0;
    cipher_control.init_vector[1] = 0;

    cipher_control.crypto_mode = crypto_mode;
    cipher_control.check_mode = check_mode;
    cipher_control.wait_mode = SCC_ENCRYPT_POLL;

    /* clear these to make sure they are set by driver */
    write_scc_register(scc_fd, SCM_INIT_VECTOR_0, 0);
    write_scc_register(scc_fd, SCM_INIT_VECTOR_1, 0);

    /* Start Encryption */
    printf("***CIPHER: Start Encrypting....\n");
    status = ioctl(scc_fd, SCC_TEST_ENCRYPT, &cipher_control);

    if (status != IOCTL_SCC_OK) {
        fprintf(stderr, "Encrypt failed\n");
        print_scc_return_code(cipher_control.function_return_code);
        had_error = 1;
    }
    else {
        if (debug) {
            dump_cipher_control_block(&cipher_control);
            print_ram_data((void*)ciphertext, SCM_BLACK_MEMORY, 32);

            read_scc_register(scc_fd, SCM_STATUS, &value);
            printf("SCM Status                  (0x%08x): ", value);
            print_scm_status_register(value);

            read_scc_register(scc_fd, SMN_STATUS, &value);
            printf("SMN Status                  (0x%08x): ", value);
            print_smn_status_register(value);
  
            read_scc_register(scc_fd, SCM_ERROR_STATUS, &value);
            printf("SCM Error Register          (0x%08x):", value);
            print_scc_error_status_register(value);
        }

        printf("***End of Encryption\n");

        if (! encrypt_only) {
            /* wipe out any remnants of encryption */
            run_zeroize_tests(scc_fd);

            /* DECRYPTION TEST */

            cipher_control.data_in = ciphertext; /* feed ciphertext back in */
            cipher_control.data_in_length = cipher_control.data_out_length;
            cipher_control.data_out = new_plaintext;
            cipher_control.data_out_length = plaintext_length +
                decrypt_padding_allowance;
        
            if (inject_crc_error) {
                ciphertext[rand()%cipher_control.data_in_length] ^= 1;
            }

            if (crypto_mode == SCC_CBC_MODE) {
                cipher_control.init_vector[0] = 0;
                cipher_control.init_vector[1] = 0;
            }

            cipher_control.crypto_mode = crypto_mode;
            cipher_control.check_mode = check_mode;
            cipher_control.wait_mode = SCC_ENCRYPT_POLL;

            /* clear these again to make sure they are set by driver */
            write_scc_register(scc_fd, SCM_INIT_VECTOR_0, 0);
            write_scc_register(scc_fd, SCM_INIT_VECTOR_1, 0);

            write_scc_register(scc_fd, SCM_INTERRUPT_CTRL,
                               SCM_INTERRUPT_CTRL_CLEAR_INTERRUPT
                               | SCM_INTERRUPT_CTRL_MASK_INTERRUPTS);

            /* Start Decryption */

            printf("**Start Decrypting...\n");
            status = ioctl(scc_fd, SCC_TEST_DECRYPT, &cipher_control);

            if (status != IOCTL_SCC_OK) {
                had_error = 1;
                fprintf(stderr, "Decrypt failed\n");
                print_scc_return_code(cipher_control.function_return_code);
            }
            else {
                int bytes_to_check = plaintext_length;

                if (debug) {

                    dump_cipher_control_block(&cipher_control);
                    print_ram_data((void*)new_plaintext, SCM_RED_MEMORY, 10);

                    read_scc_register(scc_fd, SCM_STATUS, &value);
                    printf("SCM Register                (0x%08x): ", value);
                    print_scm_status_register(value);

                    read_scc_register(scc_fd, SMN_STATUS, &value);
                    printf("SMN Register                (0x%08x): ", value);
                    print_smn_status_register(value);

                    read_scc_register(scc_fd, SCM_ERROR_STATUS, &value);
                    printf("SCM Error Register          (0x%08x):", value);
                    print_scc_error_status_register(value);
                }
 
                if (cipher_control.data_out_length != plaintext_length) {
                    printf("Error:  input plaintext length (%d) and output "
                           "plaintext length (%ld) do not match.\n",
                           plaintext_length, cipher_control.data_out_length);
                }
                while (bytes_to_check--) {
                    if (plaintext[bytes_to_check]
                        != new_plaintext[bytes_to_check]) {
                        had_error = 1;
                        printf("Error:  input plaintext (0x%02x) and "
                               "output plaintext (0x%02x) do not match at "
                               "offset %d.\n", plaintext[bytes_to_check],
                               new_plaintext[bytes_to_check], bytes_to_check);
                    }
                }
                printf("***Successful end of Cipher Test\n");
            } /* else decrypt !IOCTL_SCC_OK */
        } /* encrypt_only */
    } /* else encrypt !IOCTL_SCC_OK */

    return had_error;
}


/***********************************************************************
 set_software_alarm()
 **********************************************************************/
void set_software_alarm(int scc_fd) {
    int status;

    status = ioctl(scc_fd, SCC_TEST_SET_ALARM, NULL);
}


/***********************************************************************
 run_timer_tests()
 **********************************************************************/
void run_timer_tests(int scc_fd, uint32_t timer_iv) {
    uint32_t value;
    int i;

    printf ("Writing 0x%x into SMN_TIMER_IV\n", timer_iv);
    write_scc_register(scc_fd, SMN_TIMER_IV, timer_iv);

    /* this operation should move the initial value to the timer reg */
    printf ("Loading timer using SMN_TIMER_CONTROL\n");
    write_scc_register(scc_fd, SMN_TIMER_CONTROL, SMN_TIMER_LOAD_TIMER);

    read_scc_register(scc_fd, SMN_TIMER, &value);
    printf ("SMN_TIMER value: 0x%x\n", value);

    printf ("Starting timer using SMN_TIMER_CONTROL\n");
    write_scc_register(scc_fd, SMN_TIMER_CONTROL, SMN_TIMER_START_TIMER);

    /*
     * Kill some time - Only if compiler doesn't optimize this away!
     */
    for (i= 0; i < 100000; i++);

    /* now stop the timer */
    printf("Stopping the timer\n");
    read_scc_register(scc_fd, SMN_TIMER_CONTROL, &value);
    write_scc_register(scc_fd, SMN_TIMER_CONTROL,
                       value & ~SMN_TIMER_STOP_MASK);

    /* and see how much time we ran off */
    read_scc_register(scc_fd, SMN_TIMER, &value);
    printf ("SMN_TIMER value: 0x%x\n", value);

}

/***********************************************************************
 run_aic_tests()
 **********************************************************************/
int
run_aic_tests(int scc_fd) {
    uint32_t value;

    /* This series of writes generates FAIL mode !!! */
    write_scc_register(scc_fd, SMN_SEQUENCE_START, 4);
    write_scc_register(scc_fd, SMN_SEQUENCE_END, 5);
    write_scc_register(scc_fd, SMN_SEQUENCE_CHECK, 5);

    read_scc_register(scc_fd, SMN_STATUS, &value);
    printf("SMN Status: ");
    print_smn_status_register(value);

    
    return 0;
}


/***********************************************************************
 run_plaintext_cipher_text_tests()
 **********************************************************************/
void
run_plaintext_cipher_text_tests(int scc_fd)
{
}


/***********************************************************************
 run_zeroize_tests()
 **********************************************************************/
void
run_zeroize_tests(int scc_fd)
{
    int status;
    uint32_t value;

    printf("\n***Zeroize Test\n");

    /* set up some known values */
    write_scc_register(scc_fd, SCM_RED_MEMORY, 42);
    write_scc_register(scc_fd, SCM_RED_MEMORY+1020, 42);
    write_scc_register(scc_fd, SCM_BLACK_MEMORY, 42);
    write_scc_register(scc_fd, SCM_BLACK_MEMORY+1020, 42);

    status = ioctl(scc_fd, SCC_TEST_ZEROIZE, &value);
    if (status != IOCTL_SCC_OK) {
        perror("Ioctl SCC_TEST_ZEROIZE failed: ");
        fprintf(stderr, "Function return code: %0x\n", value);
    }
    else {
        /* no errors, verify values are gone */
        read_scc_register(scc_fd, SCM_RED_MEMORY, &value);
        if (value == 42) {
            fprintf(stderr, "Zeroize failed at 0x%x\n", SCM_RED_MEMORY);
        }
        read_scc_register(scc_fd, SCM_RED_MEMORY+1020, &value);
        if (value == 42) {
            fprintf(stderr, "Zeroize failed at 0x%x\n", SCM_RED_MEMORY+1020);
        }
        read_scc_register(scc_fd, SCM_BLACK_MEMORY, &value);
        if (value == 42) {
            fprintf(stderr, "Zeroize failed at 0x%x\n", SCM_BLACK_MEMORY);
        }
        read_scc_register(scc_fd, SCM_BLACK_MEMORY+1020, &value);
        if (value == 42) {
            fprintf(stderr, "Zeroize failed at 0x%x\n", SCM_BLACK_MEMORY+1020);
        }
    }
}


/***********************************************************************
 * write_scc_register
 **********************************************************************/
int
write_scc_register(int device,  /* OS device connect */
                   uint32_t reg, /* The register to be written */
                   uint32_t value) /* The value to store */
{
    scc_reg_access register_access;
    int status;

    register_access.reg_offset = reg;
    register_access.reg_data = value;

    status = ioctl(device, SCC_TEST_WRITE_REG, &register_access);
    if (status != IOCTL_SCC_OK) {
        perror("Ioctl SCC_TEST_WRITE_REG failed: ");
    }

    return status;
}


/***********************************************************************
 * read_scc_register
 **********************************************************************/
int
read_scc_register(int device, /* OS device connect */
                  uint32_t reg, /* The register to be written */
                  uint32_t *value) /* The location for return value */
{
    scc_reg_access register_access;
    int status;

    register_access.reg_offset = reg;
    status = ioctl(device, SCC_TEST_READ_REG, &register_access);
    if (status != IOCTL_SCC_OK) {
        perror("Ioctl SCC_TEST_READ_REG failed: ");
    }
    else {
        *value = register_access.reg_data;
    }

    return status;
}


/***********************************************************************
 get_scc_configuration()
 **********************************************************************/
scc_configuration_access*
get_scc_configuration(int scc_fd) {
    static scc_configuration_access *config = NULL;
    int status;

    if (config == NULL) {
        config = calloc(1, sizeof(scc_configuration_access));
        if (config != NULL) {
            status = ioctl(scc_fd, SCC_TEST_GET_CONFIGURATION, config);
            if (status != IOCTL_SCC_OK) {
                perror("Ioctl SCC_GET_CONFIGURATION failed: ");
                free (config);
                config = NULL;
            }
        }
    }

    return config;
}


/** Run mmap() test on SMN registers
 *
 *  This feature is not enabled, as user access to the SMN is not available
 *  on existing platforms (at time of writing).
 */
void
run_mmap_tests(int fd)
{
    volatile uint32_t *smn = 0;

    smn = mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE,  fd, 0);
    if (smn == MAP_FAILED) {
        perror("mmap failed: ");
    }
    else {
        printf("SMN appears at %08x\n", (unsigned)smn);
        printf("SMN_SEQUENCE_CHECK: %08x\n", *(smn+SMN_SEQUENCE_CHECK));
#if 0 /* this is a write-only register */
        printf("SMN_BITBANK_DECREMENT: %08x\n", *(smn+SMN_BITBANK_DECREMENT));
#endif
#if 0  /* this is killing the process */
        *(smn+SMN_PLAINTEXT_CHECK) = 42;
#endif
        printf("SMN_PLAINTEXT_CHECK: %08x\n", *(smn+SMN_PLAINTEXT_CHECK));
        printf("SMN_CIPHERTEXT_CHECK: %08x\n", *(smn+SMN_CIPHERTEXT_CHECK));
    }
    sleep(45000);
}


/**
 * Run tests to get values into and out of slots.
 */
int
run_wrap_tests (int scc_fd)
{
    int status;
    uint64_t owner1 = 42;
    uint32_t key[8];
    uint8_t black_key[48];      /* INFLEXIBLE! Warning for driver changes. */
    scc_alloc_slot_access alloc_acc;
    scc_load_slot_access load_acc;
    scc_get_slot_info_access info_acc;
    scc_encrypt_slot_access unload_acc;
    int slot_allocated = 0;
    
    strcpy((char*)key,"abcdefgh");

    alloc_acc.owner_id = owner1;
    alloc_acc.value_size_bytes = 8;
    status = ioctl(scc_fd, SCC_TEST_ALLOC_SLOT, &alloc_acc);

    if (status != 0) {
        printf("ioctl for slot alloc returned %d\n", status);
    } else {
        if (alloc_acc.scc_status != SCC_RET_OK) {
            printf("Slot alloc returned %d\n", alloc_acc.scc_status);
            status = 1;
        } else {
            printf("Using slot %d\n", alloc_acc.slot);
            slot_allocated = 1;
        }
    }

    if (status == 0) {
        load_acc.slot = alloc_acc.slot;
        load_acc.owner_id = owner1;
        load_acc.key_is_red = 1;
        load_acc.key_data = (uint8_t*)key;
        load_acc.key_data_length = alloc_acc.value_size_bytes;
        status = ioctl(scc_fd, SCC_TEST_LOAD_SLOT, &load_acc);
        if (status != 0) {
            printf("ioctl for load RED value returned %d\n", status);
        } else {
            if (load_acc.scc_status != SCC_RET_OK) {
                printf("Red value load returned %d\n", load_acc.scc_status);
                status = 1;
            }                
        }
    }

    if (status == 0) {
        info_acc.slot = alloc_acc.slot;
        info_acc.owner_id = owner1;
        status = ioctl(scc_fd, SCC_TEST_GET_SLOT_INFO, &info_acc);
        if (status != 0) {
            printf("ioctl for get slot info returned %d\n", status);
        } else {
            if (info_acc.scc_status != SCC_RET_OK) {
                printf("Get slot info returned %d\n",
                       info_acc.scc_status);
                status = 1;
            } else {
                uint32_t word;

                status = read_scc_register(scc_fd,
                                           info_acc.address%8192,
                                           &word);
                if (status == 0) {
                    if (word != key[0]) {
                        printf("First word of RED key does not match!: "
                               " 0x%x/0x%x\n", word, key[0]);
                        status = 1;
                    } else {
                        status = read_scc_register(scc_fd,
                                                   (info_acc.address%8192) + 4,
                                                   &word);
                        if (status == 0) {
                            if (word != key[1]) {
                                printf("Second word of RED key does not "
                                       "match!\n");
                                status = 1;
                            }
                        }
                    }
                }
            }
        }
    }
                            
    /* Now unload the key */
    if (status == 0) {
        unload_acc.slot = alloc_acc.slot;
        unload_acc.owner_id = owner1;
        unload_acc.key_data = black_key;
        unload_acc.key_data_length =alloc_acc.value_size_bytes;
        status = ioctl(scc_fd, SCC_TEST_ENCRYPT_SLOT, &unload_acc);
        if (status != 0) {
            printf("ioctl for encrypt value returned %d\n", status);
        } else {
            if (unload_acc.scc_status != SCC_RET_OK) {
                printf("Value unload returned %d\n",
                       unload_acc.scc_status);
                status = 1;
            } else {
                slot_allocated = 0; /* successfully unloaded and dealloc'ed */
            }
        }
    }
    
    /* Reacquire a slot */
    if ((status == 0) && !slot_allocated) {
        status = ioctl(scc_fd, SCC_TEST_ALLOC_SLOT, &alloc_acc);

        if (status != 0) {
            printf("ioctl for slot (re)alloc returned %d\n", status);
        } else {
            if (alloc_acc.scc_status != SCC_RET_OK) {
                printf("Slot (re)alloc returned %d\n", alloc_acc.scc_status);
                status = 1;
            } else {
                slot_allocated = 1;
            }
        }
    }

    /* Now reload as black key */
    if ((status == 0) && slot_allocated) {
        load_acc.key_is_red = 0;
        load_acc.key_data = black_key;
        status = ioctl(scc_fd, SCC_TEST_LOAD_SLOT, &load_acc);
        if (status != 0) {
            printf("ioctl for load BLACK value returned %d\n", status);
        } else {
            if (load_acc.scc_status != SCC_RET_OK) {
                printf("Black value load returned %d\n", load_acc.scc_status);
                status = 1;
            } else {
                uint32_t word;

                status = read_scc_register(scc_fd,
                                           info_acc.address%8192,
                                           &word);
                if (status == 0) {
                    if (word != key[0]) {
                        printf("First word of reloaded key does not match!\n");
                        status = 1;
                    } else {
                        status = read_scc_register(scc_fd,
                                                   (info_acc.address%8192) + 4,
                                                   &word);
                        if (status == 0) {
                            if (word != key[1]) {
                                printf("Second word of reloaded key does not "
                                       "match!\n");
                                status = 1;
                            } else {
                                printf("Wrapped Key test passed\n");
                            }
                        }
                    }
                }
            }
        }
    }
    /* Dealloc slot if it is still assigned */
    if (slot_allocated) {
        scc_dealloc_slot_access dealloc_acc;

        dealloc_acc.owner_id = owner1;
        dealloc_acc.slot = alloc_acc.slot;

        status = ioctl(scc_fd, SCC_TEST_DEALLOC_SLOT, &dealloc_acc);

        if (status != 0) {
            printf("ioctl for slot dealloc returned %d\n", status);
                status = 1;
        } else {
            if (dealloc_acc.scc_status != SCC_RET_OK) {
                printf("Slot dealloc returned %d\n", dealloc_acc.scc_status);
                status = 1;
            } else {
                printf("(slot successfully deallocated)\n");
            }
        }
    }

    return status;
}


/*
 * Perform one-shot requests to do something with a slot.
 *
 * @param scc_fd   File descriptor to talk to test driver
 * @param arg      Argument associated with slot request
 */
int
do_slot_function(int scc_fd, char* arg)
{
    int status = 1;
    char request = *arg++;

    /* Determine what user is requesting */
    switch(request) {
    case 'a':               /* Allocate a slot - size:owner */
        {
            uint32_t slot_size;
            uint64_t owner;

            if (sscanf(arg, "%d:%Lx", &slot_size, &owner) != 2) {
                printf("improper use of -Ka (%s); need decimal "
                       "slot size and hex owner:  -KaSSS:OOOO\n", arg);
                status = 1;
            }
            else {
                scc_alloc_slot_access acc;

                acc.owner_id = owner;
                acc.value_size_bytes = slot_size;
                status = ioctl(scc_fd, SCC_TEST_ALLOC_SLOT, &acc);
                if (status != 0) {
                    printf("ioctl for slot alloc returned %d\n", status);
                } else {
                    if (acc.scc_status != SCC_RET_OK) {
                        printf("Slot alloc returned %d\n", acc.scc_status);
                        status = 1;
                    } else {
                        scc_get_slot_info_access info_acc;

                        info_acc.slot = acc.slot;
                        info_acc.owner_id = owner;
                        status = ioctl(scc_fd, SCC_TEST_GET_SLOT_INFO,
                                       &info_acc);
                        if ((status != 0)
                            || (info_acc.scc_status != SCC_RET_OK)) {
                            printf("ioctl for slot info returned %d for "
                                   "allocated slot %d/%d\n",
                                   status, info_acc.scc_status, acc.slot);
                            status = 1;
                        } else {
                            printf("Slot %d at %x is now allocated\n",
                                   acc.slot, info_acc.address);
                        }
                    }
                }
            }
        }
    break;
    case 'd':               /* Deallocate a slot - slot:owner */
        {
            uint64_t owner;
            uint32_t slot;

            if (sscanf(arg, "%d:%Lx", &slot, &owner) != 2) {
                printf("improper use of -Kd (%s); need decimal "
                       "slot id and hex owner:  -KaSSS:OOOO\n", arg);
                status = 1;
            }
            else {
                scc_dealloc_slot_access acc;

                acc.owner_id = owner;
                acc.slot = slot;
                status = ioctl(scc_fd, SCC_TEST_DEALLOC_SLOT, &acc);
                if (status != 0) {
                    printf("ioctl for slot dealloc returned %d\n", status);
                } else {
                    if (acc.scc_status != SCC_RET_OK) {
                        printf("Slot dealloc returned %d\n", acc.scc_status);
                        status = 1;
                    } else {
                        printf("Slot %d is now deallocated\n", acc.slot);
                    }
                }
            }
        }
    break;
    case 'l':               /* load a value slot:r/b:hexvalue */
        {
            uint64_t owner;
            uint32_t slot;
            uint8_t hex_value[200];
            uint8_t value[200];
            char color;

            if ((sscanf(arg, "%d:%Lx:%c:%s", &slot, &owner, &color, hex_value)
                 != 4) || ((color != 'b') && (color != 'r'))) {
                printf("improper use of -Kl (%s); need decimal "
                       "slot id, 'r' or 'b', and hex value: "
                       "-KlSSS:OOOO:r:VVVV\n",
                       arg);
                status = 1;
            }
            else {
                scc_load_slot_access acc;
                int i;

                status = 0;
                for (i = 0; i < strlen(hex_value); i+= 2) {
                    int byte;
                    if (sscanf(hex_value+i, "%2x", &byte) != 1) {
                        printf("Hex value is not formed properly\n");
                        status = 1;
                    } else {
                        value[i/2] = byte;
                    }
                }

                if (status == 0) {
                    acc.owner_id = owner;
                    acc.slot = slot;
                    acc.key_is_red = (color == 'r');
                    acc.key_data = value;
                    acc.key_data_length = i/2;
                    status = ioctl(scc_fd, SCC_TEST_LOAD_SLOT, &acc);
                    if (status != 0) {
                        printf("ioctl for load value returned %d\n", status);
                    } else {
                        if (acc.scc_status != SCC_RET_OK) {
                            printf("Slot load value returned %d\n", acc.scc_status);
                            status = 1;
                        } else {
                            printf("Slot %d is now loaded\n", acc.slot);
                        }
                    }
                }
            }
        }
    break;
    case 'u':               /* Unload/encrypt a slot - slot:owner */
        {
            uint64_t owner;
            uint32_t slot;
            uint8_t blob[200];
            uint8_t hex_blob[400];
            int i;

            if (sscanf(arg, "%d:%Lx", &slot, &owner) != 2) {
                printf("improper use of -Ku (%s); need decimal "
                       "slot id and hex owner:  -KaSSS:OOOO\n", arg);
                status = 1;
            }
            else {
                scc_encrypt_slot_access acc;
                scc_get_slot_info_access info_acc;

                info_acc.slot = slot;
                info_acc.owner_id = owner;
                status = ioctl(scc_fd, SCC_TEST_GET_SLOT_INFO, &info_acc);

                if ((status != 0) || (info_acc.scc_status != SCC_RET_OK)) {
                    printf("ioctl for get slot info returned %d; scc"
                           " may have returned %d\n", status,
                           info_acc.scc_status);
                    status = 1;
                } else {
                    acc.owner_id = owner;
                    acc.slot = slot;
                    acc.key_data = blob;
                    acc.key_data_length = info_acc.value_size_bytes;

                    status = ioctl(scc_fd, SCC_TEST_ENCRYPT_SLOT, &acc);
                    if (status != 0) {
                        printf("ioctl for slot encrypt returned %d\n", status);
                    } else {
                        if (acc.scc_status != SCC_RET_OK) {
                            printf("Slot encrypt returned %d\n",
                                   acc.scc_status);
                            status = 1;
                        } else {
                            for (i = 0; i < acc.key_data_length; i++) {
                                sprintf(hex_blob+(2*i), "%02x", blob[i]);
                            }
                            hex_blob[2*i] = 0; /* NUL terminate */
                            printf("Encrypted value is %s\n", hex_blob);
                            acc.key_data_length = i;
                        }
                    }
                }
            }
        }
    break;
    default:
        fprintf(stderr, "Unknown flag %c", request);
        status = 1;
        break;
    }

    return status;
}


/* print_ram_data
 *
 * Print eight words per line, starting at ram, as though they
 * started at address, until count words have been printed
 *
 */
void
print_ram_data (uint32_t *ram, uint32_t address, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (i%8 == 0) {
            printf("(%04X) ", address+i*4); /* print byte address */
        }

        printf("%08x ", *ram++); /* print a word */

        if (i%8 == 7) {
            printf("\n");       /* end of line - do newline */
        }
        else if (i%8 == 3) {
            printf (" ");      /* add space in the middle */
        }
    }

    if (count%8 != 0) {         /* if didn't have mod8 words... */
        printf("\n");
    }
}


/** Interpret the SMN Status register and print out the 'on' bits and State */
void
print_smn_status_register(uint32_t status)
{
    int version_id;
    uint8_t state;
          
    version_id = (status&SMN_STATUS_VERSION_ID_MASK)
        >> SMN_STATUS_VERSION_ID_SHIFT;
    state = (status&SMN_STATUS_STATE_MASK) >> SMN_STATUS_STATE_SHIFT; 

    printf("Version %d %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s, State: %s\n",
           version_id,
           (status&SMN_STATUS_CACHEABLE_ACCESS) ? ", CACHEABLE_ACCESS" : "",
           (status&SMN_STATUS_ILLEGAL_MASTER) ? ", ILLEGAL_MASTER" : "",
           (status&SMN_STATUS_SCAN_EXIT) ? ", SCAN_EXIT" : "",
           (status&SMN_STATUS_UNALIGNED_ACCESS) ? ", UNALIGNED_ACCESS" : "",
           (status&SMN_STATUS_BYTE_ACCESS) ? ", BYTE_ACCESS" : "",
           (status&SMN_STATUS_ILLEGAL_ADDRESS) ? ", ILLEGAL_ADDRESS" : "",
           (status&SMN_STATUS_USER_ACCESS) ? ", USER_ACCESS" : "",
           (status&SMN_STATUS_DEFAULT_KEY) ? ", DEFAULT_KEY" : "",
           (status&SMN_STATUS_BAD_KEY) ? ", BAD_KEY" : "",
           (status&SMN_STATUS_ILLEGAL_ACCESS) ? ", ILLEGAL_ACCESS" : "",
           (status&SMN_STATUS_SCM_ERROR) ? ", SCM_ERROR" : "",
           (status&SMN_STATUS_SMN_STATUS_IRQ) ? ", SMN_IRQ" : "",
           (status&SMN_STATUS_SOFTWARE_ALARM) ? ", SOFTWARE_ALARM" : "",
           (status&SMN_STATUS_TIMER_ERROR) ? ", TIMER_ERROR" : "",
           (status&SMN_STATUS_PC_ERROR) ? ", PC_ERROR" : "",
           (status&SMN_STATUS_ASC_ERROR) ? ", ASC_ERROR" : "",
           (status&SMN_STATUS_SECURITY_POLICY_ERROR)
                          ? ", SECURITY_POLICY_ERROR" : "",
           (status&SMN_STATUS_DEBUG_ACTIVE)?", DEBUG_ACTIVE" : "",
           (status&SMN_STATUS_ZEROIZE_FAIL)?", ZEROIZE_FAIL" : "",
           (status&SMN_STATUS_INTERNAL_BOOT)?", INTERNAL_BOOT" : "",
           get_smn_state_name(state));
}


/** Interpret the state (a field of the SMN State register) and return its
 *  name 
**/
char *
get_smn_state_name(const uint8_t state)
{
    switch (state) {
    case SMN_STATE_START:
        return "START";
    case SMN_STATE_ZEROIZE_RAM:
        return "ZEROIZE";
    case SMN_STATE_HEALTH_CHECK:
        return "HEALTH CHECK";
    case SMN_STATE_FAIL:
        return "FAIL";
    case SMN_STATE_SECURE:
        return "SECURE";
    case SMN_STATE_NON_SECURE:
        return "NON-SECURE";
    default:
        return "UNKNOWN";
    }
}


/** Interpret the SCM Status register and print out the 'on' bits */
void
print_scm_status_register(uint32_t status)
{
    printf("%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
           (status&SCM_STATUS_LENGTH_ERROR) ? ", LENGTH_ERROR" : "",
           (status&SCM_STATUS_BLOCK_ACCESS_REMOVED) ? ", NO_BLOCK_ACCESS" : "",
           (status&SCM_STATUS_CIPHERING_DONE) ? ", CIPHERING_DONE" : "",
           (status&SCM_STATUS_ZEROIZING_DONE) ? ", ZEROIZING_DONE" : "",
           (status&SCM_STATUS_INTERRUPT_STATUS) ? ", INTERRUPT_STATUS" : "",
           (status&SCM_STATUS_SECRET_KEY) ? ", SECRET_KEY_IN_USE" : "",
           (status&SCM_STATUS_INTERNAL_ERROR) ? ", INTERNAL_ERROR" : "",
           (status&SCM_STATUS_BAD_SECRET_KEY) ? ", BAD_SECRET_KEY" : "",
           (status&SCM_STATUS_ZEROIZE_FAILED) ? ", ZEROIZE_FAILED" : "",
           (status&SCM_STATUS_SMN_BLOCKING_ACCESS)
                               ? ", SMN_BLOCKING_ACCESS" : "",
           (status&SCM_STATUS_CIPHERING) ? ", CIPHERING" : "",
           (status&SCM_STATUS_ZEROIZING) ? ", ZEROIZING" : "",
           (status&SCM_STATUS_BUSY) ? ", BUSY" : "");
           

}


/** Interpret the SCM Control register and print its meaning */
void
print_scm_control_register(uint32_t control)
{
    printf("%s%s%s\n",
           ((control&SCM_CONTROL_CIPHER_MODE_MASK)==SCM_DECRYPT_MODE) ? 
           "DECRYPT " : "ENCRYPT ",
           ((control&SCM_CONTROL_CHAINING_MODE_MASK)==SCM_CBC_MODE) ? 
           "CBC " : "ECB ",
           (control&SCM_CONTROL_START_CIPHER) ? "CipherStart":"");
}


void
print_scc_error_status_register(uint32_t error)
{
    printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
           "",
           (error&SCM_ERR_CACHEABLE_ACCESS) ? ", CACHEABLE_ACCESS" : "",
           (error&SCM_ERR_ILLEGAL_MASTER) ? ", ILLEGAL_MASTER" : "",
           (error&SCM_ERR_UNALIGNED_ACCESS) ? ", UNALIGNED_ACCESS" : "",
           (error&SCM_ERR_BYTE_ACCESS) ? ", BYTE_ACCESS" : "",
           (error&SCM_ERR_ILLEGAL_ADDRESS) ? ", ILLEGAL_ADDRESS" : "",
           (error&SCM_ERR_USER_ACCESS) ? ", USER_ACCESS" : "",
           (error&SCM_ERR_SECRET_KEY_IN_USE) ? ", SECRET_KEY_IN_USE" : "",
           (error&SCM_ERR_INTERNAL_ERROR) ? ", INTERNAL_ERROR" : "",
           (error&SCM_ERR_BAD_SECRET_KEY) ? ", BAD_SECRET_KEY" : "",
           (error&SCM_ERR_ZEROIZE_FAILED) ? ", ZEROIZE_FAILED" : "",
           (error&SCM_ERR_SMN_BLOCKING_ACCESS) ? ", SMN_BLOCKING_ACCESS" : "",
           (error&SCM_ERR_CIPHERING) ? ", CIPHERING" : "",
           (error&SCM_ERR_ZEROIZING) ? ", ZEROIZING" : "",
           (error&SCM_ERR_BUSY) ? ", BUSY" : "");
           
}


void
print_smn_command_register(uint32_t command)
{
    if (command&SMN_COMMAND_ZEROS_MASK) {
        printf(" zeroes: 0x%08x", debug&SMN_COMMAND_ZEROS_MASK);
    }
    printf("%s%s%s%s\n",
           (command&SMN_COMMAND_CLEAR_INTERRUPT) ? " CLEAR_INTERRUPT" : "",
           (command&SMN_COMMAND_CLEAR_BIT_BANK) ? " CLEAR_BITBANK" : "",
           (command&SMN_COMMAND_ENABLE_INTERRUPT) ? " ENABLE_INTERRUPT" : "",
           (command&SMN_COMMAND_SET_SOFTWARE_ALARM) ? " SET_SOFWARE_ALARM" : ""
           );
}


void
print_smn_timer_control_register(uint32_t control)
{
    if (control&SMN_TIMER_CTRL_ZEROS_MASK) {
        printf(" zeroes: 0x%08x", debug&SMN_TIMER_CTRL_ZEROS_MASK);
    }

    printf("%s%s\n",
           (control&SMN_TIMER_LOAD_TIMER) ? " LOAD_TIMER" : "",
           (control&SMN_TIMER_START_TIMER) ? " START_TIMER" : "");

    return;
}


/** generate human-readable interpretation of the SMN Debug Detector
    Register
*/
void
print_scc_debug_detector_register(uint32_t debug)
{
    if (debug&SMN_DBG_ZEROS_MASK) {
        printf(" zeroes: 0x%08x", debug&SMN_DBG_ZEROS_MASK);
    }
    printf("%s%s%s%s%s%s%s%s%s%s%s%s\n",
           (debug&SMN_DBG_D1) ? " D1" : "",
           (debug&SMN_DBG_D2) ? " D2" : "",
           (debug&SMN_DBG_D3) ? " D3" : "",
           (debug&SMN_DBG_D4) ? " D4" : "",
           (debug&SMN_DBG_D5) ? " D5" : "",
           (debug&SMN_DBG_D6) ? " D6" : "",
           (debug&SMN_DBG_D7) ? " D7" : "",
           (debug&SMN_DBG_D8) ? " D8" : "",
           (debug&SMN_DBG_D9) ? " D9" : "",
           (debug&SMN_DBG_D10) ? " D10" : "",
           (debug&SMN_DBG_D11) ? " D11" : "",
           (debug&SMN_DBG_D12) ?  " D12" : "");
}


/** print an interpretation (the symbol name) of @c code */
void
print_scc_return_code(scc_return_t code)
{
    char *msg = NULL;

    switch (code) {
    case SCC_RET_OK:
        msg = "SCC_RET_OK";
        break;
    case SCC_RET_FAIL:
        msg = "SCC_RET_FAIL";
        break;
    case SCC_RET_VERIFICATION_FAILED:
        msg = "SCC_RET_VERIFICATION_FAILED";
        break;
    case SCC_RET_TOO_MANY_FUNCTIONS:
        msg = "SCC_RET_TOO_MANY_FUNCTIONS";
        break;
    case SCC_RET_BUSY:
        msg = "SCC_RET_BUSY";
        break;
    case SCC_RET_INSUFFICIENT_SPACE:
        msg = "SCC_RET_INSUFFICIENT_SPACE";
        break;
    default:
        break;
    }

    if (msg) {
        printf("SCC return code: %s\n", msg);
    }
    else {
        printf("SCC return code: %d\n", code);
    }
}


/** Print out various values of the Cipher Control Block */
void
dump_cipher_control_block(scc_encrypt_decrypt *cipher_control)
{
    printf("data_out_length: %ld", (long)cipher_control->data_out_length);
    if (cipher_control->crypto_mode == SCC_CBC_MODE) {
        printf(", init_vector: 0x%08x%08x", cipher_control->init_vector[0],
               cipher_control->init_vector[1]);
    }
    printf("\n");
}
