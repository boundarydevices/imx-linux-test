/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--              (C) COPYRIGHT 2001,2002,2003 HANTRO PRODUCTS OY               --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--          The entire notice above must be reproduced on all copies.         --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract :  The API definition of the MPEG4 Encoder
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: MP4EncApi.h,v $
--  $Date: 2006/03/10 09:06:57 $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Enumeration for the Encoding Schemes
    2. Enumeration for the Parameter IDs
    3. Structures for the information interchange between the MPEG4 Encoder
       and the user application
    4. Enumeration for Encoder API return values
    5. Enumeration for the video frame (VOP) coding types
    6. Structure for passing input data to Encode() function
    7. Structure for the returning the function status information from the API
    8. Structure for returning the API version information
    9. Prototypes of the MPEG4 API functions

------------------------------------------------------------------------------*/

#ifndef MP4ENCAPI_H
#define MP4ENCAPI_H

#include "basetype.h"

/*------------------------------------------------------------------------------
    1. Enumeration for the Encoding Schemes
------------------------------------------------------------------------------*/

    /* Encoding schemes */
typedef enum
{
    MP4API_ENC_SCHEME_QCIF_15FPS_64KB_PLAIN = 1,
    MP4API_ENC_SCHEME_QCIF_15FPS_64KB_VP = 2,
    MP4API_ENC_SCHEME_QCIF_10FPS_32KB_VP = 3,
    MP4API_ENC_SCHEME_QCIF_30FPS_128KB_VP = 4,
    MP4API_ENC_SCHEME_CIF_15FPS_128KB_VP = 5,
    MP4API_ENC_SCHEME_CIF_15FPS_384KB_VP = 6,
    MP4API_ENC_SCHEME_CIF_15FPS_384KB_PLAIN = 7,
    MP4API_ENC_SCHEME_CIF_30FPS_384KB_VP = 8,
    MP4API_ENC_SCHEME_QCIF_15FPS_64KB_SVH = 9,
    MP4API_ENC_SCHEME_CIF_15FPS_384KB_SVH = 10,
    MP4API_ENC_SCHEME_QVGA_15FPS_384KB_VP = 11,
    MP4API_ENC_SCHEME_SUBQCIF_10FPS_32KB_PLAIN = 12,
    MP4API_ENC_SCHEME_H263_PROFILE0_LEVEL10 = 13
}
MP4API_EncScheme;

/*------------------------------------------------------------------------------
    2. Enumeration for the Parameter IDs
------------------------------------------------------------------------------*/
typedef enum
{
    /* Quantization Parameter to be used as default QP */
    /* Type of the parameter to be changed: u32 */
    MP4API_ENC_PID_QUANT_PARAM = 1,

    /* Header Extension Code Enable (can be used with VideoPackets) */
    /* Type of the parameter to be changed: u32 */
    MP4API_ENC_PID_HEC_ENABLE,

    /* Target Bit Rate */
    /* Type of the parameter to be changed: u32 */
    MP4API_ENC_PID_BIT_RATE,

    /* Video Object Sequence User data */
    /* Type of the parameter to be changed: MP4API_EncParam_UserData */
    MP4API_ENC_PID_VOS_USER_DATA,

    /* Video Object User data */
    /* Type of the parameter to be changed: MP4API_EncParam_UserData */
    MP4API_ENC_PID_VO_USER_DATA,

    /* Video Object Layer User data */
    /* Type of the parameter to be changed: MP4API_EncParam_UserData */
    MP4API_ENC_PID_VOL_USER_DATA,

    /* Group Of VOP User data */
    /* Type of the parameter to be changed: MP4API_EncParam_UserData */
    MP4API_ENC_PID_GOV_USER_DATA,

    /* Enables pre-processing */
    /* Type of the parameter to be changed: u32 */
    MP4API_ENC_PID_PREPROCESSING_ENABLE,

    /* Definition of the Pixel Aspect Ratio */
    /* Type of the parameter to be changed: */
    /* MP4API_EncParam_PixelAspectRatio */
    MP4API_ENC_PID_PIXEL_ASPECT_RATIO,

    /* Short Video Header (H.263 baseline) information */
    /* Type of the parameter to be changed: */
    /* MP4API_EncParam_ShortVideoHeader */
    MP4API_ENC_PID_SHORT_VIDEO_HEADER,

    /* Camera Movement Stabilization Control */
    /* Type of the parameter to be changed: */
    /* MP4API_EncParam_CameraStabilization */
    MP4API_ENC_PID_CAM_STAB,

    /*  MPEG4 Stream Type Information as defined in the MPEG4 Standard */
    /* Type of the parameter to be changed: */
    /* MP4API_EncParam_StreamTypeInfo */
    MP4API_ENC_PID_STREAM_TYPE_INFO,

    /* Cyclic Intra Refresh */
    /* Type of the parameter to be changed: */
    /* MP4API_EncParam_CyclicIntraRefresh */
    MP4API_ENC_PID_CYCLIC_INTRA_REFRESH,

    /* Rate Control */
    /* Type of the parameter to be changed: MP4API_EncParam_RateControl */
    MP4API_ENC_PID_RATE_CTRL,

    /* Time Code at the Group Of VOP (GOV) layer of the MPEG4 stream. */
    /* Time information in real time format (Hours, Minutes, Seconds) */
    /* Type of the parameter to be changed: MP4API_EncParam_TimeCode */
    MP4API_ENC_PID_TIME_CODE,

    /* Size of the Video Packet */
    /* Note 1: The size of a Video Packet (VP) cannot be */
    /* bigger than the size of SW/HW RLC buffer. Otherwise */
    /* An overflow can occure. */
    /* Note 2: The MPEG4 Standard restricts the size */
    /* Type of the parameter to be changed: u32 */
    MP4API_ENC_PID_VIDEO_PACKET_SIZE,

    /* Group Of VOP (GOV) Header Insert Control. Value reseted when */
    /* GOV headers inserted into stream */
    /* Type of the parameter to be changed: u32 */
    MP4API_ENC_PID_GOV_HEADER_ENABLE,

    /* Size of the picture in pixels */
    /* Note: Encoder processes a picture by dividing it into macroblocks */
    /* which are 16x16 pixels. Width and height of a picture to be */
    /* encoded have to be divisible evenly by 16. Anyhow it is possible */
    /* to set the picture size information inserted into stream to show */
    /* smaller size. */
    /* Type of the parameter to be changed: MP4API_EncParam_PictureSize */
    MP4API_ENC_PID_PICTURE_SIZE,

    /* Control Parameters for Changing the Macroblock Type */
    /* Do NOT change if not familiar with encoder details */
    /* Type of the parameter to be changed: MP4API_EncParam_IntraControl */
    MP4API_ENC_PID_INTRA_CONTROL,

    /* Defines Which Error Detection and Concealment Tools are used */
    /* Type of the parameter to be changed: MP4API_EncParam_ErrorTools */
    MP4API_ENC_PID_ERROR_TOOLS,

    /* Compression Parameters defining: */
    /*  - Rounding Control in the interpolation */
    /*  - Usage of the AC Prediction */
    /*  - Whether Motion Vectors are allowed to point out of the picture */
    /*  - Coding of the Intra DC components */
    /* Type of the parameter to be changed: */
    /* MP4API_EncParam_CompressionParameters */
    MP4API_ENC_PID_COMPRESSION_PARAMS,

    /* Time Resolution defining how many evenly spaced unit (ticks) */
    /* one second contains (e.g. if frame rate is 15 FPS */
    /* => Time Resolution = 15. Therefore TimeIncrement between  */
    /* two picture frames is 1 */
    /* Type of the parameter to be changed: u16 */
    MP4API_ENC_PID_TIME_RESOLUTION,

    /* Hardware Interrupt Interval */
    /* Do NOT change if not absolute familiar with Encoder details */
    /* Sets the (minimum) number of macroblocks encoded during one */
    /* ASIC run. */
    /* Type of the parameter to be changed: u16 */
    MP4API_ENC_PID_HW_INTERRUPT_INTERVAL,

    /* Bit Usage */
    /* Detailed information of bit usage */
    /* Type of the parameter to be read: MP4API_EncParam_BitUsage */
    MP4API_ENC_PID_BIT_USAGE,

    /* Picture Coded Information */
    /* Whether the picture (VOP) was coded or not */
    /* Picture (VOP) can be set as not coded by the Rate Control. */
    /* A VOP can be set to be not-coded if the virtual buffer is full. */
    /* The feature of skipping picture frames can be tuned by changing */
    /* the FrameSkipLimit at RateControl parameters */
    /* Type of the parameter to be read: u32 */
    MP4API_ENC_PID_PICTURE_CODED,

    /* check if  the texture VLC buffer limit was reached by ASIC */
    /* bigger buffer should be used when limit is often reached */
    /* Type of the parameter to be read: u32 */
    MP4API_ENC_PID_TXTR_VLC_BUF_FULL,

    /* setup a table where macroblock offsets are returned */
    /* Type of the parameter to be set: (u32*) */
    MP4API_ENC_PID_MB_OFFSET_TABLE,
    /* setup a table where macroblock QP are returned */
    /* Type of the parameter to be set: (u8*) */
    MP4API_ENC_PID_MB_QP_TABLE
}
MP4API_EncParamId;

/*------------------------------------------------------------------------------
    3. Structures for the information interchange between the MPEG4 Encoder
       and the user application
------------------------------------------------------------------------------*/
typedef struct  /* MP4API_EncParam_UserData */
{
    /* Enable user data insertion into stream */
    u8 u8_Enable;

    /* Pointer to user data to be inserted into stream */
    u8 *pu8_UserData;

    /* Number of bytes to be inserted into stream */
    u32 u32_Length;

}
MP4API_EncParam_UserData;

typedef struct  /* MP4API_EncParam_PixelAspectRatio */
{
    /* Pixel aspect ratio information (Specified in MPEG4 standard) */
    u8 u8_AspectRatioInfo;

    /* Pixel width if required by the AspectRatioInfo */
    u8 u8_ParWidth;

    /* Pixel height if required by the AspectRatioInfo */
    u8 u8_ParHeight;

}
MP4API_EncParam_PixelAspectRatio;

typedef struct  /* MP4API_EncParam_ShortVideoHeader */
{
    /* Enable Short Video Header Mode */
    u8 u8_ShortVideoHeaderEnable;
    /* generate H263 compatible stream */
    u8 u8_H263_Stream;
    /* Group Of Blocks header insertion frequency */
    /* Values from 0 to 8: 0 = Never add GOB Header into the stream */
    /* 1 = GOB Header with every GOB/MacroBlock-row, */
    /* 2 = every 2nd MB-row ... 8 = every 8th MB-row */
    u8 u8_GobResyncMarkerInsertion;

    /* SVH Specific parameters as defined in MPEG4 Standard */
    u8 u8_SplitScreenIndicator;
    u8 u8_DocumentCameraIndicator;
    u8 u8_FullPictureFreezeRelease;

}
MP4API_EncParam_ShortVideoHeader;

typedef struct  /* MP4API_EncParam_CameraStabilization */
{
    /* Enable Camera Movement Stabilization */
    u32 u32_CamStabEnable;

    /* Captured frame width */
    u32 u32_FrameWidth;

    /* captured frame height */
    u32 u32_FrameHeight;
}
MP4API_EncParam_CameraStabilization;

typedef struct  /* MP4API_EncParam_StreamTypeInfo */
{
    /* MPEG4 Stream type information as defined in the standard */
    /* Profile and level information */
    /* 8 = Simple profile level 0 */
    /* 1 = Simple Profile Level 1 */
    /* 2 = Simple Profile Level 2 */
    /* 3 = Simple Profile Level 3 */
    u8 u8_ProfileAndLevel;

    /* Type of the Video Signal */
    u8 u8_VideoSignalType;

    /* Video Format */
    u8 u8_VideoFormat;

    /* Video Range */
    u8 u8_VideoRange;

    /* Color Description Information */
    u8 u8_ColorDescription;
    u8 u8_ColorPrimaries;
    u8 u8_TransferCharacteristics;
    u8 u8_MatrixCoefficients;

    /* Define Random Accessible VOL Feature. */
    /* Set all the VOP to be encoded as Intra VOPs */
    u8 u8_RandomAccessibleVol;

}
MP4API_EncParam_StreamTypeInfo;

typedef struct  /* MP4API_EncParam_CyclicIntraRefresh */
{
    /* Enable Cyclic Intra Refresh */
    u32 u32_CirEnable;

    /* Number of Intra MBs in a picture (VOP) */
    u32 u32_IntraPerVop;

}
MP4API_EncParam_CyclicIntraRefresh;

typedef struct  /* MP4API_EncParam_RateControl */
{
    /* Rate Control Enable */
    u32 u32_RateCtrlEnable;

    /* Target Bit Rate (bits/second) */
    u32 u32_TargetBitRate;

    /* Target Frame Rate (frames/second) */
    u32 u32_TargetFrameRate;

    /* flag to enable/disable frame skipping */
    u32 u32_FrameSkipEnable;

    /* Enables 4 motion vectors feature */
    u32 u32_FourMotionVectorsEnable;

    /* Quantization Parameter (QP). The value is used for the Intra */
    /* VOPs and as default QP value if RateCtrl is disabled */
    u32 u32_DefaultQuantParameter;

    struct
    {
        u32 u32_Enable; /* Enable VBV model */
        u32 u32_Set;    /* if 0 use default values */
        u32 u32_BufSize;    /* VBV or HRD buffer size */
        u32 u32_InitOccupancy;  /* initial buffer occupancy */
    }
    vbv_params;
}
MP4API_EncParam_RateControl;

typedef struct  /* MP4API_EncParam_TimeCode */
{
    u32 u32_Hours;
    u32 u32_Minutes;
    u32 u32_Seconds;
}
MP4API_EncParam_TimeCode;

typedef struct  /* MP4API_EncParam_PictureSize */
{
    /* Picture Width in Pixels */
    u16 u16_Width;

    /* Picture Height in Pixels */
    u16 u16_Height;

}
MP4API_EncParam_PictureSize;

typedef struct  /* MP4API_EncParam_IntraControl */
{

    /* Penalty values when deciding the macroblock type */
    u16 u16_MvSadPenalty;
    u16 u16_IntraSadPenalty;
    u16 u16_Inter4MvSadPenalty;

}
MP4API_EncParam_IntraControl;

typedef struct  /* MP4API_EncParam_ErrorTools */
{
    /* Enable use of the Video Packets */
    u8 u8_VideoPacketsEnable;

    /* Enable use of the Data Partition */
    u8 u8_DataPartitionEnable;

    /* Enable use of the Reversible VLC instead of VLC */
    /* Note: Data partition have to be enabled if RVLC is used */
    u8 u8_RVlcEnable;

}
MP4API_EncParam_ErrorTools;

typedef struct  /* MP4API_EncParam_CompressionParameters */
{
    /* Enable AC Prediction */
    u8 u8_AcPredictionEnable;

    /* Allow Motion Vectors point out of the picture */
    u8 u8_OutsideVopMvEnable;

    /* Rounding Control for the Interpolation */
    u8 u8_RoundingCtrl;

    /* Threshold Value Defining when DC components of Intra MBs */
    /* should be coded with DC VLC Instead of AC VLC */
    /* (0 = Intra DC VLC always, 7 = Intra AC VLC always) */
    /* Dependency of QPs and the threshold defined in the MPEG4 Standard */
    u8 u8_IntraDcVlcThr;

}
MP4API_EncParam_CompressionParameters;

typedef struct  /* MP4API_EncParam_BitUsage */
{
    /* Size of the stream headers */
    u32 u32_HeaderDataSize;

    /* Size of the motion vectors */
    u32 u32_MvDataSize;

    /* Size of the texture data */
    u32 u32_TextureDataSize;

    /* Size of the pattern data (which blocks were coded) */
    u32 u32_PatternDataSize;

}
MP4API_EncParam_BitUsage;

/*------------------------------------------------------------------------------
    4. Enumeration for Encoder API return values
------------------------------------------------------------------------------*/
typedef enum
{
    MP4API_ENC_OK = 0x00000000,
    MP4API_ENC_VIDEO_PACKET_READY = 0x00000001,
    MP4API_ENC_PICTURE_READY = 0x00000002,

    /* general error codes starting with 0x10000000 */
    MP4API_ENC_ERROR = 0x10000000,
    MP4API_ENC_MEM_ALLOC_FAILED = 0x10000001,
    MP4API_ENC_INVALID_SCHEME = 0x10000002,
    MP4API_ENC_STRM_GEN_ERROR = 0x10000003,
    MP4API_ENC_INVALID_PARAM = 0x10000004,
    MP4API_ENC_NULL_POINTER = 0x10000005,
    MP4API_ENC_STRM_ALREADY_STARTED = 0x10000006,
    MP4API_ENC_VOP_NOT_READY = 0x10000007,
    MP4API_ENC_BAD_INSTANCE = 0x10000008,
    MP4API_ENC_STRM_NOT_STARTED = 0x10000009,
    MP4API_ENC_OUT_BUF_OVERFLOW = 0x1000000A,

    MP4API_ENC_FATAL_ERROR = 0x1FFFFFFF,

    /* Init error codes starting with 0x20000000 */
    MP4API_RC_INIT_ERROR = 0x20000000,
    MP4API_CS_INIT_ERROR = 0x20000001,
    MP4API_MVD_INIT_ERROR = 0x20000002,
    MP4API_STRM_INIT_ERROR = 0x20000003,

    /*  HW errors */
    MP4API_ENC_INVALID_ASIC = 0x30000000,
    MP4API_ENC_HWRESET_FAILED = 0x30000001,

    /* HOS specific errors */
    MP4API_ENC_HOS_INIT_ERROR = 0x40000000,
    MP4API_ENC_HOS_WAIT_TIMEOUT = 0x40000001,
    MP4API_ENC_HOS_WAIT_ERROR = 0x40000002
}
MP4API_EncReturnValue;

/*------------------------------------------------------------------------------
    5. Enumeration for the video frame (VOP) coding types
------------------------------------------------------------------------------*/
typedef enum
{
    MP4API_ENC_VOP_CODING_TYPE_INTRA = 0,
    MP4API_ENC_VOP_CODING_TYPE_INTER = 1
}
MP4API_EncVopCodingType;

/*------------------------------------------------------------------------------
    6. Structure for passing input data to Encode() function
------------------------------------------------------------------------------*/
typedef struct  /* MP4API_EncInput */
{
    u32 *pu32_Picture;  /* Pointer to input picture (bus address) */
    u32 u32_TimeIncrement;  /* Time resolution units after prev picture */
    u32 *pu32_OutputBuffer; /* Pointer to output stream buffer */
    u32 u32_OutBufSize; /* output buffer size in bytes (4 multiple) */
    MP4API_EncVopCodingType VopCodingType;  /* Intra or Inter */
}
MP4API_EncInput;

/*------------------------------------------------------------------------------
    7. Structure for the returning the function status information from the API
------------------------------------------------------------------------------*/
typedef struct  /* MP4API_EncOutput */
{
    MP4API_EncReturnValue Code; /* Return status information */
    u32 u32_Size;   /* Number of bytes written into output buffer */
}
MP4API_EncOutput;

/*------------------------------------------------------------------------------
    8. Structure for returning the API version information
------------------------------------------------------------------------------*/
    typedef struct /* MP4API_EncAPIVersion */
    {
        u32 u32_Major;    /* Encoder API major version */
        u32 u32_Minor;    /* Encoder API minor version */
    } MP4API_EncAPIVersion;

/*------------------------------------------------------------------------------
    9. Prototypes of the MPEG4 API functions
------------------------------------------------------------------------------*/
void *MP4API_EncoderInit(MP4API_EncScheme EncScheme);

u32 MP4API_EncoderConfig(void *pEncInst,
                         void *pNewValue, MP4API_EncParamId ParameterId);

u32 MP4API_EncoderGetInfo(void *pEncInst,
                          void *pOutput, MP4API_EncParamId ParameterId);

u32 MP4API_EncoderStartStream(void *pEncInst,
                              MP4API_EncInput * pInput,
                              MP4API_EncOutput * pOutput);

u32 MP4API_Encode(void *pEncInst,
                  MP4API_EncInput * pInput, MP4API_EncOutput * pOutput);

u32 MP4API_EncoderEndStream(void *pEncInst,
                            MP4API_EncInput * pInput,
                            MP4API_EncOutput * pOutput);

u32 MP4API_EncoderShutdown(void *pEncInst);

MP4API_EncAPIVersion MP4API_EncoderGetAPIVersion(void);

#endif /* MP4ENCAPI_H */
