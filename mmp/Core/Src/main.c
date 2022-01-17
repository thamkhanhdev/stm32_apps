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
#include "dma2d.h"
#include "fatfs.h"
#include "i2c.h"
#include "jpeg.h"
#include "libjpeg.h"
#include "mdma.h"
#include "sdmmc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "matrix.h"
#include "string.h"
#include "stdio.h"
#include <setjmp.h>
#include "gamma.h"
#include "AVI_parser.h"
#include "decode_dma.h"
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
#define MJPEG_VID_BUFFER_SIZE ((uint32_t)(1024 *96))
#define MJPEG_AUD_BUFFER_SIZE ((uint32_t)(1024 *16))


FRESULT SDResult;
FILINFO fno;
DIR dir;


static JPEG_ConfTypeDef       JPEG_Info;

uint32_t LCD_X_Size = 256, LCD_Y_Size = 192;
static uint32_t FrameRate;

extern __IO uint32_t Jpeg_HWDecodingEnd;

extern FATFS SDFatFS;  /* File system object for SD card logical drive */
extern char SDPath[4]; /* SD card logical drive path */

FIL MJPEG_File;          /* MJPEG File object */
AVI_CONTEXT AVI_Handel;  /* AVI Parser Handle*/

uint8_t MJPEG_VideoBuffer[MJPEG_VID_BUFFER_SIZE] __attribute__((section (".ram_d1_cacheable")));
uint8_t MJPEG_AudioBuffer[MJPEG_AUD_BUFFER_SIZE] __attribute__((section (".ram_d1_cacheable")));



/*
signed short Audio_Buffer[2][MAX_AUDIO_SAMPLE_RATE * MAX_AUDIO_CHANNEL / MIN_VIDEO_FLAME_RATE]  = {0};

unsigned char Flame_Buffer[MATRIXLED_Y_COUNT][MATRIXLED_X_COUNT][MATRIXLED_COLOR_COUNT] __attribute__((section (".ram_d1_cacheable")));
 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
void IRQ_ProcessMonitor( void );
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if( htim->Instance == TIM4 )
    {
        IRQ_ProcessMonitor();
    }
} */

/**
  * @brief  Init the DMA2D for YCbCr to ARGB8888 Conversion .
  * @param  xsize: Image width
  * @param  ysize: Image Height
  * @param  ChromaSampling: YCbCr CHroma sampling : 4:2:0, 4:2:2 or 4:4:4
  * @retval None
  */
static void DMA2D_Init(uint16_t xsize, uint16_t ysize, uint32_t ChromaSampling)
{
  uint32_t cssMode = JPEG_420_SUBSAMPLING, inputLineOffset = 0;

  if(ChromaSampling == JPEG_420_SUBSAMPLING)
  {
    cssMode = DMA2D_CSS_420;

    inputLineOffset = xsize % 16;
    if(inputLineOffset != 0)
    {
      inputLineOffset = 16 - inputLineOffset;
    }
  }
  else if(ChromaSampling == JPEG_444_SUBSAMPLING)
  {
    cssMode = DMA2D_NO_CSS;

    inputLineOffset = xsize % 8;
    if(inputLineOffset != 0)
    {
      inputLineOffset = 8 - inputLineOffset;
    }
  }
  else if(ChromaSampling == JPEG_422_SUBSAMPLING)
  {
    cssMode = DMA2D_CSS_422;

    inputLineOffset = xsize % 16;
    if(inputLineOffset != 0)
    {
      inputLineOffset = 16 - inputLineOffset;
    }
  }

  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/
  hdma2d.Init.Mode         = DMA2D_M2M_PFC;
  hdma2d.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = LCD_X_Size - xsize;
  hdma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* No Output Alpha Inversion*/
  hdma2d.Init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* No Output Red & Blue swap */

  /*##-2- DMA2D Callbacks Configuration ######################################*/
  hdma2d.XferCpltCallback  = NULL;

  /*##-3- Foreground Configuration ###########################################*/
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0xFF;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR;
  hdma2d.LayerCfg[1].ChromaSubSampling = cssMode;
  hdma2d.LayerCfg[1].InputOffset = inputLineOffset;
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */

  hdma2d.Instance          = DMA2D;

  /*##-4- DMA2D Initialization     ###########################################*/
  HAL_DMA2D_Init(&hdma2d);
  HAL_DMA2D_ConfigLayer(&hdma2d, 1);
}

/**
  * @brief  Copy the Decoded image to the display Frame buffer.
  * @param  pSrc: Pointer to source buffer
  * @param  pDst: Pointer to destination buffer
  * @param  ImageWidth: image width
  * @param  ImageHeight: image Height
  * @retval None
  */
static void DMA2D_CopyBuffer(uint32_t *pSrc, uint32_t *pDst, uint16_t ImageWidth, uint16_t ImageHeight)
{

  uint32_t xPos, yPos, destination;

  /*##-1- calculate the destination transfer address  ############*/
  xPos = (LCD_X_Size - JPEG_Info.ImageWidth)/2;
  yPos = (LCD_Y_Size - JPEG_Info.ImageHeight)/2;

  destination = (uint32_t)pDst + ((yPos * LCD_X_Size) + xPos) * 4;

  HAL_DMA2D_PollForTransfer(&hdma2d, 25);  /* wait for the previous DMA2D transfer to ends */
  /* copy the new decoded frame to the LCD Frame buffer*/
  HAL_DMA2D_Start(&hdma2d, (uint32_t)pSrc, destination, ImageWidth, ImageHeight);

}

void IRQ_DAC_ProcessAudio(void)
{
}

uint8_t MEDIA_DisplayJpeg(char * pPath)
{
    uint8_t u8ReturnError = 0x0U;
    uint8_t r, g, b;
    uint16_t u16Color;
    uint16_t u16Index;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    if( FR_OK==f_open(&SDFile, (TCHAR*) pPath, FA_READ) )
    {
        /* Step 1: allocate and initialize JPEG decompression object */
        /* We set up the normal JPEG error routines, then override error_exit. */
        /* cinfo.err = jpeg_std_error(&jerr.pub); */
        /* jerr.pub.error_exit = my_error_exit; */

        /* Establish the setjmp return context for my_error_exit to use. */
#if 0
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
#endif
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
           /*  nRowBuff[0] = (unsigned char *) pagebuff; */
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
#if 0
                (void) jpeg_read_scanlines(&cinfo, nRowBuff, 1);
#endif
                // MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "step 1: %i\n",cinfo.output_height);
                for( u16Index = 0U; u16Index < MATRIX_WIDTH; u16Index++ )
                {
                    /* r = (uint8_t) pagebuff[ u16Index * 3 + 0 ];
                    g = (uint8_t) pagebuff[ u16Index * 3 + 1 ];
                    b = (uint8_t) pagebuff[ u16Index * 3 + 2 ]; */

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

void SD_PlayAviVideo(void)
{
    uint32_t isfirstFrame =0 , startTime = 0;
    uint32_t file_error = 0, sd_detection_error = 0;
    uint32_t jpegOutDataAdreess = JPEG_OUTPUT_DATA_BUFFER0;
    uint32_t FrameType = 0;

    /*##-6- Open the MJPEG avi file with read access #######################*/
    if(f_open(&MJPEG_File, "video.avi", FA_READ) == FR_OK)
    {
        isfirstFrame = 1;
        FrameRate = 0;
        /* parse the AVI file Header*/
        if(AVI_ParserInit(&AVI_Handel, &MJPEG_File, MJPEG_VideoBuffer, MJPEG_VID_BUFFER_SIZE, MJPEG_AudioBuffer, MJPEG_AUD_BUFFER_SIZE) != 0)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Open file error");
            HAL_Delay(1000);
            while(1U);
        }
        startTime = HAL_GetTick();

        MATRIX_Printf( FONT_DEFAULT, 1U, 0xFFFF, 0xFFFF, 0xFFFF, "Starting...");
        HAL_Delay(1000);

        do
        {
            FrameType = AVI_GetFrame(&AVI_Handel, &MJPEG_File);

            if(FrameType == AVI_VIDEO_FRAME)
            {
                AVI_Handel.CurrentImage ++;

                /*##-7- Start decoding the current JPEG frame with DMA (Not Blocking ) Method ################*/
                JPEG_Decode_DMA(&hjpeg,(uint32_t) MJPEG_VideoBuffer ,AVI_Handel.FrameSize, jpegOutDataAdreess );

                /*##-8- Wait till end of JPEG decoding ###########################*/
                while(Jpeg_HWDecodingEnd == 0)
                {
                }

                if(isfirstFrame == 1)
                {
                    isfirstFrame = 0;
                    /*##-9- Get JPEG Info  #########################################*/
                    HAL_JPEG_GetInfo(&hjpeg, &JPEG_Info);

                    /*##-10- Initialize the DMA2D ##################################*/
                    DMA2D_Init(JPEG_Info.ImageWidth, JPEG_Info.ImageHeight, JPEG_Info.ChromaSubsampling);
                }
                /*##-11- Copy the Decoded frame to the display frame buffer using the DMA2D #*/
                DMA2D_CopyBuffer((uint32_t *)jpegOutDataAdreess, (uint32_t *)LCD_FRAME_BUFFER, JPEG_Info.ImageWidth, JPEG_Info.ImageHeight);

                jpegOutDataAdreess = (jpegOutDataAdreess == JPEG_OUTPUT_DATA_BUFFER0) ? JPEG_OUTPUT_DATA_BUFFER1 : JPEG_OUTPUT_DATA_BUFFER0;

    #ifdef USE_FRAMERATE_REGULATION
                /* Regulate the frame rate to the video native frame rate by inserting delays */
                FrameRate =  (HAL_GetTick() - startTime) + 1;
                if(FrameRate < ((AVI_Handel.aviInfo.SecPerFrame/1000) * AVI_Handel.CurrentImage))
                {
                    HAL_Delay(((AVI_Handel.aviInfo.SecPerFrame /1000) * AVI_Handel.CurrentImage) - FrameRate);
                }
    #endif /* USE_FRAMERATE_REGULATION */
            }
        } while(AVI_Handel.CurrentImage  <  AVI_Handel.aviInfo.TotalFrame);

        HAL_DMA2D_PollForTransfer(&hdma2d, 50);  /* wait for the Last DMA2D transfer to ends */

        if(AVI_Handel.CurrentImage > 0)
        {
            /*##-12- Calc the average decode frame rate #*/
            FrameRate =  (AVI_Handel.CurrentImage * 1000)/(HAL_GetTick() - startTime);
            /* Display decoding info */
            /* LCD_BriefDisplay(); */
        }
        /*Close the avi file*/
        f_close(&MJPEG_File);
    }
    else /* Can't Open avi file*/
    {
        file_error = 1;
    }

    // uint16_t u16Color = 0U;
    // uint16_t u16VideoHeight = 0U;
    // uint16_t u16VideoHeightDiv2 = u16VideoHeight >> 1;
    // uint16_t u16VideoWidth = 0U;
    // uint16_t u16VideoWidthDiv2 = u16VideoWidth >> 1;
    // uint16_t u16xTmp;
    // uint16_t u16xTmpCalibratePos;
    // uint16_t u16yTmp14;
    // uint16_t u16yTmp14CalibratePos;
    // uint16_t u16yTmp23;
    // uint16_t u16yTmp23CalibratePos;
    // uint16_t u16xSecondFrame;
    // uint16_t u16ySecondFrame;
    // uint16_t u8ErrorPos;
    // uint16_t u8HeightStart = (MATRIX_HEIGHT - u16VideoHeight) >> 1;
    // uint16_t u8WidthStart = (MATRIX_WIDTH - u16VideoWidth) >> 1;
    // MATRIX_Printf( FONT_DEFAULT, 1, 0x0, 0x0, 0xF81F, "Starting... W: %d, H: %d", u16VideoWidth, u16VideoHeight );
    // HAL_Delay(500);

//         u16VideoHeight = 0U;
//         u16VideoHeightDiv2 = u16VideoHeight >> 1;
//         u16VideoWidth = 0U;
//         u16VideoWidthDiv2 = u16VideoWidth >> 1;
//         u8HeightStart = (MATRIX_HEIGHT - u16VideoHeight) >> 1;
//         u8WidthStart = (MATRIX_WIDTH - u16VideoWidth) >> 1;

//         MATRIX_Printf( FONT_DEFAULT, 1, 0x0, 0x0, 0xF81F, "Playing next video...");
//         HAL_Delay(1000);
//         MATRIX_FillScreen(0x0);
//         MATRIX_SetCursor(0, 0);

//         for( uint16_t u8xIdx = 0U; u8xIdx < u16VideoWidthDiv2; u8xIdx++ )
//         {
//             u16xTmpCalibratePos = u16VideoWidthDiv2 + u8xIdx;
//             u16xSecondFrame = u8xIdx+u16VideoWidthDiv2;

//             for( uint16_t u8yIdx = 0U; u8yIdx < u16VideoHeightDiv2; u8yIdx++)
//             {
//                 u16yTmp14 = u8HeightStart + u16VideoHeight - u8yIdx - 1;
//                 u16yTmp23 = u8HeightStart + u16VideoHeightDiv2 - u8yIdx - 1;
//                 u16ySecondFrame = u8yIdx+u16VideoHeightDiv2;
// #if 0
//                 /* For part 1 */
//                 u16Color = Flame_Buffer[u8yIdx][u8xIdx][1]<<8U | Flame_Buffer[u8yIdx][u8xIdx][0];
//                 MATRIX_WritePixel(u8xIdx, u16yTmp14, u16Color );
//                 /* For part 2 */
//                 u16Color = Flame_Buffer[u16ySecondFrame][u8xIdx][1]<<8U | Flame_Buffer[u16ySecondFrame][u8xIdx][0];
//                 MATRIX_WritePixel(u8xIdx, u16yTmp23, u16Color );
//                 /* For part 3 */
//                 u16Color = Flame_Buffer[u16ySecondFrame][u16xSecondFrame][1]<<8U | Flame_Buffer[u16ySecondFrame][u16xSecondFrame][0];
//                 MATRIX_WritePixel(u16xTmpCalibratePos, u16yTmp23, u16Color );
//                 /* For part 4 */
//                 u16Color = Flame_Buffer[u8yIdx][u16xSecondFrame][1]<<8U | Flame_Buffer[u8yIdx][u16xSecondFrame][0];
//                 MATRIX_WritePixel(u16xTmpCalibratePos, u16yTmp14, u16Color );
// #endif
//             }
//         }
/*
        MATRIX_Printf( FONT_DEFAULT, 1U, 85, 10, 0xFFFF, "%i.%ifPs", (uint32_t) fCurrkFps, (uint32_t) ((fCurrkFps - (uint32_t) fCurrkFps) * 10) );
 */
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
    FIL stm32_fileobject;
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
  MX_TIM6_Init();
  MX_DMA2D_Init();
  MX_MDMA_Init();
  MX_JPEG_Init();
  /* USER CODE BEGIN 2 */
    MATRIX_Init( 70 );
    MATRIX_SetTextSize(1);
    MATRIX_FillScreen(0x0);
    // MATRIX_disImage(gImage_256_192, 256, 192, 0, 0, RGB_888);

    /* HAL_TIM_Base_Start_IT(&htim6); */
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
        if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 0) == FR_OK)
        {
            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "MicroSD Mounted! \n");
            /* Check free space */
            f_getfree("", &fre_clust, &pfs);
            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Total Size: \t%luMb\n",(uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5)/1024);
            MATRIX_Printf( FONT_DEFAULT, 1, 0xFFFF, 0xFFFF, 0xF81F, "Free Space: \t%luMb\n",(uint32_t)(fre_clust * pfs->csize * 0.5)/1024);

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
            }
        }
        else
        {
            // __enable_irq();
            MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "Mount Failed! \n");
        }
        HAL_Delay(2000);
    }
    else
    {
      MATRIX_Printf( FONT_DEFAULT, 1, 0, 5, 0xF81F, "-----------No SDCard---------\n");
      HAL_Delay(2000);
    }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.BaseAddress = 0x24060000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER5;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER6;
  MPU_InitStruct.BaseAddress = 0x30020000;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER7;
  MPU_InitStruct.BaseAddress = 0x30040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER8;
  MPU_InitStruct.BaseAddress = 0x38000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;

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
