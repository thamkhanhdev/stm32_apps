/*==================================================================================================
*   @file       avistream.h
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      Avi program body.
*   @details    This is a demo application showing the usage of the API.
*
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
#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "matrix.h"
#include "gamma.h"
#include "avistream.h"
#include <stdbool.h>
#include "stdio.h"
#include "ff.h"
#include "fatfs.h"

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
/* APB1 Timer clock */
#define TIM6_FREQ 80000000
#define TIM6_PRESCALER 0
#define TIM6_DELAY 300U
#if defined(RGB565)
#define TIM6_DELAY 300U
#elif defined(RGB555)
#define TIM6_DELAY 200U
#endif /* #if defined(RGB565) */

#define SET_AUDIO_SAMPLERATE(x)         \
    do                                  \
    {                                   \
        TIM6->ARR = ((unsigned int)(TIM6_FREQ / ((x) * (TIM6_PRESCALER + 1))) - TIM6_DELAY);   \
    } while(0)
    /*
     * 200U with RGB555 at 160Mhz
     * 300U with RGB565 at 160Mhz
     */
/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
static float fCurrentVol = 1.0;
static MEDIA_State_Type nCurrentState = MEDIA_STOP;
static uint16_t u16AudioFlameDataCount = 0U;
static uint8_t u8AudioDoubleBuffer = 0U;
static uint8_t u8AudioChannnelCount = 0U;

static int16_t Audio_Buffer[2U][MEDIA_AUDIO_MAX_SAMPLE_RATE * MEDIA_AUDIO_MAX_CHANNEL / MEDIA_VIDEO_MIN_FLAME_RATE] = {0};

__attribute__((section(".user_ram"))) uint8_t Flame_Buffer[MEDIA_Y_COUNT][MEDIA_X_COUNT][MEDIA_COLOR_COUNT] = {0};

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
MEDIA_EndFlag_Type nVideoEndFlag = FLAG_RESET;
MEDIA_EndFlag_Type nAudioEndFlag = FLAG_RESET;
volatile MEDIA_EndFlag_Type nAudioFlameEndFlag = FLAG_RESET;

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

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
void MEDIA_IRQ_ProcessAudioDac(void)
{
    static uint16_t u16AudDatOutCnt;

    /* Clear interrupt flag */
    TIM6->SR = 0xFFFFFFFE;

    if(( FLAG_RESET == nAudioEndFlag ) && ( MEDIA_PLAY == nCurrentState ))
    {
        if( 1U == u8AudioChannnelCount )
        {
            LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_1, ((unsigned int)((Audio_Buffer[u8AudioDoubleBuffer & 0x01][u16AudDatOutCnt] * fCurrentVol) + 0x8000) & 0x0000fff0) );
            LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_2, ((unsigned int)((Audio_Buffer[u8AudioDoubleBuffer & 0x01][u16AudDatOutCnt] * fCurrentVol) + 0x8000) & 0x0000fff0) );
            if(u16AudDatOutCnt < ((u16AudioFlameDataCount >> 1) - 1))
            {
                u16AudDatOutCnt++;
            }
            else
            {
                nAudioFlameEndFlag = FLAG_SET;
                if(u8AudioDoubleBuffer == 0)
                {
                    u8AudioDoubleBuffer = 1;
                }
                else
                {
                    u8AudioDoubleBuffer = 0;
                }
                u16AudDatOutCnt = 0;
            }
        }
        else if( 2U == u8AudioChannnelCount )
        {
            LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_1, ((unsigned int)((Audio_Buffer[u8AudioDoubleBuffer & 0x01][u16AudDatOutCnt + 0] * fCurrentVol) + 0x8000) & 0x0000fff0) );
            LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_2, ((unsigned int)((Audio_Buffer[u8AudioDoubleBuffer & 0x01][u16AudDatOutCnt + 0] * fCurrentVol) + 0x8000) & 0x0000fff0) );
            if(u16AudDatOutCnt < ((u16AudioFlameDataCount >> 1) - 2))
            {
                u16AudDatOutCnt += 2;
            }
            else
            {
                nAudioFlameEndFlag = FLAG_SET;
                if(u8AudioDoubleBuffer == 0)
                {
                    u8AudioDoubleBuffer = 1;
                }else
                {
                    u8AudioDoubleBuffer = 0;
                }
                u16AudDatOutCnt = 0;
            }
        }
    }
    else
    {
        LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_1, (0x8000 & 0x0000fff0) );
        LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_2, (0x8000 & 0x0000fff0) );
        if( 1U == u8AudioChannnelCount )
        {
            if(u16AudDatOutCnt < ((u16AudioFlameDataCount >> 1) - 1))
            {
                u16AudDatOutCnt++;
            }
            else
            {
                nAudioFlameEndFlag = FLAG_SET;
                u8AudioDoubleBuffer = 0;
                u16AudDatOutCnt = 0;
            }
        }
        else if( 2U == u8AudioChannnelCount )
        {
            if(u16AudDatOutCnt < ((u16AudioFlameDataCount >> 1) - 2))
            {
                u16AudDatOutCnt += 2;
            }
            else
            {
                nAudioFlameEndFlag = FLAG_SET;
                u8AudioDoubleBuffer = 0;
                u16AudDatOutCnt = 0;
            }
        }
    }
}

inline MEDIA_FileResult_Type MEDIA_GetPlayList(char * sFileName, Media_PlayInfo_TagType ** pList, uint8_t * u8TotalTrack)
{
    FILINFO pListFileInfo;
    FRESULT nFatResult;
    FIL pListFileObject;

    uint8_t *pToken;
    uint8_t sTempData[MEDIA_MAX_NAME_LENGHT];
    uint8_t u8TrackCount = 0U;

    Media_PlayInfo_TagType *pCurrList;
    Media_PlayInfo_TagType *pReallocList;

    nFatResult = f_stat( sFileName, &pListFileInfo);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_stat error %u\r\n",  sFileName, nFatResult);
        goto FATFS_ERROR_PROCESS;
    }

    nFatResult = f_open(&pListFileObject, (TCHAR*)&pListFileInfo.fname, FA_READ);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_open error %u\r\n",  sFileName, nFatResult);
        goto FATFS_ERROR_PROCESS;
    }

    while((f_gets((TCHAR*)sTempData, (MEDIA_MAX_NAME_LENGHT + 16), &pListFileObject) != NULL) && (u8TrackCount <= MEDIA_MAX_TRACK_COUNT))
    {
        u8TrackCount++;
    }

    if(0 == u8TrackCount)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "item doesn't exist in playlist\r\n");
        goto FILE_ERROR_PROCESS;
    }

    nFatResult = f_lseek(&pListFileObject, 0);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_lseek error %u\r\n",  sFileName, nFatResult);
        goto FATFS_ERROR_PROCESS;
    }

    pCurrList = (Media_PlayInfo_TagType *)malloc(sizeof(Media_PlayInfo_TagType) * u8TrackCount);
    if(NULL == pCurrList)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    u8TrackCount = 0;
    while( (f_gets((TCHAR*)sTempData, (MEDIA_MAX_NAME_LENGHT + 16), &pListFileObject) != NULL) && (u8TrackCount <= MEDIA_MAX_TRACK_COUNT))
    {
        pToken = (uint8_t*)strtok((char*)sTempData, ",\r\n");

        if((pToken != NULL) && (strncmp((char*)sTempData, "//", 2) != 0))
        {
            strcpy((char*)pCurrList[u8TrackCount].nFileName, (char*)pToken);
            if(FILE_OK == MEDIA_ReadAviHeader(&pCurrList[u8TrackCount]))
            {
                u8TrackCount++;
            }
        }
        else
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Read %s error\r\n", sTempData);
        }
    }

    if(0 == u8TrackCount)
    {
        free(pCurrList);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "item does not exist in playlist\r\n");
        goto FILE_ERROR_PROCESS;
    }

    pReallocList = (Media_PlayInfo_TagType *)realloc(pCurrList, (sizeof(Media_PlayInfo_TagType) * u8TrackCount));
    if(NULL == pReallocList){
        free(pCurrList);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "realloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    nFatResult = f_close(&pListFileObject);
    if(FR_OK != nFatResult){
        free(pReallocList);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_close error %u\r\n",  sFileName, nFatResult);
        goto FATFS_ERROR_PROCESS;
    }

    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Track: %i\r\n", u8TrackCount);
    // HAL_Delay(1000);

    //  for(int testloop = 0;testloop < u8TrackCount;testloop++)
    //  {
    //     float fCurrentFps = pReallocList[testloop].nAviFileInfo.fVideoFrameRate;
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFFF, "%s\r\n", pReallocList[testloop].nFileName);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFFF, "%d.%5fFPS\r\n", (uint32_t) fCurrentFps, (uint32_t)((fCurrentFps - (uint32_t) fCurrentFps) * 100000) );
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFFF, "%lu\r\n", pReallocList[testloop].nAviFileInfo.u32VideoLength);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFFF, "%.4s\r\n", pReallocList[testloop].nAviFileInfo.nVideoDataChunk.u8Fcc);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFFF, "%u\r\n", pReallocList[testloop].nAviFileInfo.u16AudioChannels);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFFF, "%luHz\r\n", pReallocList[testloop].nAviFileInfo.u32AudioSampleRate);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFFF, "%lu\r\n", pReallocList[testloop].nAviFileInfo.u32AudioLenght);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U,  0xFFF, "%.4s\r\n", pReallocList[testloop].nAviFileInfo.nAudioDataChunk.u8Fcc);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U,  0xFFF, "%lu\r\n", pReallocList[testloop].nAviFileInfo.u32AviStreamsCount);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U,  0xFFF, "movi:0x%08lx\r\n", pReallocList[testloop].nAviFileInfo.u32AviListPos);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U,  0xFFF, "idx1:0x%08lx\r\n", pReallocList[testloop].nAviFileInfo.u32AviOldIdxPos);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 28U,  0xFFF, "idx1:0x%08lx\r\n", pReallocList[testloop].nAviFileInfo.u32AviOldIdxSize);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 35U,  0xFFF, "AVI:%luB\r\n", pReallocList[testloop].nAviFileInfo.u32AviFileSize);
    //     HAL_Delay(4000);
    //     MATRIX_FillScreen(0x0);
    // }

    (*pList) = pReallocList;
    (*u8TotalTrack) = u8TrackCount;
    return FILE_OK;

    FILE_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xF800, "FILE_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        (*pList) = NULL;
        (*u8TotalTrack) = 0;
    return FILE_ERROR;

    FATFS_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xF800, "FATFS_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        (*pList) = NULL;
        (*u8TotalTrack) = 0;
    return FATFS_ERROR;

    OTHER_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xF800, "OTHER_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        (*pList) = NULL;
        (*u8TotalTrack) = 0;
    return OTHER_ERROR;
};

inline MEDIA_FileResult_Type MEDIA_ReadAviHeader(Media_PlayInfo_TagType *pItemInfo)
{
    FRESULT nFatResult;
    FIL avi_fileobject;

    uint32_t linkmap_table[LINKMAP_TABLE_SIZE];

    Media_RiffChunk_TagType *riff_chunk = NULL;
    Media_RiffList_TagType *riff_list = NULL;
    Media_Fourcc_TagType *four_cc = NULL;
    Media_Avi_MainHeader_TagType *avi_main_header = NULL;
    Media_RiffList_TagType *strl_list = NULL;
    Media_Avi_StreamHeader_TagType *avi_stream_header = NULL;
    Media_RiffChunk_TagType *strf_chunk = NULL;
    Media_BitMapInfoHeader_TagType *bitmap_info_header = NULL;
    Media_WaveFormatEx_TagType *wave_format_ex = NULL;
    Media_RiffList_TagType *unknown_list = NULL;
    Media_RiffList_TagType *movi_list = NULL;
    Media_RiffChunk_TagType *idx1_chunk = NULL;

    uint8_t strl_list_find_loop_count = 0;

    float fVideoFrameRate = 0;
    uint32_t u32VideoLength = 0U;
    uint16_t u16AudioChannels = 0U;
    uint32_t u32AudioSampleRate = 0U;
    uint32_t u32AudioLenght = 0U;
    uint32_t u32AviStreamsCount = 0U;
    uint32_t u32AviListPos = 0U;
    uint32_t u32AviOldIdxPos = 0U;
    uint32_t u32AviOldIdxSize = 0U;
    uint32_t u32AviFileSize = 0U;
    uint32_t audio_suggest_buffer_size = 0U;
    uint16_t video_width = 0U;
    uint16_t video_height = 0U;
    uint8_t chunk_name_temp[5];

    Media_Fourcc_TagType nAudioDataChunk;
    Media_Fourcc_TagType nVideoDataChunk;

    uint8_t video_stream_find_flag = RESET;
    uint8_t audio_stream_find_flag = RESET;

    UINT read_data_byte_result;

    nFatResult = f_open(&avi_fileobject, pItemInfo->nFileName, FA_READ);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "f_open NG nFatResult=%d\r\n", nFatResult);
        goto FATFS_ERROR_PROCESS;
    }

    avi_fileobject.cltbl = linkmap_table;
    linkmap_table[0] = LINKMAP_TABLE_SIZE;
    nFatResult = f_lseek(&avi_fileobject, CREATE_LINKMAP);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Link error");
        goto FATFS_ERROR_PROCESS;
    }

    riff_chunk = (Media_RiffChunk_TagType *)malloc(sizeof(Media_RiffChunk_TagType));
    if(NULL == riff_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Malloc error");
        goto OTHER_ERROR_PROCESS;
    }
    nFatResult = f_read(&avi_fileobject, riff_chunk, sizeof(Media_RiffChunk_TagType), &read_data_byte_result);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "RIFF f_read error");
        goto FATFS_ERROR_PROCESS;
    }

    u32AviFileSize = riff_chunk->u32Cb + sizeof(Media_RiffChunk_TagType);

    if(strncmp((char*)riff_chunk->nFcc.u8Fcc, "RIFF", sizeof(Media_Fourcc_TagType)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Not RIFF");
        goto FILE_ERROR_PROCESS;
    }

    four_cc = (Media_Fourcc_TagType *)malloc(sizeof(Media_Fourcc_TagType));
    if(NULL == four_cc)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "four_cc error");
        goto OTHER_ERROR_PROCESS;
    }
    nFatResult = f_read(&avi_fileobject, four_cc, sizeof(Media_Fourcc_TagType), &read_data_byte_result);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "four_cc read error");
        goto FATFS_ERROR_PROCESS;
    }

    if(strncmp((char*)four_cc->u8Fcc, "AVI ", sizeof(Media_Fourcc_TagType)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "RIFF not AVI");
        goto FILE_ERROR_PROCESS;
    }

    riff_list = (Media_RiffList_TagType *)malloc(sizeof(Media_RiffList_TagType));
    if(NULL == riff_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "riff_list malloc");
        goto OTHER_ERROR_PROCESS;
    }
    nFatResult = f_read(&avi_fileobject, riff_list, sizeof(Media_RiffList_TagType), &read_data_byte_result);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "riff_list f_read");
        goto FATFS_ERROR_PROCESS;
    }

    if((strncmp((char*)riff_list->nFcc.u8Fcc, "LIST", sizeof(Media_Fourcc_TagType)) != 0) || (strncmp((char*)riff_list->nFccListType.u8Fcc, "hdrl ", sizeof(Media_Fourcc_TagType)) != 0))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found hdrl");
        goto FILE_ERROR_PROCESS;
    }

    avi_main_header = (Media_Avi_MainHeader_TagType *)malloc(sizeof(Media_Avi_MainHeader_TagType));
    if(NULL == avi_main_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_main_header malloc");
        goto OTHER_ERROR_PROCESS;
    }
    nFatResult = f_read(&avi_fileobject, avi_main_header, sizeof(Media_Avi_MainHeader_TagType), &read_data_byte_result);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_main_header read");
        goto FATFS_ERROR_PROCESS;
    }

    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U, 0xFFF, "1. %.4s", avi_main_header->fcc.u8Fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U, 0xFFF, "2. %u", avi_main_header->u32Cb );
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U, 0xFFF, "3. %u", avi_main_header->u32MicroSecPerFrame);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U, 0xFFF, "4. %u", avi_main_header->u32MaxBytesPerSec );
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U, 0xFFF, "5. %u", avi_main_header->u32PaddingGranularity);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U, 0xFFF, "6. %i", avi_main_header->u32Flags);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U, 0xFFF, "7. %u", avi_main_header->u32TotalFrames);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U, 0xFFF, "8. %u", avi_main_header->u32InitialFrames);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U, 0xFFF, "9. %u", avi_main_header->u32Streams);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U,  7U, 0xFFF, "1. %u", avi_main_header->u32SuggestedBufferSize);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U, 0xFFF, "2. %u", avi_main_header->u32Width);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U, 0xFFF, "3. %u", avi_main_header->u32Height);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 28U, 0xFFF, "4. %u", read_data_byte_result);
    // HAL_Delay(5000);
    // MATRIX_FillScreen(0x0);

    if(strncmp((char*)avi_main_header->nFcc.u8Fcc, "avih", sizeof(Media_Fourcc_TagType)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found avi main header");
        goto FILE_ERROR_PROCESS;
    }
    if(MEDIA_AVIF_HASINDEX != (avi_main_header->u32Flags & MEDIA_AVIF_HASINDEX))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi not index");
        goto FILE_ERROR_PROCESS;
    }
    if((MEDIA_X_COUNT != avi_main_header->u32Width) && (MEDIA_Y_COUNT != avi_main_header->u32Height))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wrong size");
        goto FILE_ERROR_PROCESS;
    }

    u32AviStreamsCount = avi_main_header->u32Streams;

    strl_list = (Media_RiffList_TagType *)malloc(sizeof(Media_RiffList_TagType));
    if(NULL == strl_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_list malloc");
        goto OTHER_ERROR_PROCESS;
    }
    avi_stream_header = (Media_Avi_StreamHeader_TagType *)malloc(sizeof(Media_Avi_StreamHeader_TagType));
    if(NULL == avi_stream_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi stream malloc");
        goto OTHER_ERROR_PROCESS;
    }
    strf_chunk = (Media_RiffChunk_TagType *)malloc(sizeof(Media_RiffChunk_TagType));
    if(NULL == strf_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strf_chunk malloc");
        goto OTHER_ERROR_PROCESS;
    }
    bitmap_info_header = (Media_BitMapInfoHeader_TagType *)malloc(sizeof(Media_BitMapInfoHeader_TagType));
    if(NULL == bitmap_info_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header malloc");
        goto OTHER_ERROR_PROCESS;
    }
    wave_format_ex = (Media_WaveFormatEx_TagType *)malloc(sizeof(Media_WaveFormatEx_TagType));
    if(NULL == wave_format_ex)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wave_format_ex malloc");
        goto OTHER_ERROR_PROCESS;
    }

    do
    {
        nFatResult = f_read(&avi_fileobject, strl_list, sizeof(Media_RiffList_TagType), &read_data_byte_result);
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_list_f_read");
            goto FATFS_ERROR_PROCESS;
        }
        if((strncmp((char*)strl_list->nFcc.u8Fcc, "LIST", sizeof(Media_Fourcc_TagType)) == 0) && (strncmp((char*)strl_list->nFccListType.u8Fcc, "strl", sizeof(Media_Fourcc_TagType)) == 0))
        {
            nFatResult = f_read(&avi_fileobject, avi_stream_header, sizeof(Media_Avi_StreamHeader_TagType), &read_data_byte_result);
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_stream_header f_read ");
                goto FATFS_ERROR_PROCESS;
            }

            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFFF, "1. %.4s\r\n", avi_stream_header->fcc.u8Fcc);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFFF, "2. %u\r\n", avi_stream_header->u32Cb);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFFF, "3. %.4s\r\n", avi_stream_header->nFccType.u8Fcc);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFFF, "4. %.4s\r\n", avi_stream_header->nFccHandler.u8Fcc);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFFF, "5. %u\r\n", avi_stream_header->u32Flags);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFFF, "6. %u\r\n", avi_stream_header->u16Priority);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFFF, "7. %u\r\n", avi_stream_header->u16Language);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U,  0xFFF, "8. %u\r\n", avi_stream_header->u32InitialFrames);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U,  0xFFF, "9. %u\r\n", avi_stream_header->u32Scale);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U,  7U,  0xFFF, "1. %u\r\n", avi_stream_header->u32Rate);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U,  0xFFF, "2. %u\r\n", avi_stream_header->u32Start);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U,  0xFFF, "3. %u\r\n", avi_stream_header->u32Length);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 28U,  0xFFF, "4. %u\r\n", avi_stream_header->u32SuggestedBufferSize);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 35U,  0xFFF, "5. %i\r\n", avi_stream_header->u32Quality);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 42U,  0xFFF, "6. %u\r\n", avi_stream_header->u32SampleSize);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 49U,  0xFFF, "7. %i\r\n", avi_stream_header->Media_RcFrame.u16LeftPos);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 56U,  0xFFF, "8. %i\r\n", avi_stream_header->Media_RcFrame.u16TopPos);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 63U,  0xFFF, "9. %i\r\n", avi_stream_header->Media_RcFrame.u16RightPos);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 84U,  7U,  0xFFF, "1. %i\r\n", avi_stream_header->Media_RcFrame.u16BottomPos);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 84U, 14U,  0xFFF, "2. %i\r\n", read_data_byte_result);
            // HAL_Delay(5000);
            // MATRIX_FillScreen(0x0);

            if(strncmp((char*)avi_stream_header->nFcc.u8Fcc, "strh", sizeof(Media_Fourcc_TagType)) == 0)
            {
                if((strncmp((char*)avi_stream_header->nFccType.u8Fcc, "vids", sizeof(Media_Fourcc_TagType)) == 0) && (video_stream_find_flag == RESET))
                {
                    if((avi_stream_header->u32Flags & (MEDIA_AVISF_DISABLED | MEDIA_AVISF_VIDEO_PALCHANGES)) == 0)
                    {
                        if( (MEDIA_VIDEO_MAX_FLAME_RATE >= (uint32_t)((float)avi_stream_header->u32Rate / (float)avi_stream_header->u32Scale)) && \
                            (MEDIA_VIDEO_MIN_FLAME_RATE <= ((float)avi_stream_header->u32Rate / (float)avi_stream_header->u32Scale)))
                        {
                            nFatResult = f_read(&avi_fileobject, strf_chunk, sizeof(Media_RiffChunk_TagType), &read_data_byte_result);
                            if(FR_OK != nFatResult)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_chunk f_read");
                                goto FATFS_ERROR_PROCESS;
                            }
                            if(strncmp((char*)strf_chunk->nFcc.u8Fcc, "strf", sizeof(Media_Fourcc_TagType)) == 0)
                            {
                                nFatResult = f_read(&avi_fileobject, bitmap_info_header, sizeof(Media_BitMapInfoHeader_TagType), &read_data_byte_result);
                                if(FR_OK != nFatResult)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header f_read");
                                    goto FATFS_ERROR_PROCESS;
                                }

                                /* MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,   7U,  0xFF, "1. %u\r\n", bitmap_info_header->u32Size);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  14U,  0xFF, "2. %u\r\n", bitmap_info_header->s32Width);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  21U,  0xFF, "3. %u\r\n", bitmap_info_header->s32Height);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  28U,  0xFF, "4. %u\r\n", bitmap_info_header->u16Planes);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  35U,  0xFF, "5. %u\r\n", bitmap_info_header->u16BitCount);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  42U,  0xFF, "6. %u\r\n", bitmap_info_header->u32Compression);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  49U,  0xFF, "7. %u\r\n", bitmap_info_header->u32SizeImage);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  56U,  0xFF, "8. %u\r\n", bitmap_info_header->s32XPelsPerMeter);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  42U,  7U,  0xFF, "1. %u\r\n", bitmap_info_header->s32YPelsPerMeter);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  42U, 14U,  0xFF, "2. %u\r\n", bitmap_info_header->u32ClrUsed);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  42U, 21U,  0xFF, "3. %u\r\n", bitmap_info_header->u32ClrImportant);
                                HAL_Delay(5000);
                                MATRIX_FillScreen(0x0); */

                                if((MEDIA_X_COUNT == bitmap_info_header->s32Width) && (MEDIA_Y_COUNT == bitmap_info_header->s32Height))
                                {
                                    if(MEDIA_AVI_BPP24 == bitmap_info_header->u16BitCount)
                                    {
                                        if(MEDIA_AVI_BI_RGB == bitmap_info_header->u32Compression)
                                        {
                                            fVideoFrameRate = (float) ((float)avi_stream_header->u32Rate / (float)avi_stream_header->u32Scale);
                                            u32VideoLength = avi_stream_header->u32Length;
                                            snprintf((char*)chunk_name_temp, 5, "%02udb", strl_list_find_loop_count);
                                            memmove(nVideoDataChunk.u8Fcc, chunk_name_temp, sizeof(Media_Fourcc_TagType));
                                            video_stream_find_flag = SET;
                                        }
                                        else
                                        {
                                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not MEDIA_AVI_BI_RGB: %i", bitmap_info_header->u32Compression );
                                        }
                                    }
                                    else if( MEDIA_AVI_BPP16 == bitmap_info_header->u16BitCount )
                                    {
                                        fVideoFrameRate = (float) ((float)avi_stream_header->u32Rate / (float)avi_stream_header->u32Scale);
                                        u32VideoLength = avi_stream_header->u32Length;
                                        snprintf((char*)chunk_name_temp, 5, "%02udc", strl_list_find_loop_count);
                                        memmove(nVideoDataChunk.u8Fcc, chunk_name_temp, sizeof(Media_Fourcc_TagType));
                                        video_stream_find_flag = SET;
                                    }
                                }
                                else
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wrong size video");
                                }
                                nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(Media_BitMapInfoHeader_TagType));
                                if(FR_OK != nFatResult)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header f_lseek");
                                goto FATFS_ERROR_PROCESS;
                                }
                            }
                            else
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found strl_chunk");
                            }
                            nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(Media_RiffChunk_TagType));
                            if(FR_OK != nFatResult)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_chunk f_lseek");
                                goto FATFS_ERROR_PROCESS;
                            }
                        }
                        else
                        {
                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "out of flame range");
                        }
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "stream disabled");
                    }
                }
                else if((strncmp((char*)avi_stream_header->nFccType.u8Fcc, "auds", sizeof(Media_Fourcc_TagType)) == 0)  && (audio_stream_find_flag == RESET))
                {
                    if((avi_stream_header->u32Flags & MEDIA_AVISF_DISABLED) == 0)
                    {
                        if( (MEDIA_AUDIO_MAX_SAMPLE_RATE >= (avi_stream_header->u32Rate / avi_stream_header->u32Scale)) && \
                            (MEDIA_AUDIO_MIN_SAMPLE_RATE <= (avi_stream_header->u32Rate / avi_stream_header->u32Scale)))
                        {
                            nFatResult = f_read(&avi_fileobject, strf_chunk, sizeof(Media_RiffChunk_TagType), &read_data_byte_result);
                            if(FR_OK != nFatResult)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_chunk f_read");
                                goto FATFS_ERROR_PROCESS;
                            }
                            if(strncmp((char*)strf_chunk->nFcc.u8Fcc, "strf", sizeof(Media_Fourcc_TagType)) == 0)
                            {
                                nFatResult = f_read(&avi_fileobject, wave_format_ex, sizeof(Media_WaveFormatEx_TagType), &read_data_byte_result);
                                if(FR_OK != nFatResult)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wave_format_ex f_read");
                                    goto FATFS_ERROR_PROCESS;
                                }

                                /* MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFF, "1. %04x\r\n", wave_format_ex->wFormatTag);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFF, "2. %u\r\n", wave_format_ex->nChannels);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFF, "3. %u\r\n", wave_format_ex->nSamplesPerSec);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFF, "4. %u\r\n", wave_format_ex->nAvgBytesPerSec);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFF, "5. %u\r\n", wave_format_ex->nBlockAlign);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFF, "6. %u\r\n", wave_format_ex->wBitsPerSample);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFF, "7. %u\r\n", wave_format_ex->cbSize);
                                HAL_Delay(5000);
                                MATRIX_FillScreen(0x0); */

                                if( MEDIA_WAVE_FORMAT_PCM == wave_format_ex->wFormatTag )
                                {
                                    if( (MEDIA_AUDIO_MAX_CHANNEL >= wave_format_ex->nChannels) && (MEDIA_AUDIO_MIN_CHANNEL <= wave_format_ex->nChannels) )
                                    {
                                        if( (MEDIA_AUDIO_MAX_SAMPLE_RATE >= wave_format_ex->nSamplesPerSec) && (MEDIA_AUDIO_MIN_SAMPLE_RATE <= wave_format_ex->nSamplesPerSec) )
                                        {
                                            if( 16U == wave_format_ex->wBitsPerSample )
                                            {
                                                u16AudioChannels = wave_format_ex->nChannels;
                                                u32AudioSampleRate = wave_format_ex->nSamplesPerSec;
                                                u32AudioLenght = avi_stream_header->u32Length;
                                                audio_suggest_buffer_size = avi_stream_header->u32SuggestedBufferSize;
                                                snprintf((char*)chunk_name_temp, 5, "%02uwb", strl_list_find_loop_count);
                                                memmove(nAudioDataChunk.u8Fcc, chunk_name_temp, sizeof(Media_Fourcc_TagType));
                                                audio_stream_find_flag = SET;
                                            }
                                            else
                                            {
                                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not 16bit audio");
                                            }
                                        }
                                        else
                                        {
                                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "out of audio range");
                                        }
                                    }
                                    else
                                    {
                                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio channels out range");
                                    }
                                }
                                else
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not PCM");
                                }

                                nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(Media_WaveFormatEx_TagType));
                                if(FR_OK != nFatResult)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header f_lssek");
                                    goto FATFS_ERROR_PROCESS;
                                }
                            }
                            else
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found strf_chunk");
                            }

                            nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(Media_RiffChunk_TagType));
                            if(FR_OK != nFatResult)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strf_chunk f_lseek");
                                goto FATFS_ERROR_PROCESS;
                            }
                        }
                        else
                        {
                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "out range audio sample");
                        }
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "stream disabled");
                    }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not vid or audio");
                }
                if((audio_stream_find_flag == SET) &&
                    ( ((wave_format_ex->nSamplesPerSec * wave_format_ex->nChannels) >= (audio_suggest_buffer_size + wave_format_ex->nBlockAlign * 2)) ||
                    ( (wave_format_ex->nSamplesPerSec * wave_format_ex->nChannels) <= (audio_suggest_buffer_size - wave_format_ex->nBlockAlign * 2))))
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio buff size wrong");

                    /* MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFF, "1. %u\r\n", wave_format_ex->nSamplesPerSec);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFF, "2. %u\r\n", wave_format_ex->nChannels);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFF, "3. %u\r\n", fVideoFrameRate);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFF, "4. %u\r\n", audio_suggest_buffer_size);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFF, "5. %u\r\n", wave_format_ex->nBlockAlign);
                    HAL_Delay(5000);
                    MATRIX_FillScreen(0x0); */

                    audio_stream_find_flag = RESET;
                }
            }
            else
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wrong strh header");
            }

            nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(Media_Avi_StreamHeader_TagType));
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_header_stream f_lseek");
                goto FATFS_ERROR_PROCESS;
            }
            strl_list_find_loop_count++;
        }
        else
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found strl list");
        }

        nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + strl_list->u32Cb - sizeof(Media_Fourcc_TagType));
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_header_stream f_lseek");
            goto FATFS_ERROR_PROCESS;
        }

        if(f_tell(&avi_fileobject) >= (riff_chunk->u32Cb + sizeof(Media_RiffChunk_TagType) - 1))
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not find audio video str head");
            goto FILE_ERROR_PROCESS;
        }
    } while( (video_stream_find_flag != SET) || (audio_stream_find_flag != SET) );

    unknown_list = (Media_RiffList_TagType *)malloc(sizeof(Media_RiffList_TagType));
    if(NULL == unknown_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unknow malloc");
        goto OTHER_ERROR_PROCESS;
    }

    do
    {
        nFatResult = f_read(&avi_fileobject, unknown_list, sizeof(Media_RiffList_TagType), &read_data_byte_result);
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unkown_list f_read");
            goto FATFS_ERROR_PROCESS;
        }
        if((strncmp((char*)unknown_list->nFcc.u8Fcc, "LIST", sizeof(Media_Fourcc_TagType)) == 0) && (strncmp((char*)unknown_list->nFccListType.u8Fcc, "movi", sizeof(Media_Fourcc_TagType)) == 0)){
        nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(Media_RiffList_TagType));
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unknow_list f_lseek");
            goto FATFS_ERROR_PROCESS;
        }

        }else{
        nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + unknown_list->u32Cb - sizeof(Media_Fourcc_TagType));
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unknown_list f_lseek");
            goto FATFS_ERROR_PROCESS;
        }
        }

        if(f_tell(&avi_fileobject) >= (riff_chunk->u32Cb + sizeof(Media_RiffChunk_TagType) - 1))
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not find movi list");
            goto FILE_ERROR_PROCESS;
        }

    } while(strncmp((char*)unknown_list->nFccListType.u8Fcc, "movi", sizeof(Media_Fourcc_TagType)) != 0);

    movi_list = (Media_RiffList_TagType *)malloc(sizeof(Media_RiffList_TagType));
    if(NULL == movi_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "movi_list malloc err");
        goto OTHER_ERROR_PROCESS;
    }

    nFatResult = f_read(&avi_fileobject, movi_list, sizeof(Media_RiffList_TagType), &read_data_byte_result);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "movi_list f_read");
        goto FATFS_ERROR_PROCESS;
    }

    u32AviListPos = f_tell(&avi_fileobject) - sizeof(Media_Fourcc_TagType);

    nFatResult = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + movi_list->u32Cb - sizeof(Media_Fourcc_TagType));
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "movi_list f_lseek");
        goto FATFS_ERROR_PROCESS;
    }

    idx1_chunk = (Media_RiffChunk_TagType *)malloc(sizeof(Media_RiffChunk_TagType));
    if(NULL == idx1_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "idx1_chunk malloc");
        goto OTHER_ERROR_PROCESS;
    }
    nFatResult = f_read(&avi_fileobject, idx1_chunk, sizeof(Media_RiffChunk_TagType), &read_data_byte_result);
    if(FR_OK != nFatResult)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "idx1_chunk f_read");
        goto FATFS_ERROR_PROCESS;
    }
    u32AviOldIdxPos = f_tell(&avi_fileobject);
    u32AviOldIdxSize = idx1_chunk->u32Cb;

    pItemInfo->nAviFileInfo.u16AviWidth = video_width;
    pItemInfo->nAviFileInfo.u16AviHeight = video_height;
    pItemInfo->nAviFileInfo.fVideoFrameRate = fVideoFrameRate;
    pItemInfo->nAviFileInfo.u32VideoLength = u32VideoLength;
    memmove(pItemInfo->nAviFileInfo.nVideoDataChunk.u8Fcc, nVideoDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType));
    pItemInfo->nAviFileInfo.u16AudioChannels = u16AudioChannels;
    pItemInfo->nAviFileInfo.u32AudioSampleRate = u32AudioSampleRate;
    pItemInfo->nAviFileInfo.u32AudioLenght = u32AudioLenght;
    memmove(pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType));
    pItemInfo->nAviFileInfo.u32AviStreamsCount = u32AviStreamsCount;
    pItemInfo->nAviFileInfo.u32AviListPos = u32AviListPos;
    pItemInfo->nAviFileInfo.u32AviOldIdxPos = u32AviOldIdxPos;
    pItemInfo->nAviFileInfo.u32AviOldIdxSize = u32AviOldIdxSize;
    pItemInfo->nAviFileInfo.u32AviFileSize = u32AviFileSize;

    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFFF, "1. %u\r\n", (uint32_t) pItemInfo->nAviFileInfo.fVideoFrameRate);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFFF, "2. %u\r\n", pItemInfo->nAviFileInfo.u32VideoLength);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFFF, "3. %.4s\r\n", pItemInfo->nAviFileInfo.nVideoDataChunk.u8Fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFFF, "4. %u\r\n", pItemInfo->nAviFileInfo.u16AudioChannels);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFFF, "5. %u\r\n", pItemInfo->nAviFileInfo.u32AudioSampleRate);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFFF, "6. %u\r\n", pItemInfo->nAviFileInfo.u32AudioLenght);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFFF, "7. %.4s\r\n", pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U,  0xFFF, "8. %u\r\n", pItemInfo->nAviFileInfo.u32AviStreamsCount);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U,  0xFFF, "9. %u\r\n", pItemInfo->nAviFileInfo.u32AviListPos);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U,  7U,  0xFFF, "1. %u\r\n", pItemInfo->nAviFileInfo.u32AviOldIdxPos);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U,  0xFFF, "2. %u\r\n", pItemInfo->nAviFileInfo.u32AviOldIdxSize);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U,  0xFFF, "3. %u\r\n", pItemInfo->nAviFileInfo.u32AviFileSize);
    // HAL_Delay(1000);
    // MATRIX_FillScreen(0x0);

    f_close(&avi_fileobject);
    free(riff_chunk);
    free(riff_list);
    free(four_cc);
    free(avi_main_header);
    free(strl_list);
    free(avi_stream_header);
    free(strf_chunk);
    free(bitmap_info_header);
    free(wave_format_ex);
    free(unknown_list);
    free(movi_list);
    free(idx1_chunk);
    return FILE_OK;

    FILE_ERROR_PROCESS:
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xF800, "FILE_ERROR_PROCESS\n");
    HAL_Delay(1000);
    MATRIX_FillScreen(0x0);
    f_close(&avi_fileobject);
    free(riff_chunk);
    free(riff_list);
    free(four_cc);
    free(avi_main_header);
    free(strl_list);
    free(avi_stream_header);
    free(strf_chunk);
    free(bitmap_info_header);
    free(wave_format_ex);
    free(unknown_list);
    free(movi_list);
    free(idx1_chunk);
    return FILE_ERROR;

    FATFS_ERROR_PROCESS:
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xF800, "FATFS_ERROR_PROCESS\n");
    HAL_Delay(1000);
    MATRIX_FillScreen(0x0);
    f_close(&avi_fileobject);
    free(riff_chunk);
    free(riff_list);
    free(four_cc);
    free(avi_main_header);
    free(strl_list);
    free(avi_stream_header);
    free(strf_chunk);
    free(bitmap_info_header);
    free(wave_format_ex);
    free(unknown_list);
    free(movi_list);
    free(idx1_chunk);
    return FATFS_ERROR;

    OTHER_ERROR_PROCESS:
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xF800, "OTHER_ERROR_PROCESS\n");
    HAL_Delay(1000);
    MATRIX_FillScreen(0x0);
    f_close(&avi_fileobject);
    free(riff_chunk);
    free(riff_list);
    free(four_cc);
    free(avi_main_header);
    free(strl_list);
    free(avi_stream_header);
    free(strf_chunk);
    free(bitmap_info_header);
    free(wave_format_ex);
    free(unknown_list);
    free(movi_list);
    free(idx1_chunk);
    return OTHER_ERROR;
}


inline MEDIA_FileResult_Type MEDIA_ReadAviStream(Media_PlayInfo_TagType *pItemInfo, uint32_t u32CurrentFrame)
{
    static FIL avi_fileobject;
    FRESULT nFatResult;
    UINT read_data_byte_result;
    static uint32_t linkmap_table[LINKMAP_TABLE_SIZE];

    Media_Avi_Index_TagType avi_old_index_0, avi_old_index_1;
    Media_ChunkHeader_TagType nStreamChunkHeader;

    uint32_t u32ActualCurrentFrame;
    uint32_t u32Idx1;

    static int8_t previous_filename[MEDIA_MAX_NAME_LENGHT];

    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "FN: %.4s %i\n", pItemInfo->nFileName, u32CurrentFrame);
    // float fCurrentFps = pItemInfo->nAviFileInfo.fVideoFrameRate;
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 17U,  0xFFF, "%s\r\n", pItemInfo->nFileName);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 24U,  0xFFF, "%d.%5fFPS\r\n", (uint32_t) fCurrentFps, (uint32_t)((fCurrentFps - (uint32_t) fCurrentFps) * 100000) );
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 31U,  0xFFF, "%lu\r\n", pItemInfo->nAviFileInfo.u32VideoLength);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 38U,  0xFFF, "%.4s\r\n", pItemInfo->nAviFileInfo.nVideoDataChunk.u8Fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 45U,  0xFFF, "%u\r\n", pItemInfo->nAviFileInfo.u16AudioChannels);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 52U,  0xFFF, "%luHz\r\n", pItemInfo->nAviFileInfo.u32AudioSampleRate);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 59U,  0xFFF, "%lu\r\n", pItemInfo->nAviFileInfo.u32AudioLenght);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 17U,  0xFFF, "%.4s\r\n", pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 24U,  0xFFF, "%lu\r\n", pItemInfo->nAviFileInfo.u32AviStreamsCount);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 31U,  0xFFF, "movi:0x%08lx\r\n", pItemInfo->nAviFileInfo.u32AviListPos);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 38U,  0xFFF, "idx1:0x%08lx\r\n", pItemInfo->nAviFileInfo.u32AviOldIdxPos);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 45U,  0xFFF, "idx1:0x%08lx\r\n", pItemInfo->nAviFileInfo.u32AviOldIdxSize);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 52U,  0xFFF, "AVI:%luB\r\n", pItemInfo->nAviFileInfo.u32AviFileSize);
    // HAL_Delay(4000);
    // MATRIX_FillScreen(0x0);

    if(pItemInfo->nAviFileInfo.u32VideoLength < u32CurrentFrame)
    {
        u32ActualCurrentFrame = pItemInfo->nAviFileInfo.u32VideoLength - 1;
    }
    else
    {
        u32ActualCurrentFrame = u32CurrentFrame;
    }

    if(strcmp((char*)pItemInfo->nFileName, (char*)previous_filename) != 0)
    {
        uint8_t u8Timeout = 10U;

        LL_TIM_DisableCounter( TIM6 );
        LL_TIM_DisableCounter(TIM_DAT);
        LL_TIM_DisableCounter( TIM3 );
        memset(Audio_Buffer, 0, (2 * MEDIA_AUDIO_MAX_SAMPLE_RATE * MEDIA_AUDIO_MAX_CHANNEL / MEDIA_VIDEO_MIN_FLAME_RATE));

        do
        {
            /**
             * Note: When read stream data, sometime they are corrupted. In this case we can work around below step by steps.
             * 1. Close the current opened file.
             * 2. Un-mount the current directory.
             * 3. Add few delayed times.
             * 4. Mount sd-card again.
             * 5. Open the current file.
             * 6. If this behavior failed, try to repeat it until timeout occured.
             */
            (void) f_close(&avi_fileobject);
            (void) f_mount(&SDFatFS, (TCHAR const*) SDPath, 1);
            HAL_Delay( 5U );
            (void) f_mount(&SDFatFS, (TCHAR const*) SDPath, 1);
            nFatResult = f_open(&avi_fileobject, (TCHAR*)pItemInfo->nFileName, FA_READ);
        } while ( (u8Timeout--) && (FR_OK != nFatResult));

        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_avi_stream f_open\r\n");
            goto FATFS_ERROR_PROCESS;
        }

        avi_fileobject.cltbl = linkmap_table;
        linkmap_table[0] = LINKMAP_TABLE_SIZE;
        nFatResult = f_lseek(&avi_fileobject, CREATE_LINKMAP);
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_avi_header linkmap\r\n");
            goto FATFS_ERROR_PROCESS;
        }

        SET_AUDIO_SAMPLERATE(pItemInfo->nAviFileInfo.u32AudioSampleRate);
        u8AudioChannnelCount = pItemInfo->nAviFileInfo.u16AudioChannels;
    }

    for(u32Idx1 = 0;u32Idx1 < pItemInfo->nAviFileInfo.u32AviStreamsCount;u32Idx1++)
    {
        nFatResult = f_lseek(&avi_fileobject, (pItemInfo->nAviFileInfo.u32AviOldIdxPos + sizeof(Media_Avi_Index_TagType) * (pItemInfo->nAviFileInfo.u32AviStreamsCount * u32ActualCurrentFrame + u32Idx1)));
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_index f_lseek\r\n");
            goto FATFS_ERROR_PROCESS;
        }

        nFatResult = f_read(&avi_fileobject, &avi_old_index_0, sizeof(Media_Avi_Index_TagType), &read_data_byte_result);
        if(FR_OK != nFatResult)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_old_idx f_read\r\n");
            goto FATFS_ERROR_PROCESS;
        }

        if(strncmp((char*)avi_old_index_0.dwu8ChunkId.u8Fcc, (char*)pItemInfo->nAviFileInfo.nVideoDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
        {
            nFatResult = f_lseek(&avi_fileobject, (avi_old_index_0.u32Offset + pItemInfo->nAviFileInfo.u32AviListPos));
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame chunk f_lseek\r\n");
                goto FATFS_ERROR_PROCESS;
            }
            nFatResult = f_read(&avi_fileobject, &nStreamChunkHeader, sizeof(Media_ChunkHeader_TagType), &read_data_byte_result);
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame chunk f_read\r\n");
                goto FATFS_ERROR_PROCESS;
            }

            if(strncmp((char*)nStreamChunkHeader.u8ChunkId , (char*)pItemInfo->nAviFileInfo.nVideoDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
            {
                nFatResult = f_read(&avi_fileobject, Flame_Buffer, (MEDIA_Y_COUNT * MEDIA_X_COUNT * MEDIA_COLOR_COUNT), &read_data_byte_result);
                if(FR_OK != nFatResult)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read\r\n");
                    goto FATFS_ERROR_PROCESS;
                }
            }
            else
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "vid flame chunk wrong\r\n");
            }
        }

        if(strncmp((char*)avi_old_index_0.dwu8ChunkId.u8Fcc, (char*)pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
        {
            u16AudioFlameDataCount = avi_old_index_0.u32Size;
        }

        if(0 == u32ActualCurrentFrame)
        {
            if(strncmp((char*)avi_old_index_0.dwu8ChunkId.u8Fcc, (char*)pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
            {
                nFatResult = f_lseek(&avi_fileobject, (avi_old_index_0.u32Offset + pItemInfo->nAviFileInfo.u32AviListPos));
                if(FR_OK != nFatResult)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio chunk f_lseek\r\n");
                    goto FATFS_ERROR_PROCESS;
                }

                nFatResult = f_read(&avi_fileobject, &nStreamChunkHeader, sizeof(Media_ChunkHeader_TagType), &read_data_byte_result);
                if(FR_OK != nFatResult)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio chunk f_read\r\n");
                    goto FATFS_ERROR_PROCESS;
                }

                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "u8ChunkId: %.4s\nu32ChunkSize: %i", nStreamChunkHeader.u8ChunkId, nStreamChunkHeader.u32ChunkSize );
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);

                if(strncmp((char*)nStreamChunkHeader.u8ChunkId , (char*)pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
                {
                    // nFatResult = f_read(&avi_fileobject, &Audio_Buffer[0][0], nStreamChunkHeader.u32ChunkSize, &read_data_byte_result);
                    // if(FR_OK != nFatResult)
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read\r\n");
                    //     goto FATFS_ERROR_PROCESS;
                    // }
                    // else
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    // }
                    u8AudioDoubleBuffer = 0;
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio dat chunk wrong\r\n");
                }
            }

            nFatResult = f_lseek(&avi_fileobject, (pItemInfo->nAviFileInfo.u32AviOldIdxPos + sizeof(Media_Avi_Index_TagType) * (pItemInfo->nAviFileInfo.u32AviStreamsCount * (u32ActualCurrentFrame + 1) + u32Idx1)));
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_idx f_lseek\r\n");
                goto FATFS_ERROR_PROCESS;
            }
            nFatResult = f_read(&avi_fileobject, &avi_old_index_1, sizeof(Media_Avi_Index_TagType), &read_data_byte_result);
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_old_idx f_read\r\n");
                goto FATFS_ERROR_PROCESS;
            }

            if(strncmp((char*)avi_old_index_1.dwu8ChunkId.u8Fcc, (char*)pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
            {
                nFatResult = f_lseek(&avi_fileobject, (avi_old_index_1.u32Offset + pItemInfo->nAviFileInfo.u32AviListPos));
                if(FR_OK != nFatResult)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio dat chunk f_lseek\r\n");
                    goto FATFS_ERROR_PROCESS;
                }
                nFatResult = f_read(&avi_fileobject, &nStreamChunkHeader, sizeof(Media_ChunkHeader_TagType), &read_data_byte_result);
                if(FR_OK != nFatResult)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio data chunk f_read\r\n");
                    goto FATFS_ERROR_PROCESS;
                }

                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "u8ChunkId: %.4s\nu32ChunkSize: %i", nStreamChunkHeader.u8ChunkId, nStreamChunkHeader.u32ChunkSize );
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);

                if(strncmp((char*)nStreamChunkHeader.u8ChunkId , (char*)pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
                {
                    // nFatResult = f_read(&avi_fileobject, &Audio_Buffer[1][0], nStreamChunkHeader.u32ChunkSize, &read_data_byte_result);
                    // if(FR_OK != nFatResult)
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read\r\n");
                    //     goto FATFS_ERROR_PROCESS;
                    // }
                    // else
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    // }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio dat chunk wrong\r\n");
                }
            }
        }
        else if((pItemInfo->nAviFileInfo.u32VideoLength - 1) <= u32ActualCurrentFrame)
        {
            memset(Audio_Buffer, 0, (2 * MEDIA_AUDIO_MAX_SAMPLE_RATE * MEDIA_AUDIO_MAX_CHANNEL / MEDIA_VIDEO_MIN_FLAME_RATE));
        }
        else
        {
            nFatResult = f_lseek(&avi_fileobject, (pItemInfo->nAviFileInfo.u32AviOldIdxPos + sizeof(Media_Avi_Index_TagType) * (pItemInfo->nAviFileInfo.u32AviStreamsCount * (u32ActualCurrentFrame + 1) + u32Idx1)));
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_idx f_lseek\r\n");
                goto FATFS_ERROR_PROCESS;
            }
            nFatResult = f_read(&avi_fileobject, &avi_old_index_1, sizeof(Media_Avi_Index_TagType), &read_data_byte_result);
            if(FR_OK != nFatResult)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_old_idx f_read 3\r\n");
                goto FATFS_ERROR_PROCESS;
            }

            if(strncmp((char*)avi_old_index_1.dwu8ChunkId.u8Fcc, (char*)pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
            {
                nFatResult = f_lseek(&avi_fileobject, (avi_old_index_1.u32Offset + pItemInfo->nAviFileInfo.u32AviListPos));
                if(FR_OK != nFatResult)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio dat chunk f_lseek as\r\n");
                    goto FATFS_ERROR_PROCESS;
                }
                nFatResult = f_read(&avi_fileobject, &nStreamChunkHeader, sizeof(Media_ChunkHeader_TagType), &read_data_byte_result);
                if(FR_OK != nFatResult)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read au dat chunk f_read\r\n");
                    goto FATFS_ERROR_PROCESS;
                }

                if(strncmp((char*)nStreamChunkHeader.u8ChunkId , (char*)pItemInfo->nAviFileInfo.nAudioDataChunk.u8Fcc, sizeof(Media_Fourcc_TagType)) == 0)
                {
                    nFatResult = f_read(&avi_fileobject, &Audio_Buffer[~u8AudioDoubleBuffer & 0x01][0], nStreamChunkHeader.u32ChunkSize, &read_data_byte_result);
                    if(FR_OK != nFatResult)
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read NSS\r\n");
                        goto FATFS_ERROR_PROCESS;
                    }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio dat chunk wrong\r\n");
                }
            }
        }
    }

    LL_TIM_EnableCounter( TIM6 );
    LL_TIM_EnableCounter( TIM6);
    LL_TIM_EnableCounter( TIM_DAT);
    LL_TIM_EnableCounter( TIM3 );
    memmove(previous_filename, pItemInfo->nFileName, MEDIA_MAX_NAME_LENGHT);
    return FILE_OK;

    FATFS_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xF800, "FATFS_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
    return FATFS_ERROR;
}

inline MEDIA_State_Type MEDIA_GetCurrentState(void)
{
    return nCurrentState;
}

inline void MEDIA_SetCurrentState(MEDIA_State_Type nSetState)
{
    nCurrentState = nSetState;
}

inline void MEDIA_SetAudioFlameFlag(MEDIA_EndFlag_Type nValue)
{
    nAudioFlameEndFlag = nValue;
}

inline MEDIA_EndFlag_Type MEDIA_GetAudioFlameFlag(void)
{
    return nAudioFlameEndFlag;
}

inline void MEDIA_UpdateStream(void)
{
    uint8_t r, g, b;
    uint16_t u16Color;

    uint8_t u8xIdx;
    uint8_t u8yIdx;

    for( u8xIdx = 0U; u8xIdx < MATRIX_WIDTH; u8xIdx++ )
    {
        for( u8yIdx = 0U; u8yIdx < MATRIX_HEIGHT; u8yIdx++)
        {
#if defined(RGB444) /* RGB444 */
            r =  (Flame_Buffer[u8yIdx][u8xIdx][1] >> 3U) & 0x1F;
            g = ((Flame_Buffer[u8yIdx][u8xIdx][1] << 2U) & 0x1C) | ((Flame_Buffer[u8yIdx][u8xIdx][0] >> 6U) & 0x3);
            b =   Flame_Buffer[u8yIdx][u8xIdx][0] & 0x1F;

            r = pgm_read_byte(&gamma_table[(r * 255) >> 5]); // Gamma correction table maps
            g = pgm_read_byte(&gamma_table[(g * 255) >> 5]); // 5-bit input to 4-bit output
            b = pgm_read_byte(&gamma_table[(b * 255) >> 5]);

            u16Color = ((r << 12) & 0xF000) |
                        ((g <<  7) &  0x780) |
                        ((b <<  1) &   0x1E) ;
#elif defined(RGB555) /* RGB555 */
            r =  (Flame_Buffer[u8yIdx][u8xIdx][1] >> 3U) & 0x1F;
            g = ((Flame_Buffer[u8yIdx][u8xIdx][1] << 3U) & 0x38) | ((Flame_Buffer[u8yIdx][u8xIdx][0] >> 5U) & 0x7);
            b =   Flame_Buffer[u8yIdx][u8xIdx][0] & 0x1F;

            r = pgm_read_byte(&gamma_table[(r * 255) >> 5]); // Gamma correction table maps
            g = pgm_read_byte(&gamma_table[(g * 255) >> 6]); // 5-bit input to 4-bit output
            b = pgm_read_byte(&gamma_table[(b * 255) >> 5]);
            u16Color =  ((r << 12) & 0xF000) | ((r << 8) & 0x800) | // 4/4/4 -> 5/5/5
                        ((g <<  7) &  0x780) | ((g << 3) &  0x40) |
                        ((b <<  1) &   0x1E) | ((b >> 3) &   0x1) ;
#elif defined(RGB565) /* RGB565 */
            r =  (Flame_Buffer[u8yIdx][u8xIdx][1] >> 3U) & 0x1F;
            g = ((Flame_Buffer[u8yIdx][u8xIdx][1] << 3U) & 0x38) | ((Flame_Buffer[u8yIdx][u8xIdx][0] >> 5U) & 0x7);
            b =   Flame_Buffer[u8yIdx][u8xIdx][0] & 0x1F;

            r = pgm_read_byte(&gamma_table[(r * 255) >> 5]); // Gamma correction table maps
            g = pgm_read_byte(&gamma_table[(g * 255) >> 6]); // 5-bit input to 4-bit output
            b = pgm_read_byte(&gamma_table[(b * 255) >> 5]);
            u16Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                        (g <<  7) | ((g & 0xC) << 3) |
                        (b <<  1) | ( b        >> 3);
#endif
            MATRIX_WritePixel( u8xIdx, MATRIX_HEIGHT - u8yIdx - 1, u16Color, 1U );
        }
    }
}

uint32_t MEDIA_GetAudioBufferAddress(void)
{
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "Audio: 0x%08X!\n", Audio_Buffer );
    return (uint32_t) Audio_Buffer;
}
uint32_t MEDIA_GetVideoBufferAddress(void)
{
    return (uint32_t) Flame_Buffer;
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif

/** @} */
