/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ***********************************************************************************************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dac.h"
#include "fatfs.h"
#include "i2c.h"
#include "libjpeg.h"
#include "sdmmc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "matrix.h"
#include "256_192.h"
#include "string.h"
#include "stdio.h"
#include "anime.h"
#include "nature.h"
#include "neu.h"
#include "pic1.h"
#include "phung.h"
#include <setjmp.h>
#include "gamma.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

FRESULT SDResult;
FILINFO fno;
DIR dir;

uint16_t nobytsread;
uint16_t gIndex=0;
extern __IO int audiotxcomplete;
extern __IO uint8_t readingvideo;
extern __IO char leaves[];
extern __IO uint8_t black[];
extern __IO uint8_t white[];
extern __IO int pixx,pixy;   //pixy=caddr,pixx=raddr
uint16_t framesize;
uint16_t frameno=0;
uint16_t movilocation;
uint16_t framelocation;
uint16_t audindex=0;
extern JSAMPLE * image_buffer;	/* Points to large array of R,G,B-order data */
extern int image_height;	/* Number of rows in image */
extern int image_width;		/* Number of columns in image */

// __attribute__((aligned(128))) unsigned char pagebuff[0x500];       //buffer to store frame
__attribute__((aligned(128))) unsigned char pagebuff[0x500];       //buffer to store frame

// struct jpeg_decompress_struct cinfo;
// struct jpeg_error_mgr jerr;


struct jpeg_decompress_struct cinfo;

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

struct my_error_mgr jerr __attribute__((section (".ram_d1_cacheable")));
int row_stride __attribute__((section (".ram_d1_cacheable"))); /* physical row width in output buffer */
JSAMPROW nRowBuff[1] __attribute__((section (".ram_d1_cacheable"))); /* Output row buffer */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
void IRQ_ProcessMonitor( void );
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if( htim->Instance == TIM4 )
    {
        IRQ_ProcessMonitor();
    }
}

/*
 * Here's the routine that will replace the standard error_exit method:
 */

void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

    MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "my_error_exit!\n");
  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

FRESULT scan_files (char* pat)
{
    UINT i;
    uint16_t color = 65535;
    char path[20];
    sprintf (path, "%s",pat);

    SDResult = f_opendir(&dir, path);                       /* Open the directory */
    if (SDResult == FR_OK)
    {
        for (;;)
        {
            SDResult = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (SDResult != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR)     /* It is a directory */
            {
                if (!(strcmp ("SYSTEM~1", fno.fname))) continue;
                MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "Dir: %s\r\n", fno.fname);
                color -= 1024;
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                SDResult = scan_files(path);                     /* Enter the directory */
                if (SDResult != FR_OK) break;
                path[i] = 0;
            }
            else
            {                                       /* It is a file. */
                MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "File: %s/%s\r\n", path, fno.fname);
                color -= 1024;
            }
        }
        f_closedir(&dir);
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "Error open dir!\n");
    }
    return SDResult;
}


static int bufftoint(char *buff)
{
    int x=buff[3];
    x=x<<8;
    x=x|buff[2];
    x=x<<8;
    x=x|buff[1];
    x=x<<8;
    x=x|buff[0];

    return x;
}


uint8_t MEDIA_DisplayJpeg(char * pPath)
{
    uint8_t u8ReturnError = 0x0U;
    uint8_t r, g, b;
    uint16_t u16Color;
    uint16_t u16Index;

    if( FR_OK==f_open(&SDFile, (TCHAR*) pPath, FA_READ) )
    {
        /* Step 1: allocate and initialize JPEG decompression object */
        /* We set up the normal JPEG error routines, then override error_exit. */
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;

        /* Establish the setjmp return context for my_error_exit to use. */
        if (setjmp(jerr.setjmp_buffer))
        {
            /* If we get here, the JPEG code has signaled an error.
             * We need to clean up the JPEG object, close the input file, and return.
                */
            jpeg_destroy_decompress(&cinfo);
            MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "Destroy Compression!\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            f_close(&SDFile);
            u8ReturnError = 0x1U;
        }
        else
        {
            /* Now we can initialize the JPEG decompression object. */
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "jpeg_create_decompress: %i\n",0);
            jpeg_create_decompress(&cinfo);
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "jpeg_stdio_src: %i\n",0);
            /* Step 2: specify data source (eg, a file) */
            jpeg_stdio_src(&cinfo, &SDFile);
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "jpeg_read_header: %i\n",0);
            /* Step 3: read file parameters with jpeg_read_header() */
            (void) jpeg_read_header(&cinfo, TRUE);
            /* Step 4: set parameters for decompression */
            /* In this example, we don't need to change any of the defaults set by
             * jpeg_read_header(), so we do nothing here.
                */
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "jpeg_start_decompress: %i\n",0);
            /* Step 5: Start decompressor */
            (void) jpeg_start_decompress(&cinfo);
            /* We may need to do some setup of our own at this point before reading
             * the data.  After jpeg_start_decompress() we have the correct scaled
                * output image dimensions available, as well as the output colormap
                * if we asked for color quantization.
                * In this example, we need to make an output work buffer of the u16RightPos size.
                */
            /* JSAMPLEs per row in output buffer */
            /* row_stride = cinfo.output_width * cinfo.output_components; */
            /* Make a one-row-high sample array that will go away when done with image */
            /* nRowBuff[0] = (unsigned char *) malloc( row_stride ); */
            nRowBuff[0] = (unsigned char *) pagebuff;
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "print: %i\n",0);
            /* Step 6: while (scan lines remain to be read) */
            /*           jpeg_read_scanlines(...); */
            while( cinfo.output_scanline < cinfo.output_height)
            {
                /* jpeg_read_scanlines expects an array of pointers to scanlines.
                 * Here the array is only one element long, but you could ask for
                    * more than one scanline at a time if that's more convenient.
                    */
                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "step 0: %i\n",cinfo.output_height);
                (void) jpeg_read_scanlines(&cinfo, nRowBuff, 1);
                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "step 1: %i\n",cinfo.output_height);
                for( u16Index = 0U; u16Index < MATRIX_WIDTH; u16Index++ )
                {
                    r = (uint8_t) pagebuff[ u16Index * 3 + 0 ];
                    g = (uint8_t) pagebuff[ u16Index * 3 + 1 ];
                    b = (uint8_t) pagebuff[ u16Index * 3 + 2 ];

                    r = pgm_read_byte(&gamma_table[(r * 255) >> 8]); // Gamma correction table maps
                    g = pgm_read_byte(&gamma_table[(g * 255) >> 8]); // 8-bit input to 4-bit output
                    b = pgm_read_byte(&gamma_table[(b * 255) >> 8]);

                    u16Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                                (g <<  7) | ((g & 0xC) << 3) |
                                (b <<  1) | ( b        >> 3);

                    MATRIX_WritePixel( u16Index, cinfo.output_scanline - 1, u16Color );
                }
            }

            // MATRIX_SetCursor(0, 5);
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "%s\r\n",  pPath);

            /* Step 7: Finish decompression */
            (void) jpeg_finish_decompress(&cinfo);
                /* Step 8: Release JPEG decompression object */
            /* This is an important step since it will release a good deal of memory. */
            jpeg_destroy_decompress(&cinfo);
            /* After finish_decompress, we can close the input file. */
            f_close(&SDFile);
            /**
             * In case use the static buffer as a local pointer, it must not be free caused from it shall re-use another behavior
             * free(nRowBuff[0]);
             */
        }
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "can't open !\n");
        u8ReturnError = 0x2U;
    }

    return u8ReturnError;
}

typedef enum
{
  FILE_OK = 0,
  FATFS_ERROR = 1,
  FILE_ERROR = 2,
  OTHER_ERROR = 10
} READ_FILE_RESULT;

#define BPP24 24
#define BPP16 16
#define BI_RGB 0x00000000
#define BMP_FILE_TYPE 0x4D42
#define BMP_FILE_HEADER_SIZE 14
#define BMP_INFO_HEADER_SIZE 40
#define BMP_DEFAULT_HEADER_SIZE (BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE)


#pragma pack(2)
typedef struct BITMAPFILEHEADER
{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;
#pragma pack()
typedef struct BITMAPINFOHEADER
{
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;


#define CHUNKHEADER_SIZE 8
#define RIFF_CHUNK_SIZE (4 + CHUNKHEADER_SIZE)
#define WAVEFORMATEX_CHUNK_SIZE (18 + CHUNKHEADER_SIZE)

typedef struct CHUNKHEADER
{
    uint8_t chunkID[4];
    uint32_t chunkSize;
} CHUNKHEADER;

typedef struct RIFF
{
    CHUNKHEADER header;
    uint8_t format[4];
} RIFF;

#define WAVE_FORMAT_PCM 0x0001

#pragma pack(2)
typedef struct WAVEFORMATEX
{
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
} WAVEFORMATEX;
#pragma pack()

#define FCC(ch4) ((((uint32_t)(ch4) & 0xFF) << 24) |     \
                  (((uint32_t)(ch4) & 0xFF00) << 8) |    \
                  (((uint32_t)(ch4) & 0xFF0000) >> 8) |  \
                  (((uint32_t)(ch4) & 0xFF000000) >> 24))

/*
typedef struct _fourcc {
  uint32_t fcc;
} FOURCC;
*/
typedef struct _fourcc
{
    uint8_t fcc[4];
} FOURCC;

typedef struct _riffchunk
{
    FOURCC fcc;
    uint32_t  cb;
} RIFFCHUNK, * LPRIFFCHUNK;

typedef struct _rifflist
{
    FOURCC fcc;
    uint32_t  cb;
    FOURCC fccListType;
} RIFFLIST, * LPRIFFLIST;

#define RIFFROUND(cb) ((cb) + ((cb)&1))
#define RIFFNEXT(pChunk) (LPRIFFCHUNK)((uint8_t *)(pChunk) + sizeof(RIFFCHUNK) + RIFFROUND(((LPRIFFCHUNK)pChunk)->cb))

/* ==================== avi header structures ===========================
 *     main header for the avi file (compatibility header)
 */
#define ckidMAINAVIHEADER FCC('avih')
typedef struct _avimainheader
{
    FOURCC fcc;                    // 'avih'
    uint32_t  cb;                     // size of this structure -8
    uint32_t  dwMicroSecPerFrame;     // frame display rate (or 0L)
    uint32_t  dwMaxBytesPerSec;       // max. transfer rate
    uint32_t  dwPaddingGranularity;   // pad to multiples of this size; normally 2K.
    uint32_t  dwFlags;                // the ever-present flags
    #define AVIF_HASINDEX        0x00000010 // Index at end of file?
    #define AVIF_MUSTUSEINDEX    0x00000020
    #define AVIF_ISINTERLEAVED   0x00000100
    #define AVIF_TRUSTCKTYPE     0x00000800 // Use CKType to find key frames
    #define AVIF_WASCAPTUREFILE  0x0001000
    #define AVIF_COPYRIGHTED     0x0001000
    uint32_t  dwTotalFrames;          // # frames in first movi list
    uint32_t  dwInitialFrames;
    uint32_t  dwStreams;
    uint32_t  dwSuggestedBufferSize;
    uint32_t  dwWidth;
    uint32_t  dwHeight;
    uint32_t  dwReserved[4];
} AVIMAINHEADER;

#define ckidODML          FCC('odml')
#define ckidAVIEXTHEADER  FCC('dmlh')
typedef struct _aviextheader
{
   FOURCC  fcc;                    // 'dmlh'
   uint32_t   cb;                     // size of this structure -8
   uint32_t   dwGrandFrames;          // total number of frames in the file
   uint32_t   dwFuture[61];           // to be defined later
} AVIEXTHEADER;

/* structure of an AVI stream header riff chunk */
#define ckidSTREAMLIST   FCC('strl')

#ifndef ckidSTREAMHEADER
#define ckidSTREAMHEADER FCC('strh')
#endif
typedef struct _avistreamheader
{
   FOURCC fcc;          // 'strh'
   uint32_t  cb;           // size of this structure - 8

   FOURCC fccType;      // stream type codes

   #ifndef streamtypeVIDEO
   #define streamtypeVIDEO FCC('vids')
   #define streamtypeAUDIO FCC('auds')
   #define streamtypeMIDI  FCC('mids')
   #define streamtypeTEXT  FCC('txts')
   #endif

   FOURCC fccHandler;
   uint32_t  dwFlags;
   #define AVISF_DISABLED          0x00000001
   #define AVISF_VIDEO_PALCHANGES  0x0001000

   uint16_t   wPriority;
   uint16_t   wLanguage;
   uint32_t  dwInitialFrames;
   uint32_t  dwScale;
   uint32_t  dwRate;       // dwRate/dwScale is stream tick rate in ticks/sec
   uint32_t  dwStart;
   uint32_t  dwLength;
   uint32_t  dwSuggestedBufferSize;
   uint32_t  dwQuality;
   uint32_t  dwSampleSize;
   struct
   {
        uint16_t left;
        uint16_t top;
        uint16_t right;
        uint16_t bottom;
    } rcFrame;
} AVISTREAMHEADER;

/* structure of an AVI stream format chunk */
#ifndef ckidSTREAMFORMAT
#define ckidSTREAMFORMAT FCC('strf')
#endif
//
// avi stream formats are different for each stream type
//
// BITMAPINFOHEADER for video streams
// WAVEFORMATEX or PCMWAVEFORMAT for audio streams
// nothing for text streams
// nothing for midi streams

typedef struct _avioldindex_entry
{
   FOURCC   dwChunkId;
   uint32_t   dwFlags;

#ifndef AVIIF_LIST
    #define AVIIF_LIST       0x00000001
    #define AVIIF_KEYFRAME   0x00000010
#endif

    #define AVIIF_NO_TIME    0x00000100
    #define AVIIF_COMPRESSOR 0x0FFF0000  // unused?
    uint32_t   dwOffset;    // offset of riff chunk header for the data
    uint32_t   dwSize;      // size of the data (excluding riff header size)
} aIndex;          // size of this array

//
// structure of old style AVI index
//
#define ckidAVIOLDINDEX FCC('idx1')
typedef struct _avioldindex
{
   FOURCC  fcc;        // 'idx1'
   uint32_t   cb;         // size of this structure -8
   aIndex avoidindex;
} AVIOLDINDEX;

typedef struct
{
    float video_frame_rate;
    uint32_t video_length;
    FOURCC video_data_chunk_name;
    uint16_t audio_channels;
    uint32_t audio_sampling_rate;
    uint32_t audio_length;
    FOURCC audio_data_chunk_name;
    uint32_t avi_streams_count;
    uint32_t movi_list_position;
    uint32_t avi_old_index_position;
    uint32_t avi_old_index_size;
    uint32_t avi_file_size;
    uint16_t avi_height;
    uint16_t avi_width;
} AVI_INFO;

typedef struct
{
    char file_name[_MAX_LFN];
    AVI_INFO avi_info;
} PLAY_INFO;

/* APB1 Timer clock 80Mhz */
#define TIM6_FREQ 80000000
#define TIM6_PRESCALER 0

#define TIM7_FREQ 108000000
#define TIM7_PRESCALER 150

#define TIM14_FREQ 108000000
#define TIM14_PRESCALER 181

#define IR_MODULATION_UNIT_TOLERANCE 50

#define SET_AUDIO_SAMPLERATE(x) \
    do                                  \
    {                                   \
        TIM6->ARR = ((unsigned int)(TIM6_FREQ / ((x) * (TIM6_PRESCALER + 1))) - 200U);   \
    } while(0)

//   __HAL_TIM_SET_AUTORELOAD(&htim6, ((unsigned int)(TIM6_FREQ / ((x) * (TIM6_PRESCALER + 1))) - 1))
// #define SET_AUDIO_SAMPLERATE(x)

#define COLOR_R 0
#define COLOR_G 1
#define COLOR_B 2

#define VERTICAL 0
#define HORIZONTAL 1

#define PLAY 0
#define STOP 1
#define PAUSE 2

#define LOOP_NO 0
#define LOOP_SINGLE 1
#define LOOP_ALL 2

#define LINKMAP_TABLE_SIZE 0x14

#define MATRIXLED_X_COUNT 256
#define MATRIXLED_Y_COUNT 192

#define MATRIXLED_COLOR_COUNT 2
#define MATRIXLED_PWM_RESOLUTION 256

#define SPI_SEND_COMMAND_COUNT 4

#define SPI_DELAY_TIME_0 200
#define SPI_DELAY_TIME_1 500

#define FONT_DATA_PIXEL_SIZE_X 5
#define FONT_DATA_PIXEL_SIZE_Y 8
#define FONT_DATA_MAX_CHAR 192
#define DISPLAY_TEXT_TIME 500
#define DISPLAY_TEXT_MAX 45

#define MIN_VIDEO_FLAME_RATE 15
#define MAX_VIDEO_FLAME_RATE 60
#define MIN_AUDIO_SAMPLE_RATE 44099
#define MAX_AUDIO_SAMPLE_RATE 48000
#define MIN_AUDIO_CHANNEL 1
#define MAX_AUDIO_CHANNEL 2

#define IMAGE_READ_DELAY_FLAME 2

#define DISPLAY_FPS_AVERAGE_TIME 0.4

#define MAX_PLAYLIST_COUNT 9
#define MAX_TRACK_COUNT 10

#define SKIP_TIME 5

volatile unsigned char Video_End_Flag = RESET;
volatile unsigned char Audio_End_Flag = RESET;

float Volume_Value_float = 1;
unsigned char Status = PLAY;
unsigned int Audio_Flame_Data_Count __attribute__((section (".ram_d1_cacheable")));
unsigned char Audio_Double_Buffer __attribute__((section (".ram_d1_cacheable")));
unsigned char Audio_Channnel_Count __attribute__((section (".ram_d1_cacheable")));

volatile unsigned char Audio_Flame_End_flag = RESET;

signed short Audio_Buffer[2][MAX_AUDIO_SAMPLE_RATE * MAX_AUDIO_CHANNEL / MIN_VIDEO_FLAME_RATE] __attribute__((section (".ram_d1_cacheable"))) = {0};

unsigned char Flame_Buffer[MATRIXLED_Y_COUNT][MATRIXLED_X_COUNT][MATRIXLED_COLOR_COUNT] ;

READ_FILE_RESULT SD_ReadAviHeader(PLAY_INFO *play_info)
{
    FRESULT fatfs_result;
    FIL avi_fileobject;

    uint32_t linkmap_table[LINKMAP_TABLE_SIZE];

    RIFFCHUNK *riff_chunk = NULL;
    RIFFLIST *riff_list = NULL;
    FOURCC *four_cc = NULL;
    AVIMAINHEADER *avi_main_header = NULL;
    RIFFLIST *strl_list = NULL;
    AVISTREAMHEADER *avi_stream_header = NULL;
    RIFFCHUNK *strf_chunk = NULL;
    BITMAPINFOHEADER *bitmap_info_header = NULL;
    WAVEFORMATEX *wave_format_ex = NULL;
    RIFFLIST *unknown_list = NULL;
    RIFFLIST *movi_list = NULL;
    RIFFCHUNK *idx1_chunk = NULL;
    //aIndex *avi_old_index;

    uint8_t strl_list_find_loop_count = 0;

    float video_frame_rate = 0;
    uint32_t video_length = 0U;
    uint16_t audio_channels = 0U;
    uint32_t audio_sampling_rate = 0U;
    uint32_t audio_length = 0U;
    uint32_t avi_streams_count = 0U;
    uint32_t movi_list_position = 0U;
    uint32_t avi_old_index_position = 0U;
    uint32_t avi_old_index_size = 0U;
    uint32_t avi_file_size = 0U;
    uint32_t audio_suggest_buffer_size = 0U;
    uint16_t video_width = 0U;
    uint16_t video_height = 0U;
    uint8_t chunk_name_temp[5];

    FOURCC audio_data_chunk_name;
    FOURCC video_data_chunk_name;

    uint8_t video_stream_find_flag = RESET;
    uint8_t audio_stream_find_flag = RESET;

    UINT read_data_byte_result;


    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x1AF, "-->Read HD");
    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0x1AF, "-->Read HD %s\r\n", play_info->file_name);
    HAL_Delay(400);

    // char path[30] = "2.avi";
    // sprintf(path,"%s", "2.avi");
    // for( uint8_t i = 0; i < 3; i++ )
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1, 1U, 0, 10 * i, 0xFFFF, "%d %d %d %d", play_info->file_name[4*i+0], play_info->file_name[4*i+1], play_info->file_name[4*i+2], play_info->file_name[4*i+3]);
    // }
    // MATRIX_Printf( FONT_DEFAULT, 1, 1U, 0U, 30, 0xFFFF, "%s\r\n", (const TCHAR*) path);

    fatfs_result = f_open(&avi_fileobject, play_info->file_name, FA_READ);
    if(FR_OK != fatfs_result)
    {
        // printf("SD_ReadAviHeader f_open NG fatfs_result=%d\r\n", fatfs_result);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "f_open NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "f_open %s\r\n", (char*)(play_info->file_name));
    }

    avi_fileobject.cltbl = linkmap_table;
    linkmap_table[0] = LINKMAP_TABLE_SIZE;
    fatfs_result = f_lseek(&avi_fileobject, CREATE_LINKMAP);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Link error");
        // printf("SD_ReadAviHeader create linkmap table NG fatfs_result=%d\r\n", fatfs_result);
        // printf("need linkmap table size %lu\r\n", linkmap_table[0]);
        goto FATFS_ERROR_PROCESS;
    }

    riff_chunk = (RIFFCHUNK *)malloc(sizeof(RIFFCHUNK));
    if(NULL == riff_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Malloc error");
        // printf("riff_chunk malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Malloc 0x%X\r\n",riff_chunk);
    }

    fatfs_result = f_read(&avi_fileobject, riff_chunk, sizeof(RIFFCHUNK), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "RIFF f_read error %u", fatfs_result);
        // printf("riff_chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    /*
    printf("RIFF�`�����Nｯ���q:%.4s\r\n", riff_chunk->fcc.fcc);
    printf("RIFF�`�����N�T�C�Y:%u\r\n", riff_chunk->cb);
    */

    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0x1AF, "-->Reading...\n");
    HAL_Delay(200);


    avi_file_size = riff_chunk->cb + sizeof(RIFFCHUNK);

    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0x1AF, "-->avi_file_size: %u\n", avi_file_size);
    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xF8A0, "RIFF fcc: %.4s\r\n", riff_chunk->fcc.fcc);
    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xF80A, "RIFF cb: %u\r\n", riff_chunk->cb);

    if(strncmp((char*)riff_chunk->fcc.fcc, "RIFF", sizeof(FOURCC)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Not RIFF");
        // printf("this file is not RIFF format\r\n");
        goto FILE_ERROR_PROCESS;
    }

    four_cc = (FOURCC *)malloc(sizeof(FOURCC));
    if(NULL == four_cc)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "four_cc error");
        // printf("four_cc malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, four_cc, sizeof(FOURCC), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "four_cc read error");
        // printf("four_cc f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }
    /*
     printf("RIFF�`�����N�t�H�[���^�C�v:%.4s\r\n", four_cc->fcc);
    //*/
    if(strncmp((char*)four_cc->fcc, "AVI ", sizeof(FOURCC)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "RIFF not AVI");
        // printf("RIFF form type is not AVI \r\n");
        goto FILE_ERROR_PROCESS;
    }

    //hdrl���X�g����������
    riff_list = (RIFFLIST *)malloc(sizeof(RIFFLIST));
    if(NULL == riff_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "riff_list malloc");
        // printf("riff_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, riff_list, sizeof(RIFFLIST), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "riff_list f_read");
        // printf("riff_list f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }
    /*
     printf("  hdrl���X�gｯ���q:%.4s\r\n", riff_list->fcc.fcc);
    printf("  hdrl���X�g�T�C�Y:%u\r\n", riff_list->cb);
    printf("  hdrl���X�g�^�C�v:%.4s\r\n", riff_list->fccListType.fcc);
    //*/
    if((strncmp((char*)riff_list->fcc.fcc, "LIST", sizeof(FOURCC)) != 0) || (strncmp((char*)riff_list->fccListType.fcc, "hdrl ", sizeof(FOURCC)) != 0))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not found hdrl");
        // printf("could not find hdrl list\r\n");
        goto FILE_ERROR_PROCESS;
    }

    avi_main_header = (AVIMAINHEADER *)malloc(sizeof(AVIMAINHEADER));
    if(NULL == avi_main_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_main_header malloc");
        // printf("avi_main_header malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, avi_main_header, sizeof(AVIMAINHEADER), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_main_header read");
        // printf("avi_main_header f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    // MATRIX_FillScreen(0x0);
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U,  9U, 0xFFF, "1. %.4s", avi_main_header->fcc.fcc);
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 18U, 0xFFF, "2. %u", avi_main_header->cb );
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 27U, 0xFFF, "3. %u", avi_main_header->dwMicroSecPerFrame);
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 36U, 0xFFF, "4. %u", avi_main_header->dwMaxBytesPerSec );
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 45U, 0xFFF, "5. %u", avi_main_header->dwPaddingGranularity);
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 54U, 0xFFF, "6. %i", avi_main_header->dwFlags);
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 63U, 0xFFF, "7. %u", avi_main_header->dwTotalFrames);
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 72U, 0xFFF, "8. %u", avi_main_header->dwInitialFrames);
    // MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 81U, 0xFFF, "9. %u", avi_main_header->dwStreams);
    // MATRIX_Printf( FONT_DEFAULT, 1U, 64,  9U, 0xFFF, "1. %u", avi_main_header->dwSuggestedBufferSize);
    // MATRIX_Printf( FONT_DEFAULT, 1U, 64, 18U, 0xFFF, "2. %u", avi_main_header->dwWidth);
    // MATRIX_Printf( FONT_DEFAULT, 1U, 64, 27U, 0xFFF, "3. %u", avi_main_header->dwHeight);
    // MATRIX_Printf( FONT_DEFAULT, 1U, 64, 36U, 0xFFF, "4. %u", read_data_byte_result);
    // HAL_Delay(4000);
    // MATRIX_FillScreen(0x0);

    if(strncmp((char*)avi_main_header->fcc.fcc, "avih", sizeof(FOURCC)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not found avi main header");
        // printf("could not find avi main header\r\n");
        goto FILE_ERROR_PROCESS;
    }
    if(AVIF_HASINDEX != (avi_main_header->dwFlags & AVIF_HASINDEX))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi not index");
        // printf("avi file does not have index\r\n");
        goto FILE_ERROR_PROCESS;
    }
    if((MATRIXLED_X_COUNT < avi_main_header->dwWidth) || (MATRIXLED_Y_COUNT < avi_main_header->dwHeight))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "wrong size");
        // printf("wrong size avi file\r\n");
        goto FILE_ERROR_PROCESS;
    }
    else
    {
        video_width = avi_main_header->dwWidth;
        video_height = avi_main_header->dwHeight;
    }

    /*
     if(2 != avi_main_header->dwStreams){
        printf("avi file has too few or too many streams\r\n");
        goto FILE_ERROR_PROCESS;
    }
    //*/

    avi_streams_count = avi_main_header->dwStreams;

    strl_list = (RIFFLIST *)malloc(sizeof(RIFFLIST));
    if(NULL == strl_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "strl_list malloc");
        // printf("strl_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    avi_stream_header = (AVISTREAMHEADER *)malloc(sizeof(AVISTREAMHEADER));
    if(NULL == avi_stream_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi stream malloc");
        // printf("avi_stream_header malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    strf_chunk = (RIFFCHUNK *)malloc(sizeof(RIFFCHUNK));
    if(NULL == strf_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "strf_chunk malloc");
        // printf("strf_chunk malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    bitmap_info_header = (BITMAPINFOHEADER *)malloc(sizeof(BITMAPINFOHEADER));
    if(NULL == bitmap_info_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "bitmap_info_header malloc");
        // printf("bitmap_info_header malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    wave_format_ex = (WAVEFORMATEX *)malloc(sizeof(WAVEFORMATEX));
    if(NULL == wave_format_ex)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "wave_format_ex malloc");
        // printf("wave_format_ex malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    do
    {
        fatfs_result = f_read(&avi_fileobject, strl_list, sizeof(RIFFLIST), &read_data_byte_result);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "strl_list_f_read");
            // printf("strl_list f_read NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        if((strncmp((char*)strl_list->fcc.fcc, "LIST", sizeof(FOURCC)) == 0) && (strncmp((char*)strl_list->fccListType.fcc, "strl", sizeof(FOURCC)) == 0))
        {
            fatfs_result = f_read(&avi_fileobject, avi_stream_header, sizeof(AVISTREAMHEADER), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_stream_header f_read ");
                // printf("avi_stream_header f_read NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }

            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFFF, "1. %.4s\r\n", avi_stream_header->fcc.fcc);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFFF, "2. %u\r\n", avi_stream_header->cb);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFFF, "3. %.4s\r\n", avi_stream_header->fccType.fcc);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFFF, "4. %.4s\r\n", avi_stream_header->fccHandler.fcc);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFFF, "5. %u\r\n", avi_stream_header->dwFlags);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFFF, "6. %u\r\n", avi_stream_header->wPriority);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFFF, "7. %u\r\n", avi_stream_header->wLanguage);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U,  0xFFF, "8. %u\r\n", avi_stream_header->dwInitialFrames);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U,  0xFFF, "9. %u\r\n", avi_stream_header->dwScale);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U,  7U,  0xFFF, "1. %u\r\n", avi_stream_header->dwRate);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U,  0xFFF, "2. %u\r\n", avi_stream_header->dwStart);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U,  0xFFF, "3. %u\r\n", avi_stream_header->dwLength);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 28U,  0xFFF, "4. %u\r\n", avi_stream_header->dwSuggestedBufferSize);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 35U,  0xFFF, "5. %i\r\n", avi_stream_header->dwQuality);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 42U,  0xFFF, "6. %u\r\n", avi_stream_header->dwSampleSize);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 49U,  0xFFF, "7. %i\r\n", avi_stream_header->rcFrame.left);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 56U,  0xFFF, "8. %i\r\n", avi_stream_header->rcFrame.top);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 63U,  0xFFF, "9. %i\r\n", avi_stream_header->rcFrame.right);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 84U,  7U,  0xFFF, "1. %i\r\n", avi_stream_header->rcFrame.bottom);
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 84U, 14U,  0xFFF, "2. %i\r\n", read_data_byte_result);
            // HAL_Delay(400);
            // MATRIX_FillScreen(0x0);

            if(strncmp((char*)avi_stream_header->fcc.fcc, "strh", sizeof(FOURCC)) == 0)
            {
                if((strncmp((char*)avi_stream_header->fccType.fcc, "vids", sizeof(FOURCC)) == 0) && (video_stream_find_flag == RESET))
                {
                    if((avi_stream_header->dwFlags & (AVISF_DISABLED | AVISF_VIDEO_PALCHANGES)) == 0)
                    {
                        if( (MAX_VIDEO_FLAME_RATE >= (uint32_t)((float)avi_stream_header->dwRate / (float)avi_stream_header->dwScale)) && \
                            (MIN_VIDEO_FLAME_RATE <= ((float)avi_stream_header->dwRate / (float)avi_stream_header->dwScale)))
                        {
                            fatfs_result = f_read(&avi_fileobject, strf_chunk, sizeof(RIFFCHUNK), &read_data_byte_result);
                            if(FR_OK != fatfs_result)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "strl_chunk f_read");
                                // printf("strf_chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                            if(strncmp((char*)strf_chunk->fcc.fcc, "strf", sizeof(FOURCC)) == 0)
                            {
                                fatfs_result = f_read(&avi_fileobject, bitmap_info_header, sizeof(BITMAPINFOHEADER), &read_data_byte_result);
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "bitmap_info_header f_read");
                                    // printf("bitmap_info_header f_read NG fatfs_result=%d\r\n", fatfs_result);
                                    goto FATFS_ERROR_PROCESS;
                                }

                                /* MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,   7U,  0xFF, "1. %u\r\n", bitmap_info_header->biSize);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  14U,  0xFF, "2. %u\r\n", bitmap_info_header->biWidth);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  21U,  0xFF, "3. %u\r\n", bitmap_info_header->biHeight);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  28U,  0xFF, "4. %u\r\n", bitmap_info_header->biPlanes);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  35U,  0xFF, "5. %u\r\n", bitmap_info_header->biBitCount);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  42U,  0xFF, "6. %u\r\n", bitmap_info_header->biCompression);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  49U,  0xFF, "7. %u\r\n", bitmap_info_header->biSizeImage);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  56U,  0xFF, "8. %u\r\n", bitmap_info_header->biXPelsPerMeter);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  42U,  7U,  0xFF, "1. %u\r\n", bitmap_info_header->biYPelsPerMeter);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  42U, 14U,  0xFF, "2. %u\r\n", bitmap_info_header->biClrUsed);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  42U, 21U,  0xFF, "3. %u\r\n", bitmap_info_header->biClrImportant);
                                HAL_Delay(400);
                                MATRIX_FillScreen(0x0); */

                                if((video_width == bitmap_info_header->biWidth) && (video_height == bitmap_info_header->biHeight))
                                {
                                    if(BPP24 == bitmap_info_header->biBitCount)
                                    {
                                        if(BI_RGB == bitmap_info_header->biCompression)
                                        {
                                            video_frame_rate = (float) ((float)avi_stream_header->dwRate / (float)avi_stream_header->dwScale);
                                            video_length = avi_stream_header->dwLength;
                                            snprintf((char*)chunk_name_temp, 5, "%02udb", strl_list_find_loop_count);
                                            memmove(video_data_chunk_name.fcc, chunk_name_temp, sizeof(FOURCC));
                                            video_stream_find_flag = SET;
                                            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F,  1U, 0U,  0U, 0xFFFF, "video_stream read.");
                                            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, 21U,  0xFF, "1. %u\r\n", avi_stream_header->dwRate);
                                            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, 28U,  0xFF, "2. %u\r\n", avi_stream_header->dwScale);
                                            // float test = 15.8467221615153;
                                            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 35U,  0xFF, "VidRate: %5.4f\n", test );
                                            // HAL_Delay(400);
                                            // MATRIX_FillScreen(0x0);
                                            //printf("video_strem read success\r\n");
                                        }
                                        else
                                        {
                                            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not BI_RGB: %i", bitmap_info_header->biCompression );
                                            // printf("video flame is not BI_RGB\r\n");
                                        }
                                    }
                                    else if( BPP16 == bitmap_info_header->biBitCount )
                                    {
                                        video_frame_rate = (float) ((float)avi_stream_header->dwRate / (float)avi_stream_header->dwScale);
                                        video_length = avi_stream_header->dwLength;
                                        snprintf((char*)chunk_name_temp, 5, "%02udc", strl_list_find_loop_count);
                                        memmove(video_data_chunk_name.fcc, chunk_name_temp, sizeof(FOURCC));
                                        video_stream_find_flag = SET;
                                        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "not 24bpp");
                                        // HAL_Delay(400);
                                        // MATRIX_FillScreen(0x0);
                                        // printf("video flame is not 24bpp\r\n");
                                    }
                                }
                                else
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "wrong size video");
                                    // printf("wrong size video stream\r\n");
                                }
                                fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(BITMAPINFOHEADER));
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "bitmap_info_header f_lseek");
                                    // printf("bitmap_info_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                                }
                            }
                            else
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not found strl_chunk");
                                // printf("could not find strf_chunk\r\n");
                            }
                            fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(RIFFCHUNK));
                            if(FR_OK != fatfs_result)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "strl_chunk f_lseek");
                                // printf("strf_chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                        }
                        else
                        {
                            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "out of flame range");
                            // printf("video flame rate is out of range\r\n");
                        }
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "stream disabled");
                        // printf("stream is disabled\r\n");
                    }
                }
                else if((strncmp((char*)avi_stream_header->fccType.fcc, "auds", sizeof(FOURCC)) == 0)  && (audio_stream_find_flag == RESET))
                {
                    if((avi_stream_header->dwFlags & AVISF_DISABLED) == 0)
                    {
                        if( (MAX_AUDIO_SAMPLE_RATE >= (avi_stream_header->dwRate / avi_stream_header->dwScale)) && \
                            (MIN_AUDIO_SAMPLE_RATE <= (avi_stream_header->dwRate / avi_stream_header->dwScale)))
                        {
                            fatfs_result = f_read(&avi_fileobject, strf_chunk, sizeof(RIFFCHUNK), &read_data_byte_result);
                            if(FR_OK != fatfs_result)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "strl_chunk f_read");
                                // printf("strf_chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                            if(strncmp((char*)strf_chunk->fcc.fcc, "strf", sizeof(FOURCC)) == 0)
                            {
                                fatfs_result = f_read(&avi_fileobject, wave_format_ex, sizeof(WAVEFORMATEX), &read_data_byte_result);
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "wave_format_ex f_read");
                                    // printf("wave_format_ex f_read NG fatfs_result=%d\r\n", fatfs_result);
                                    goto FATFS_ERROR_PROCESS;
                                }

                                /* MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFF, "1. %04x\r\n", wave_format_ex->wFormatTag);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFF, "2. %u\r\n", wave_format_ex->nChannels);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFF, "3. %u\r\n", wave_format_ex->nSamplesPerSec);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFF, "4. %u\r\n", wave_format_ex->nAvgBytesPerSec);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFF, "5. %u\r\n", wave_format_ex->nBlockAlign);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFF, "6. %u\r\n", wave_format_ex->wBitsPerSample);
                                MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFF, "7. %u\r\n", wave_format_ex->cbSize); */

                                if( WAVE_FORMAT_PCM == wave_format_ex->wFormatTag )
                                {
                                    if( (MAX_AUDIO_CHANNEL >= wave_format_ex->nChannels) && (MIN_AUDIO_CHANNEL <= wave_format_ex->nChannels) )
                                    {
                                        if( (MAX_AUDIO_SAMPLE_RATE >= wave_format_ex->nSamplesPerSec) && (MIN_AUDIO_SAMPLE_RATE <= wave_format_ex->nSamplesPerSec) )
                                        {
                                            if( 16U == wave_format_ex->wBitsPerSample )
                                            {
                                                audio_channels = wave_format_ex->nChannels;
                                                audio_sampling_rate = wave_format_ex->nSamplesPerSec;
                                                audio_length = avi_stream_header->dwLength;
                                                audio_suggest_buffer_size = avi_stream_header->dwSuggestedBufferSize;
                                                snprintf((char*)chunk_name_temp, 5, "%02uwb", strl_list_find_loop_count);
                                                memmove(audio_data_chunk_name.fcc, chunk_name_temp, sizeof(FOURCC));
                                                audio_stream_find_flag = SET;
                                                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 10U, 0xFFFF, "audio read!");
                                                // HAL_Delay(400);
                                                // MATRIX_FillScreen(0x0);
                                                //printf("audio_strem read success\r\n");
                                            }
                                            else
                                            {
                                                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not 16bit audio");
                                                // printf("audio bits per sample is not 16bit\r\n");
                                            }
                                        }
                                        else
                                        {
                                            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "out of audio range");
                                            // printf("audio sample rate is out of range\r\n");
                                        }
                                    }
                                    else
                                    {
                                        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "audio channels out range");
                                        // printf("audio channels is out of range\r\n");
                                    }
                                }
                                else
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not PCM");
                                    // printf("audio format is not linearPCM\r\n");
                                }

                                fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(WAVEFORMATEX));
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "bitmap_info_header f_lssek");
                                    // printf("bitmap_info_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                    goto FATFS_ERROR_PROCESS;
                                }
                            }
                            else
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not found strf_chunk");
                                // printf("could not find strf_chunk\r\n");
                            }

                            fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(RIFFCHUNK));
                            if(FR_OK != fatfs_result)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "strf_chunk f_lseek");
                                // printf("strf_chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                        }
                        else
                        {
                            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "out range audio sample");
                            // printf("audio sample rate is out of range\r\n");
                        }
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "stream disabled");
                        // printf("stream is disabled\r\n");
                    }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not vid or audio");
                    // printf("stream is not video or audio\r\n");
                }
#if 0
                if((audio_stream_find_flag == SET) &&
                    ( (((wave_format_ex->nSamplesPerSec * wave_format_ex->nChannels * 2) / video_frame_rate) >= (audio_suggest_buffer_size + wave_format_ex->nBlockAlign * 2)) ||
                    ( ((wave_format_ex->nSamplesPerSec * wave_format_ex->nChannels * 2) / video_frame_rate) <= (audio_suggest_buffer_size - wave_format_ex->nBlockAlign * 2))))
#else
                if((audio_stream_find_flag == SET) &&
                    ( ((wave_format_ex->nSamplesPerSec * wave_format_ex->nChannels) >= (audio_suggest_buffer_size + wave_format_ex->nBlockAlign * 2)) ||
                    ( (wave_format_ex->nSamplesPerSec * wave_format_ex->nChannels) <= (audio_suggest_buffer_size - wave_format_ex->nBlockAlign * 2))))
#endif
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "audio buff size wrong");

                    /* MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFF, "1. %u\r\n", wave_format_ex->nSamplesPerSec);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFF, "2. %u\r\n", wave_format_ex->nChannels);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFF, "3. %u\r\n", video_frame_rate);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFF, "4. %u\r\n", audio_suggest_buffer_size);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFF, "5. %u\r\n", wave_format_ex->nBlockAlign);
                    HAL_Delay(400);
                    MATRIX_FillScreen(0x0); */

                    /* We will inrestigate it soon */
                    audio_stream_find_flag = RESET;
                    // printf("audio buffer size is wrong\r\n");
                }
            }
            else
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "wrong strh header");
                // printf("wrong strh header\r\n");
            }

            fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(AVISTREAMHEADER));
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_header_stream f_lseek");
                // printf("avi_stream_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            strl_list_find_loop_count++;
        }
        else
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not found strl list");
            // printf("could not find strl list\r\n");
        }

        fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + strl_list->cb - sizeof(FOURCC));
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_header_stream f_lseek");
            // printf("avi_stream_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_header_stream f_lseek done!");
            // HAL_Delay(400);
            // MATRIX_FillScreen(0x0);
        }

        if(f_tell(&avi_fileobject) >= (riff_chunk->cb + sizeof(RIFFCHUNK) - 1))
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not find audio video str head");
            // printf("could not find playable audio video stream header\r\n");
            goto FILE_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 10U, 0xFFFF, "Find str_header");
            // HAL_Delay(800);
            // MATRIX_FillScreen(0x0);
        }
    } while( (video_stream_find_flag != SET) || (audio_stream_find_flag != SET) );

    // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0, 0xFFFF, "%s header done!!!", (TCHAR*) play_info->file_name);
    // HAL_Delay(400);
    // MATRIX_FillScreen(0x0);
    //printf("read video audio header success\r\n");
    //printf("%.4s\r\n", video_data_chunk_name.fcc);
    //printf("%.4s\r\n", audio_data_chunk_name.fcc);

    unknown_list = (RIFFLIST *)malloc(sizeof(RIFFLIST));
    if(NULL == unknown_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "unknow malloc");
        // printf("unknown_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    //movi���X�g�T��
    do
    {
        fatfs_result = f_read(&avi_fileobject, unknown_list, sizeof(RIFFLIST), &read_data_byte_result);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "unkown_list f_read");
            // printf("unknown_list f_read NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        if((strncmp((char*)unknown_list->fcc.fcc, "LIST", sizeof(FOURCC)) == 0) && (strncmp((char*)unknown_list->fccListType.fcc, "movi", sizeof(FOURCC)) == 0)){
        fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(RIFFLIST));
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "unknow_list f_lseek");
            // printf("unknown_list f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }

        }else{
        fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + unknown_list->cb - sizeof(FOURCC));
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "unknown_list f_lseek");
            // printf("unknown_list f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        }

        if(f_tell(&avi_fileobject) >= (riff_chunk->cb + sizeof(RIFFCHUNK) - 1))
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "not find movi list");
            // printf("could not find movi list\r\n");
            goto FILE_ERROR_PROCESS;
        }

    } while(strncmp((char*)unknown_list->fccListType.fcc, "movi", sizeof(FOURCC)) != 0);
    // MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "movi list found");
    // HAL_Delay(400);
    // MATRIX_FillScreen(0x0);
    //printf("movi list found\r\n");

    movi_list = (RIFFLIST *)malloc(sizeof(RIFFLIST));
    if(NULL == movi_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "movi_list malloc err");
        // printf("movi_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    fatfs_result = f_read(&avi_fileobject, movi_list, sizeof(RIFFLIST), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "movi_list f_read");
        // printf("movi_list f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    movi_list_position = f_tell(&avi_fileobject) - sizeof(FOURCC);

    fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + movi_list->cb - sizeof(FOURCC));
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "movi_list f_lseek");
        // printf("movi_list f_lseek NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    idx1_chunk = (RIFFCHUNK *)malloc(sizeof(RIFFCHUNK));
    if(NULL == idx1_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "idx1_chunk malloc");
        // printf("idx1_chunk malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, idx1_chunk, sizeof(RIFFCHUNK), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "idx1_chunk f_read");
        // printf("idx1_chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }
    avi_old_index_position = f_tell(&avi_fileobject);
    avi_old_index_size = idx1_chunk->cb;

    play_info->avi_info.avi_width = video_width;
    play_info->avi_info.avi_height = video_height;
    play_info->avi_info.video_frame_rate = video_frame_rate;
    play_info->avi_info.video_length = video_length;
    memmove(play_info->avi_info.video_data_chunk_name.fcc, video_data_chunk_name.fcc, sizeof(FOURCC));
    play_info->avi_info.audio_channels = audio_channels;
    play_info->avi_info.audio_sampling_rate = audio_sampling_rate;
    play_info->avi_info.audio_length = audio_length;
    memmove(play_info->avi_info.audio_data_chunk_name.fcc, audio_data_chunk_name.fcc, sizeof(FOURCC));
    play_info->avi_info.avi_streams_count = avi_streams_count;
    play_info->avi_info.movi_list_position = movi_list_position;
    play_info->avi_info.avi_old_index_position = avi_old_index_position;
    play_info->avi_info.avi_old_index_size = avi_old_index_size;
    play_info->avi_info.avi_file_size = avi_file_size;

    // MATRIX_FillScreen(0x0);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFFF, "1. %u\r\n", (uint32_t) play_info->avi_info.video_frame_rate);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFFF, "2. %u\r\n", play_info->avi_info.video_length);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFFF, "3. %.4s\r\n", play_info->avi_info.video_data_chunk_name.fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFFF, "4. %u\r\n", play_info->avi_info.audio_channels);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFFF, "5. %u\r\n", play_info->avi_info.audio_sampling_rate);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFFF, "6. %u\r\n", play_info->avi_info.audio_length);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFFF, "7. %.4s\r\n", play_info->avi_info.audio_data_chunk_name.fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U,  0xFFF, "8. %u\r\n", play_info->avi_info.avi_streams_count);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U,  0xFFF, "9. %u\r\n", play_info->avi_info.movi_list_position);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U,  7U,  0xFFF, "1. %u\r\n", play_info->avi_info.avi_old_index_position);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U,  0xFFF, "2. %u\r\n", play_info->avi_info.avi_old_index_size);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U,  0xFFF, "3. %u\r\n", play_info->avi_info.avi_file_size);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 28U,  0xFFF, "4. %u\r\n", play_info->avi_info.avi_width);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 35U,  0xFFF, "5. %u\r\n", play_info->avi_info.avi_height);
    // HAL_Delay(4000);
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
    //free(avi_old_index);
    return FILE_OK;

    FILE_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "FILE_ERROR_PROCESS\n");
        HAL_Delay(3000);
        MATRIX_FillScreen(0x0);
        // printf("\r\n");
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
    //free(avi_old_index);
    return FILE_ERROR;

    FATFS_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "FATFS_ERROR_PROCESS\n");
        HAL_Delay(3000);
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
    //free(avi_old_index);
    return FATFS_ERROR;

    OTHER_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "OTHER_ERROR_PROCESS\n");
        HAL_Delay(3000);
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
    //free(avi_old_index);
    return OTHER_ERROR;
}

READ_FILE_RESULT SD_GetPlayList(char *playlist_filename, PLAY_INFO **playlist, uint8_t *track_count)
{
    FILINFO playlist_fileinfo;
    FRESULT fatfs_result;
    uint32_t bytesread;

    uint8_t *token_pointer;
    uint8_t playlist_data[_MAX_LFN];
    uint8_t track_count_temp = 0;

    PLAY_INFO *playlist_temp0;
    PLAY_INFO *playlist_temp1;

    MATRIX_SetCursor(0U, 0U);

    fatfs_result = f_stat(playlist_filename, &playlist_fileinfo);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "%s f_stat error %u\r\n", playlist_filename, fatfs_result);
        // printf("%s f_stat error %u\r\n", playlist_filename, fatfs_result);
        //fatfs_result = f_open(&SDFile, playlist_filename, FA_WRITE|FA_OPEN_ALWAYS);
        //fatfs_result = f_write(&SDFile, Playlist_Default_Message, 461, NULL);
        //fatfs_result = f_close(&SDFile);
        goto FATFS_ERROR_PROCESS;
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "%s f_stat done %u\r\n", playlist_filename, fatfs_result);
    }
    fatfs_result = f_open(&SDFile, (TCHAR*)&playlist_fileinfo.fname, FA_READ);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "%s f_open error %u\r\n", playlist_filename, fatfs_result);
        // printf("%s f_open error %u\r\n", playlist_filename, fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "%s %s f_open done %u\r\n", playlist_filename, playlist_fileinfo.fname, fatfs_result);
    }

    // fatfs_result = f_read(&SDFile, playlist_data, sizeof(playlist_data), (UINT*)&bytesread);
    // if(FR_OK != fatfs_result)
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "f_read error %u\r\n", fatfs_result);
    //     HAL_Delay(400);
    //     MATRIX_FillScreen(0x0);
    //     // printf("%s f_read error %u\r\n", playlist_filename, fatfs_result);
    //     goto FATFS_ERROR_PROCESS;
    // }
    // else
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "This content: %s\r\n", playlist_data);
    //     HAL_Delay(400);
    //     MATRIX_FillScreen(0x0);
    // }


    while((f_gets((TCHAR*)playlist_data, (_MAX_LFN + 16), &SDFile) != NULL) && (track_count_temp <= MAX_TRACK_COUNT))
    {
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0x07E0, "L: ");
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "L:%s\r\n", strtok((char*)playlist_data, "\r\n"));
        // HAL_Delay(400);
        printf("%s\r\n", strtok((char*)playlist_data, "\r\n"));
        track_count_temp++;
    }

    //printf("\r\n");
    if(0 == track_count_temp)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "No track count\r\n");
        // printf("item does not exist in playlist\r\n");
        goto FILE_ERROR_PROCESS;
    }

    fatfs_result = f_lseek(&SDFile, 0);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "%s f_lseek error %u\r\n", playlist_filename, fatfs_result);
        // printf("%s f_lseek error %u\r\n", playlist_filename, fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    playlist_temp0 = (PLAY_INFO *)malloc(sizeof(PLAY_INFO) * track_count_temp);
    if(NULL == playlist_temp0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "malloc error\r\n");
        // printf("malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    HAL_Delay(400);
    track_count_temp = 0;
    while( (f_gets((TCHAR*)playlist_data, (_MAX_LFN + 16), &SDFile) != NULL) && (track_count_temp <= MAX_TRACK_COUNT))
    {
        token_pointer = (uint8_t*)strtok((char*)playlist_data, ",\r\n");

        if((token_pointer != NULL) && (strncmp((char*)playlist_data, "//", 2) != 0))
        {
            strcpy((char*)playlist_temp0[track_count_temp].file_name, (char*)token_pointer);
            MATRIX_FillScreen(0U);
            MATRIX_SetCursor(0U, 0U);
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Read %s, 0x%X\r\n", playlist_temp0[track_count_temp].file_name, token_pointer);
            if(FILE_OK == SD_ReadAviHeader(&playlist_temp0[track_count_temp]))
            {
                track_count_temp++;
            }
        }
        else
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Read %s error\r\n", playlist_data);
        }
    }

    if(0 == track_count_temp)
    {
        free(playlist_temp0);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "item does not exist in playlist\r\n");
        // printf("item does not exist in playlist\r\n");
        goto FILE_ERROR_PROCESS;
    }

    playlist_temp1 = (PLAY_INFO *)realloc(playlist_temp0, (sizeof(PLAY_INFO) * track_count_temp));
    if(NULL == playlist_temp1){
        free(playlist_temp0);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "realloc error\r\n");
        // printf("realloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    fatfs_result = f_close(&SDFile);
    if(FR_OK != fatfs_result){
        free(playlist_temp1);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "%s f_close error %u\r\n", playlist_filename, fatfs_result);
        // printf("%s f_close error %u\r\n", playlist_filename, fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    MATRIX_FillScreen(0U);
    MATRIX_SetCursor(0U, 0U);
    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Track: %i\r\n", track_count_temp);

    //  for(int testloop = 0;testloop < track_count_temp;testloop++)
    //  {
    //     float fCurrentFps = playlist_temp1[testloop].avi_info.video_frame_rate;
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U,  9U,  0xFFF, "%s\r\n", playlist_temp1[testloop].file_name);
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 18U,  0xFFF, "%d.%5fFPS\r\n", (uint32_t) fCurrentFps, (uint32_t)((fCurrentFps - (uint32_t) fCurrentFps) * 4000) );
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 27U,  0xFFF, "%lu\r\n", playlist_temp1[testloop].avi_info.video_length);
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 36U,  0xFFF, "%.4s\r\n", playlist_temp1[testloop].avi_info.video_data_chunk_name.fcc);
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 45U,  0xFFF, "%u\r\n", playlist_temp1[testloop].avi_info.audio_channels);
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 54U,  0xFFF, "%luHz\r\n", playlist_temp1[testloop].avi_info.audio_sampling_rate);
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 63U,  0xFFF, "%lu\r\n", playlist_temp1[testloop].avi_info.audio_length);
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 72U,  0xFFF, "%.4s\r\n", playlist_temp1[testloop].avi_info.audio_data_chunk_name.fcc);
    //     MATRIX_Printf( FONT_DEFAULT, 1U,  0U, 81U,  0xFFF, "%lu\r\n", playlist_temp1[testloop].avi_info.avi_streams_count);
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 60U, 9U,  0xFFF, "movi:0x%08lx\r\n", playlist_temp1[testloop].avi_info.movi_list_position);
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 60U, 18U,  0xFFF, "idx1:0x%08lx\r\n", playlist_temp1[testloop].avi_info.avi_old_index_position);
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 60U, 27U,  0xFFF, "idx1:0x%08lx\r\n", playlist_temp1[testloop].avi_info.avi_old_index_size);
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 60U, 36U,  0xFFF, "AVI:%luB\r\n", playlist_temp1[testloop].avi_info.avi_file_size);
    //     HAL_Delay(5000);
    //     MATRIX_FillScreen(0x0);
    // }

    (*playlist) = playlist_temp1;
    (*track_count) = track_count_temp;
    return FILE_OK;

    FILE_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "FILE_ERROR_PROCESS\r\n");
        (*playlist) = NULL;
        (*track_count) = 0;
    return FILE_ERROR;

    FATFS_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "FATFS_ERROR_PROCESS\r\n");
        (*playlist) = NULL;
        (*track_count) = 0;
    return FATFS_ERROR;

    OTHER_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "OTHER_ERROR_PROCESS\r\n");
        (*playlist) = NULL;
        (*track_count) = 0;
    return OTHER_ERROR;
};

READ_FILE_RESULT SD_ReadAviStream(PLAY_INFO *play_info, uint32_t read_frame_count)
{
    FRESULT fatfs_result;
    UINT read_data_byte_result;
    static uint32_t linkmap_table[LINKMAP_TABLE_SIZE] __attribute__((section (".ram_d1_cacheable")));

    aIndex avi_old_index_0, avi_old_index_1;
    CHUNKHEADER stream_chunk_header;

    uint32_t read_frame_count_temp;
    uint32_t idx1_search_loop;

    static FIL avi_fileobject;
    static int8_t previous_filename[_MAX_LFN] __attribute__((section (".ram_d1_cacheable")));

    // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFF, "FN: %.4s %i\n", play_info->file_name, read_frame_count);
    // float fCurrentFps = play_info->avi_info.video_frame_rate;
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 17U,  0xFFF, "%s\r\n", play_info->file_name);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 24U,  0xFFF, "%d.%5fFPS\r\n", (uint32_t) fCurrentFps, (uint32_t)((fCurrentFps - (uint32_t) fCurrentFps) * 10000) );
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 31U,  0xFFF, "%lu\r\n", play_info->avi_info.video_length);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 38U,  0xFFF, "%.4s\r\n", play_info->avi_info.video_data_chunk_name.fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 45U,  0xFFF, "%u\r\n", play_info->avi_info.audio_channels);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 52U,  0xFFF, "%luHz\r\n", play_info->avi_info.audio_sampling_rate);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 59U,  0xFFF, "%lu\r\n", play_info->avi_info.audio_length);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 17U,  0xFFF, "%.4s\r\n", play_info->avi_info.audio_data_chunk_name.fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 24U,  0xFFF, "%lu\r\n", play_info->avi_info.avi_streams_count);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 31U,  0xFFF, "movi:0x%08lx\r\n", play_info->avi_info.movi_list_position);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 38U,  0xFFF, "idx1:0x%08lx\r\n", play_info->avi_info.avi_old_index_position);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 45U,  0xFFF, "idx1:0x%08lx\r\n", play_info->avi_info.avi_old_index_size);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 52U,  0xFFF, "AVI:%luB\r\n", play_info->avi_info.avi_file_size);
    // HAL_Delay(4000);
    // MATRIX_FillScreen(0x0);

    if(play_info->avi_info.video_length < read_frame_count)
    {
        read_frame_count_temp = play_info->avi_info.video_length - 1;
    }
    else
    {
        read_frame_count_temp = read_frame_count;
    }

    if(strcmp((char*)play_info->file_name, (char*)previous_filename) != 0)
    {

        // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, 7U, 0xFFFFU, "Starting..\r\n");
        // HAL_Delay(1000);
        // MATRIX_FillScreen(0x0);
        // __HAL_TIM_DISABLE(&htim6);
        memset(Audio_Buffer, 0, (2 * MAX_AUDIO_SAMPLE_RATE * MAX_AUDIO_CHANNEL / MIN_VIDEO_FLAME_RATE));

        fatfs_result = f_close(&avi_fileobject);

        fatfs_result = f_open(&avi_fileobject, (TCHAR*)play_info->file_name, FA_READ);
        // fatfs_result = f_open(&avi_fileobject, "T1.avi", FA_READ);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read_avi_stream f_open\r\n");
            // printf("read_avi_stream f_open NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFFU, "read_avi_stream f_open\r\n");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }

        avi_fileobject.cltbl = linkmap_table;
        linkmap_table[0] = LINKMAP_TABLE_SIZE;
        fatfs_result = f_lseek(&avi_fileobject, CREATE_LINKMAP);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read_avi_header linkmap\r\n");
            // printf("read_avi_header create linkmap table NG fatfs_result=%d\r\n", fatfs_result);
            // printf("need linkmap table size %lu\r\n", linkmap_table[0]);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFFU, "read_avi_header linkmap\r\n");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }

        SET_AUDIO_SAMPLERATE(play_info->avi_info.audio_sampling_rate);
        Audio_Channnel_Count = play_info->avi_info.audio_channels;
    }

    for(idx1_search_loop = 0;idx1_search_loop < play_info->avi_info.avi_streams_count;idx1_search_loop++)
    {
        fatfs_result = f_lseek(&avi_fileobject, (play_info->avi_info.avi_old_index_position + sizeof(aIndex) * (play_info->avi_info.avi_streams_count * read_frame_count_temp + idx1_search_loop)));
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read_index f_lseek\r\n");
            // printf("read_index f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFFU, "read_index f_lseek\r\n");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }
        fatfs_result = f_read(&avi_fileobject, &avi_old_index_0, sizeof(aIndex), &read_data_byte_result);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_old_idx f_read\r\n");
            // printf("avi_old_index f_read NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFFU, "avi_old_idx f_read\r\n");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }

        if(strncmp((char*)avi_old_index_0.dwChunkId.fcc, (char*)play_info->avi_info.video_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
        {
            fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_0.dwOffset + play_info->avi_info.movi_list_position));
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read vid flame chunk f_lseek\r\n");
                // printf("read video flame chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFFU, "read vid flame chunk f_lseek\r\n");
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);
            }
            fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read vid flame chunk f_read\r\n");
                // printf("read video flame chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFFU, "read vid flame chunk f_read\r\n");
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);
            }
            /*
             printf("�f���t���[���`�����NID:%.4s\r\n", stream_chunk_header.chunkID);
            printf("�f���t���[���`�����N�T�C�Y:%u\r\n", stream_chunk_header.chunkSize);
            printf("\r\n");
            //*/
            if(strncmp((char*)stream_chunk_header.chunkID , (char*)play_info->avi_info.video_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
            {
                fatfs_result = f_read(&avi_fileobject, Flame_Buffer, (play_info->avi_info.avi_height * play_info->avi_info.avi_width * MATRIXLED_COLOR_COUNT), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read vid flame f_read\r\n");
                    // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read vid flame f_read\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }
            }
            else
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "vid flame chunk wrong\r\n");
                // printf("video flame chunk id is wrong\r\n");
            }
        }

        if(strncmp((char*)avi_old_index_0.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
        {
            Audio_Flame_Data_Count = avi_old_index_0.dwSize;
        }
        // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFF, "Parse Frame: %i\n", read_frame_count_temp);
        // HAL_Delay(1000);
        // MATRIX_FillScreen(0x0);

        if(0 == read_frame_count_temp)
        {
            if(strncmp((char*)avi_old_index_0.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
            {
                fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_0.dwOffset + play_info->avi_info.movi_list_position));
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read audio chunk f_lseek\r\n");
                    // printf("read audio data chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read audio chunk f_lseek\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }
                fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read audio chunk f_read\r\n");
                    // printf("read audio data chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFF, "read audio chunk f_read\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }

                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFF, "ChunkID: %.4s\nChunkSize: %i", stream_chunk_header.chunkID, stream_chunk_header.chunkSize );
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);

                if(strncmp((char*)stream_chunk_header.chunkID , (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
                {
                    // fatfs_result = f_read(&avi_fileobject, &Audio_Buffer[0][0], stream_chunk_header.chunkSize, &read_data_byte_result);
                    // if(FR_OK != fatfs_result)
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    //     // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                    //     goto FATFS_ERROR_PROCESS;
                    // }
                    // else
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    // }
                    Audio_Double_Buffer = 0;
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "audio dat chunk wrong\r\n");
                // printf("audio data chunk id is wrong\r\n");
                }
            }

            fatfs_result = f_lseek(&avi_fileobject, (play_info->avi_info.avi_old_index_position + sizeof(aIndex) * (play_info->avi_info.avi_streams_count * (read_frame_count_temp + 1) + idx1_search_loop)));
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read_idx f_lseek\r\n");
                // printf("read_index f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read_idx f_lseek\r\n");
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);
            }
            fatfs_result = f_read(&avi_fileobject, &avi_old_index_1, sizeof(aIndex), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_old_idx f_read\r\n");
                // printf("avi_old_index f_read NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_old_idx f_read\r\n");
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);
            }

            if(strncmp((char*)avi_old_index_1.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
            {
                fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_1.dwOffset + play_info->avi_info.movi_list_position));
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read audio dat chunk f_lseek\r\n");
                    // printf("read audio data chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read audio dat chunk f_lseek\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }
                fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read audio data chunk f_read\r\n");
                    // printf("read audio data chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Read audio data chunk f_read\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }

                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "ChunkID: %.4s\nChunkSize: %i", stream_chunk_header.chunkID, stream_chunk_header.chunkSize );
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);

                if(strncmp((char*)stream_chunk_header.chunkID , (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
                {
                    // fatfs_result = f_read(&avi_fileobject, &Audio_Buffer[1][0], stream_chunk_header.chunkSize, &read_data_byte_result);
                    // if(FR_OK != fatfs_result)
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    //     // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                    //     goto FATFS_ERROR_PROCESS;
                    // }
                    // else
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    // }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "audio dat chunk wrong\r\n");
                // printf("audio data chunk id is wrong\r\n");
                }
            }
        }
        else if((play_info->avi_info.video_length - 1) <= read_frame_count_temp)
        {
            memset(Audio_Buffer, 0, (2 * MAX_AUDIO_SAMPLE_RATE * MAX_AUDIO_CHANNEL / MIN_VIDEO_FLAME_RATE));
        }
        else
        {
            fatfs_result = f_lseek(&avi_fileobject, (play_info->avi_info.avi_old_index_position + sizeof(aIndex) * (play_info->avi_info.avi_streams_count * (read_frame_count_temp + 1) + idx1_search_loop)));
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read_idx f_lseek\r\n");
                // printf("read_index f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            fatfs_result = f_read(&avi_fileobject, &avi_old_index_1, sizeof(aIndex), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "avi_old_idx f_read 3\r\n");
                // printf("avi_old_index f_read NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }

            if(strncmp((char*)avi_old_index_1.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
            {
                fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_1.dwOffset + play_info->avi_info.movi_list_position));
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read audio dat chunk f_lseek as\r\n");
                    // printf("read audio data chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "read au dat chunk f_read\r\n");
                    // printf("read audio data chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                /*
                printf("�����f�[�^1�`�����NID:%.4s\r\n", stream_chunk_header.chunkID);
                printf("�����f�[�^1�`�����N�T�C�Y:%u\r\n", stream_chunk_header.chunkSize);
                printf("\r\n");
                //*/
                if(strncmp((char*)stream_chunk_header.chunkID , (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
                {
                    fatfs_result = f_read(&avi_fileobject, &Audio_Buffer[~Audio_Double_Buffer & 0x01][0], stream_chunk_header.chunkSize, &read_data_byte_result);
                    if(FR_OK != fatfs_result)
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read NSS\r\n");
                        // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                        goto FATFS_ERROR_PROCESS;
                    }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "audio dat chunk wrong\r\n");
                    // printf("audio data chunk id is wrong\r\n");
                }
            }
        }
    }

    // __HAL_TIM_ENABLE(&htim6);
    memmove(previous_filename, play_info->file_name, _MAX_LFN);
    return FILE_OK;

    FATFS_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "FATFS_ERROR_PROCESS\r\n");
        HAL_Delay(3000);
        MATRIX_FillScreen(0x0);
        // snprintf((char*)previous_filename, _MAX_LFN, "*");
    return FATFS_ERROR;
}


void SD_PlayAviVideo(void)
{
    uint8_t r, g, b;
    uint16_t u16Color;

    PLAY_INFO *playlist;

    // unsigned char playlist_count = 0;
    unsigned char track_count = 0;
    unsigned char all_track_count = 0;
    unsigned int frame_count = 0;
    char playlist_filename[_MAX_LFN] = "list.txt";

    float flame_rate;

    unsigned char loop_status = LOOP_ALL;
    // unsigned char ledpanel_brightness = 100;
    // unsigned char volume_value = 5;

    // unsigned int previous_sw_value = 0;

    // char display_text_buffer[DISPLAY_TEXT_MAX];
    // unsigned short display_text_flame;
    // unsigned short display_text_flame_count = 0;
    // char status_text[DISPLAY_TEXT_MAX] = {0};

    // char movie_time[20] = {0};
    // unsigned char movie_total_time_min, movie_total_time_sec;
    // unsigned char movie_current_time_min, movie_current_time_sec;

    // char mediainfo_char_buffer[DISPLAY_TEXT_MAX];

    // unsigned char display_OSD = SET;
    // unsigned char display_status = SET;
    // unsigned char display_mediainfo = RESET;

    // unsigned short tim7_count_value;
    // unsigned int tim7_count_add = 0;
    // unsigned char display_fps_average_count = 0;

    /* CLOCK_DisableAllIrq(); */
    /* __asm("cpsid i\n"); */
    SD_GetPlayList(playlist_filename, &playlist, &all_track_count);
    /* __asm("cpsie i\n"); */
    /* CLOCK_EnableAllIrq(); */
    MATRIX_Printf( FONT_DEFAULT, 1, 0, 0, 0xF81F, "Get Playlist done!\r\nTrackCount: %i", all_track_count );
    HAL_Delay(2000);
    MATRIX_FillScreen(0x0);
    MATRIX_SetCursor(0, 0);

    SD_ReadAviStream(&playlist[0], 0);
    // display_text_flame_count = 0;
    float fCurrkFps = 0;

    uint32_t u32PreFrame = 0UL;
    uint32_t u32CurrTick = 0UL;
    uint32_t u32PreTick = 0UL;
    uint16_t u16VideoHeight = playlist[track_count].avi_info.avi_height;
    uint16_t u16VideoHeightDiv2 = u16VideoHeight >> 1;
    uint16_t u16VideoWidth = playlist[track_count].avi_info.avi_width;
    uint16_t u16VideoWidthDiv2 = u16VideoWidth >> 1;
    uint16_t u16xTmp;
    uint16_t u16xTmpCalibratePos;
    uint16_t u16yTmp14;
    uint16_t u16yTmp14CalibratePos;
    uint16_t u16yTmp23;
    uint16_t u16yTmp23CalibratePos;
    uint16_t u16xSecondFrame;
    uint16_t u16ySecondFrame;
    uint16_t u8ErrorPos;
    uint16_t u8HeightStart = (MATRIX_HEIGHT - u16VideoHeight) >> 1;
    uint16_t u8WidthStart = (MATRIX_WIDTH - u16VideoWidth) >> 1;
    MATRIX_Printf( FONT_DEFAULT, 1, 0x0, 0x0, 0xF81F, "Starting... W: %d, H: %d", u16VideoWidth, u16VideoHeight );
    HAL_Delay(500);

    while(1)
    {
        flame_rate = playlist[track_count].avi_info.video_frame_rate;
        // display_text_flame = (unsigned short)(flame_rate * 500) / DISPLAY_TEXT_TIME;
        SD_ReadAviStream(&playlist[track_count], frame_count);

        if(PLAY == Status)
        {
            if(frame_count < playlist[track_count].avi_info.video_length)
            {
                frame_count++;
            }
            else
            {
                frame_count = 0;
                Video_End_Flag = SET;
                Audio_End_Flag = SET;
            }
        } else if(PAUSE == Status)
        {
            (void) 1U;
        }
        else
        {
            frame_count = 0;
        }

        if((Video_End_Flag == SET) && (Audio_End_Flag == SET))
        {
            switch(loop_status)
            {
            case LOOP_NO:
                Status = STOP;
                // display_text_flame_count = 0;
                // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "Stop");
                break;
            case LOOP_SINGLE:
                // display_text_flame_count = 0;
                // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "%02u:%s", track_count, playlist[track_count].file_name);
                break;
            case LOOP_ALL:
                if(track_count < (all_track_count - 1))
                {
                    track_count++;
                }
                else
                {
                    track_count = 0;
                }
                // display_text_flame_count = 0;
                // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "%02u:%s", track_count, playlist[track_count].file_name);
                break;
            default:
                Status = STOP;
                // display_text_flame_count = 0;
                // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "Stop");
                break;
            }

            fCurrkFps = 0;
            u32PreFrame = 0UL;
            u32CurrTick = 0UL;
            u32PreTick = 0UL;
            u16VideoHeight = playlist[track_count].avi_info.avi_height;
            u16VideoHeightDiv2 = u16VideoHeight >> 1;
            u16VideoWidth = playlist[track_count].avi_info.avi_width;
            u16VideoWidthDiv2 = u16VideoWidth >> 1;
            u8HeightStart = (MATRIX_HEIGHT - u16VideoHeight) >> 1;
            u8WidthStart = (MATRIX_WIDTH - u16VideoWidth) >> 1;

            MATRIX_Printf( FONT_DEFAULT, 1, 0x0, 0x0, 0xF81F, "Playing next video...");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            MATRIX_SetCursor(0, 0);

            Video_End_Flag = RESET;
            Audio_End_Flag = RESET;
        }

        u8ErrorPos = 0U;
        // MATRIX_DispImage( Flame_Buffer, 0U, 0U, 128U, 64U );
        for( uint16_t u8xIdx = 0U; u8xIdx < u16VideoWidthDiv2; u8xIdx++ )
        {
            if( u8xIdx == (u16VideoWidthDiv2-1) )
            {
                u8ErrorPos = 1U;
                u16xTmpCalibratePos = 0U;
            }
            else
            {
                u16xTmpCalibratePos = u16VideoWidthDiv2 + u8xIdx + 1U;
            }

            u16xTmp = u8xIdx+1;
            u16xSecondFrame = u8xIdx+u16VideoWidthDiv2;

            for( uint16_t u8yIdx = 0U; u8yIdx < u16VideoHeightDiv2; u8yIdx++)
            {
                u16yTmp14 = u8HeightStart + u16VideoHeight - u8yIdx - 1U;
                u16yTmp23 = u8HeightStart + u16VideoHeightDiv2 - u8yIdx - 1U;
                u16yTmp14CalibratePos = u16yTmp14 - u8ErrorPos;
                u16yTmp23CalibratePos = u16yTmp23 - u8ErrorPos;
                u16ySecondFrame = u8yIdx+u16VideoHeightDiv2;

                /* For part 1 */
                u16Color = Flame_Buffer[u8yIdx][u8xIdx][1]<<8U | Flame_Buffer[u8yIdx][u8xIdx][0];
                MATRIX_WritePixel(u16xTmp, u16yTmp14, u16Color );
                /* For part 2 */
                u16Color = Flame_Buffer[u16ySecondFrame][u8xIdx][1]<<8U | Flame_Buffer[u16ySecondFrame][u8xIdx][0];
                MATRIX_WritePixel(u16xTmp, u16yTmp23, u16Color );
                /* For part 3 */
                u16Color = Flame_Buffer[u16ySecondFrame][u16xSecondFrame][1]<<8U | Flame_Buffer[u16ySecondFrame][u16xSecondFrame][0];
                MATRIX_WritePixel(u16xTmpCalibratePos, u16yTmp23CalibratePos, u16Color );
                /* For part 4 */
                u16Color = Flame_Buffer[u8yIdx][u16xSecondFrame][1]<<8U | Flame_Buffer[u8yIdx][u16xSecondFrame][0];
                MATRIX_WritePixel(u16xTmpCalibratePos, u16yTmp14CalibratePos, u16Color );
            }
        }

        MATRIX_Printf( FONT_DEFAULT, 1U, 85, 10, 0xFFFF, "%i.%ifPs", (uint32_t) fCurrkFps, (uint32_t) ((fCurrkFps - (uint32_t) fCurrkFps) * 10) );
        /* MATRIX_UpdateScreen(); */
        u32CurrTick = HAL_GetTick();
        /* OFF_LED(); */
        // while(Audio_Flame_End_flag == RESET);
        Audio_Flame_End_flag = RESET;
        // HAL_Delay(500/flame_rate - (u32CurrTick - u32PreTick) - 1U);
        if( u32CurrTick - u32PreTick >= 500)
        {
            fCurrkFps = (frame_count - u32PreFrame) << 1;

            u32PreTick = u32CurrTick;
            u32PreFrame = frame_count;
        }
        // HAL_Delay(20);
        /* ON_LED(); */
        // HAL_Delay(500/flame_rate);
    }
}
/* USER CODE END 0 */

FIL stm32_fileobject;

    char sPath[30] = "2.avi";
    uint16_t i;
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
    HAL_SD_CardInfoTypeDef CardInfo;
    FATFS *pfs;
    DWORD fre_clust;
    HAL_SD_CardStatusTypeDef pState;
    uint32_t byteswritten, bytesread;                     /* File write/read counts */
    uint8_t wtext[] = "This is STM32 working with FatFs\n"; /* File write buffer */
    uint8_t rtext[100];                                   /* File read buffer */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SDMMC1_SD_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_FATFS_Init();
  MX_LIBJPEG_Init();
  MX_DAC1_Init();
  MX_I2C4_Init();
  /* USER CODE BEGIN 2 */
    MATRIX_Init( 60 );
    MATRIX_SetTextSize(1);
    MATRIX_FillScreen(0x0);
    // MATRIX_disImage(gImage_256_192, 256, 192, 0, 0, RGB_888);

    HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
    HAL_TIM_Base_Start_IT(&htim4);
    __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,MATRIX_getBrightness());

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    if( MSD_OK == BSP_SD_Init() )
    {
        BSP_SD_GetCardInfo(&CardInfo);
        MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "-----------CardInfo-----------\n");
        MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, " - Type:  %7u\n - Ver:   %7u\n - Class: %7u\n - Add:   %7u\n - BNbr:  %7u\n - Bsize: %7u\n - LNbr:  %7u\n - LSize: %7u\n", CardInfo.CardType, CardInfo.CardVersion, CardInfo.Class, CardInfo.RelCardAdd, CardInfo.BlockNbr, CardInfo.BlockSize, CardInfo.LogBlockNbr, CardInfo.LogBlockSize);
        // MATRIX_disImage(gImage_256_192, 256, 192, 0, 0, RGB_888);
        if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK)
        {
            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "MicroSD Mounted! \n");
            /* Check free space */
            f_getfree("", &fre_clust, &pfs);
            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Total Size: \t%luMb\n",(uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5)/1024);
            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Free Space: \t%luMb\n",(uint32_t)(fre_clust * pfs->csize * 0.5)/1024);

            /*##-4- Create and Open a new text file object with write access #####*/
            if(f_open(&stm32_fileobject, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
            {
                /* 'STM32.TXT' file Open for write Error */
                MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Create file error!\n");
            }
            else
            {

                MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xFFFF, "STM32 address: 0x%X!\n", (uint32_t) &stm32_fileobject);
                MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xFFFF, "SDFile address: 0x%X!\n", (uint32_t) &SDFile);

                MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Created file *STM32.TXT*\n");
                /*##-5- Write data to the text file ################################*/
                SDResult = f_write(&stm32_fileobject, wtext, sizeof(wtext), (void *)&byteswritten);

                if((byteswritten == 0) || (SDResult != FR_OK))
                {
                    /* 'STM32.TXT' file Write or EOF Error */
                    MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Write file error!\n");
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Wrote file *STM32.TXT*\n");
                    /*##-6- Close the open text file #################################*/
                    f_close(&stm32_fileobject);

                    /*##-7- Open the text file object with read access ###############*/
                    if(f_open(&stm32_fileobject, "STM32.TXT", FA_READ) != FR_OK)
                    {
                        /* 'STM32.TXT' file Open for read Error */
                        MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Open file error!\n");
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Opened file *STM32.TXT*\n");
                        /*##-8- Read data from the text file ###########################*/
                        SDResult = f_read(&stm32_fileobject, rtext, sizeof(rtext), (UINT*)&bytesread);

                        if((bytesread == 0) || (SDResult != FR_OK)) /* EOF or Error */
                        {
                            /* 'STM32.TXT' file Read or EOF Error */
                            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Read file error!\n");
                        }
                        else
                        {
                            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Read file *STM32.TXT*\n");
                            /*##-9- Close the open text file #############################*/
                            f_close(&stm32_fileobject);

                            /*##-10- Compare read data with the expected data ############*/
                            if ((bytesread != byteswritten))
                            {
                                /* Read data is different from the expected data */
                                MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Data Different!\n");
                            }
                            else
                            {
                                /* Success of the demo: no error occurrence */
                                MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "--> Test Card Sucessfully!\n");
                            }
                        }
                    }
                }
            }

            // HAL_Delay(1000);
            // MEDIA_DisplayJpeg("img/natural.jpg");
            // MATRIX_SetCursor(0, 0);
            // HAL_Delay(1000);
            // MEDIA_DisplayJpeg("img/anime.jpg");
            // MATRIX_SetCursor(0, 0);
            // HAL_Delay(1000);
            // MEDIA_DisplayJpeg("img/phung.jpg");
            // MATRIX_SetCursor(0, 0);
            // HAL_Delay(1000);
            // MEDIA_DisplayJpeg("img/256x192.jpg");
            // MATRIX_SetCursor(0, 0);

            SD_PlayAviVideo();

            while(1)
            {
                // for (i = 0; i < 500; i++)
                // {
                //     u32PreTick = HAL_GetTick();
                //     sprintf(sPath,"vid1/digi%i.jpg", i);
                //     MEDIA_DisplayJpeg(sPath);
                //     u32CurrentDelay = HAL_GetTick();

                //     if( u32CurrentDelay - u32PreTick > 200)
                //     {
                //         u32PreTick = u32CurrentDelay;
                //         fCurrkFps = 500.0/(u32CurrentDelay - u32PreTick);
                //     }
                //     MATRIX_Printf( FONT_DEFAULT, 1U, 5, 10, 0xAAAA, "%i.%ifPs", (uint32_t) fCurrkFps, (uint32_t) ((fCurrkFps - (uint32_t) fCurrkFps) * 10) );
                // }

                // for (i = 0; i < 500; i++)
                // {
                //     u32PreTick = HAL_GetTick();
                //     sprintf(sPath,"vid2/hua%i.jpg", i);
                //     MEDIA_DisplayJpeg(sPath);
                //     u32CurrentDelay = HAL_GetTick();

                //     if( u32CurrentDelay - u32PreTick > 200)
                //     {
                //         u32PreTick = u32CurrentDelay;
                //         fCurrkFps = 500.0/(u32CurrentDelay - u32PreTick);
                //     }
                //     MATRIX_Printf( FONT_DEFAULT, 1U, 5, 10, 0xAAAA, "%i.%ifPs", (uint32_t) fCurrkFps, (uint32_t) ((fCurrkFps - (uint32_t) fCurrkFps) * 10) );
                // }
                // MEDIA_DisplayJpeg("img/natural.jpg");
                // // MATRIX_SetCursor(0, 0);
                // // HAL_Delay(1000);
                // MEDIA_DisplayJpeg("img/anime.jpg");
                // // MATRIX_SetCursor(0, 0);
                // // HAL_Delay(1000);
                // MEDIA_DisplayJpeg("img/phung.jpg");
                // // MATRIX_SetCursor(0, 0);
                // // HAL_Delay(1000);
                // MEDIA_DisplayJpeg("img/256x192.jpg");
                // MATRIX_SetCursor(0, 0);
            }
        }
        else
        {
            // __enable_irq();
            MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "Mount Failed! \n");
        }
        HAL_Delay(2000);
    }
    MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "-----------No SDCard---------\n");
    HAL_Delay(2000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 70;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.SubRegionDisable = 0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x08000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_2MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO_URO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x20000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.BaseAddress = 0x24060000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER5;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER6;
  MPU_InitStruct.BaseAddress = 0x30020000;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER7;
  MPU_InitStruct.BaseAddress = 0x30040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER8;
  MPU_InitStruct.BaseAddress = 0x38000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER9;
  MPU_InitStruct.BaseAddress = 0x38800000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4KB;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER10;
  MPU_InitStruct.BaseAddress = 0x40000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512MB;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL2;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
