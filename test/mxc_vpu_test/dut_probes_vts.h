/*
 ***********************************************************************
 * Copyright 2005-2012 Freescale Semiconductor, Inc.
 * All modifications are confidential and proprietary information
 * of Freescale Semiconductor, Inc. ALL RIGHTS RESERVED.
 ***********************************************************************
 */

/*****************************************************************************
** dut_probes_vts.h
**
** Use of Freescale code is governed by terms and conditions
** stated in the accompanying licensing statement.
**
** Description: Implements structures and enumulations used by DUT
**              when it invokes callback functions.
**
** Author:
**     Phil Chen   <b02249@freescale.com>
**     Larry Lou   <b06557@freescale.com>
**
** Revision History:
** -----------------
** 1.0  11/01/2007  Larry Lou   create this file
** 1.1  03/17/2008  Larry Lou   modify the probe invoking mode
** 1.2  07/14/2008  Larry Lou   add a new probe E_INPUT_BITSTREAM
*****************************************************************************/

#ifndef _DUT_PROBES_VTS_H_
#define _DUT_PROBES_VTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * <Typedefs>
 *****************************************************************************/

/*
** Enumeration type to distinguish different probes
*/
typedef enum
{
    E_INPUT_LENGTH,     /* get length of an input bitstream unit */
    E_INPUT_STARTTIME,  /* record start time of reading an input bitstream unit */
    E_INPUT_ENDTIME,    /* record end time of reading an input bitstream unit */
    E_OUTPUT_FRAME,     /* record get an output frame */
    E_OUTPUT_STARTTIME, /* record start time of outputting an decoded frame */
    E_OUTPUT_ENDTIME,   /* end time of outputting an decoded frame */
    E_OPEN_BITSTREAM,   /* open bitstream */
    E_CLOSE_BITSTREAM,  /* close bitstream */
    E_READ_BITSTREAM,   /* get bitstream */
    E_SEEK_BITSTREAM,   /* seek bitstream */
    E_TELL_BITSTREAM,   /* get the read bytes of bitstream */
    E_FEOF_BITSTREAM,   /* check the end of file */
    E_GET_CHUNK,        /* get a chunk in file play mode */
    E_GET_CHUNK_HDR,    /* get a chunk header in file play mode */
    E_GET_STREAM_HDR,   /* get the stream header in file play mode */
    E_SEND_SIGNAL,      /* send a signal to host */
    E_DECFRM_START,     /* record start time of decoding one frame */
    E_DECFRM_END,       /* record end time of decoding one frame */
} PROBE_TYPE;

/*
** Structure used in probe for storing a frame [E_STORE_OUTPUT_FRAME]
*/
typedef struct
{
    unsigned char * puchLumY;     // Y component buffer pointer
    unsigned char * puchChrU;     // U component buffer pointer
    unsigned char * puchChrV;     // V component buffer pointer
    long            iFrmWidth;    // Image width
    long            iFrmHeight;   // Image height
    long            iBufStrideY;  // Memory stride of Y component
    long            iBufStrideUV; // Memory stride of UV component
} FRAME_COPY_INFO;

/*
** Structure used in probe for read bitstream [E_READ_BITSTREAM]
*/
typedef struct
{
    int    hBitstream;          /* bitstream handle */
    void * pBitstreamBuf;       /* bitstream buffer address of DUT */
    long   iLength;             /* length of bitstream buffer */
} DUT_STREAM_READ;

/*
** Structure used in probe for read bitstream [E_SEEK_BITSTREAM]
*/
typedef struct
{
    int    hBitstream;          /* bitstream handle */
    long   iOffset;             /* number of bytes from iOrig */
    int    iOrig;               /* Initial position */
} DUT_STREAM_SEEK;

/*
** Structure used in probe for open bitstream [E_OPEN_BITSTREAM]
*/
typedef struct
{
    char * strBitstream;        /* bitstream name */
    char * strMode;             /* open mode */
} DUT_STREAM_OPEN;

/*
** Structure used in probe for read a packet
** [E_READ_CHUNK/E_GET_CHUNK_HDR/E_GET_STREAM_HDR]
*/
typedef struct
{
    int    hBitstream;          /* bitstream handle */
    void * pBitstreamBuf;       /* bitstream buffer address of DUT */
} DUT_GET_PACKET;

/*
** Function type definition for all probes
** input value:
**       Para1:  Probe type
**       Para2:  specific data structure pointer for a certain type
*/
typedef long ( * FuncProbeDut )( PROBE_TYPE, void * );

#ifdef __cplusplus
}
#endif

#endif /* _DUT_PROBES_VTS_H_ */

