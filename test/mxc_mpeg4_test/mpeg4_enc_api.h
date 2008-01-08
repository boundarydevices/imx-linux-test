///////////////////////////////////////////////////////////////////////////
//
//  Motorola India Electronics Limited
//
//
//  Primary Author  : 
//
//                  This code is the property of Motorola.
//          (C) Copyright 2004 Motorola,Inc. All Rights Reserved.
//                       MOTOROLA INTERNAL USE ONLY
//
//  History :
//  Date            Author       Version    Description
//
//  June,2004      Chandra         0.1        Created
//  Aug, 2004      Chandra         1.0        RELEASE 1.0 with all review 
//                                            comments
//  Oct 1, 2004    Chandra         1.1        Removed application context
//                                            from the configuration structure
//  Oct 14,2004    Chandra         1.2        Removed references to video
//                                            packet size
//  
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ARM_MPEG4E_H
#define ARM_MPEG4E_H

/*!
*******************************************************************************
*
* \file
*    mpeg4_enc_api.h
* \brief
*    MPEG4 encoder API header function definitions
*
*    Notes:
*    This file outlines the structure and the APIs required to call a MPEG4
*    encoder from an application. The function level calls include
*
*    (1) eMpeg4EncoderQueryMemory
*    (2) eMpeg4EncoderSetDefaultParam
*    (3) eMpeg4EncoderInit
*    (4) eMpeg4EncoderGetNextFrameNum
*    (5) eMpeg4EncoderEncode
*    (6) eMpeg4EncoderFree
*
*    \author
*       Chandrasekhar Lakshmanan        chandra.lakshmanan@motorola.com
*
*******************************************************************************
*/

//!Defines
#define PRINT_PARAMETERS       0
#define MPEG4E_MAX_NUM_MEM_REQS       31

/*! defines to specify type of memory. Only one of the two in each group
 *  (speed and MPEG4E_usage) shall be on but not both.
 */
#define E_MPEG4E_SLOW_MEMORY       0x1   /*! slower memory is acceptable */
#define E_MPEG4E_FAST_MEMORY       0x2   /*! faster memory is preferable */
#define E_MPEG4E_STATIC_MEMORY     0x4   /*! content is used over API calls */
#define E_MPEG4E_SCRATCH_MEMORY    0x8   /*! content is not used over
                                             successessive Decode API calls */

/*! retrieve the type of memory */

#define IS_FAST_MEMORY(memType)    (memType & E_MPEG4D_FAST_MEMORY)
#define IS_SLOW_MEMORY(memType)    (memType & E_MPEG4D_SLOW_MEMORY)
#define IS_STATIC_MEMORY(memType)  (memType & E_MPEG4D_STATIC_MEMORY)
#define IS_SCRATCH_MEMORY(memType) (memType & E_MPEG4D_SCRATCH_MEMORY)

//! Enumeration types
typedef enum 
{
    E_INTRA_CODING = 0,
    E_PREDICTIVE_CODING,
    E_NOT_CODED,
}eMPEG4ECodingType;

typedef enum
{
    /* Success full completion */
    E_MPEG4E_SUCCESS = 0, 
    E_MPEG4E_REACHED_END,
    E_MPEG4E_SKIPPED,

    /* warning messages */
    E_MPEG4E_BUFFER_OVERFLOW = 11,
    E_MPEG4E_WRONG_PARAM_VALUE,

    /* recoverable errors */
    E_MPEG4E_LOW_MEMORY = 31,
    E_MPEG4E_WRONG_ALIGNMENT,

    /* Error messages, not recoverable */
    E_MPEG4E_WRONG_FORMAT = 51,
}eMPEG4ERetType;


typedef enum
{
   E_MPEG4E_BYTE_ALIGN = 0,
   E_MPEG4E_HALFWORD_ALIGN,
   E_MPEG4E_THIRDBYTE_ALIGN,
   E_MPEG4E_WORD_ALIGN,
}eMPEG4EAlign;
//! End of Enumuration types


//! Structure definitions    
typedef struct
{
    int     s32Seconds;           //!< Number of Seconds
    int     s32MilliSeconds;      //!< Number of Millisecond
}
sMpeg4ETimeStruct;

/*! Structure to hold each memory block requests from the encoder.
 *  The size and alignment are must to meet crteria, whereas others
 *  help to achive the performace projected. Usage is provided to help
 *  the application to decide if the memory has to be saved, in case of
 *  memory scarcity. Type of the memory has a direct impact on the 
 *  performance. Priority of the block shall act as a hint, in case not 
 *  all requested FAST memory is available. There is no gurantee that 
 *  the priority will be unique for all the memory blocks.
 */ 
typedef struct
 {
     int    s32Size;              //!< Size of the memory to be allocated
     int    s32Align;             //!< memory MPEG4E_usage -  static/scratch      
     int    s32Type;              //!< type of the memory slow/fast        
     int    s32Priority;          //!< how important the block is 
     int    s32CurrSize;          //!< For encoder internal MPEG4E_usage
     void  *pvBuffer;             //!< Pointer to the memory
}sMpeg4EMemBlock;

typedef struct
{
     int              s32NumReqs;
     sMpeg4EMemBlock  asMemBlks[MPEG4E_MAX_NUM_MEM_REQS];
}sMpeg4EMemAllocInfo;
            
typedef struct sMpeg4EncoderStruct
{
    unsigned char  *pu8OutputBufferPtr;           //!< Ptr to Output bitstream 
                                                  //!< buffer (1 frame)
    int             s32MaxOutputBufferSize;       //!< Size of output buffer
    int             s32SourceWidth;               //!< Width of source sequence
    int             s32SourceHeight;              //!< Height of source sequence
    int             s32SourceFrameRate;
    int             s32EncodingFrameRate;

    unsigned char  *pu8CurrYAddr;                 //!< Current Frame luminance data
    unsigned char  *pu8CurrCBAddr;                //!< Current Frame Chrominance data
    unsigned char  *pu8CurrCRAddr;                //!< Current Frame Chrominance data

    int             s32StartFrameTime;
    int             s32IntraPeriod;
    int             s32Profile;
    int             s32Level;

    // Error Resilience
    int             s32IntraRefreshMethod;
    int             s32NumForcedIntra;
    unsigned char  *pu8SetRandomIntraMB;           //!< forced intra
    int             s32VideoPacketEnable;          //!< Enable Video Packets
    int             s32DataPartitioningEnable;     //!< Enable Data Partitioning
    int             s32UseRVLC;                    //!< Use RVLC tables
    int             s32ResyncMarkSpacing;          //!< Bits per packet threshold
    int             s32ResyncMarkMBSpacing;        //!< Max num of MBs per packet

    // Quant/AC-DC prediction
    int             s32IntraVopQuant;              //!< Initial Intra quantizer
    int             s32InterVopQuant;              //!< Initial Inter quantizer
    int             s32IntraDcVlcThresh;           //!< Intra DC VLC MPEG4E_usage Threshold
    int             s32IntraAcPredFlag;

    // Motion Estimation
    unsigned char   au8MEAlgorithm[255];
    int             s32MbIntegerSearchDistance;    //!< in half pels, must be even

    // Rate Control Parameters
    int             s32TargetBitRate;              //!< In bits/sec (= 0 for static quant)
    unsigned char   u8DelayFactor;                 //!< 1  - minimum delay, 
                                                   //!< 31 - max delay
                                                   //!< 0  - default delay (minimum) 
    unsigned char   u8QualityTradeoff;             //!< 1  - highest spatial quality
                                                   //!< 31 - highest temporal quality
                                                   //!< 0  - default quality (16) 

    // Rounding Control
    int            s32RoundingControl;             //!< Enable=1, Disable=0
    int            s32InitialRoundValue;

    // H.263 baseline mode
    int            s32ShortVideoHeader;           //!< H.263 bitstream with SVH
    // H.263 baseline mode with no short video header
    int            s32H263NoShortVideoHeader;     //!< Pure H.263 bitstream

    void          *pvData;                        //!< Ptr to Encoder datastructure
    int            s32NextFrameNumber;            //!< Required next frame number 
    int            s32BitstreamSize;              //!< Size of output bitstream buffer
    eMPEG4ECodingType    eFrameType;              //!< Coding type for frame.

    sMpeg4ETimeStruct    s32TotalTimeElapsed;     //!< Time increment 
    sMpeg4EMemAllocInfo  sMemInfo;
}
sMpeg4EncoderConfig;
//! End of structural definitions


//! Set Default parameters
/*!
*  \brief
*  Default parmaters assignment to the primary MPEG4
*  structure parameters. These parameters could be
*  changed by the user by overwriting the default set
*  parameters with the desired values. This should be the
*  first function call.
*  All important parameters such as source width, source height
*  source/encoding frame rate, bit rate, level and data partiton 
*  requirement are set before calling this function
*/ 
eMPEG4ERetType eMpeg4EncoderSetDefaultParam(sMpeg4EncoderConfig *psMpeg4EncPtr);


//! MPEG4 memory allocation info
/*!
*  \brief
*   This is a query memory routine that returns the memory
*   requirement such as size, alignment, type (FAST or SLOW)
*   to the application.
*/ 
eMPEG4ERetType eMpeg4EncoderQueryMemory(sMpeg4EncoderConfig *psMpeg4EncPtr);

//! Initialize Encoder
/*!
*  \brief
*   Initialization function that initializes the frame
*   pointers and other parameters in the encoder so as to
*   provide the settings and frame work for video encode.
*/ 
eMPEG4ERetType eMpeg4EncoderInit(sMpeg4EncoderConfig *psMpeg4EncPtr);

//! Get next frame information
/*!
*  \brief
*   This is an user to get the information on the next frame
*   to be encoded. The routine is called before the calling
*   the encoding frame routine. Information regarding the next
*   frame number and it's type are the important parameters
*   that could be observed here.
*/ 

eMPEG4ERetType  eMpeg4EncoderGetNextFrameNum(sMpeg4EncoderConfig *psMpeg4EncPtr);
//! Encode Image
/*!
*  \brief
*   Function call to encode a single frame/VOP.
*/ 
eMPEG4ERetType eMpeg4EncoderEncode(sMpeg4EncoderConfig *psMpeg4EncPtr);

//! Free Encoder
/*!
*  \brief
*   Function call to free resources. It is to be called when
*   encoding for all the frames are completed.
*/ 
eMPEG4ERetType eMpeg4EncoderFree(sMpeg4EncoderConfig *psMpeg4EncPtr);


#endif
