
#ifndef DECODERINTERFACE_H
#define DECODERINTERFACE_H

////////////////////////////////////////////////////////////////////////////////
//
//              MPEG-4 Visual Decoder developed by IPRL Chicago
//              Copyright 1999 Motorola Inc.
//
//  File Description:   Contains header info for decoder interface.
//
//  Author(s):          (1) vijaypy (vijaypy@motorola.com)
//
//  Version History:
//  Jun/16/2004 - Vijay: Created
//  Jun/23/2004 - Vijay: Review Comments Incorporated.
//  Jun/28/2004 - Debashis: Memquery implemented, removed RGB format structures
//  Aug/11/2004 - Anurag: Implemented APIs for skipping frames.
//  Aug/19/2004 - Anurag: Incorporated Review Comments.
//  Aug/24/2004 - Anurag: Exposed VOP_Type to Application.
//  Aug/10/2004 - Debashis Additional type in memquery
//  Sep/07/2004 - Debashis Renamed file to mpeg4_dec_api.h
//  Nov/09/2004 - Debashis Suport of rewind and starting decoding from pframe
//  Dec/17/2004 - Debashis Support for IPU by providing quant values
//                         Support for changed P frame decoding
//
////////////////////////////////////////////////////////////////////////////////

/*! \file
 * The interface definitions for the Mpeg4 / H.263 decoder library
 */

#define MAX_NUM_MEM_REQS 30         /*! Maximum Number of MEM Requests  */
#define MAX_VIDEO_OBJECTS 1         /*! Maximum Number of Video Objects */
#define VOL_INDEX 0                 /*! VOLs per Video Object           */

/*! \def SCP_MASK
 *  Mask for extracting 24 bit Start Code Prefix from 32 bit MPEG4 Start Codes.
 *  User should not alter the mask.
 */

/*! \def SVH_MASK
 *  Mask for extracting 24 bit Start Code Prefix for Short Video Header from 32
 *  bits. User should not alter the mask.
 */

/*! \def ZERO_MASK_16
 *  Mask for extracting 16 bit 0 from 32 bits. User should not alter the mask.
 */

/*! \def ZERO_MASK_8
 *  Mask for extracting 8 bit 0 from 32 bits. User should not alter the mask.
 */

#define SCP_MASK       0x00ffffff
#define SVH_MASK       0x00ffffff
#define ZERO_MASK_16   0x0000ffff
#define ZERO_MASK_8    0x000000ff


/* defines to specify type of memory. Only one of the two in each group
 * (speed and usage) shall be on but not both.
 */
#define E_MPEG4D_SLOW_MEMORY       0x1   /*! slower memory is acceptable */
#define E_MPEG4D_FAST_MEMORY       0x2   /*! faster memory is preferable */
#define E_MPEG4D_STATIC_MEMORY     0x4   /*! content is used over API calls */
#define E_MPEG4D_SCRATCH_MEMORY    0x8   /*! content is not used over
                                             successessive Decode API calls */

/*! retrieve the type of memory */

#define MPEG4D_IS_FAST_MEMORY(memType)    (memType & E_MPEG4D_FAST_MEMORY)
#define MPEG4D_IS_SLOW_MEMORY(memType)    (memType & E_MPEG4D_SLOW_MEMORY)
#define MPEG4D_IS_STATIC_MEMORY(memType)  (memType & E_MPEG4D_STATIC_MEMORY)
#define MPEG4D_IS_SCRATCH_MEMORY(memType) (memType & E_MPEG4D_SCRATCH_MEMORY)

/*! different type of decoding scheme, if FF/RW is supported */
#define MPEG4D_START_DECODE_AT_IFRAME      1
#define MPEG4D_START_DECODE_AT_PFRAME      2
#define MPEG4D_COLLECT_IMB_FROM_PFRAME     3

/*! All the API return one of the following */

typedef enum
{
    /* Successfull return values */
    E_MPEG4D_SUCCESS = 0,         /*!< Success                             */
    E_MPEG4D_ERROR_CONCEALED,     /*!< Error in the bitstream, but concealed */
    E_MPEG4D_ENDOF_BITSTREAM,     /*!< End of Bit Stream                   */

    /* Successful return with warning, decoding can continue */
    /* Start with number 11 */
    E_MPEG4D_SKIPPED_END_OF_SEQUENCE_CODE = 11,
                                    /*!< Code located within the bitstream */

    /* recoverable error return, correct the situation and continue */
    E_MPEG4D_NOT_ENOUGH_BITS = 31,/*!< Not enough bits are provided        */
    E_MPEG4D_OUT_OF_MEMORY,       /*!< Out of Memory                       */
    E_MPEG4D_WRONG_ALIGNMENT,     /*!< Incorrect Memory alignment          */
    E_MPEG4D_SIZE_CHANGED,        /*!< Image size changed                  */
    E_MPEG4D_INVALID_ARGUMENTS,   /*!< API arguments are invalid           */

    /* irrecoverable error type */
    E_MPEG4D_ERROR_STREAM = 51,   /*!< Errored Bitstream                   */
    E_MPEG4D_FAILURE,             /*!< Failure                             */
    E_MPEG4D_UNSUPPORTED,         /*!< Unsupported Format                  */
    E_MPEG4D_NO_IFRAME,           /*!< MPEG4D_first frame is not an I frame       */
    E_MPEG4D_SIZE_NOT_FOUND,      /*!< Frame size not found in bitstream   */
    E_MPEG4D_NOT_INITIALIZED      /*!< Decoder Not Initialised             */

} eMpeg4DecRetType;


/*! \enum Specifies the current functional state of the Decoder */
typedef enum
{
    E_MPEG4D_INVALID = 0,        /*!< Invalid Decoder State */
    E_MPEG4D_PLAY,               /*!< Decoder is decoding frames */
    E_MPEG4D_FF,                 /*!< Decoder is skipping frames */
    E_MPEG4D_REW                 /*!< Decoder is skipping frames in a direction
                                      opposite to PLAY and FF. Current Decoder
                                      doesn't support REW feature */
} eDecodeState;

/*! \enum Enumeration of possible buffer alignment */
typedef enum
{
    E_MPEG4D_ALIGN_NONE = 0,  /*!< buffer can start at any place           */
    E_MPEG4D_ALIGN_HALF_WORD, /*!< start address's last bit has to be 0    */
    E_MPEG4D_ALIGN_WORD       /*!< start adresse's last 2 bits has to be 0 */
} eMpeg4DecMemAlignType;

/*! \enum Enumeration type for the MB encoding */
typedef enum
{
    E_MPEG4_MB_INTRA = 0,
    E_MPEG4_MB_INTER_1_MV,
    E_MPEG4_MB_INTER_4_MV,
    E_MPEG4_MB_CONCEALED,
    E_MPEG4_MB_SKIPPED,
    E_MPEG4_MB_UNKNOWN
} eMpeg4DecMbType;


/*! Different structure definitions */

/*! structure for Y Cb Cr output from the decoder. */
typedef struct
{
    unsigned char   *pu8YBuf;     /*!< Y Buf       */
    int             s32YBufLen;   /*!< Y Buf Len   */
    unsigned char   *pu8CbBuf;    /*!< Cb Buf      */
    int             s32CbBufLen;  /*!< Cb Buf Len  */
    unsigned char   *pu8CrBuf;    /*!< Cr Buf      */
    int             s32CrBufLen;  /*!< Cr Buf Len  */
} sMpeg4DecYCbCrBuffer;


/*! Structure to hold each memory block requests from the decoder.
 *  The size and alignment are must to meet crteria, whereas others
 *  help to achive the performace projected. MPEG4D_Usage is provided to help
 *  the application to decide if the memory has to be saved, in case of
 *  memory scarcity. Type of the memory has a direct impact on the
 *  performance. Priority of the block shall act as a hint, in case not
 *  all requested FAST memory is available. There is no gurantee that
 *  the priority will be unique for all the memory blocks.
 */

typedef struct
{
    int 	s32Size;         /*!< size of the memory block            */
    int 	s32Type;         /*!< type of the memory - slow/fast and
                                  static/scratch                      */
    int     s32Priority;     /*!< how important the block is          */
    int 	s32Align;        /*!< alignment of the memory block       */
    void 	*pvBuffer;       /*!< pointer to allocated memory buffer  */
} sMpeg4DecMemBlock;


/*! Structure to hold all the memory requests from the decoder  */

typedef struct
{
    int               s32NumReqs;                   /*!< Number of blocks  */
    sMpeg4DecMemBlock asMemBlks[MAX_NUM_MEM_REQS];  /*!< array of requests */
} sMpeg4DecMemAllocInfo;

/*! Structure for Vop Time */
typedef struct
{
    int s32Seconds;
    int s32MilliSeconds;
} sMpeg4DecTime;

/*! Structure for defining the decoder output. */
typedef struct
{
    sMpeg4DecYCbCrBuffer sOutputBuffer;           /*!< Decoded frame      */
    signed char         *p8MbQuants;              /*!< Quant values       */
    unsigned short int 	 u16FrameWidth;           /*!< FrameWidth         */
    unsigned short int 	 u16FrameHeight;          /*!< FrameHeight        */
    unsigned short int   u16DecodingScheme;       /*!< enable decoding at P */

    unsigned short int   u16TicksPerSec;          /*!< Time Ticks Per Sec */
    sMpeg4DecTime        sTime;                   /*!< Current Decode Time */
    int                  s32TimeIncrementInTicks; /*!< Time Increment from
                                                       prev vop decode time
                                                       in ticks */
    unsigned char        u8VopType;               /*!< Current VOP Type */
} sMpeg4DecoderParams;


/*! Structure for defining the Scene Params. Currently it is not useful
 *  as the parameters can have only signle value (1).
 */
typedef  struct
{
    int 	s32NumberOfVos;              /*!< Number of video objects       */
    int 	as32NumberOfVols[MAX_VIDEO_OBJECTS];
                                         /*!< Number of video object layers */
} sMpeg4DecVisualSceneParams;


/*! This structure defines the decoder context. All the decoder API
 *  needs this structure as one of its argument.
 */
typedef struct
{
    sMpeg4DecMemAllocInfo   sMemInfo;      /*!< memory requirements info */
    sMpeg4DecoderParams     sDecParam;     /*!< decoder parameters       */
    sMpeg4DecVisualSceneParams  sVisParam; /*!< visual scence info       */
    void                    *pvMpeg4Obj;   /*!< decoder library object   */
    void                    *pvAppContext; /*!< Anything app specific    */
    eDecodeState            eState;        /*!< Indicates current Decoder
                                                State */
} sMpeg4DecObject;



//Function Declarations

/*! Function to query memory requrement of the decoder */
eMpeg4DecRetType  eMPEG4DQuerymem(sMpeg4DecObject *psMp4Obj,
                                  unsigned char *pu8BitBuffer,
                                  signed long int s32NumBytes);

/*! Function to initialize the decoder */
eMpeg4DecRetType  eMPEG4DInit (sMpeg4DecObject *psMp4Obj);

/*! API to decode single frame of the encoded bitstream */
eMpeg4DecRetType  eMPEG4DDecode (sMpeg4DecObject *psMp4Obj);

/*! Function to free the resources allocated by decoder, if any */
eMpeg4DecRetType  eMPEG4DFree (sMpeg4DecObject *psMp4Obj);


/*! \brief
 * Skips specified number of frames. It may further skips all P Frames until
 * an I Frame, or start decoding immedeately, depending on the decoding
 * scheme specified in the decoder param.
 */
eMpeg4DecRetType  eMPEG4DSkipFrames (sMpeg4DecObject *psMp4Obj,
                                     unsigned int u32NumSkipFrames,
                                     unsigned int *pu32NumSkippedFrames);

/*! \brief
 *        Skips to next INTRA frame
 */
eMpeg4DecRetType  eMPEG4DSkip2NextIFrame (sMpeg4DecObject *psMp4Obj,
                                          unsigned int *pu32NumSkippedFrames);

/*! \brief
 *    Rewinds the given number of frames starting from the current frame.
 * The decoder can later skip all the P frames, till the next I frame, or can
 * start decoding immedeately, even if it is p frame.
 */
eMpeg4DecRetType eMPEG4DRewindFrames (sMpeg4DecObject *psMp4Obj,
                                      unsigned int u32NumSkipFrames,
                                      int *pu32NumSkippedFrames);


/*! \brief
 * Call back function for reading the input buffer by the decoder.
 *
 * Whenver the internul buffer inside the decoder is empty, the decoder will
 * call this function, passing the application context also. The application
 * shall implement a function to provide the required portion of the bitstream
 * of the given size, starting at the given offset in the provided buffer.
 */
extern int cbkMPEG4DBufRead (int s32BufLen, unsigned char *pu8Buf,
                             int s32Offset, void *pvAppContext);

#endif  /* ifndef DECODERINTERFACE_H */
