/*
 * Copyright (C) 2005-2012 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*****************************************************************************
** dut_api_vts.h
**
** Description: Implements structures used by DUTs
**              and gives the VTS used APIs of DUTs.
**
** Author:
**     Phil Chen   <b02249@freescale.com>
**     Larry Lou   <b06557@freescale.com>
**
** Revision History:
** -----------------
** 1.0  11/01/2007  Larry Lou   create this file
** 1.7  11/08/2007  Larry Lou	add macro DUT_API_HEADER_VERSION and API QueryAPIVersion_dut()
** 1.8  03/18/2008  Larry Lou	update the DUT_API_HEADER_VERSION from ver1.7 to ver1.8
** 2.0  10/31/2008  Larry Lou	update the wrapper API from 1.9 to 2.0
*****************************************************************************/

#ifndef _DUT_API_VTS_H_
#define _DUT_API_VTS_H_

#include "dut_probes_vts.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* <Macros>
*****************************************************************************/
/* DLL symbol export indicator for WinCE or Win32 DLL */
#if defined(LIBDUTDEC_EXPORTS) && ( defined(TGT_OS_WINCE) || defined(TGT_OS_WIN32) )
    #define DLL_EXPORTS __declspec(dllexport)
#else
    #define DLL_EXPORTS
#endif

/*
** DUT header file version
** DUT should output this version information by implementing the function QueryVersion_dut() in wrapper.
** So, VTS framework can get this string to see if the DUT version matches VTS version.
*/
#define WRAPPER_API_VERSION 21  /* ver 2.1 */

/*****************************************************************************
 * <Typedefs>
 *****************************************************************************/
/*
** return value of DUTs
*/
typedef enum
{
    E_DEC_INIT_OK_DUT = 0,   /* decoder is initialized successfully */
    E_DEC_INIT_ERROR_DUT,    /* encounter errer during initializing */
    E_DEC_FRAME_DUT,         /* finish outputting one or more frame */
    E_DEC_FINISH_DUT,        /* finish decoding all the bitstream */
    E_DEC_ALLOUT_DUT,        /* all decoded frames have been output */
    E_DEC_ERROR_DUT,         /* encounter error during decoding */
    E_DEC_REL_OK_DUT,        /* decoder is released successfully */
    E_DEC_REL_ERROR_DUT,     /* encounter error during releasing */
    E_GET_VER_OK_DUT,        /* query version successfully */
    E_GET_VER_ERROR_DUT,     /* encounter error during querying version */
} DEC_RETURN_DUT;

/*
** decoder type
*/
typedef enum
{
    E_DUT_TYPE_UNKNOWN = 0,     /* unknown decoder */
    E_DUT_TYPE_DIVX,            /* DivX decoder */
    E_DUT_TYPE_XVID,            /* XVID decoder (NOT SUPPORTED) */
    E_DUT_TYPE_H263,            /* H.263 decoder (NOT SUPPORTED) */
    E_DUT_TYPE_H264,            /* H.264 decoder */
    E_DUT_TYPE_MPG2,            /* MPEG-2 decoder */
    E_DUT_TYPE_MPG4,            /* MPEG-4 decoder */
    E_DUT_TYPE_WMV9,            /* WMV9 decoder */
    E_DUT_TYPE_RV89,            /* RealVideo decoder */
    E_DUT_TYPE_MJPG,            /* MJPEG decoder */
    E_DUT_TYPE_DIV3 = 10,       /* DivX 3.11 decoder */
    E_DUT_TYPE_DIV4,            /* DivX 4 decoder */
    E_DUT_TYPE_DX50,            /* DivX 5/6 decoder */
    E_DUT_TYPE_RV20,            /* RV20 decoder */
    E_DUT_TYPE_RV30,            /* RealVideo 8 decoder */
    E_DUT_TYPE_RV40,            /* RealVideo 9/10 decoder */
    E_DUT_TYPE_RVG2,            /* raw TCK format */
    E_DUT_TYPE_WMV3,            /* VC-1 SP/MP decoder */
    E_DUT_TYPE_WVC1,            /* VC-1 AP decoder */
    E_DUT_TYPE_WMV1,            /* WMV 7 video */
    E_DUT_TYPE_WMV2 = 20,       /* WMV 8 video */
    E_DUT_TYPE_MAX
} DUT_TYPE;

/*
* structure for DUT initialization
* it is used as the second input parameter of VideoDecInit()
*/
typedef struct
{
    unsigned char * strInFile;  /* input file name */
    unsigned char * strOutFile; /* output yuv file name */
    unsigned int    uiWidth;    /* video frame width */
    unsigned int    uiHeight;   /* video frame height */
    FuncProbeDut    pfProbe;    /* VTS probe */
    DUT_TYPE        eDutType;   /* DUT type */
    long            iInputMode; /* stream input mode : 0 - streaming mode */
                                /*                     1 - file-play mode */
} DUT_INIT_CONTXT_2_1;

/********************************************************************
 *
 * VideoDecInit_dut - Initialize the DUT
 *
 * Description:
 *   Initialize the decoder, inlude allocating buffers, allocating and
 *   initializing decoder object and so on.
 *   This funtion is implemented by the DUT and invoked by VTS.
 *
 * Arguments:
 *   _ppDecObj              [OUT]   pointer of decoder object pointer
 *   _psInitContxt          [IN]    pointer of structure contain REF init infomation.
 *
 * Return:
 *   E_DEC_INIT_OK_DUT      decoder is initialized successfully
 *   E_DEC_INIT_ERROR_DUT   encounter errer during initializing
 *
 ********************************************************************/
DEC_RETURN_DUT DLL_EXPORTS VideoDecInit( void ** _ppDecObj,
                             void *  _psInitContxt );

/********************************************************************
 *
 * VideoDecFrame_dut - decode and output one frame
 *
 * Description:
 *   Decode bitstream and output one decoded frame before return.
 *   This funtion is implemented by the DUT and invoked by VTS.
 *
 * Arguments:
 *   _pDecObj               [IN/OUT]decoder object pointer
 *   _pParam                [IN]    pointer of other parameters
 *                                  (not use, but for the next version )
 * Return:
 *   E_DEC_FRAME_DUT        finish output one or more frame
 *   E_DEC_FINISH_DUT       finish decoding all the bitstream
 *   E_DED_ALLOUT_DUT       all decoded frames have been output
 *   E_DEC_ERROR_DUT        encounter error during decoding
 ********************************************************************/
DEC_RETURN_DUT DLL_EXPORTS VideoDecRun( void * _pDecObj,
                            void * _pParam );


/********************************************************************
 *
 * VideoDecRelease_dut - release the DUT
 *
 * Description:
 *   Release the allocated memory for the decoder under test,
 *   and close file if any file is opened.
 *   This funtion is implemented by the DUT and invoked by VTS.
 *
 * Arguments:
 *   _pDecObj               [IN/OUT]pointer of decoder object
 *
 * Return:
 *   E_DEC_REL_OK_DUT       decoder is released successfully
 *   E_DEC_REL_ERROR_DUT    encounter error during releasing
 *
 ********************************************************************/
DEC_RETURN_DUT DLL_EXPORTS VideoDecRelease( void * _pDecObj );


/********************************************************************
 *
 * QueryVersion_dut - query the dut wrapper version
 *
 * Description:
 *   Query the dut header version. It should copy DUT_API_HEADER_VERSION to
 *   the array _auchVer[].
 *   This funtion is implemented by the DUT and invoked by VTS.
 *   Sample code: *_piAPIVersion = WRAPPER_API_VERSION;
 *
 * Arguments:
 *   _piAPIVersion          [OUT]   pointer of the API version
 *
 * Return:
 *   E_GET_VER_OK_DUT       query version successfully
 *   E_GET_VER_ERROR_DUT    encounter error during querying version
 *
 ********************************************************************/
void DLL_EXPORTS QueryAPIVersion( long * _piAPIVersion );


#ifdef __cplusplus
}
#endif

#endif /* _DUT_API_VTS_H_ */

