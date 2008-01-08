/*
 * Copyright (C) 2005, Freescale Semiconductor, Inc. All Rights Reserved
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * FREESCALE SEMICONDUCTOR, INC.
 *
 * Test app for MPEG4 decode with Post-filter and Post-processing
 *
 */

#define PROFILE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <linux/videodev.h>

#include <asm/arch/mxc_pf.h>
#include <asm/arch/mxcfb.h>
#include "mpeg4_dec_api.h"


#define BIT_BUFFER_SIZE   2048
#define ERROR               -1
#define SUCCESS              0

/*!
*******************************************************************************
*   Structures
*******************************************************************************
*/

typedef struct
{
    int      s32Id;
    int      s32NumBytesRead;
    char     *ps8EncStrFname;
    char     *ps8DecStrFname;
    FILE     *fpInput;
} sIOFileParams;


/*!
*******************************************************************************
*   Function Declarations
*******************************************************************************
*/
eMpeg4DecRetType eMPEG4DecoderLoop (sMpeg4DecObject *psMpeg4DecObject);
int cbkMPEG4DBufRead (int s32EncStrBufLen, unsigned char *pu8EncStrBuf,
                      int s32Offset, void *pvAppContext);
int s32AllocateMem4Decoder (sMpeg4DecMemAllocInfo *psMemAllocInfo);
void vFreeMemAllocated4Dec (sMpeg4DecMemAllocInfo *psMemAllocInfo);
void vFreeMemAllocated4App (sMpeg4DecObject* psMpeg4DecObject);

static int pf_init(uint32_t width, uint32_t height, uint32_t stride);
static int pf_uninit(void);
static int pf_start(int in_buf_idx, unsigned long out_buf);
static int pf_wait(void);

static int pp_init(uint32_t width, uint32_t height);
static int pp_start(struct v4l2_buffer * buf);
void pp_uninit(void);

int fd_pf;
int fd_v4l;
int g_loop_count;
int g_rotate;
int g_output = 3;
int g_frame_size;
int g_num_buffers;
char * g_pf_inbuf[2];
char * g_pf_qp_buf;
int g_pf_qp_buf_size;
struct v4l2_buffer g_buf;
int g_display_top = 0;
int g_display_left = 0;
int g_display_width = 176;
int g_display_height = 144;

pf_buf g_in_buf[2];
//static int n = 0;

void
PrintUsage (void)
{
    printf ("\n************************************************************\n");
    printf ("Usage: mpeg4_pf_test [OPTIONS] <EncStrFilename>\n");
    printf ("* <out_kev> is the output kev file name, default is test_kev.\n");
    printf ("* For OPTIONS, see bellow\n");
    printf ("* Frames can be decoded in 2 mutually exclusive modes.\n");
    printf ("* \n");
    printf ("* MODE 1:\n");
    printf ("* -------\n");
    printf ("* None of the options are present or -d option is present.\n");
    printf ("* -d may or may not be followed by any -sk option.\n");
    printf ("* \n");
    printf ("* Example: \"-d end -sk 20 10\" , \"-d 30 -sk 20 10\"\n");
    printf ("* For \"-d end -sk 20 10\" option, Decoder will decode till\n");
    printf ("* end and will skip 20 frames starting from Frame No. 10\n");
    printf ("* \n");
    printf ("* DECODE_OPTIONS :\n");
    printf ("*                   -d  DecodeNumFrames \n");
    printf ("*                   -d  end \n");
    printf ("*                   -sk SkipNumFrames StartSkipFrameNum \n");
    printf ("*                   -sk I StartSkipFrameNum \n");
    printf ("*                   -ss \n");
    printf ("* \n");
    printf ("* DecodeNumFrames   - Number of Frames to decode \n");
    printf ("* SkipNumFrames     - Number of Frames to skip \n");
    printf ("* StartSkipFrameNum - Start skipping from this Frame\n");
    printf ("* -sk I             - Skip to Next I Frame \n");
    printf ("* -ss               - Single Step \n");
    printf ("************************************************************\n\n");
}

unsigned char         *pu8BitBuffer = NULL;

int process_cmdline(int argc, char **argv)
{
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-ow") == 0) {
                        g_display_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-oh") == 0) {
                        g_display_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ot") == 0) {
                        g_display_top = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-ol") == 0) {
                        g_display_left = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-d") == 0) {
                        g_output = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-r") == 0) {
                        g_rotate = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-l") == 0) {
                        g_loop_count = atoi(argv[++i]);
                }
        }

        return 0;
}

int main (int argc, char *argv[])
{
    sMpeg4DecObject       *psMpeg4DecObject = NULL;
    eMpeg4DecRetType       eDecRetVal = E_MPEG4D_FAILURE;
    sMpeg4DecMemAllocInfo *psMemAllocInfo;
    sIOFileParams          sIOFileParamsObj;
    int                    s32BitBufferLen = 0;
    int                    s32BytesRead = 0;

    if (process_cmdline(argc, argv) < 0) {
            printf("MXC Video4Linux Output Device Test\n\n" \
                   "Syntax: mxc_v4l2_output.out\n -d <output display #>\n" \
                   "-iw <input width> -ih <input height>\n [-f <WBMP|fourcc code>]" \
                   "[-ow <display width>\n -oh <display height>]\n" \
                   "[-fr <frame rate (fps)>] [-r <rotate mode 0-7>] [-l <loop count>]\n" \
                   "<input YUV file>\n\n");
            return -1;
    }


    sIOFileParamsObj.s32NumBytesRead = 0;


    sIOFileParamsObj.ps8EncStrFname = argv[argc-1];

    if ( (sIOFileParamsObj.fpInput = fopen (sIOFileParamsObj.ps8EncStrFname,
                                            "rb")) == NULL)
    {
        printf ("Error : Unable to open Input Bitstream file \"%s\" \n",
                sIOFileParamsObj.ps8EncStrFname);

        return ERROR;
    }


    psMpeg4DecObject = (sMpeg4DecObject *) malloc (sizeof (sMpeg4DecObject));
    if (psMpeg4DecObject == NULL)
    {
        printf ("Unable to allocate memory for Mpeg4 Decoder structure\n");

        goto cleanup00;
    }
    psMpeg4DecObject->pvAppContext = (void *) &sIOFileParamsObj;


    /*!
    *   Calling Query Mem Function
    */
    s32BitBufferLen = BIT_BUFFER_SIZE;
    pu8BitBuffer = (unsigned char *) malloc (sizeof(unsigned char) *
                                             s32BitBufferLen);
    if (pu8BitBuffer == NULL)
    {
        printf ("Unable to allocate memory for BitBuffer\n");

        /*! Freeing Memory allocated by the Application */
        vFreeMemAllocated4App (psMpeg4DecObject);
        goto cleanup0;
    }

    s32BytesRead = fread (pu8BitBuffer, sizeof(unsigned char), s32BitBufferLen,
                          sIOFileParamsObj.fpInput);
    sIOFileParamsObj.s32NumBytesRead += s32BytesRead;

    if (s32BytesRead < s32BitBufferLen)
    {
       /*! Let Decoder handle the Error */
       printf ("ERROR - Insufficient bytes in encoded Bitstream file \"%s\""
              "Bytes Read=%d\n", sIOFileParamsObj.ps8EncStrFname, s32BytesRead);
    }

    sIOFileParamsObj.s32NumBytesRead = 0;

    eDecRetVal =  eMPEG4DQuerymem (psMpeg4DecObject, pu8BitBuffer,
                                   s32BytesRead);
    if (eDecRetVal != E_MPEG4D_SUCCESS)
    {
        printf ("Function eMPEG4DQuerymem() resulted in failure\n");
        printf ("MPEG4D Error Type : %d\n", eDecRetVal);

        /*! Freeing Memory allocated by the Application */
        vFreeMemAllocated4App (psMpeg4DecObject);
        goto cleanup0;
    }

    printf("Calling PF init\n");
    pf_init(psMpeg4DecObject->sDecParam.u16FrameWidth,
         psMpeg4DecObject->sDecParam.u16FrameHeight,
         psMpeg4DecObject->sDecParam.u16FrameWidth);

        printf("Initializing Post Processing\n");
        pp_init(psMpeg4DecObject->sDecParam.u16FrameWidth,
            psMpeg4DecObject->sDecParam.u16FrameHeight);

        fb_setup();

    /*!
    *   Allocating Memory for Output Buffer
    */
#ifdef PF_TEST
    g_in_buf[0].y_offset = g_in_buf[1].y_offset = 0x100;
    g_in_buf[0].u_offset = g_in_buf[1].u_offset = g_in_buf[0].y_offset + psMpeg4DecObject->sDecParam.sOutputBuffer.s32YBufLen + 0x100;
    g_in_buf[0].v_offset = g_in_buf[1].v_offset = g_in_buf[0].u_offset + psMpeg4DecObject->sDecParam.sOutputBuffer.s32CbBufLen + 0x100;

    psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf = g_pf_inbuf[0] + g_in_buf[0].y_offset;
    psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CbBuf =
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf + g_in_buf[0].u_offset;
    psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CrBuf =
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf + g_in_buf[0].v_offset;
#else
    g_in_buf[0].y_offset = g_in_buf[1].y_offset = 0;
    g_in_buf[0].u_offset = g_in_buf[1].u_offset = 0;
    g_in_buf[0].v_offset = g_in_buf[1].v_offset = 0;

    psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf = g_pf_inbuf[0];
    psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CbBuf =
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf + psMpeg4DecObject->sDecParam.sOutputBuffer.s32YBufLen;
    psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CrBuf =
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CbBuf + psMpeg4DecObject->sDecParam.sOutputBuffer.s32CbBufLen;
#endif


    /* Allocate memory to hold the quant values, make sure that we round it
     * up in the higher side, as non-multiple of 16 will be extended to
     * next 16 bits value
     */
    psMpeg4DecObject->sDecParam.p8MbQuants = (signed char*)
        malloc (((psMpeg4DecObject->sDecParam.u16FrameWidth +15) >> 4) *
                ((psMpeg4DecObject->sDecParam.u16FrameHeight +15) >> 4) *
                sizeof (signed char));

    if (psMpeg4DecObject->sDecParam.p8MbQuants == NULL)
    {
        printf ("Unable to allocate memory for quant values\n");
        vFreeMemAllocated4App (psMpeg4DecObject);
        goto cleanup0;
    }

    /*!
    *   Allocating Memory for MPEG4 Decoder
    */
    psMemAllocInfo = &(psMpeg4DecObject->sMemInfo);
    if (s32AllocateMem4Decoder (psMemAllocInfo) == ERROR)
    {
        printf ("Unable to allocate memory for Mpeg4 Decoder\n");

        /*! Freeing Memory allocated by the Application */
        vFreeMemAllocated4App (psMpeg4DecObject);
        goto cleanup0;
    }


    /*!
    *   Calling MPEG4 Decoder Init Function
    */
    eDecRetVal = eMPEG4DInit (psMpeg4DecObject);


    if ((eDecRetVal != E_MPEG4D_SUCCESS) &&
        (eDecRetVal != E_MPEG4D_ENDOF_BITSTREAM))
    {
        printf ("eMPEG4DInit() failed with return value %d\n", eDecRetVal);

        /*!  Freeing Memory allocated by the Application */
        vFreeMemAllocated4App (psMpeg4DecObject);
        goto cleanup0;
    }

   /*!
    *   Calling MPEG4 Decoder Function
    */
    eDecRetVal = eMPEG4DecoderLoop (psMpeg4DecObject);

        pf_uninit();
        pp_uninit();

    if (eDecRetVal == E_MPEG4D_ENDOF_BITSTREAM)
    {
        printf ("Completed decoding the bit stream\n");
    }
    else if (eDecRetVal != E_MPEG4D_SUCCESS)
    {
        printf ("Function eMPEG4DecoderLoop() failed with %d\n", eDecRetVal);

        /*!  Freeing Memory allocated by the Application */
        vFreeMemAllocated4App (psMpeg4DecObject);
        goto cleanup0;
    }


    /*!
    *   Freeing resources used by MPEG4 Decoder
    */
    /* When does Decoder allocate its internal memory.
       Depending on whether its done at Init() stage /
       Query_Mem() state/ Decode() state, i will have
       call this function to synchronise with the
       allocation point
    */
    eDecRetVal = eMPEG4DFree (psMpeg4DecObject);

    if ((eDecRetVal != E_MPEG4D_SUCCESS) &&
        (eDecRetVal != E_MPEG4D_ENDOF_BITSTREAM))
    {
        printf ("Function eMPEG4DFree() failed with %d\n", eDecRetVal);

        /*! Freeing Memory allocated by the Application */
        vFreeMemAllocated4App (psMpeg4DecObject);
        goto cleanup0;
    }

    /*! Freeing Memory allocated by the Application */
    vFreeMemAllocated4App (psMpeg4DecObject);

cleanup0:
    free (pu8BitBuffer);
cleanup00:
    return eDecRetVal;

}



/*!
*******************************************************************************
*    Function Definitions
*******************************************************************************
*/

/*!
*******************************************************************************
* Description: It decodes the encoded bitstream. This function calls Decode
*              and Skip API's for decoding and skipping frames. It supports
*              different combinations of skipping and decoding frames.
*              Description of these combinations is provided in the Usage
*              section. It exemplifies how Decode and Skip API's can be used.
*              User may want to use these according to the design of the
*              application.
*
* \param[in]   psMpeg4DecObject - is a pointer to Mpeg4 Decoder
*                                 Configuration structure.
* \param[in]   psDecArgs        - is a pointer to a structure which holds
*                                 Frame no's for Decoding and Skipping Frames
*
* \return      eMpeg4DecRetType, returns the Decoder Status
* \remarks
*              Global Variables: None
*
* \attention   Range Issues: None
* \attention   Special Issues: None
*
*/

eMpeg4DecRetType eMPEG4DecoderLoop (sMpeg4DecObject* psMpeg4DecObject)
{
        int cur_buf = 0;
        struct v4l2_buffer v4lbuf;

#ifdef PROFILE
        FILE * flog;
        struct timespec temp_time, dec_time, pp_time, pf_time, time1, time2, time3, time4
                ;
        unsigned long time, pf_temp_time;;
#endif
        struct timeval tv_start, tv_end;
        uint64_t total_time;

        eMpeg4DecRetType   eDecRetVal = E_MPEG4D_FAILURE;
        unsigned long        frame_count = 0;        /*!< Decoded Frame Num */
        sMpeg4DecoderParams* pDecPar = &psMpeg4DecObject->sDecParam;
        char*                hw_qp;
        char*                quant = pDecPar->p8MbQuants;
        unsigned short       row, col;

        memset(&v4lbuf, 0, sizeof(struct v4l2_buffer));

#ifdef PROFILE
        dec_time.tv_nsec = 0;
        dec_time.tv_sec = 0;
        pf_time.tv_nsec = 0;
        pf_time.tv_sec = 0;
        pp_time.tv_nsec = 0;
        pp_time.tv_sec = 0;

        if ((flog = fopen("/tmp/mpeg4log", "w")) == NULL) {
                printf("Unable to open log file\n");
        }

        clock_gettime(CLOCK_REALTIME, &time1);
#endif
        gettimeofday(&tv_start, 0);

        // Decode 1st frame
        eDecRetVal = eMPEG4DDecode (psMpeg4DecObject);
        if (eDecRetVal != E_MPEG4D_SUCCESS) {
#ifdef PROFILE
                fclose(flog);
#endif
                return eDecRetVal;
        }
        ++frame_count;

#ifdef PROFILE
        clock_gettime(CLOCK_REALTIME, &time4);
        temp_time.tv_sec = time4.tv_sec - time1.tv_sec;
        temp_time.tv_nsec = time4.tv_nsec - time1.tv_nsec;
        dec_time.tv_sec += temp_time.tv_sec;
        dec_time.tv_nsec += temp_time.tv_nsec;
        time = (temp_time.tv_sec * 1000000) + (temp_time.tv_nsec / 1000);
        fprintf(flog, "dec\t= %ld us\n", time);
#endif
        while (1)
        {
                if (g_num_buffers-- > 0) {
                        v4lbuf.index = g_num_buffers;
                        v4lbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                        v4lbuf.memory = V4L2_MEMORY_MMAP;
                        if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &v4lbuf) < 0)
                        {
                                printf("VIDIOC_QUERYBUF failed\n");
                                break;
                        }
                }
                else {
                        // DQBUF here.
                        v4lbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                        v4lbuf.memory = V4L2_MEMORY_MMAP;
                        if (ioctl(fd_v4l, VIDIOC_DQBUF, &v4lbuf) < 0)
                        {
                                printf("VIDIOC_DQBUF failed\n");
                                break;
                        }
                }
#ifdef PROFILE
                clock_gettime(CLOCK_REALTIME, &time1);
                temp_time.tv_sec = time1.tv_sec - time4.tv_sec;
                temp_time.tv_nsec = time1.tv_nsec - time4.tv_nsec;
                pp_time.tv_sec += temp_time.tv_sec;
                pp_time.tv_nsec += temp_time.tv_nsec;
                time = (temp_time.tv_sec * 1000000) + (temp_time.tv_nsec / 1000);
                fprintf(flog, "pp\t= %ld us\n", time);
#endif

                hw_qp = g_pf_qp_buf;
                quant = pDecPar->p8MbQuants;
                for (row  = 0; row < (pDecPar->u16FrameHeight + 15) >> 4; row++)
                {
                        hw_qp = (char *)((uint32_t)(hw_qp + 3) & ~3UL);
                        for (col = 0; col < (pDecPar->u16FrameWidth + 15) >> 4; col++)
                                *hw_qp++ = *quant++;
                }

                if (pf_start(cur_buf, v4lbuf.m.offset) < 0)
                        break;
		usleep(1);
                if (pf_wait() < 0) {
                        printf("Error waiting for post-filter to complete.\n");
                        break;
                }

#ifdef PROFILE
                clock_gettime(CLOCK_REALTIME, &time2);
                temp_time.tv_sec = time2.tv_sec - time1.tv_sec;
                temp_time.tv_nsec = time2.tv_nsec - time1.tv_nsec;
                pf_time.tv_sec += temp_time.tv_sec;
                pf_time.tv_nsec += temp_time.tv_nsec;
                pf_temp_time = (temp_time.tv_sec * 1000000) + (temp_time.tv_nsec / 1000);
#endif

                // Decode next frame while filtering previous frame
                cur_buf = !cur_buf;
#if PF_TEST
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf = g_pf_inbuf[cur_buf] + g_in_buf[cur_buf].y_offset;
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CbBuf =
                            psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf + g_in_buf[cur_buf].u_offset;
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CrBuf =
                            psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf + g_in_buf[cur_buf].v_offset;
#else
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf = g_pf_inbuf[cur_buf];
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CbBuf =
                            psMpeg4DecObject->sDecParam.sOutputBuffer.pu8YBuf + psMpeg4DecObject->sDecParam.sOutputBuffer.s32YBufLen;
                psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CrBuf =
                            psMpeg4DecObject->sDecParam.sOutputBuffer.pu8CbBuf + psMpeg4DecObject->sDecParam.sOutputBuffer.s32CbBufLen;
#endif
                eDecRetVal = eMPEG4DDecode (psMpeg4DecObject);

#ifdef PROFILE
                clock_gettime(CLOCK_REALTIME, &time3);
                temp_time.tv_sec = time3.tv_sec - time2.tv_sec;
                temp_time.tv_nsec = time3.tv_nsec - time2.tv_nsec;
                dec_time.tv_sec += temp_time.tv_sec;
                dec_time.tv_nsec += temp_time.tv_nsec;
                time = (temp_time.tv_sec * 1000000) + (temp_time.tv_nsec / 1000);
                fprintf(flog, "dec\t= %ld us\n", time);
#endif
                if (eDecRetVal == E_MPEG4D_ENDOF_BITSTREAM)
                        break;

                /* In all cases, the last frame is decoded, increment decode count */
                if (eDecRetVal != E_MPEG4D_SUCCESS)
                {
                        printf ("\nError status %d\n", eDecRetVal);
                        break;
                }
                frame_count++;

                // Wait for post filter to finish
//                if (pf_wait() < 0) {
//                        printf("Error waiting for post-filter to complete.\n");
//                        break;
//                }
#ifdef PROFILE
                clock_gettime(CLOCK_REALTIME, &time4);
                temp_time.tv_sec = time4.tv_sec - time3.tv_sec;
                temp_time.tv_nsec = time4.tv_nsec - time3.tv_nsec;
                pf_time.tv_sec += temp_time.tv_sec;
                pf_time.tv_nsec += temp_time.tv_nsec;
                pf_temp_time += (temp_time.tv_sec * 1000000) + (temp_time.tv_nsec / 1000);
                fprintf(flog, "pf\t= %ld us\n", pf_temp_time);
#endif

                v4lbuf.timestamp.tv_sec = 0;//tv_start.tv_sec +
//                        psMpeg4DecObject->sDecParam.sTime.s32Seconds;
                v4lbuf.timestamp.tv_usec = 0;//tv_start.tv_usec +
//                        (psMpeg4DecObject->sDecParam.sTime.s32MilliSeconds * 1000);
                if (pp_start(&v4lbuf) < 0) {
                        break;
                }
         }

#ifdef PROFILE
        fclose(flog);

        time  = dec_time.tv_sec * 1000000;
        time += dec_time.tv_nsec / 1000;
        total_time = time;
        if (time >= 1000)
                printf("Decode Time for %ld frames = %ld us = %ld fps\n",
                     frame_count, time, (frame_count * 1000) / (time / 1000));

        time  = pf_time.tv_sec * 1000000;
        time += pf_time.tv_nsec / 1000;
        total_time += time;
        if (time >= 1000)
                printf("Post-filter Time for %ld frames = %ld us = %ld fps\n",
                     frame_count, time, (frame_count * 1000) / (time / 1000));

        time  = pp_time.tv_sec * 1000000;
        time += pp_time.tv_nsec / 1000;
        total_time += time;
        if (time >= 1000)
                printf("Post-processing time for %ld frames = %ld us = %ld fps\n",
                     frame_count, time, (frame_count * 1000) / (time / 1000));

        if (total_time)
                printf("Total Time for %ld frames = %lld us = %ld fps\n",
                     frame_count, total_time, (frame_count * 1000000ULL) / total_time);
#endif
        gettimeofday(&tv_end, 0);
        total_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000ULL;
        total_time += tv_end.tv_usec - tv_start.tv_usec;
        if (total_time)
                printf("\nTotal Time for %ld frames = %lld us = %ld fps\n",
                     frame_count, total_time, (frame_count * 1000000ULL) / total_time);

        return eDecRetVal;
}


/*!
*******************************************************************************
* Description: cbkMPEG4DBufRead() is used by the decoder to get a new input
*              buffer for decoding. It copies the "s32EncStrBufLen" of encoded
*              data into "pu8EncStrBuf". This function is called by the decoder
*              in eMPEG4DInit() and eMPEG4DDecode() functions, when it runs out
*              of current bit stream input buffer.
*
* \param[in]   s32EncStrBufLen - is the Length of the Buffer. This buffer will
*                                hold Enoded Bit Stream.
* \param[in]   pu8EncStrBuf    - is a pointer to a Buffer which will hold
*                                Encoded Bit Stream.
* \param[in]   s32Offset       - offset of the first byte starting from start
*                                of bitstream
* \param[in]   pvAppContext    - is a pointer which points to information
*                                about a specifc instance of a Decoder.
*
* \return     u32BytesRead, returns number of bytes copied in to the Encoded
*                            Stream Buffer.
* \remarks
*             Global Variables: None
*
*/

int cbkMPEG4DBufRead (int s32EncStrBufLen, unsigned char *pu8EncStrBuf,
                      int s32Offset, void *pvAppContext)
{
    unsigned int   u32BytesRead = 0;
    int cachedbytes = 0;
    sIOFileParams *psIOFileParamsObj = (sIOFileParams *) pvAppContext;

    //printf("offset = %d, num to read = %d\n", s32Offset, s32EncStrBufLen);

    if (psIOFileParamsObj->s32NumBytesRead != s32Offset) {
        /* there may be better way of setting the file pointer */
        fseek (psIOFileParamsObj->fpInput, s32Offset, SEEK_SET);
        psIOFileParamsObj->s32NumBytesRead = s32Offset;
    }
    if (s32Offset < BIT_BUFFER_SIZE) {
        cachedbytes = BIT_BUFFER_SIZE - s32Offset;
        u32BytesRead = (cachedbytes > s32EncStrBufLen) ? s32EncStrBufLen : cachedbytes;
        memcpy(pu8EncStrBuf, pu8BitBuffer + s32Offset, u32BytesRead);
    }

    if (u32BytesRead < s32EncStrBufLen) {
            u32BytesRead += fread (pu8EncStrBuf, sizeof(unsigned char), s32EncStrBufLen-u32BytesRead,
                                  psIOFileParamsObj->fpInput);
    }

    psIOFileParamsObj->s32NumBytesRead += u32BytesRead;
    if (u32BytesRead == 0)
    {
        if (feof (psIOFileParamsObj->fpInput) == 0)
        {
            return -1;
        }
    }
    return u32BytesRead;
}


/*!
*******************************************************************************
* Description: This is an application domain fucntion. It allocates memory
*              required by the Decoder.
*
* \param[in]  psMemAllocInfo - is a pointer to a structure which holds
*             allocated memory. This allocated memory is required by the
*             decoder.
*
* \return     returns the status of Memory Allocation, ERROR/ SUCCESS
* \remarks
*             Global Variables: None
*
*/

int s32AllocateMem4Decoder (sMpeg4DecMemAllocInfo *psMemAllocInfo)
{
    int s32MemBlkCnt = 0;
    sMpeg4DecMemBlock  *psMemInfo;

    for (s32MemBlkCnt = 0; s32MemBlkCnt < psMemAllocInfo->s32NumReqs;
         s32MemBlkCnt++)
    {
        psMemInfo = &psMemAllocInfo->asMemBlks[s32MemBlkCnt];
        psMemInfo->pvBuffer = malloc(psMemInfo->s32Size);
        if (psMemInfo->pvBuffer == NULL)
        {
            return ERROR;
        }
    }

    return SUCCESS;
}


/*!
*******************************************************************************
* Description: This is an application domain fucntion. It deallocates memory
*              which was allocated by "s32AllocateMem4Decoder()".
*
* \param[in]  psMpeg4DecMemAllocInfo - is a pointer to a structure which holds
*             allocated memory. This allocated memory was being used by the
*             decoder.
* \return     returns the status of Memory Allocation, ERROR/ SUCCESS
* \remarks
*             Global Variables: None
*
*/

void vFreeMemAllocated4Dec (sMpeg4DecMemAllocInfo *psMemAllocInfo)
{
    int s32MemBlkCnt = 0;

    for (s32MemBlkCnt = 0; s32MemBlkCnt < psMemAllocInfo->s32NumReqs;
         s32MemBlkCnt++)
    {
        if (psMemAllocInfo->asMemBlks[s32MemBlkCnt].pvBuffer != NULL)
        {
            free (psMemAllocInfo->asMemBlks[s32MemBlkCnt].pvBuffer);
            psMemAllocInfo->asMemBlks[s32MemBlkCnt].pvBuffer = NULL;
        }
    }

}

/*!
*******************************************************************************
* Description: It deallocates all the memory which was allocated by Application
*
* \param[in]  psMpeg4DecObject - is a pointer to Mpeg4 Decoder
*                                Configuration structure.
* \param[in]  psDecodeParams   - is a pointer to a structure which holds
*                                Frame no's for Decoding and Skipping Frames
* \return     none
* \remarks
*             Global Variables: None
*
*/

void vFreeMemAllocated4App (sMpeg4DecObject* psMpeg4DecObject)
{
    sMpeg4DecMemAllocInfo   *psMemAllocInfo;
    sIOFileParams *psIOFileParamsObj = (sIOFileParams *)
                                         psMpeg4DecObject->pvAppContext;

    psMemAllocInfo = &(psMpeg4DecObject->sMemInfo);

    /*!
    *   Freeing Memory Allocated by the Application for Decoder
    */
    vFreeMemAllocated4Dec (psMemAllocInfo);
    psMemAllocInfo = NULL;

    psIOFileParamsObj->ps8EncStrFname = NULL;
    psIOFileParamsObj->ps8DecStrFname = NULL;

    if (psIOFileParamsObj->fpInput != NULL)
    {
        fclose (psIOFileParamsObj->fpInput);
        psIOFileParamsObj->fpInput = NULL;
    }
    psIOFileParamsObj = NULL;

    if (psMpeg4DecObject != NULL)
    {
        psMpeg4DecObject->pvMpeg4Obj = NULL;
        psMpeg4DecObject->pvAppContext = NULL;

        free (psMpeg4DecObject);
        psMpeg4DecObject = NULL;
    }

}


static int pf_init(uint32_t width, uint32_t height, uint32_t stride)
{
        int i;
        int retval = 0;
        pf_init_params pf_init;
        pf_reqbufs_params pf_reqbufs;

        if ((fd_pf = open("/dev/mxc_ipu_pf", O_RDWR, 0)) < 0)
        {
                printf("Unable to open pf device\n");
                retval = -1;
                goto err0;
        }

        memset(&pf_init, 0, sizeof(pf_init));
        pf_init.pf_mode = 3;
        pf_init.width = width;
        pf_init.height = height;
        pf_init.stride = stride;
//        pf_init.qp_size = 0x10000;
        if (ioctl(fd_pf, PF_IOCTL_INIT, &pf_init) < 0)
        {
                printf("PF_IOCTL_INIT failed\n");
                retval = -1;
                goto err1;
        }
	g_pf_qp_buf_size = pf_init.qp_size;
        g_pf_qp_buf = mmap(NULL, pf_init.qp_size,
                         PROT_READ | PROT_WRITE, MAP_SHARED,
                         fd_pf, pf_init.qp_paddr);
        if (g_pf_qp_buf == NULL) {
                printf("v4l2_out test: mmap for qp buffer failed\n");
                retval = -1;
                goto err1;
        }

        pf_reqbufs.count = 2;
//        pf_reqbufs.req_size = 0x10000;
        pf_reqbufs.req_size = 0;
        if (ioctl(fd_pf, PF_IOCTL_REQBUFS, &pf_reqbufs) < 0)
        {
                printf("PF_IOCTL_REQBUFS failed\n");
                retval = -1;
                goto err1;
        }

        for (i = 0; i < pf_reqbufs.count; i++) {

                g_in_buf[i].index = i;
                if (ioctl(fd_pf, PF_IOCTL_QUERYBUF, &g_in_buf[i]) < 0)
                {
                        printf("PF_IOCTL_QUERYBUF failed\n");
                        retval = -1;
                        goto err1;
                }

                g_pf_inbuf[i] = mmap (NULL, g_in_buf[i].size,
                                 PROT_READ | PROT_WRITE, MAP_SHARED,
                                 fd_pf, g_in_buf[i].offset);
                if (g_pf_inbuf[i] == NULL) {
                        printf("v4l2_out test: mmap for input buffer failed\n");
                        retval = -1;
                        goto err1;
                }
        }

        printf("PF driver initialized\n");
        return retval;
err1:
        close(fd_pf);
err0:
        return retval;

}

static int pf_uninit(void)
{
        pf_reqbufs_params pf_reqbufs;
        int retval = 0;

        printf("Closing PF driver");

        munmap(g_pf_inbuf[0], g_in_buf[0].size);
        munmap(g_pf_inbuf[1], g_in_buf[1].size);
        munmap(g_pf_qp_buf, g_pf_qp_buf_size);

        pf_reqbufs.count = 0;   // Zero deallocates buffers
        if (ioctl(fd_pf, PF_IOCTL_REQBUFS, &pf_reqbufs) < 0)
        {
                printf("PF_IOCTL_REQBUFS failed\n");
                retval = -1;
                goto err1;
        }

        if (ioctl(fd_pf, PF_IOCTL_UNINIT, NULL) < 0)
        {
                printf("PF uninit failed\n");
                retval = -1;
        }

        close(fd_pf);
        printf(" - Done\n");
err1:
        return retval;
}

static int pf_start(int in_buf_idx, unsigned long out_buf)
{
        int retval = 0;
        pf_start_params pf_st;

        memset(&pf_st, 0, sizeof(pf_st));
        pf_st.wait = 0;
        pf_st.in = g_in_buf[in_buf_idx];
        pf_st.out.index = -1;
        pf_st.out.size = 0;     //unused
        pf_st.out.offset = out_buf;

        if ((retval = ioctl(fd_pf, PF_IOCTL_START, &pf_st)) < 0)
        {
                printf("PF start failed: %d\n", retval);
                goto err1;
        }
err1:
        return retval;
}

static int pf_wait(void)
{
        int retval = 0;

        if ((retval = ioctl(fd_pf, PF_IOCTL_WAIT, NULL)) < 0)
        {
                printf("PF wait failed: %d\n", retval);
        }
        return retval;
}

static int pp_init(uint32_t width, uint32_t height)
{
        struct v4l2_control ctrl;
        struct v4l2_requestbuffers buf_req;
        struct v4l2_format fmt;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        int retval = 0;

        if ((fd_v4l = open("/dev/video16", O_RDWR, 0)) < 0)
        {
                printf("Unable to open /dev/video16\n");
                retval = -1;
                goto err0;
        }

        if (ioctl(fd_v4l, VIDIOC_S_OUTPUT, &g_output) < 0)
        {
                printf("set output failed\n");
                retval = -1;
                goto err1;
        }

        memset(&cropcap, 0, sizeof(cropcap));
        cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if (ioctl(fd_v4l, VIDIOC_CROPCAP, &cropcap) < 0)
        {
                printf("get crop capability failed\n");
                retval = -1;
                goto err1;
        }
        printf("cropcap.bounds.width = %d\ncropcap.bound.height = %d\n" \
               "cropcap.defrect.width = %d\ncropcap.defrect.height = %d\n",
               cropcap.bounds.width, cropcap.bounds.height,
               cropcap.defrect.width, cropcap.defrect.height);

        crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        crop.c.top = g_display_top;
        crop.c.left = g_display_left;
        crop.c.width = g_display_width;
        crop.c.height = g_display_height;
        if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
        {
                printf("set crop failed\n");
                retval = -1;
                goto err1;
        }

        // Set rotation
        ctrl.id = V4L2_CID_PRIVATE_BASE;
        ctrl.value = g_rotate;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
        {
                printf("set ctrl failed\n");
                retval = -1;
                goto err1;
        }

        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        fmt.fmt.pix.width=width;
        fmt.fmt.pix.height=height;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;

        if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                retval = -1;
                goto err1;
        }

        if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0)
        {
                printf("get format failed\n");
                retval = -1;
                goto err1;
        }

	g_frame_size = fmt.fmt.pix.sizeimage;

        memset(&buf_req, 0, sizeof(buf_req));
        buf_req.count = 5;
        buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf_req.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd_v4l, VIDIOC_REQBUFS, &buf_req) < 0)
        {
                printf("request buffers failed\n");
                retval = -1;
                goto err1;
        }
        g_num_buffers = buf_req.count;

/*
        for (i = 0; i < g_num_buffers; i++)
        {
                memset(&g_buf, 0, sizeof (g_buf));
                g_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                g_buf.memory = V4L2_MEMORY_MMAP;
                g_buf.index = i;
                if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &g_buf) < 0)
                {
                        printf("VIDIOC_QUERYBUF error\n");
                        // goto cleanup;
                        pp_cleanup();
                }
                buffers[i].length = g_buf.length;
                buffers[i].offset = (size_t) g_buf.m.offset;
                printf("VIDIOC_QUERYBUF: length = %d, offset = %d\n",  buffers[i].length, buffers[i].offset);
                buffers[i].start = mmap (NULL, buffers[i].length,
                                         PROT_READ | PROT_WRITE, MAP_SHARED,
                                         fd_v4l, buffers[i].offset);
                if (buffers[i].start == NULL) {
                        printf("v4l2_out test: mmap failed\n");
                }
        }
*/
	g_buf.index = 0;
        printf("v4l2_output test: Allocated %d buffers\n", buf_req.count);

        return 0;
err1:
        close(fd_v4l);
err0:
        return retval;

}

static int pp_start(struct v4l2_buffer * buf)
{
        enum v4l2_buf_type type;
        static int pp_count = 0;

        if (ioctl(fd_v4l, VIDIOC_QBUF, buf) < 0)
        {
                printf("VIDIOC_QBUF failed\n");
                return -1;
        }

        if ( pp_count == 1 ) { // Start playback after buffers queued
                type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                if (ioctl (fd_v4l, VIDIOC_STREAMON, &type) < 0) {
                        printf("Could not start stream\n");
                        return -1;
                }
        }
        pp_count++;
        return 0;
}

void pp_uninit(void)
{
        struct v4l2_requestbuffers buf_req;
        int type;

        printf(" closing post processing\n");

        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl (fd_v4l, VIDIOC_STREAMOFF, &type);

        buf_req.count = 0;
        buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf_req.memory = V4L2_MEMORY_MMAP;
        ioctl(fd_v4l, VIDIOC_REQBUFS, &buf_req);

	close(fd_v4l);
}

int fb_setup(void)
{
        char fbdev[] = "/dev/fb0";
        struct fb_var_screeninfo fb_var;
        struct fb_fix_screeninfo fb_fix;
        struct mxcfb_color_key color_key;
        struct mxcfb_gbl_alpha alpha;
        int retval = -1;
        int fd_fb;
        unsigned short * fb0;
        __u32 screen_size;
        int h, w;

//        fbdev[7] = '0' + g_output;
        if ((fd_fb = open(fbdev, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", fbdev);
                retval = -1;
                goto err0;
        }

        if ( ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
                goto err1;
        }
        if (g_display_height == 0)
                g_display_height = fb_var.yres;

        if (g_display_width == 0)
                g_display_width = fb_var.xres;

        alpha.alpha = 255;
        alpha.enable = 1;
        if ( ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0) {
                retval = 0;
                goto err1;
        }

        color_key.color_key = 0x00080808;
        color_key.enable = 1;
        if ( ioctl(fd_fb, MXCFB_SET_CLR_KEY, &color_key) < 0) {
                retval = 0;
                goto err1;
        }

        if ( ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
                goto err1;
        }
        screen_size = fb_var.xres * fb_var.yres * fb_var.bits_per_pixel / 8;

        /* Map the device to memory*/
        fb0 = (unsigned short *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
        if ((int)fb0 == -1)
        {
                printf("\nError: failed to map framebuffer device 0 to memory.\n");
                retval = -1;
                goto err1;
        }


        if (fb_var.bits_per_pixel == 16) {
                for (h = 0; h < g_display_height; h++) {
                        for (w = 0; w < g_display_width; w++) {
                                fb0[(h*fb_var.xres) + w] = 0x0841;
                        }
                }
        }
        else if (fb_var.bits_per_pixel == 24) {
                for (h = 0; h < g_display_height; h++) {
                        for (w = 0; w < g_display_width; w++) {
                                unsigned char * addr = (unsigned char *)fb0 + ((h*fb_var.xres) + w) * 3;
                                *addr++ = 8;
                                *addr++ = 8;
                                *addr++ = 8;
                        }
                }
        }
        else if (fb_var.bits_per_pixel == 32) {
                for (h = 0; h < g_display_height; h++) {
                        for (w = 0; w < g_display_width; w++) {
                                unsigned char * addr = (unsigned char *)fb0 + ((h*fb_var.xres) + w) * 4;
                                *addr++ = 8;
                                *addr++ = 8;
                                *addr++ = 8;
                        }
                }
        }

        retval = 0;
        munmap(fb0, screen_size);
err1:
        close(fd_fb);
err0:
        return retval;
}

