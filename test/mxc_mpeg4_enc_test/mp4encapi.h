/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--                   (C) COPYRIGHT 2003 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--          The entire notice above must be reproduced on all copies.         --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : Hantro MPEG4 Encoder API
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: mp4encapi.h,v $
--  $Revision: 1.3 $
--  $Name: Rel5251-HW_Encoder_Control_Code_GLA-Linux_2_6-iMX31-src-1_1 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Type definition for encoder instance
    2. Enumerations for API parameters
    3. Structures for API function parameters
    4. Encoder API function prototypes

------------------------------------------------------------------------------*/

#ifndef __MP4ENCAPI_H__
#define __MP4ENCAPI_H__

#include "basetype.h"

#ifdef __cplusplus
extern "C"
    {
#endif

/*------------------------------------------------------------------------------
    1. Type definition for encoder instance
------------------------------------------------------------------------------*/

typedef void *MP4EncInst;

/*------------------------------------------------------------------------------
    2. Enumerations for API parameters
------------------------------------------------------------------------------*/

/* Function return values */
typedef enum
{
    ENC_OK = 0,
    ENC_VOP_READY = 1,
    ENC_GOV_READY = 2,
    ENC_VOP_READY_VBV_FAIL = 3,

    ENC_ERROR = -1,
    ENC_NULL_ARGUMENT = -2,
    ENC_INVALID_ARGUMENT = -3,
    ENC_MEMORY_ERROR = -4,
    ENC_EWL_ERROR = -5,
    ENC_EWL_MEMORY_ERROR = -6,
    ENC_INVALID_STATUS = -7,
    ENC_OUTPUT_BUFFER_OVERFLOW = -8,
    ENC_HW_ERROR = -9,
    ENC_HW_TIMEOUT = -10,
    ENC_SYSTEM_ERROR = -11,
    ENC_INSTANCE_ERROR = -12
}
MP4EncRet;

/* Stream type for initialization */
typedef enum
{
    MPEG4_PLAIN_STRM = 0,
    MPEG4_VP_STRM = 1,
    MPEG4_VP_DP_STRM = 2,
    MPEG4_VP_DP_RVLC_STRM = 3,
    MPEG4_SVH_STRM = 4,
    H263_STRM = 5
}
MP4EncStrmType;

/* Profile and level for initialization */
typedef enum
{
    MPEG4_SIMPLE_PROFILE_LEVEL_0  = 8,
    MPEG4_SIMPLE_PROFILE_LEVEL_0B = 9,
    MPEG4_SIMPLE_PROFILE_LEVEL_1 = 1,
    MPEG4_SIMPLE_PROFILE_LEVEL_2 = 2,
    MPEG4_SIMPLE_PROFILE_LEVEL_3 = 3,
    MPEG4_ADV_SIMPLE_PROFILE_LEVEL_3 = 243,
    MPEG4_ADV_SIMPLE_PROFILE_LEVEL_4 = 244,
    MPEG4_ADV_SIMPLE_PROFILE_LEVEL_5 = 245,
    H263_PROFILE_0_LEVEL_10 = 1001,
    H263_PROFILE_0_LEVEL_20 = 1002,
    H263_PROFILE_0_LEVEL_30 = 1003,
    H263_PROFILE_0_LEVEL_40 = 1004,
    H263_PROFILE_0_LEVEL_50 = 1005,
    H263_PROFILE_0_LEVEL_60 = 1006,
    H263_PROFILE_0_LEVEL_70 = 1007
}
MP4EncProfileAndLevel;

/* User data type */
typedef enum
{
    MPEG4_VOS_USER_DATA,
    MPEG4_VO_USER_DATA,
    MPEG4_VOL_USER_DATA,
    MPEG4_GOV_USER_DATA
}
MP4EncUsrDataType;

/* VOP type for encoding */
typedef enum
{
    INTRA_VOP = 0,
    PREDICTED_VOP = 1,
    NOTCODED_VOP              /* Used just as a return type */
} MP4EncVopType;


/*------------------------------------------------------------------------------
    3. Structures for API function parameters
------------------------------------------------------------------------------*/

/* Configuration info for initialization */
typedef struct
{
    MP4EncStrmType strmType;
    MP4EncProfileAndLevel profileAndLevel;
    i32 width;
    i32 height;
    i32 frmRateNum;
    i32 frmRateDenom;
}
MP4EncCfg;

/* Coding control parameters */
typedef struct
{
    i32 insHEC;
    i32 insGOB;
    i32 insGOV;
    i32 vpSize;
}
MP4EncCodingCtrl;

/* Rate control parameters */
typedef struct
{
    i32 vopRc;
    i32 mbRc;
    i32 vopSkip;
    i32 qpHdr;
    i32 qpHdrMin;
    i32 qpHdrMax;
    i32 bitPerSecond;
    i32 vbv;
    i32 cir;
}
MP4EncRateCtrl;

/* Encoder input structure */
typedef struct
{
    u32 busLuma;
    u32 busChromaU;
    u32 busChromaV;
    u32 *pOutBuf;
    u32 outBufBusAddress;
    i32 outBufSize;
    MP4EncVopType vopType;
    i32 timeIncr;

    /* Table for video packet sizes, length must be 1024*sizeof(i32) bytes.
     * Encoder stores the size of each video packet (bytes) in encoded VOP
     * in the table. Zero value is written after last video packet.
     * Only 1023 first video packet sizes are stored in the table. */
    i32 *pVpSizes;
}
MP4EncIn;

/* Time code structure for encoder output */
typedef struct
{
    i32 hours;
    i32 minutes;
    i32 seconds;
    i32 timeIncr;
    i32 timeRes;
}
TimeCode;

/* Encoder output structure */
typedef struct
{
    TimeCode timeCode;
    MP4EncVopType vopType;
    i32 strmSize;
}
MP4EncOut;

/* Input cropping and camera stabilization parameters */
typedef struct
{
    i32 origWidth;
    i32 origHeight;
    i32 xOffset;
    i32 yOffset;
    i32 stabArea;
}
MP4EncCropCfg;

/* Cropping information */
typedef struct
{
    i32 xOffset;
    i32 yOffset;
}
MP4EncFrmPos;

/* Version information */
typedef struct
{
    i32 major;    /* Encoder API major version */
    i32 minor;    /* Encoder API minor version */
} MP4EncApiVersion;

/*------------------------------------------------------------------------------
    4. Encoder API function prototypes
------------------------------------------------------------------------------*/

/* Version information */
MP4EncApiVersion MP4EncGetVersion(void);

/* Initialization & release */
MP4EncRet MP4EncInit(MP4EncCfg * pEncCfg, MP4EncInst * instAddr);
MP4EncRet MP4EncRelease(MP4EncInst inst);

/* Encoder configuration */
MP4EncRet MP4EncSetCodingCtrl(MP4EncInst inst, MP4EncCodingCtrl *
                                pCodeParams);
MP4EncRet MP4EncGetCodingCtrl(MP4EncInst inst, MP4EncCodingCtrl *
                                pCodeParams);
MP4EncRet MP4EncSetRateCtrl(MP4EncInst inst, MP4EncRateCtrl * pRateCtrl);
MP4EncRet MP4EncGetRateCtrl(MP4EncInst inst, MP4EncRateCtrl * pRateCtrl);
MP4EncRet MP4EncSetUsrData(MP4EncInst inst, const u8 * pBuf, u32 length,
                             MP4EncUsrDataType type);

/* Stream generation */
MP4EncRet MP4EncStrmStart(MP4EncInst inst, MP4EncIn * pEncIn,
                            MP4EncOut * pEncOut);
MP4EncRet MP4EncStrmEncode(MP4EncInst inst, MP4EncIn * pEncIn,
                             MP4EncOut * pEncOut);
MP4EncRet MP4EncStrmEnd(MP4EncInst inst, MP4EncIn * pEncIn,
                          MP4EncOut * pEncOut);

/* Pre processing: filter */
MP4EncRet MP4EncSetSmooth(MP4EncInst inst, u32 status);
MP4EncRet MP4EncGetSmooth(MP4EncInst inst, u32 * status);

/* Pre processing: camera stabilization */
MP4EncRet MP4EncSetCrop(MP4EncInst inst, MP4EncCropCfg * pCropCfg);
MP4EncRet MP4EncGetCrop(MP4EncInst inst, MP4EncCropCfg * pCropCfg);
MP4EncRet MP4EncGetLastFrmPos(MP4EncInst inst, MP4EncFrmPos * pFrmPos);

#ifdef __cplusplus
    }
#endif

#endif /*__MP4ENCAPI_H__*/
