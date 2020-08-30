/*==================================================================================================
*   @file       mjpeg.h
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      Reserved - API header.
*   @details    Reserved API functions.
*
*   @addtogroup Reserved
*   @{
*/
/*==================================================================================================
*   Project              : Reserved.
*   Platform             : Reserved
*
*   SW Version           : 1.0.0
*   Build Version        : Reserved
*
*   Copyright 2020 Semiconductor, Inc.
*   All Rights Reserved.
==================================================================================================*/

#ifndef MJPEG_H
#define MJPEG_H

#ifdef __cplusplus
extern "C"{
#endif


/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stddef.h>
#include "fatfs.h"
/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
#define CMARK 0xA9

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/

enum AtomEnum {
    FTYP, // -
    WIDE, // -
    MDAT, // -
    MOOV, // +
    MVHD, // -
    TRAK, // +
    TKHD, // -
    TAPT, // +
    CLEF, // -
    PROF, // -
    ENOF, // -
    EDTS, // +
    ELST, // -
    MDIA, // +
    MDHD, // -
    HDLR, // -
    MINF, // +
    VMHD, // -
    SMHD, // -
    DINF, // +
    DREF, // -
    STBL, // +
    STSD, // -
    STTS, // -
    STSC, // -
    STSZ, // -
    STCO, // -
    UDTA, // +
    FREE, // -
    SKIP, // -
    META, // +
    LOAD, // -
    IODS, // -
    ILST, // +
    KEYS, // -
    DATA, // -
    TRKN, // +
    DISK, // +
    CPIL, // +
    PGAP, // +
    TMPO, // +
    GNRE, // +
    COVR, // -
    CNAM, // -
    CART, // -
    CALB, // -
    CGEN, // -
    CDAY, // -
    CTOO, // -
    CWRT, // -
    NONE, // -
};
/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
struct
{
    uint16_t buf_type;
    uint32_t frame_size;
} jpeg_read;

volatile struct
{
    int8_t id, \
           done, \
           resynch, \
           draw_icon;
    uint32_t paused_chunk, \
             resynch_entry;
} mjpeg_touch;

struct
{
    uint32_t *firstChunk, *prevChunk, *prevSamples, *samples, *totalSamples, *videoStcoCount;
    FIL *fp_video_stsc, *fp_video_stsz, *fp_video_stco, *fp_frame;
} pv_src;

struct
{
    uint32_t *firstChunk, *prevChunk, *prevSamples, *samples, *soundStcoCount;
    FIL *fp_sound_stsc, *fp_sound_stsz, *fp_sound_stco;
} ps_src;

struct
{
    uint32_t numEntry;
    FIL fp;
} sound_stts;

struct
{
    uint32_t numEntry;
    FIL fp;
} sound_stsc;

struct
{
    uint32_t sampleSize,
             numEntry;
    FIL fp;
} sound_stsz;


struct
{
    uint32_t numEntry;
    FIL fp;
} sound_stco;

struct
{
    uint32_t numEntry;
    FIL fp;
} video_stts;

struct
{
    uint32_t numEntry;
    FIL fp;
} video_stsc;

struct
{
    uint32_t sampleSize,
             numEntry;
    FIL fp;
} video_stsz;

struct
{
    uint32_t numEntry;
    FIL fp;
} video_stco;


typedef struct samples_struct
{
    uint32_t numEntry;
    FIL *fp;
} samples_struct;

typedef struct sound_flag
{
    uint32_t process, complete;
} sound_flag;

typedef struct video_flag
{
    uint32_t process, complete;
} video_flag;

typedef struct sound_format
{
    char audioFmtString[4];
    uint8_t reserved[10];
    uint16_t version,
             revision,
               vendor,
               numChannel,
               sampleSize,
               complesionID,
               packetSize,
               sampleRate,
             reserved1;
} sound_format;

typedef struct esds_format
{
    char esdsString[4];
//	uint8_t reserved[22];
    uint32_t maxBitrate, avgBitrate;
} esds_format;


typedef struct media_sound
{
    sound_flag flag;
    sound_format format;
    uint32_t timeScale,
             duration;
} media_sound;

typedef struct media_video
{
    video_flag flag;
    uint32_t timeScale,
             duration;
    int16_t width, height, frameRate, startPosX, startPosY;
    char videoFmtString[5], videoCmpString[15], playJpeg;
} media_video;

volatile struct
{
    media_sound sound;
    media_video video;
} media;

typedef struct
{
    int output_scanline, frame_size, rasters, buf_size;
    uint16_t *p_raster;
} raw_video_typedef;

/*==================================================================================================
*                                GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/


/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/

extern uint32_t b2l(void* val, size_t t);
//inline uint32_t getAtomSize(void* atom);
extern int PlayMotionJpeg(int id);

#ifdef __cplusplus
}
#endif

#endif /* MJPEG_H */

/** @} */
