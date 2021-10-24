/*==================================================================================================
*   @file       avistream.h
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      API header.
*   @details    Contains declaration of the avi API functions.
*
*   @addtogroup AVI STREAM
*   @{
*/
/*==================================================================================================
*   Project              : Avi stream
*   Platform             : Reserved
*
*   SW Version           : 1.0.0
*   Build Version        : Reserved
*
*   Copyright 2020 Inc.
*   All Rights Reserved.
==================================================================================================*/

#ifndef AVI_STREAM_H
#define AVI_STREAM_H

#ifdef __cplusplus
extern "C"{
#endif


/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "stm32f2xx.h"
#include "Std_Types.h"
/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
#define MEDIA_AVI_BPP24 (24U)
#define MEDIA_AVI_BPP16 (16U)
#define MEDIA_AVI_BI_RGB (0x00000000UL)
#define MEDIA_WAVE_FORMAT_PCM (0x0001U)

#define MEDIA_AVIF_HASINDEX        (0x00000010UL) /* Index at end of file? */
#define MEDIA_AVIF_MUSTUSEINDEX    (0x00000020UL)
#define MEDIA_AVIF_ISINTERLEAVED   (0x00000100UL)
#define MEDIA_AVIF_TRUSTCKTYPE     (0x00000800UL) /* Use CKType to find key frames */
#define MEDIA_AVIF_WASCAPTUREFILE  (0x00010000UL)
#define MEDIA_AVIF_COPYRIGHTED     (0x00010000UL)

#define MEDIA_AVISF_DISABLED          (0x00000001UL)
#define MEDIA_AVISF_VIDEO_PALCHANGES  (0x00010000UL)


#ifndef MEDIA_AVIIF_LIST
    #define MEDIA_AVIIF_LIST       (0x00000001UL)
    #define MEDIA_AVIIF_KEYFRAME   (0x00000010UL)
#endif

#define MEDIA_AVIIF_NO_TIME    (0x00000100UL)
#define MEDIA_AVIIF_COMPRESSOR (0x0FFF0000UL)

#define MEDIA_X_COUNT 128
#define MEDIA_Y_COUNT 64
#define MEDIA_COLOR_COUNT 2
#define MEDIA_VIDEO_MIN_FLAME_RATE 15
#define MEDIA_VIDEO_MAX_FLAME_RATE 60
#define MEDIA_AUDIO_MIN_SAMPLE_RATE 44099
#define MEDIA_AUDIO_MAX_SAMPLE_RATE 48000

#define MEDIA_AUDIO_MIN_CHANNEL 1
#define MEDIA_AUDIO_MAX_CHANNEL 2

#define MEDIA_MAX_TRACK_COUNT (100U)
#define MEDIA_MAX_NAME_LENGHT (200U)


#define VERTICAL 0
#define HORIZONTAL 1

#define PLAY 0
#define STOP 1
#define PAUSE 2

#define LOOP_NO 0
#define LOOP_SINGLE 1
#define LOOP_ALL 2

#define LINKMAP_TABLE_SIZE 0x14

#define FONT_DATA_PIXEL_SIZE_X 5
#define FONT_DATA_PIXEL_SIZE_Y 8
#define FONT_DATA_MAX_CHAR 192
#define DISPLAY_TEXT_TIME 1000
#define DISPLAY_TEXT_MAX 45



#define IMAGE_READ_DELAY_FLAME 2

#define DISPLAY_FPS_AVERAGE_TIME 0.4

#define MAX_PLAYLIST_COUNT 9

#define SKIP_TIME 5

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/
typedef enum
{
    FILE_OK = 0,
    FATFS_ERROR = 1,
    FILE_ERROR = 2,
    OTHER_ERROR = 10
} MEDIA_FileResult_Type;

typedef enum
{
    MEDIA_STOP,
    MEDIA_PLAY,
    MEDIA_PAUSE
} MEDIA_State_Type;

typedef enum
{
    FLAG_SET,
    FLAG_RESET
} MEDIA_EndFlag_Type;
/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
#pragma pack(2)
typedef struct Media_BitMapFileHeader
{
    uint16_t u16Type;
    uint32_t u32Size;
    uint16_t u16Reserved1;
    uint16_t u16Reserved2;
    uint32_t u32OffBits;
} Media_BitMapFileHeader_TagType;

#pragma pack()
typedef struct Media_BitMapInfoHeader
{
    uint32_t u32Size;
    int32_t s32Width;
    int32_t s32Height;
    uint16_t u16Planes;
    uint16_t u16BitCount;
    uint32_t u32Compression;
    uint32_t u32SizeImage;
    int32_t s32XPelsPerMeter;
    int32_t s32YPelsPerMeter;
    uint32_t u32ClrUsed;
    uint32_t u32ClrImportant;
} Media_BitMapInfoHeader_TagType;

/* Un-used */
typedef struct Media_ChunkHeader
{
    uint8_t u8ChunkId[4U];
    uint32_t u32ChunkSize;
} Media_ChunkHeader_TagType;

/* Un-used */
typedef struct Media_Fiff
{
    Media_ChunkHeader_TagType nHeader;
    uint8_t u8Format[4U];
} Media_Fiff_TagType;

#pragma pack(2)
typedef struct Media_WaveFormatEx
{
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
} Media_WaveFormatEx_TagType;
#pragma pack()

typedef struct Media_Fourcc
{
    uint8_t u8Fcc[4U];
} Media_Fourcc_TagType;

typedef struct Media_RiffChunk
{
    Media_Fourcc_TagType nFcc;
    uint32_t  u32Cb;
} Media_RiffChunk_TagType, * Media_LpRiffChunk_TagType;

typedef struct Media_RiffList
{
    Media_Fourcc_TagType nFcc;
    uint32_t u32Cb;
    Media_Fourcc_TagType nFccListType;
} Media_RiffList_TagType;

/**
  * @brief  Avi header structures.
  * @details    Main header for the avi file (compatibility header).
  *
  * @Note 'avih' field.
  */
typedef struct Media_Avi_MainHeader
{
    Media_Fourcc_TagType nFcc; /* Type of this structure 'avih'. */
    uint32_t u32Cb; /* Size of this structure -8. */
    uint32_t u32MicroSecPerFrame; /* Frame display rate (or 0L). */
    uint32_t u32MaxBytesPerSec; /* Max. transfer rate. */
    uint32_t u32PaddingGranularity; /* Pad to multiples of this size; normally 2K. */
    uint32_t u32Flags; /* The ever-present flags. */
    uint32_t u32TotalFrames; /* Frames in first movi list. */
    uint32_t u32InitialFrames; /* Ingore that. */
    uint32_t u32Streams; /* Number of streams in the file. */
    uint32_t u32SuggestedBufferSize; /* Size of buffer required to hold chunks of the file. */
    uint32_t u32Width; /* Width of video stream. */
    uint32_t u32Height; /* Height of video stream. */
    uint32_t u32Reserved[4U]; /* Un-used. */
} Media_Avi_MainHeader_TagType;

/**
  * @brief
  * @details
  *
  * @Note dmlh field.
  */
typedef struct Media_Avi_ExtHeader
{
   Media_Fourcc_TagType nFcc; /* 'dmlh' */
   uint32_t u32Cb; /* Size of this structure -8 */
   uint32_t u32GrandFrames; /* Total number of frames in the file */
   uint32_t u32Reserved[61]; /* To be defined later */
} Media_Avi_ExtHeader_TagType;

/**
  * @brief
  * @details
  *
  * @Note strh field.
  */
typedef struct Media_Avi_StreamHeader
{
    Media_Fourcc_TagType nFcc;          // 'strh'
    uint32_t  u32Cb;           // size of this structure - 8
    Media_Fourcc_TagType nFccType;      // stream type codes
    Media_Fourcc_TagType nFccHandler;
    uint32_t u32Flags;
    uint16_t u16Priority;
    uint16_t u16Language;
    uint32_t u32InitialFrames;
    uint32_t u32Scale;
    uint32_t u32Rate;       // u32Rate/u32Scale is stream tick rate in ticks/sec
    uint32_t u32Start;
    uint32_t u32Length;
    uint32_t u32SuggestedBufferSize;
    uint32_t u32Quality;
    uint32_t u32SampleSize;
    struct
    {
        uint16_t u16LeftPos;
        uint16_t u16TopPos;
        uint16_t u16RightPos;
        uint16_t u16BottomPos;
    } Media_RcFrame;
} Media_Avi_StreamHeader_TagType;

/**
  * @brief
  * @details
  *
  * @Note strf field.
  */
/* structure of an AVI stream format chunk */
// #ifndef ckidSTREAMFORMAT
// #define ckidSTREAMFORMAT FCC('strf')
// #endif
//
// avi stream formats are different for each stream type
//
// Media_BitMapInfoHeader_TagType for video streams
// Media_WaveFormatEx_TagType or PCMWAVEFORMAT for audio streams
// nothing for text streams
// nothing for midi streams

typedef struct Media_Avi_Index
{
    Media_Fourcc_TagType dwu8ChunkId;
    uint32_t u32Flags;
    uint32_t u32Offset;    // offset of riff chunk header for the data
    uint32_t u32Size;      // size of the data (excluding riff header size)
} Media_Avi_Index_TagType;          // size of this array

/**
  * @brief
  * @details
  *
  * @Note idx1 field.
  */
//
// structure of old style AVI index
//
// #define ckidMedia_Avi_OldIndex_TagType FCC('idx1')
typedef struct _Media_Avi_OldIndex_TagType
{
   Media_Fourcc_TagType nFcc;        // 'idx1'
   uint32_t   u32Cb;         // size of this structure -8
   Media_Avi_Index_TagType nOldIndex;
} Media_Avi_OldIndex_TagType;

/**
  * @brief
  * @details
  *
  * @Note  field.
  */
typedef struct Media_Avi_FileInfo
{
    float fVideoFrameRate;
    uint32_t u32VideoLength;
    Media_Fourcc_TagType nVideoDataChunk;
    uint16_t u16AudioChannels;
    uint32_t u32AudioSampleRate;
    uint32_t u32AudioLenght;
    Media_Fourcc_TagType nAudioDataChunk;
    uint32_t u32AviStreamsCount;
    uint32_t u32AviListPos;
    uint32_t u32AviOldIdxPos;
    uint32_t u32AviOldIdxSize;
    uint32_t u32AviFileSize;
    uint16_t u16AviHeight;
    uint16_t u16AviWidth;
} Media_Avi_FileInfo_TagType;

typedef struct
{
    char nFileName[MEDIA_MAX_NAME_LENGHT];
    Media_Avi_FileInfo_TagType nAviFileInfo;
} Media_PlayInfo_TagType;

/*==================================================================================================
*                                GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
extern MEDIA_EndFlag_Type nVideoEndFlag;
extern MEDIA_EndFlag_Type nAudioEndFlag;
extern uint8_t Flame_Buffer[MEDIA_Y_COUNT][MEDIA_X_COUNT][MEDIA_COLOR_COUNT];

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
void MEDIA_IRQ_ProcessAudioDac(void);
MEDIA_FileResult_Type MEDIA_GetPlayList(char * sFileName, Media_PlayInfo_TagType ** pList, uint8_t * u8TotalTrack);
MEDIA_FileResult_Type MEDIA_ReadAviHeader(Media_PlayInfo_TagType *pItemInfo);
MEDIA_FileResult_Type MEDIA_ReadAviStream(Media_PlayInfo_TagType *pItemInfo, uint32_t u32CurrentFrame);
MEDIA_State_Type MEDIA_GetCurrentState(void);
void MEDIA_SetCurrentState(MEDIA_State_Type nSetState);
MEDIA_EndFlag_Type MEDIA_GetAudioFlameFlag(void);
void MEDIA_SetAudioFlameFlag(MEDIA_EndFlag_Type nValue);

void MEDIA_UpdateStream(void);

uint32_t MEDIA_GetAudioBufferAddress(void);
uint32_t MEDIA_GetVideoBufferAddress(void);

#ifdef __cplusplus
}
#endif

#endif /* AVI_STREAM_H */

/** @} */
