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
 * @file rng_test.c
 * This is a reference test program for the RNG block.
 *
 * @section rngintro Introduction
 * The RNG Block Guide (PISA-RNGA_32IP-BG 1.3) is the basis for creating this
 * test program and the driver which it uses.

 * - Reading/writing any accessible register in the RNG.
 * - Reading arbitrary amounts of  random data from the Output FIFO.
 * - Writing 'extra' Entropy values to the RNG.

 * @section rngnosupport Features Not Demonstrated
 * - Verification scenarios.  These must be done through Register reads/writes,
 *   as above.
 * - Sleeping/Waking the RNG.  Can be done through Register read/write.

 * @section rngimpl Implementation split
 * This section will describe out the implementation of the various features
 * is split between this test program and the reference driver.

 * @subsection rngrw Reading & writing accessible registers of the RNG.

 * @subsection rngentropy Adding Entropy to the RNG
 *
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <getopt.h>

#include <inttypes.h>
#include "rng_test_driver.h"
#include "apihelp.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static int use_kernel_api = FALSE;

/* test routines */
void displayConfiguration(int device);
void checkSafeRegisters(int device);
void rngGetEntropy(int device, int byte_count);
void rngGetUserEntropy(fsl_shw_uco_t * user_context, int byte_count);
void rngAddEntropy(int device, uint32_t entropy);
void rngAddUserEntropy(fsl_shw_uco_t * user_context, uint32_t entropy);

/* utility print functions */
void printEntropy(const unsigned char *data, size_t count);
void printRngStatusRegister(const uint32_t status);
void printRngControlRegister(const uint32_t status);

/* utility register access functions */
int writeRngRegister(int device, uint32_t address, uint32_t value);
int readRngRegister(int device, uint32_t address, uint32_t * value);

fsl_shw_uco_t context;
int rngTestFD;			/* The RNG Test device */
char *rngTestDevicePath = "/dev/rng_test";	/* the test driver */

static void setup_test_fd()
{
	static unsigned inited = 0;

	if (!inited) {
		/* Open up the RNG device */
		rngTestFD = open(rngTestDevicePath, O_RDWR);
		if (rngTestFD < 0) {
			perror("Cannot open RNG test device");
			fsl_shw_deregister_user(&context);
			exit(1);
		}
		inited = 1;
	}
}

static void setup_user_ctx()
{
	fsl_shw_return_t code;
	static unsigned inited = 0;

	if (!inited) {
		/* Open up the RNG device */
		fsl_shw_uco_init(&context, 10);
		code = fsl_shw_register_user(&context);
		if (code != FSL_RETURN_OK_S) {
			printf
			    ("fsl_shw_register_user() failed with error: %s\n",
			     fsl_error_string(code));
			exit(1);
		}
		inited = 1;
	}
}

/*!***************************************************************************
 *  main
 ****************************************************************************/
int main(int argCount,		/* # command-line arguments */
	 char *argList[])
{				/* pointers to command-line arguments */
	/* Declare and initialize variables */
	int argumentSwitch;
	char *testsToRun = "csmtes";	/* default list of tests to be run in order */
	extern char *optarg;

	/* Process command line arguments - until we come up empty */
	while ((argumentSwitch = getopt(argCount, argList, "E:O:R:SW:Z:"))
	       != EOF) {
		switch (argumentSwitch) {
		case 'E':	/* Get n bytes of Entropy */
			if (use_kernel_api) {
				setup_test_fd();
				rngGetEntropy(rngTestFD, atoi(optarg));
			} else {
				setup_user_ctx();
				rngGetUserEntropy(&context, atoi(optarg));
			}
			break;
		case 'O':	/* Set options */
			switch (*optarg) {
			case 'b':	/* set O_NONBLOCK flag on read/write request */
				break;
			case 'k':	/* Use kernel API for read/write request */
				use_kernel_api = TRUE;
				break;
			default:
				fprintf(stderr, "unknown option %c\n", *optarg);
			}
			break;
		case 'R':{	/* Read a specific register */
				unsigned int offset;
				uint32_t value;
				int err;

				/* get the offset */
				if (sscanf(optarg, "%x", &offset) != 1) {
					fprintf(stderr,
						"improper use of -R %s\n",
						optarg);
				} else {
					setup_test_fd();
					err =
					    readRngRegister(rngTestFD, offset,
							    &value);
					if (!err) {
						printf("0x%05x: %08x\n", offset,
						       value);
					} else {
						perror("Reading register: ");
					}
				}
			}
			testsToRun = "";
			break;
		case 'S':{	/* Print 'Safe' registers */
				uint32_t control;
				uint32_t status;

				setup_test_fd();

#ifdef FSL_HAVE_RNGA
				readRngRegister(rngTestFD, RNGA_STATUS,
						&status);
				readRngRegister(rngTestFD, RNGA_CONTROL,
						&control);
#else
				readRngRegister(rngTestFD, RNGC_STATUS,
						&status);
				readRngRegister(rngTestFD, RNGC_CONTROL,
						&control);
#endif
				printRngStatusRegister(status);
				printRngControlRegister(control);

				break;
			}
		case 'W':	/* write a specific register */
			{
				unsigned int offset;
				unsigned int value;
				int err;

				if (sscanf(optarg, "%x:%x", &offset, &value) !=
				    2) {
					fprintf(stderr,
						"improper use of -W (%s)\n",
						optarg);
				} else {
					setup_test_fd();
					err =
					    writeRngRegister(rngTestFD, offset,
							     value);
					if (!err) {
						err =
						    readRngRegister(rngTestFD,
								    offset,
								    &value);
						if (!err) {
							printf
							    ("0x%05x => %08x\n",
							     offset, value);
						} else {
							perror
							    ("Reading register after write: ");
						}
					} else {
						perror("Writing register: ");
					}
				}
			}
			testsToRun = "";
			break;
		case 'Z':
			{
				uint32_t more_entropy;

				if (sscanf(optarg, "%x", &more_entropy) != 1) {
					fprintf(stderr,
						"improper use of -Z (%s)\n",
						optarg);
				} else if (use_kernel_api) {
					setup_test_fd();
					rngAddEntropy(rngTestFD, more_entropy);
				} else {
					setup_user_ctx();
					rngAddUserEntropy(&context,
							  more_entropy);
				}
			}
			break;
		default:
			fprintf(stderr,
				"Usage: rng_test [-E entropybytes] -Zval"
				" [-Oswitch] [-S] [-Wreg:val] [-Rreg]]\n");
			exit(1);
			break;
		}
	}
	exit(0);
}				/* main */

int readRngRegister(int device, uint32_t address, uint32_t * value)
{
	rng_test_reg_access_t ra;
	int errorCode = 0;

	ra.reg_offset = address;
	errorCode = ioctl(device, RNG_TEST_READ_REG, &ra);
	if (errorCode < 0) {
		perror("Could not read register: ");
		return errno;
	} else {
		*value = ra.reg_data;
	}

	return errorCode;
}

int writeRngRegister(int device, uint32_t address, uint32_t value)
{
	rng_test_reg_access_t ra;
	int errorCode;

	ra.reg_offset = address;
	ra.reg_data = value;
	errorCode = ioctl(device, RNG_TEST_WRITE_REG, &ra);
	if (errorCode < 0) {
		perror("Could not write register: ");
		return errno;
	}

	return 0;
}

void rngGetEntropy(int device, int byte_count)
{
	unsigned char *entropy;
	ssize_t error_code = -EIO;

	if (byte_count < 0) {
		fprintf(stderr, "Byte count of %d not accepted\n", byte_count);
	}

	entropy = calloc(byte_count, sizeof(uint8_t));

	if (entropy != NULL) {
		rng_test_get_random_t ge;

		ge.count_bytes = byte_count;
		ge.random = entropy;
		ge.function_return_code = -1;	/* something invalid */

		printf("Getting random from RNG TEST driver\n");
		error_code = ioctl(device, RNG_TEST_GET_RANDOM, &ge);
		if (error_code >= 0) {
			if (ge.function_return_code != FSL_RETURN_OK_S) {
				fprintf(stderr, "Could not read entropy: %s\n",
					fsl_error_string(ge.
							 function_return_code));
			} else {
				printEntropy(entropy, ge.count_bytes);
			}
		} else {
			perror("ioctl() to test device failed.");
		}

		free(entropy);
	} else {
		fprintf(stderr, "Could not get enough memory for entropy\n");
	}
}

void rngGetUserEntropy(fsl_shw_uco_t * ctx, int byte_count)
{
	unsigned char *entropy;
	fsl_shw_return_t code;

	if (byte_count < 0) {
		fprintf(stderr, "Byte count of %d not accepted\n", byte_count);
	}

	entropy = calloc(byte_count, sizeof(uint8_t));

	if (entropy != NULL) {

		code = fsl_shw_get_random(ctx, byte_count, entropy);
		if (code == FSL_RETURN_OK_S) {
			printEntropy(entropy, byte_count);
		} else {
			fprintf(stderr, "Could not read entropy: %s\n",
				fsl_error_string(code));
		}
		free(entropy);
	} else {
		fprintf(stderr, "Could not get enough memory for entropy\n");
	}
}

void rngAddEntropy(int device, uint32_t entropy)
{
	rng_test_add_entropy_t ae;
	int error_code;

	ae.count_bytes = sizeof(entropy);
	ae.entropy = (uint8_t *) & entropy;
	error_code = ioctl(device, RNG_TEST_ADD_ENTROPY, &ae);
	if (error_code != 0) {
		fprintf(stderr, "rng error: %d", ae.function_return_code);
	}
}

void rngAddUserEntropy(fsl_shw_uco_t * ctx, uint32_t entropy)
{
	fsl_shw_return_t code;

	code = fsl_shw_add_entropy(ctx, sizeof(entropy), (uint8_t *) & entropy);
	if (code != FSL_RETURN_OK_S) {
		fprintf(stderr, "Could not read entropy: %s\n",
			fsl_error_string(code));
	}
}

void printEntropy(const unsigned char *data, size_t count)
{
	const unsigned int valuesPerLine = 20;

	unsigned int i;
	for (i = 0; i < count; i++) {
		printf("%02x ", *data++);
		if (i % valuesPerLine == valuesPerLine - 1) {
			printf("\n");
		}
	}

	/* if loop didn't end with newline, gen one now */
	if (count % valuesPerLine) {
		printf("\n");
	}
}

#ifdef FSL_HAVE_RNGA
void printRngStatusRegister(const uint32_t status)
{
	printf("RNGA Status: 0x%08x: ", status);
	if (status & RNGA_STATUS_OSCILLATOR_DEAD) {
		printf("OscillatorDead, ");
	}
	if (status & RNGA_STATUS_ZEROS1_MASK) {
		printf("zeros1=0x%x, ", status & RNGA_STATUS_ZEROS1_MASK);
	}
	if (status & RNGA_STATUS_ZEROS2_MASK) {
		printf("zeros2=0x%x, ", status & RNGA_STATUS_ZEROS2_MASK);
	}
	if (status & RNGA_STATUS_SLEEP) {
		printf("Sleeping, ");
	}
	if (status & RNGA_STATUS_ERROR_INTERRUPT) {
		printf("ErrorInterrupt, ");
	}
	if (status & RNGA_STATUS_FIFO_UNDERFLOW) {
		printf("FifoUnderflow, ");
	}
	if (status & RNGA_STATUS_LAST_READ_STATUS) {
		printf("LastReadStatus, ");
	}
	if (status & RNGA_STATUS_SECURITY_VIOLATION) {
		printf("SecurityViolation, ");
	}
	printf("fifo size: %d, fifo level: %d\n",
	       (status & RNGA_STATUS_OUTPUT_FIFO_SIZE_MASK) >>
	       RNGA_STATUS_OUTPUT_FIFO_SIZE_SHIFT,
	       (status & RNGA_STATUS_OUTPUT_FIFO_LEVEL_MASK) >>
	       RNGA_STATUS_OUTPUT_FIFO_LEVEL_SHIFT);
}

void printRngControlRegister(const uint32_t control)
{
	printf("RNGA Control: 0x%08x: ", control);
	if (control & RNGA_CONTROL_ZEROS_MASK) {
		printf("zeros: 0x%x, ", control & RNGA_CONTROL_ZEROS_MASK);
	}
	if (control & RNGA_CONTROL_SLEEP) {
		printf("sleep, ");
	}
	if (control & RNGA_CONTROL_CLEAR_INTERRUPT) {
		printf("ClearInterrupt!, ");
	}
	if (control & RNGA_CONTROL_MASK_INTERRUPTS) {
		printf("InterruptsMasked, ");
	}
	if (control & RNGA_CONTROL_HIGH_ASSURANCE) {
		printf("HighAssurance, ");
	}
	if (control & RNGA_CONTROL_GO) {
		printf("Go");
	}
	printf("\n");
}

#else				/* RNGC ... */

typedef struct registerNames_t
{
    uint32_t code;
    char* name;
} registerNames_t;

registerNames_t RNGCStatusNames[] =
    {{RNGC_STATUS_SEC_STATE,        "SecureState"},
     {RNGC_STATUS_BUSY,             "Busy"},
     {RNGC_STATUS_SLEEP,            "Sleeping"},
     {RNGC_STATUS_RESEED,           "ReseedRequired"},
     {RNGC_STATUS_ST_DONE,          "SelfTestDone"},
     {RNGC_STATUS_SEED_DONE,        "SeedDone"},
     {RNGC_STATUS_NEXT_SEED_DONE,   "NextSeedDone"},
     {RNGC_STATUS_ERROR,            "Error"},
     {RNGC_STATUS_ST_PF_PRNG,       "PRNGSelfTestFail"},
     {RNGC_STATUS_ST_PF_TRNG,       "TRNGSelfTestFail"}};

#define RNGCSTATUSNAMESLENGTH                                               \
    sizeof(RNGCStatusNames)/sizeof(registerNames_t)

void printRngStatusRegister(const uint32_t status)
{
    uint32_t i;
	printf("RNG Status: 0x%08x: ", status);

    for (i = 0; i < RNGCSTATUSNAMESLENGTH; i++) {
        if (status & RNGCStatusNames[i].code) {
            printf("%s, ", RNGCStatusNames[i].name);
        }
    }
    printf("\n");

	printf("SelfTest: 0x%02x\n",
	       (status & RNGC_STATUS_STAT_TEST_PF_MASK) >>
	       RNGC_STATUS_STAT_TEST_PF_SHIFT);
	printf("fifo level: %d, fifo size: %d\n",
	       (status & RNGC_STATUS_FIFO_LEVEL_MASK) >>
	       RNGC_STATUS_FIFO_LEVEL_SHIFT,
	       (status & RNGC_STATUS_FIFO_SIZE_MASK) >>
	       RNGC_STATUS_FIFO_SIZE_SHIFT);

}

registerNames_t RNGCControlNames[] =
    {{RNGC_CONTROL_AUTO_SEED,       "AutoSeed"},
     {RNGC_CONTROL_MASK_DONE,       "MaskDone"},
     {RNGC_CONTROL_MASK_ERROR,      "MaskError"},
     {RNGC_CONTROL_VERIF_MODE,      "VerificationMode"},
     {RNGC_CONTROL_CTL_ACC,         "ControlAccessMode"}};

#define RNGCCONTROLNAMESLENGTH                                              \
    sizeof(RNGCStatusNames)/sizeof(registerNames_t)

void printRngControlRegister(const uint32_t control)
{
    uint32_t i;
	printf("RNG Control: 0x%08x: ", control);

    for (i = 0; i < RNGCCONTROLNAMESLENGTH; i++) {
        if (control & RNGCControlNames[i].code) {
            printf("%s, ", RNGCControlNames[i].name);
        }
    }
    printf("\n");
}

#endif				/* RNGA or RNGC */
