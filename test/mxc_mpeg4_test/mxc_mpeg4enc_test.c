/*
 * Copyright 2005-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * FREESCALE SEMICONDUCTOR, INC.
 *
 * Test app for MPEG4 encode
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpeg4_enc_api.h"

#define OUT_BUFFER_SIZE 0x10000
char y_buffer[352 * 288 * 2];
char out_buffer[OUT_BUFFER_SIZE];
int g_width = 176;
int g_height = 144;
int g_frame_rate = 30;


int process_cmdline(int argc, char **argv)
{
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-w") == 0) {
                        g_width = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-h") == 0) {
                        g_height = atoi(argv[++i]);
                }
                else if (strcmp(argv[i], "-f") == 0) {
                        g_frame_rate = atoi(argv[++i]);
                }
        }

        if ((g_width == 0) || (g_height == 0)) {
                return -1;
        }
        return 0;
}


void EncoderLoop (sMpeg4EncoderConfig  *psMpeg4EncPtr, FILE * in_file, FILE * out_file)
{
        int loop_count = 0;
        char * outbuf = psMpeg4EncPtr->pu8OutputBufferPtr;
        int frame_size = (psMpeg4EncPtr->s32SourceWidth * psMpeg4EncPtr->s32SourceHeight * 3) / 2;
        int retval = E_MPEG4E_SUCCESS;

        while (retval == E_MPEG4E_SUCCESS)
        {
                retval = eMpeg4EncoderGetNextFrameNum(psMpeg4EncPtr);
                if (retval != E_MPEG4E_SUCCESS ) {
                        printf("eMpeg4EncoderGetNextFrameNum returned %d\n", retval);
                        break;
                }


                retval = fread(psMpeg4EncPtr->pu8CurrYAddr, 1, frame_size, in_file);
                if (retval < frame_size)
                        break;
                //psMpeg4EncPtr->pu8OutputBufferPtr = outbuf;

//                printf("loop %d\n", loop_count++);

                /* Encode one frame.*/
                retval =  eMpeg4EncoderEncode(psMpeg4EncPtr);

//                printf("inbuf = 0x%08X,  outbuf = 0x%08X\n", psMpeg4EncPtr->pu8CurrYAddr, psMpeg4EncPtr->pu8OutputBufferPtr);

                if (retval != E_MPEG4E_SUCCESS ) {
                        printf("eMpeg4EncoderEncode returned %d\n", retval);
                        break;
                }
                if (psMpeg4EncPtr->s32BitstreamSize <= 0) {
                        continue;
                }

//                printf("writing %d bytes from 0x%08X\n", psMpeg4EncPtr->s32BitstreamSize, psMpeg4EncPtr->pu8OutputBufferPtr);
                fwrite(psMpeg4EncPtr->pu8OutputBufferPtr, 1, psMpeg4EncPtr->s32BitstreamSize, out_file);

//                printf("writing done\n");
                /* The output encoded frame is ready for use.  */
                /*Set output bitstream pointers*/

        }
        printf("encode done - %d\n", retval);
        return;
}


int main(int argc, char **argv)
{
        int status;
        int i;
        void * pvUnalignedMem;
        int s32Extra, s32Mask;
        FILE * in_file = 0;
        FILE * out_file = 0;
        sMpeg4EncoderConfig sMpeg4EncPtr;
        sMpeg4EMemAllocInfo *MemInfo = &(sMpeg4EncPtr.sMemInfo);

        process_cmdline(argc, argv);

        if ((out_file = fopen(argv[argc-1], "wb")) < 0)
        {
                printf("Unable to open output file\n");
                return;
        }

        if ((in_file = fopen(argv[argc-2], "rb")) < 0)
        {
                printf("Unable to open input file\n");
                return;
        }

        //Setting Default Parameters
        eMpeg4EncoderSetDefaultParam(&sMpeg4EncPtr);

        // User to override the required parameters
        sMpeg4EncPtr.s32TargetBitRate = 64000;
        sMpeg4EncPtr.s32Profile = 0;
//        sMpeg4EncPtr.s32Level   = 1;

        sMpeg4EncPtr.s32SourceWidth = g_width;
        sMpeg4EncPtr.s32SourceHeight = g_height;
        sMpeg4EncPtr.s32SourceFrameRate = g_frame_rate;
        sMpeg4EncPtr.s32EncodingFrameRate = g_frame_rate;

        sMpeg4EncPtr.pu8CurrYAddr = y_buffer;
        sMpeg4EncPtr.pu8CurrCBAddr = y_buffer + (g_width*g_height);
        sMpeg4EncPtr.pu8CurrCRAddr = sMpeg4EncPtr.pu8CurrCBAddr + (g_width*g_height)/4;

        sMpeg4EncPtr.pu8OutputBufferPtr = out_buffer;
        sMpeg4EncPtr.s32MaxOutputBufferSize = OUT_BUFFER_SIZE;


        // Get the encoder memory parameters
        eMpeg4EncoderQueryMemory(&sMpeg4EncPtr);

        /* Give memory to the encoder for the size, type and alignment returned */

      	for(i=0; i< MemInfo->s32NumReqs ;i++)
        {
                // Allocate memory according to this request. This example does not consider alignment,
                // user should take care of this using any available routines
                if (MemInfo->asMemBlks[i].s32Size <= 0)
                    continue;
                MemInfo->asMemBlks[i].pvBuffer = malloc(MemInfo->asMemBlks[i].s32Size);
                if (MemInfo->asMemBlks[i].s32Align == 0)  {

                    MemInfo->asMemBlks[i].pvBuffer = (void*)malloc (MemInfo->asMemBlks[i].s32Size);

                } else {
                    /* needs alignment, how are you going to do it ? */
                    // pvUnalignedMem = (void*)malloc (psMemEntry[i].s32Size);
                    if ( MemInfo->asMemBlks[i].s32Align == E_MPEG4E_HALFWORD_ALIGN) {
                        s32Extra = 1;
                        s32Mask = -2;
                    } else {
                        s32Extra = 3;  // word aligned
                        s32Mask = -4;
                    }

                    pvUnalignedMem = (void*)malloc (MemInfo->asMemBlks[i].s32Size + s32Extra);
                    MemInfo->asMemBlks[i].pvBuffer = (void*)(((int)pvUnalignedMem + s32Extra) &
                                         (s32Mask));
                }
        }

        /* Initialize the encoder. */
        status = eMpeg4EncoderInit(&sMpeg4EncPtr);

        if (status != E_MPEG4E_SUCCESS) {
                printf ("Encoder ERROR: Library returned %d while initialising\n", status);
        	return 0;
        }

        // Write the initial bits into the file
        if (sMpeg4EncPtr.s32BitstreamSize > 0)
        {
                fwrite(sMpeg4EncPtr.pu8OutputBufferPtr, 1, sMpeg4EncPtr.s32BitstreamSize, out_file);
        }

         	/* Encode the bit stream and produce the outputs */
        EncoderLoop (&sMpeg4EncPtr, in_file, out_file); // sub function to call the main encoding API

        /*Free memory resources*/
        eMpeg4EncoderFree (&sMpeg4EncPtr);

err:
        fclose(in_file);
        fclose(out_file);
}

