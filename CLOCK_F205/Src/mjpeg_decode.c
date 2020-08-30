/*================================================================================================*/
/**
*   @file       mjpeg.c
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      mjpeg program body.
*   @details    This is a demo application showing the usage of the API.
*
*   This file contains sample code only. It is not part of the production code deliverables.
*
*   @addtogroup Reserved
*   @{
*/
/*==================================================================================================
*   Project              : Reserved
*   Platform             : Reserved
*
*   SW Version           : 1.0.0
*   Build Version        : Reserved
*
*   Copyright 2020 Semiconductor, Inc.
*   All Rights Reserved.
==================================================================================================*/
#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "mjpeg_decode.h"
#include "libjpeg.h"
// #include "cdjpeg.h"
#include "matrix.h"

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/
static uint8_t atomHasChild[] = {0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0 ,0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define ATOM_ITEMS (sizeof(atomHasChild) / sizeof(atomHasChild[0]))

static const uint8_t atomTypeString[ATOM_ITEMS][5] =
{
    "ftyp", // -
    "wide", // -
    "mdat", // -
    "moov", // +
    "mvhd", // -
    "trak", // +
    "tkhd", // -
    "tapt", // +
    "clef", // -
    "prof", // -
    "enof", // -
    "edts", // +
    "elst", // -
    "mdia", // +
    "mdhd", // -
    "hdlr", // -
    "minf", // +
    "vmhd", // -
    "smhd", // -
    "dinf", // +
    "dref", // -
    "stbl", // +
    "stsd", // -
    "stts", // -
    "stsc", // -
    "stsz", // -
    "stco", // -
    "udta", // +
    "free", // -
    "skip", // -
    "meta", // +
    "load", // -
    "iods", // -
    "ilst", // +
    "keys", // -
    "data", // -
    "trkn", // +
    "disk", // +
    "cpil", // +
    "pgap", // +
    "tmpo", // +
    "gnre", // +
    "covr", // -
    {CMARK, 'n', 'a', 'm', '\0'}, // -
    {CMARK, 'A', 'R', 'T', '\0'}, // -
    {CMARK, 'a', 'l', 'b', '\0'}, // -
    {CMARK, 'g', 'e', 'n', '\0'}, // -
    {CMARK, 'd', 'a', 'y', '\0'}, // -
    {CMARK, 't', 'o', 'o', '\0'}, // -
    {CMARK, 'w', 'r', 't', '\0'}, // -
    "----", // -
};

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/
__attribute__( ( always_inline ) ) static __INLINE size_t getAtomSize(void* atom)
{
/*
    ret =  *(uint8_t*)(atom + 0) << 24;
    ret |= *(uint8_t*)(atom + 1) << 16;
    ret |= *(uint8_t*)(atom + 2) << 8;
    ret |= *(uint8_t*)(atom + 3);
*/
    return __REV(*(uint32_t*)atom);
}


__attribute__( ( always_inline ) ) static __INLINE size_t getSampleSize(void* atombuf, int bytes, FIL *fp)
{
    // my_fread(atombuf, 1, bytes, fp);
    return getAtomSize(atombuf);
}


uint32_t b2l(void* val, size_t t){
    uint32_t ret = 0;
    size_t tc = t;

    for(;t > 0;t--){
        ret |= *(uint8_t*)(val + tc - t) << 8 * (t - 1);
    }

    return ret;
}

int collectAtoms(FIL *fp, size_t parentAtomSize, size_t child)
{
    int index, is;
    size_t atomSize, totalAtomSize = 0;
    uint32_t timeScale = 0, duration = 0;
    uint8_t atombuf[512];

    FIL fp_tmp;

    memset(atombuf, '\0', sizeof(atombuf));

    do{
        atomSize = getSampleSize(atombuf, 8, fp);

        for(index = 0;index < ATOM_ITEMS;index++)
        {
            if(!strncmp((char*)&atombuf[4], (char*)&atomTypeString[index][0], 4))
            {
                // debug.printf("\r\n");
                // for(is = child;is > 0;is--) debug.printf(" ");
                // debug.printf("%s %d", (char*)&atomTypeString[index][0], atomSize);
                break;
            }
        }

//		if(index >= ATOM_ITEMS){
//			debug.printf("\r\nInvalid atom:%s %d", (char*)&atombuf[4], atomSize);
//			return -1;
//		}
        if(index >= ATOM_ITEMS)
        {
            // debug.printf("\r\nunrecognized atom:%s %d", (char*)&atombuf[4], atomSize);

            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0,"\r\nunrecognized atom:%s %d", (char*)&atombuf[4], atomSize);
            HAL_Delay( 1000U );
            MATRIX_FillScreen( 0x0U );
            goto NEXT;
        }

        memcpy((void*)&fp_tmp, (void*)fp, sizeof(MY_FILE));

        switch(index)
        {
        case MDHD:
            my_fseek(fp, 12, SEEK_CUR); // skip ver/flag  creationtime modificationtime

            timeScale = getSampleSize(atombuf, 4, fp); // time scale
            duration = getSampleSize(atombuf, 4, fp); // duration

            break;
        case HDLR:
            my_fseek(fp, 4, SEEK_CUR); // skip flag ver
            my_fread(atombuf, 1, 4, fp); // Component type
            my_fread(atombuf, 1, 4, fp); // Component subtype
            if(!strncmp((char*)atombuf, (const char*)"soun", 4))
            {
                media.sound.flag.process = 1;
                media.sound.timeScale = timeScale;
                media.sound.duration = duration;
            }
            if(!strncmp((char*)atombuf, (const char*)"vide", 4))
            {
                media.video.flag.process = 1;
                media.video.timeScale = timeScale;
                media.video.duration = duration;
            }
            break;
        case TKHD:
            my_fseek(fp, 74, SEEK_CUR); // skip till width, height data
            my_fread(atombuf, 1, 4, fp); // width
            if(getAtomSize(atombuf)){
                media.video.width = getAtomSize(atombuf);
            }
            my_fread(atombuf, 1, 4, fp); // height
            if(getAtomSize(atombuf)){
                media.video.height = getAtomSize(atombuf);
            }
            break;
        case STSD:
            my_fseek(fp, 8, SEEK_CUR); // skip Reserved(6bytes)/Data Reference Index
            my_fread(atombuf, 1, 4, fp); // next atom size
            my_fread(atombuf, 1, getAtomSize(atombuf) - 4, fp);
            if(media.video.flag.process && !media.video.flag.complete){
                memset((void*)media.video.videoFmtString, '\0', sizeof(media.video.videoFmtString));
                memset((void*)media.video.videoCmpString, '\0', sizeof(media.video.videoCmpString));
                memcpy((void*)media.video.videoFmtString, (void*)atombuf, 4);
                memcpy((void*)media.video.videoCmpString, (void*)&atombuf[47], atombuf[46]);
                if(strncmp((char*)media.video.videoFmtString, "jpeg", 4) == 0){
                    media.video.playJpeg = 1; // JPEG
                } else {
                    media.video.playJpeg = 0; // Uncompression
                }
            }
            if(media.sound.flag.process && !media.sound.flag.complete){
                memcpy((void*)&media.sound.format, (void*)atombuf, sizeof(sound_format));
                media.sound.format.version = (uint16_t)b2l((void*)&media.sound.format.version, sizeof(uint16_t));
                media.sound.format.revision = (uint16_t)b2l((void*)&media.sound.format.revision, sizeof(media.sound.format.revision));
                media.sound.format.vendor = (uint16_t)b2l((void*)&media.sound.format.vendor, sizeof(media.sound.format.vendor));
                media.sound.format.numChannel = (uint16_t)b2l((void*)&media.sound.format.numChannel, sizeof(media.sound.format.numChannel));
                media.sound.format.sampleSize = (uint16_t)b2l((void*)&media.sound.format.sampleSize, sizeof(media.sound.format.sampleSize));
                media.sound.format.complesionID = (uint16_t)b2l((void*)&media.sound.format.complesionID, sizeof(media.sound.format.complesionID));
                media.sound.format.packetSize = (uint16_t)b2l((void*)&media.sound.format.packetSize, sizeof(media.sound.format.packetSize));
                media.sound.format.sampleRate = (uint16_t)b2l((void*)&media.sound.format.sampleRate, sizeof(uint16_t));
            }
            break;
        case STTS:
            my_fseek(fp, 4, SEEK_CUR); // skip flag ver
            if(media.video.flag.process && !media.video.flag.complete){
                video_stts.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&video_stts.fp, (void*)fp, sizeof(MY_FILE));
            }
            if(media.sound.flag.process && !media.sound.flag.complete){
                sound_stts.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&sound_stts.fp, (void*)fp, sizeof(MY_FILE));
            }
            break;
        case STSC:
            my_fseek(fp, 4, SEEK_CUR); // skip flag ver
            if(media.video.flag.process && !media.video.flag.complete){
                video_stsc.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&video_stsc.fp, (void*)fp, sizeof(MY_FILE));
            }
            if(media.sound.flag.process && !media.sound.flag.complete){
                sound_stsc.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&sound_stsc.fp, (void*)fp, sizeof(MY_FILE));
            }
            break;
        case STSZ:
            my_fseek(fp, 4, SEEK_CUR); // skip flag ver
            if(media.video.flag.process && !media.video.flag.complete){
                video_stsz.sampleSize = getSampleSize(atombuf, 4, fp);
                video_stsz.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&video_stsz.fp, (void*)fp, sizeof(MY_FILE));
            }
            if(media.sound.flag.process && !media.sound.flag.complete){
                sound_stsz.sampleSize = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&sound_stsz.fp, (void*)fp, sizeof(MY_FILE));
                sound_stsz.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&sound_stsz.fp, (void*)fp, sizeof(MY_FILE));
            }
            break;
        case STCO:
            my_fseek(fp, 4, SEEK_CUR); // skip flag ver
            if(media.video.flag.process && !media.video.flag.complete){
                video_stco.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&video_stco.fp, (void*)fp, sizeof(MY_FILE));

                media.video.flag.process = 0;
                media.video.flag.complete = 1;
            }
            if(media.sound.flag.process && !media.sound.flag.complete){
                sound_stco.numEntry = getSampleSize(atombuf, 4, fp);
                memcpy((void*)&sound_stco.fp, (void*)fp, sizeof(MY_FILE));

                media.sound.flag.process = 0;
                media.sound.flag.complete = 1;
            }
            break;
        default:
            break;
        }

        memcpy((void*)fp, (void*)&fp_tmp, sizeof(MY_FILE));

        if(index < ATOM_ITEMS && atomHasChild[index]){
            memcpy((void*)&fp_tmp, (void*)fp, sizeof(MY_FILE));

            if(collectAtoms(fp, atomSize - 8, child + 1) != 0){ // Re entrant
                return -1;
            }

            memcpy((void*)fp, (void*)&fp_tmp, sizeof(MY_FILE));
        }

NEXT:
        my_fseek(fp, atomSize - 8, SEEK_CUR);
        totalAtomSize += atomSize;
    }while(parentAtomSize > totalAtomSize);

    return 0;
}

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
/**
  * @brief   Resfresh function for active display.
  * @details
  *             We have some plans for scan each bit matrix. it shall be listed below. Each case shall use the difference sources, timing..
  *             1. Decode pixel to the actual possition, IRQ only need move data to peripherals.
  *                 For example scan with 5bit per color per pixel
  *                 For each | w | instance, they store MATRIX_HEIGHT pixels.
  *                 |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |
  *                 | b0 | b1 | b2 | b3 | b4 | b0 | b1 | b2 | b3 | b4 | b0 | b1 | b2 | b3 | b4 |
  *                 |---------- r0 ----------|---------- r1 ----------|---------- r2 ----------|
  *                 Total elements are MATRIX_WIDTH * MATRIX_HEIGHT * MAX_BIT. Actual only need to scan a half
  *                 part of one matrix.
  *             2. Only use the one data array gBuff[MATRIX_WIDTH * MATRIX_HEIGHT]. In this case, only prefer use the color code are RGB565, RGB555, RGB444...
  *                 Each interrupt event occured, we want to print out each bit possition like as below algorithm. This exemple shall be declare RGB565 encode
  *                 | R4 R3 R2 R1 R0 | G5 G4 G3 G2 G1 G0 | B4 B3 B2 B1 B0 |
  *                 u16TempDat = gBuff[ y * MATRIX_WIDTH + x ];
  *                 DAT_P->ODR = gBuff[ y * MATRIX_WIDTH + x ];
  * @param  None.
  * @retval None.
  */
int PlayMotionJpeg(int id)
{
    register uint32_t i, j, k;
    int ret = 0;

    struct jpeg_decompress_struct jdinfo;
    struct jpeg_error_mgr jerr;
    // djpeg_dest_ptr dest_mgr = NULL;

    uint32_t fps, frames, prevFrames, sample_time_limit;
    uint32_t samples, frameDuration, numEntry;
    uint32_t prevChunkSound, prevSamplesSound, firstChunkSound, samplesSound;
    uint32_t firstChunk = 0, totalSamples = 0, prevChunk = 0, prevSamples = 0, totalBytes = 0;
    uint32_t videoStcoCount, soundStcoCount, stco_reads;
    uint32_t prevSamplesBuff[60];

    uint8_t atombuf[512];
    uint8_t fpsCnt = 0;
    const char fps1Hz[] = "|/-\\";
    char timeStr[20];

    raw_video_typedef raw;

    // drawBuff_typedef drawBuff;
    // _drawBuff = &drawBuff;

    FIL fp_sound, fp_frame, fp_frame_cp, \
            fp_stsc, fp_stsz, fp_stco, \
            fp_sound_stsc, fp_sound_stsz, fp_sound_stco, \
            fp_jpeg, \
            *fp = '\0';

    if( FR_OK == f_open( &SDFile,"/mov/test.mov", FA_OPEN_EXISTING | FA_READ) )
    {
        OFF_LED();
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "test.mov opened!\n");
        HAL_Delay( 1000U );
        MATRIX_FillScreen( 0x0U );

        int hasChild = atomHasChild[UDTA];
        atomHasChild[UDTA] = 0;

        if( collectAtoms(&SDFile, SDFile.fsize, 0) != 0)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read error file contents.!\n");
            HAL_Delay( 1000U );
            MATRIX_FillScreen( 0x0U );
            f_close(&SDFile);
            atomHasChild[UDTA] = hasChild;
        }
        else
        {
            atomHasChild[UDTA] = hasChild;

        }


    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "test.mov corrupted!\n");
        HAL_Delay( 1000U );
        MATRIX_FillScreen( 0x0U );
    }



}
/*================================================================================================*/

#ifdef __cplusplus
}
#endif

/** @} */
