/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/**
 * @file scc2_test.c
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
#include "fsl_shw.h"
#include "scc2_test_driver.h"

/* test routines */
void display_configuration(int device);
int check_safe_registers(int device);
int dump_registers(int device);
int run_aic_tests(int scc_fd);
int run_cipher_tests(int scc_fd);
void run_timer_tests(int device, uint32_t);
void set_software_alarm(int device);

/* utility print functions */
void print_ram_data (uint32_t *ram, uint32_t address, int count);
void print_smn_status_register(uint32_t);
void print_scm_version_register(uint32_t);
void print_scm_control_register(uint32_t);
void print_scm_status_register(uint32_t);
void print_scm_part_owner_register(uint32_t);
void print_scm_part_engaged_register(uint32_t);
void print_scm_acc_register(uint32_t);
void print_scm_error_status_register(uint32_t);
char *get_smn_state_name(const uint8_t);
void print_smn_timer_control_register(const uint32_t);
void print_smn_command_register(const uint32_t);
void print_scc_debug_detector_register(uint32_t);
void print_scc_return_code(const scc_return_t);

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
        plaintext[i] = i % 256;
    }
}


/** Normal number of bytes to encrypt */
#define DEFAULT_PLAINTEXT_LENGTH 16


/** */
int plaintext_length = DEFAULT_PLAINTEXT_LENGTH;


/** */
int byte_offset = 0;


/** Normal number of bytes of padding to add to ciphertext storage */
#define DEFAULT_PADDING_ALLOWANCE 32

int encrypt_padding_allowance = DEFAULT_PADDING_ALLOWANCE;

int decrypt_padding_allowance = 0;

/** */
scc_verify_t check_mode = SCC_VERIFY_MODE_NONE;

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
    char *scc_device_path = "/dev/scc2_test";
    int argument_switch;
    int test_switch;
    uint32_t timer_value = 0x5f0000;
    char *test_to_run = "Crps"; /* default list of tests to be run in order */
    int test_status = 0;

    init_plaintext();

    /* Open up the SCC device */
    /* Yes, I know, there is a command option to change the device path. */
    scc_fd = open(scc_device_path, O_RDWR);
    if (scc_fd < 0) {
        perror("Cannot open SCC device");
        exit (1);
    }

    /* Process command line arguments - until we come up empty */
    while ( (argument_switch = getopt(arg_count, arg_list, "L:P:R:S:T:W:"))
            != EOF ) {
        switch (argument_switch) {
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
            fprintf(stderr, "Usage: scc2_test [-P sccpath] [-T timerIV]"
                    " [-Roff] [Woff:hex_value]\n"
                    " [-Sopt] where options can be\n"
                    "    c(corrupt ciphertext before decrypting)\n"
                    "    D(print debug messages)\n"
                    "    l(set Length of plaintext for cipher test)\n"
                    "    m(+set/-reset CBC mode)\n"
                    "    o(change byte offset of ciphertext)"
                    "    Q(brief output mode)"
                    " [-L tests] where tests can be\n"
                    "    a(set sw alarm)\n"
                    "    c(run AIC checks)\n"
                    "    C(show config)\n"
                    "    p(partition/encrypt/decrypt)\n"
                    "    r(print Registers)\n"
                    "    s(print Safe registers)\n"
                    "    t(test timer)\n"
                    );
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
        case 'p':
            test_status |= run_cipher_tests(scc_fd);
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

    status = read_scc_register(scc_fd, SMN_STATUS_REG, &value);
    if (!status) {
        printf("(%04X) SMN Status                      (0x%08x): ",
               SMN_STATUS_REG, value);
        print_smn_status_register(value);
    }
    else {
        printf("(Could not get SCM Status Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_VERSION_REG, &value);
    if (!status) {
        printf("(%04X) SCM Version Register:           (0x%08x): ",
               SCM_VERSION_REG, value);
        print_scm_version_register(value);
    } else {
        had_error++;
        printf("(Could not get SCM Version Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_STATUS_REG, &value);
    if (!status) {
        printf("(%04X) SCM Status                      (0x%08x): ",
               SCM_STATUS_REG, value);
        print_scm_status_register(value);
    }
    else {
        printf("(Could not get SCM Status Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_ERR_STATUS_REG, &value);
    if (!status) {
        printf("(%04X) SCM Error Register              (0x%08x): ",
               SCM_ERR_STATUS_REG, value);
        print_scm_error_status_register(value);
    }
    else {
        printf("(Could not get SCM Error Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_INT_CTL_REG, &value);
    if (!status) {
        printf("(%04X) SCM Interrupt Control Register:  0x%08x\n",
               SCM_INT_CTL_REG, value);
    }
    else {
        printf("(Could not get SCM Interrupt Control Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_FAULT_ADR_REG, &value);
    if (!status) {
        printf("(%04X) SCM Fault Address Register:      0x%08x\n",
               SCM_FAULT_ADR_REG, value);
    }
    else {
        printf("(Could not get SCM Fault Add Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_PART_OWNERS_REG, &value);
    if (!status) {
        printf("(%04X) SCM Partition Owner Register:   (0x%08x): ",
               SCM_PART_OWNERS_REG, value);
        print_scm_part_owner_register(value);
    }
    else {
        printf("(Could not get SCM Partition Owner Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_PART_ENGAGED_REG, &value);
    if (!status) {
        printf("(%04X) SCM Partition Engaged Register: (0x%08x): ",
               SCM_PART_ENGAGED_REG, value);
        print_scm_part_engaged_register(value);
    }
    else {
        printf("(Could not get SCM Partition Engaged Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_C_BLACK_ST_REG, &value);
    if (!status) {
        printf("(%04X) SCM Cipher Black RAM Start Register: 0x%08x\n",
               SCM_C_BLACK_ST_REG, value);
    }
    else {
        printf("(Could not get SCM Cipher Black RAM Start Register)\n");
        had_error++;
    }

    return (had_error != 0);
}


int
dump_registers(int scc_fd)
{
    int status;
    uint32_t value;
    uint32_t config;
    uint32_t engaged;
    uint32_t owned;
    int part_count;
    int i;
    int had_error = 0;

    status = read_scc_register(scc_fd, SCM_VERSION_REG, &config);
    if (!status) {
        printf("(%04X) SCM Version Register:          (0x%08x): ",
               SCM_VERSION_REG, config);
        print_scm_version_register(config);
    } else {
        had_error++;
        printf("(Could not get SCM Version Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_INT_CTL_REG, &value);
    if (!status)
        printf("(%04X) SCM Interrupt Control Register: 0x%08x\n",
               SCM_INT_CTL_REG, value);
    else {
        had_error++;
        printf("(Could not get SCM Interrupt Control Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_STATUS_REG, &value);
    if (!status) {
        printf("(%04X) SCM Status Register:           (0x%08x): ",
               SCM_STATUS_REG, value);
        print_scm_status_register(value);
    } else {
        had_error++;
        printf("(Could not get SCM Status Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_ERR_STATUS_REG, &value);
    if (!status) {
        printf("(%04X) SCM Error Status Register      (0x%08x): ",
               SCM_ERR_STATUS_REG, value);
        print_scm_error_status_register(value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Error Status Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_FAULT_ADR_REG, &value);
    if (!status) {
        printf("(%04X) SCM Fault Address:              0x%08x\n",
                SCM_FAULT_ADR_REG, value);
    }
    else {
        printf("(Could not get SCM Fault Address Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_PART_OWNERS_REG, &owned);
    if (!status) {
        printf("(%04X) SCM Partition Owner Register:  (0x%08x): ",
                SCM_PART_OWNERS_REG, owned);
        print_scm_part_owner_register(owned);
    }
    else {
        printf("(Could not get SCM Partition Owner Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_PART_ENGAGED_REG, &engaged);
    if (!status) {
        printf("(%04X) SCM Partition Engaged Register: (0x%08x): ",
               SCM_PART_ENGAGED_REG, engaged);
        print_scm_part_engaged_register(engaged);
    }
    else {
        printf("(Could not get SCM Partition Engaged Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_UNIQUE_ID0_REG, &value);
    if (!status) {
        printf("(%04X) SCM Unique ID0 Register:     0x%08x\n",
               SCM_UNIQUE_ID0_REG, value);
    }
    else {
        printf("(Could not get SCM Unique ID0 Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SCM_UNIQUE_ID1_REG, &value);
    if (!status) {
        printf("(%04X) SCM Unique ID1 Register:     0x%08x\n",
               SCM_UNIQUE_ID1_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Unique ID1 Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_UNIQUE_ID2_REG, &value);
    if (!status) {
        printf("(%04X) SCM Unique ID2 Register:     0x%08x\n",
               SCM_UNIQUE_ID2_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Unique ID2 Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_UNIQUE_ID3_REG, &value);
    if (!status) {
        printf("(%04X) SCM Unique ID3 Register:     0x%08x\n",
               SCM_UNIQUE_ID3_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Unique ID3 Register)\n");
    }

    status = read_scc_register(scc_fd,  SCM_C_BLACK_ST_REG, &value);
    if (!status) {
        printf("(%04X) SCM Black Start Register:     0x%08x\n",
                SCM_C_BLACK_ST_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Black Start Register)\n");
    }

    status = read_scc_register(scc_fd, SCM_DBG_STATUS_REG, &value);
    if (!status) {
        printf("(%04X) SCM Debug Status Register:     0x%08x\n",
               SCM_DBG_STATUS_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SCM Debug Status Register)\n");
    }

    for (i = 0; i < 4; i++) {
        unsigned iv_reg = SCM_AES_CBC_IV0_REG + 4*i;

        status = read_scc_register(scc_fd, iv_reg, &value);
        if (!status) {
            printf("(%04X) SCM AES CBC IV%d Register:     0x%08x\n",
                   iv_reg, i, value);
        }
        else {
            had_error++;
            printf("(Could not get SCM AES CBC IV%d Register)\n", i);
        }
    }

    part_count = ((config & SCM_VER_NP_MASK) >> SCM_VER_NP_SHIFT) + 1;
    for (i = 0; i < part_count; i++) {
        unsigned smid_reg = SCM_SMID0_REG + 8*i;
        unsigned acc_reg = SCM_ACC0_REG + 8*i;
        uint32_t smid_value;
        uint32_t acc_value;

        status = read_scc_register(scc_fd, smid_reg, &smid_value);
        if (!status) {
            printf("(%04X) SCM SMID%d Register:     0x%08x\n",
                   smid_reg, i, smid_value);
        }
        else {
            had_error++;
            printf("(Could not get SCM SMID%d Register)\n", i);
        }

        status = read_scc_register(scc_fd, acc_reg, &acc_value);
        if (!status) {
            printf("(%04X) SCM ACC%d Register:     (0x%08x): ",
                   acc_reg, i, acc_value);
            print_scm_acc_register(acc_value);
        }
        else {
            had_error++;
            printf("(Could not get SCM ACC%d Register)\n", i);
        }
    }

    printf("\n");               /* Visually divide the halves */

    status = read_scc_register(scc_fd, SMN_STATUS_REG, &value);
    if (!status) {
        printf("(%04X) SMN Status Register            (0x%08x): ",
               SMN_STATUS_REG, value);
        print_smn_status_register(value);
    }
    else {
        printf("(Could not get SMN Status Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_SEQ_START_REG, &value);
    if (!status) {
        printf("(%04X) SMN Sequence Start Register:    0x%08x\n",
               SMN_SEQ_START_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Sequence Start Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_SEQ_END_REG, &value);
    if (!status) {
        printf("(%04X) SMN Sequence End Register:      0x%08x\n",
               SMN_SEQ_END_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Sequence End Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_SEQ_CHECK_REG, &value);
    if (!status) {
        printf("(%04X) SMN Sequence Check Register:    0x%08x\n",
               SMN_SEQ_CHECK_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Sequence Check Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_BB_CNT_REG, &value);
    if (!status) {
        printf("(%04X) SMN Bit Count Register:         0x%08x\n",
               SMN_BB_CNT_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Bit Count Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_BB_INC_REG, &value);
    if (!status) {
        printf("(%04X) SMN Bit Bank Inc Size Register: 0x%08x\n",
               SMN_BB_INC_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Bit Bank Inc Size Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_COMPARE_REG, &value);
    if (!status) {
        printf("(%04X) SMN Compare  Size Register:     0x%08x\n",
               SMN_COMPARE_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Compare Size Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_PT_CHK_REG, &value);
    if (!status) {
        printf("(%04X) SMN Plaintext  Check Register:  0x%08x\n",
               SMN_PT_CHK_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Plaintext Check Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_CT_CHK_REG, &value);
    if (!status) {
        printf("(%04X) SMN Ciphertext  Check Register: 0x%08x\n",
               SMN_CT_CHK_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Ciphertext Check Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_TIMER_IV_REG, &value);
    if (!status) {
        printf("(%04X) SMN Timer IV Register:          0x%08x\n",
               SMN_TIMER_IV_REG, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Timer IV Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_TIMER_CTL_REG, &value);
    if (!status) {
        printf("(%04X) SMN Timer Control Register     (0x%08x): ",
               SMN_TIMER_CTL_REG, value);
        print_smn_timer_control_register(value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Timer Control Register)\n");
    }

    status = read_scc_register(scc_fd, SMN_HAC_REG, &value);
    if (!status) {
        printf("(%04X) SMN High Assurance Control Register:   (0x%08x)\n",
               SMN_HAC_REG, value);
    }
    else {
        printf("(Could not get SMN High Assurance Control Register)\n");
        had_error++;
    }

    status = read_scc_register(scc_fd, SMN_REG_BANK_SIZE, &value);
    if (!status) {
        printf("(%04X) SMN Register Bank Size:            (0x%08x)\n",
               SMN_REG_BANK_SIZE, value);
    }
    else {
        had_error++;
        printf("(Could not get SMN Register Bank Size)\n");
    }

    return (had_error != 0);
}


/***********************************************************************
 set_software_alarm()
 **********************************************************************/
void set_software_alarm(int scc_fd) {
    int status;

    status = ioctl(scc_fd, SCC2_TEST_SET_ALARM, NULL);
}


/***********************************************************************
 run_timer_tests()
 **********************************************************************/
void run_timer_tests(int scc_fd, uint32_t timer_iv) {
    uint32_t value;
    int i;
    int a = 0;

    printf ("Writing 0x%x into SMN_TIMER_IV\n", timer_iv);
    write_scc_register(scc_fd, SMN_TIMER_IV_REG, timer_iv);

    /* this operation should move the initial value to the timer reg */
    printf ("Loading timer using SMN_TIMER_CONTROL\n");
    write_scc_register(scc_fd, SMN_TIMER_CTL_REG, SMN_TIMER_LOAD_TIMER);

    read_scc_register(scc_fd, SMN_TIMER_REG, &value);
    printf ("SMN_TIMER value: 0x%x\n", value);

    printf ("Starting timer using SMN_TIMER_CONTROL\n");
    write_scc_register(scc_fd, SMN_TIMER_CTL_REG, SMN_TIMER_START_TIMER);

    /*
     * Kill some time
     */
    for (i = 0; i < 100000; i++) {
        ++a;
    };
    i = 0;

    /* now stop the timer */
    printf("Stopping the timer\n");
    read_scc_register(scc_fd, SMN_TIMER_CTL_REG, &value);
    write_scc_register(scc_fd, SMN_TIMER_CTL_REG,
                       value & ~SMN_TIMER_STOP_MASK);

    /* and see how much time we ran off */
    read_scc_register(scc_fd, SMN_TIMER_REG, &value);
    printf ("SMN_TIMER value: 0x%x\n", value);

}

/***********************************************************************
 run_aic_tests()
 **********************************************************************/
int
run_aic_tests(int scc_fd) {
    uint32_t value;

    /* This series of writes generates FAIL mode !!! */
    write_scc_register(scc_fd, SMN_SEQ_START_REG, 4);
    write_scc_register(scc_fd, SMN_SEQ_END_REG, 5);
    write_scc_register(scc_fd, SMN_SEQ_CHECK_REG, 5);

    read_scc_register(scc_fd, SMN_STATUS_REG, &value);
    printf("SMN Status: ");
    print_smn_status_register(value);


    return 0;
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

    status = ioctl(device, SCC2_TEST_WRITE_REG, &register_access);
    if (status != IOCTL_SCC2_OK) {
        perror("Ioctl SCC2_TEST_WRITE_REG failed: ");
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
    status = ioctl(device, SCC2_TEST_READ_REG, &register_access);
    if (status != IOCTL_SCC2_OK) {
        perror("Ioctl SCC2_TEST_READ_REG failed: ");
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
            status = ioctl(scc_fd, SCC2_TEST_GET_CONFIGURATION, config);
            if (status != IOCTL_SCC2_OK) {
                perror("Ioctl SCC_GET_CONFIGURATION failed: ");
                free (config);
                config = NULL;
            }
        }
    }

    return config;
}


static uint8_t secret_data[64] = "All mimsy were the borogoves... All mimsy were the borogoves...";

/**
 * Do the encrypt-decrypt part of the partition/cipher tests
 */
int
do_enc_dec(int scc_fd, uint32_t part_virtual_addr)
{
    int status;
    scc_part_cipher_access cipher_info;
    uint8_t encrypted_secret[sizeof(secret_data)];
    uint8_t decrypted_secret[sizeof(secret_data)];

    /* Load our secret into the Red RAM */
    cipher_info.virt_address = part_virtual_addr;
    cipher_info.red_offset = 48;
    cipher_info.black_address = secret_data;
    cipher_info.size_bytes = sizeof(secret_data);
    status = ioctl(scc_fd, SCC2_TEST_LOAD_PART, &cipher_info);
    if (status != 0) {
        printf("ioctl to write to Red RAM returned %d\n", status);
        goto out;
    }

    /* Encrypt it into encrypted_secret */
    memset(encrypted_secret, 0, sizeof(encrypted_secret));
    cipher_info.iv = (uint64_t)0x12345678ull;
    cipher_info.black_address = encrypted_secret;
    status = ioctl(scc_fd, SCC2_TEST_ENCRYPT_PART, &cipher_info);
    if (status != 0) {
        printf("ioctl to encrypt region returned %d\n", status);
        goto out;
    }
    if (cipher_info.scc_status != SCC_RET_OK) {
        printf("scc_encrypt_region returned %d\n", status);
        status = 1;
        goto out;
    }

    /* Decrypt it into the Red RAM */
    cipher_info.red_offset = 16;
    status = ioctl(scc_fd, SCC2_TEST_DECRYPT_PART, &cipher_info);
    if (status != 0) {
        printf("ioctl to decrypt region returned %d\n", status);
        goto out;
    }
    if (cipher_info.scc_status != SCC_RET_OK) {
        printf("scc_decrypt_region returned %d\n", status);
        status = 1;
        goto out;
    }

    /* Read back our secret from the Red RAM */
    memset(decrypted_secret, 0, sizeof(decrypted_secret));
    cipher_info.black_address = decrypted_secret;
    status = ioctl(scc_fd, SCC2_TEST_READ_PART, &cipher_info);
    if (status != 0) {
        printf("ioctl to read from Red RAM returned %d\n", status);
        goto out;
    }

    if (memcmp(secret_data, decrypted_secret, sizeof(secret_data)) != 0) {
        printf("Decrypt did not retrieve plaintext data\n", status);
        status = 1;
        goto out;
    }

 out:
    return status;
}

/**
 * Run partition-level functions through their paces
 */
int
run_cipher_tests(int scc_fd)
{
    int i;
    int status;
    scc_partition_access part_info;
    scc_partition_access part_copy;
    uint32_t part_virt;
    uint8_t umid[16];

    part_info.virt_address = 0; /* signal for cleanup */
    part_info.smid = 0;

    memset(umid, 0, sizeof(umid));
    umid[sizeof(umid) - 1] = 0x42;

    printf("Beginning partition / cipher tests\n");

    status = ioctl(scc_fd, SCC2_TEST_ALLOC_PART, &part_info);
    if (status != IOCTL_SCC2_OK) {
        printf("ioctl for part alloc returned %d\n", status);
        goto cleanup;
    }
    if (part_info.scc_status != SCC_RET_OK) {
        printf("scc2_alloc_part() returned %d while trying to find partition\n", part_info.scc_status);
        status = 1;
        goto cleanup;
    }

    /* Now we need to engage the partition */
    part_info.umid = umid;
    part_info.permissions = 0x700; /* me, me, me */
    status = ioctl(scc_fd, SCC2_TEST_ENGAGE_PART, &part_info);
    if (status != IOCTL_SCC2_OK) {
        printf("ioctl for part engage returned %d\n", status);
        goto cleanup;
    }
    if (part_info.scc_status != SCC_RET_OK) {
        printf("scc2_engage_part() returned %d while trying to engage partition\n", part_info.scc_status);
        status = 1;
        goto cleanup;
    }

    part_copy = part_info;

    for(i = 0; i < 10; i++) {
        status = do_enc_dec(scc_fd, part_info.virt_address);
        if (status != IOCTL_SCC2_OK) {
            goto cleanup;
        }
    }

    status = ioctl(scc_fd, SCC2_TEST_RELEASE_PART, &part_info);
    if (status != IOCTL_SCC2_OK) {
        printf("ioctl for part release returned %d\n", status);
        goto cleanup;
    }
    if (part_info.scc_status != SCC_RET_OK) {
        printf("scc2_dealloc_part() returned %d while trying to deallocate partition\n", part_info.scc_status);
        status = 1;
        goto cleanup;
    }
    part_info.virt_address = 0; /* so cleanup knows it is gone */

    /* Try to deallocate it again... */
    status = ioctl(scc_fd, SCC2_TEST_RELEASE_PART, &part_copy);
    if (status != IOCTL_SCC2_OK) {
        printf("ioctl for second part release returned %d\n", status);
        goto cleanup;
    }
    if (part_copy.scc_status == SCC_RET_OK) {
        printf("scc2_dealloc_part() returned %d while trying second deallocate\n", part_copy.scc_status);
        status = 1;
        goto cleanup;
    }
    printf("Partition/cipher test is successful\n");

 cleanup:
    if (part_info.virt_address != 0) {
        int err_status = ioctl(scc_fd, SCC2_TEST_RELEASE_PART, &part_info);
        if (err_status != IOCTL_SCC2_OK) {
            printf("ioctl for part release returned %d while trying to cleanup\n", status);
        } else {
            if (part_info.scc_status != SCC_RET_OK) {
                printf("scc_dealloc_part() returned %d while trying to cleanup\n", part_info.scc_status);
            }
        }
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
    printf("Version %d %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
           (status & SMN_STATUS_VERSION_ID_MASK) >> SMN_STATUS_VERSION_ID_SHIFT,
           (status & SMN_STATUS_ILLEGAL_MASTER) ? "IllMaster " : "",
           (status & SMN_STATUS_SCAN_EXIT) ? "ScanExit " : "",
           (status & SMN_STATUS_PERIP_INIT) ? "PeripInit " : "",
           (status & SMN_STATUS_SMN_ERROR) ? "SMNError " : "",
           (status & SMN_STATUS_SOFTWARE_ALARM) ? "SWAlarm " : "",
           (status & SMN_STATUS_TIMER_ERROR) ? "TimerErr " : "",
           (status & SMN_STATUS_PC_ERROR) ? "PTCTErr " : "",
           (status & SMN_STATUS_BITBANK_ERROR) ? "BitbankErr " : "",
           (status & SMN_STATUS_ASC_ERROR) ? "ASCErr " : "",
           (status & SMN_STATUS_SECURITY_POLICY_ERROR) ? "SecPlcyErr " : "",
           (status & SMN_STATUS_SEC_VIO_ACTIVE_ERROR) ? "SecVioAct " : "",
           (status & SMN_STATUS_INTERNAL_BOOT) ? "IntBoot " : "",
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

static char* srs_names[] =
{
	"SRS_Reset",
	"SRS_All_Ready",
	"SRS_ZeroizeBusy",
	"SRS_CipherBusy",
	"SRS_AllBusy",
	"SRS_ZeroizeDoneCipherReady",
	"SRS_CipherDoneZeroizeReady",
	"SRS_ZeroizeDoneCipherBusy",
	"SRS_CipherDoneZeroizeBusy",
	"SRS_UNKNOWN_STATE_9",
	"SRS_TransitionalA",
	"SRS_TransitionalB",
	"SRS_TransitionalC",
	"SRS_TransitionalD",
	"SRS_AllDone",
	"SRS_UNKNOWN_STATE_E",
	"SRS_FAIL"
};


void
print_scm_version_register(uint32_t version)
{
    printf("Bpp: %u, Bpcb: %u, np: %u, maj: %u, min: %u\n",
           1 << ((version & SCM_VER_BPP_MASK) >> SCM_VER_BPP_SHIFT),
           ((version & SCM_VER_BPCB_MASK) >> SCM_VER_BPCB_SHIFT) + 1,
           ((version & SCM_VER_NP_MASK) >> SCM_VER_NP_SHIFT) + 1,
           (version & SCM_VER_MAJ_MASK) >> SCM_VER_MAJ_SHIFT,
           (version & SCM_VER_MIN_MASK) >> SCM_VER_MIN_SHIFT);
}


/** Interpret the SCM Status register and print out the 'on' bits */
void
print_scm_status_register(uint32_t status)
{
    printf("%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
           (status & SCM_STATUS_KST_DEFAULT_KEY) ? "KST_DefaultKey " : "",
           /* reserved */
           (status & SCM_STATUS_KST_WRONG_KEY) ? "KST_WrongKey " : "",
           (status & SCM_STATUS_KST_BAD_KEY) ? "KST_BadKey " : "",
           (status & SCM_STATUS_ERR) ? "Error " : "",
           (status & SCM_STATUS_MSS_FAIL) ? "MSS_FailState " : "",
           (status & SCM_STATUS_MSS_SEC) ? "MSS_SecureState " : "",
           (status & SCM_STATUS_RSS_FAIL) ? "RSS_FailState " : "",
           (status & SCM_STATUS_RSS_SEC) ? "RSS_SecureState " : "",
           (status & SCM_STATUS_RSS_INIT) ? "RSS_Initializing " : "",
           (status & SCM_STATUS_UNV) ? "UID_Invalid " : "",
           (status & SCM_STATUS_BIG) ? "BigEndian " : "",
           (status & SCM_STATUS_USK) ? "SecretKey " : "",
           srs_names[(status & SCM_STATUS_SRS_MASK) >> SCM_STATUS_SRS_SHIFT]);
}


/** Interpret the SCM Control register and print its meaning */
void
print_scm_control_register(uint32_t control)
{
    printf("TBD\n");
}

/**
 * Names of the SCM Error Codes
 */
static
char* scm_err_code[] =
{
	"Unknown_0",
	"UnknownAddress",
	"UnknownCommand",
	"ReadPermErr",
	"WritePermErr",
	"DMAErr",
	"EncBlockLenOvfl",
	"KeyNotEngaged",
	"ZeroizeCmdQOvfl",
	"CipherCmdQOvfl",
	"ProcessIntr",
	"WrongKey",
	"DeviceBusy",
	"DMAUnalignedAddr",
	"Unknown_E",
	"Unknown_F",
};

void
print_scm_error_status_register(uint32_t error)
{
    if (error == 0)
        printf("\n");
    else
        printf("%s%sErrorCode: %s, SMSState: %s, SCMState: %s\n",
               (error & SCM_ERRSTAT_ILM) ? "ILM, " : "",
               (error & SCM_ERRSTAT_SUP) ? "SUP, " : "",
               scm_err_code[(error & SCM_ERRSTAT_ERC_MASK) >> SCM_ERRSTAT_ERC_SHIFT],
               get_smn_state_name((error & SCM_ERRSTAT_SMS_MASK) >> SCM_ERRSTAT_SMS_SHIFT),
               srs_names[(error & SCM_ERRSTAT_SRS_MASK) >> SCM_ERRSTAT_SRS_SHIFT]);
}

void
print_scm_part_owner_register(uint32_t value)
{
    int i;

    for (i = 15; i >= 0; i--) {
        unsigned op = (value >> (SCM_POWN_SHIFT*i)) & SCM_POWN_MASK;
        switch (op) {
        case SCM_POWN_PART_FREE:
            break;
        case SCM_POWN_PART_UNUSABLE:
            printf("un%02d ", i);
            break;
        case SCM_POWN_PART_OTHER:
            printf("ot%02d ", i);
            break;
        case SCM_POWN_PART_OWNED:
            printf("rd%02d ", i);
            break;
        }
    }
    printf("\n");
}


void
print_scm_part_engaged_register(uint32_t value)
{
    printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
           (value & 0x8000) ? "15 " : "",
           (value & 0x4000) ? "14 " : "",
           (value & 0x2000) ? "13 " : "",
           (value & 0x1000) ? "12 " : "",
           (value & 0x0800) ? "11 " : "",
           (value & 0x0400) ? "10 " : "",
           (value & 0x0200) ? "9 " : "",
           (value & 0x0100) ? "8 " : "",
           (value & 0x0080) ? "7 " : "",
           (value & 0x0040) ? "6 " : "",
           (value & 0x0020) ? "5 " : "",
           (value & 0x0010) ? "4 " : "",
           (value & 0x0008) ? "3 " : "",
           (value & 0x0004) ? "2 " : "",
           (value & 0x0002) ? "1 " : "",
           (value & 0x0001) ? "0" : "");
}

void
print_scm_acc_register(uint32_t value)
{
    printf("%s%s%s%s%s%s%s%s%s%s%s\n",
           (value & FSL_PERM_NO_ZEROIZE) ? "NO_ZERO " : "",
           (value & FSL_PERM_TRUSTED_KEY_READ) ? "TRUSTED_KEY " : "",
           (value & FSL_PERM_HD_S) ? "SUP_DIS " : "",
           (value & FSL_PERM_HD_R) ? "HD_RD " : "",
           (value & FSL_PERM_HD_W) ? "HD_WR " : "",
           (value & FSL_PERM_HD_X) ? "HD_EX " : "",
           (value & FSL_PERM_TH_R) ? "TH_RD " : "",
           (value & FSL_PERM_TH_W) ? "TH_WR " : "",
           (value & FSL_PERM_OT_R) ? "OT_RD " : "",
           (value & FSL_PERM_OT_W) ? "OT_WR " : "",
           (value & FSL_PERM_OT_X) ? "OT_EX" : "");
}

void
print_smn_command_register(uint32_t command)
{
    printf("TBD\n");
#if 0
    if (command&SMN_COMMAND_ZEROS_MASK) {
        printf(" zeroes: 0x%08x", debug&SMN_COMMAND_ZEROS_MASK);
    }
    printf("%s%s%s%s\n",
           (command&SMN_COMMAND_CLEAR_INTERRUPT) ? " CLEAR_INTERRUPT" : "",
           (command&SMN_COMMAND_CLEAR_BIT_BANK) ? " CLEAR_BITBANK" : "",
           (command&SMN_COMMAND_ENABLE_INTERRUPT) ? " ENABLE_INTERRUPT" : "",
           (command&SMN_COMMAND_SET_SOFTWARE_ALARM) ? " SET_SOFWARE_ALARM" : ""
           );
#endif
}


void
print_smn_timer_control_register(uint32_t control)
{
    printf("%s\n",
           (control&SMN_TIMER_START_TIMER) ? "STARTED" : "");
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
           (debug & 1) ? " SV1" : "",
           (debug & 2) ? " SV2" : "",
           (debug & 4) ? " SV3" : "",
           (debug & 8) ? " SV4" : "",
           (debug & 16) ? " SV5" : "",
           (debug & 32) ? " SV6" : "",
           (debug & 64) ? " LSV7" : "",
           (debug & 128) ? " LSV8" : "",
           (debug & 256) ? " LSV9" : "",
           (debug & 512) ? " LSV10" : "",
           (debug & 1024) ? " LSV11" : "",
           (debug & 2048) ?  " LSV12" : "");
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
