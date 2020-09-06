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
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "libjpeg.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "matrix.h"
#include "string.h"
#include "stdio.h"
#include "anime.h"
#include "nature.h"
#include "phung.h"
#include "ff.h"

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

DMA2D_HandleTypeDef hdma2d;

JPEG_HandleTypeDef hjpeg;

RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;

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
__IO uint8_t pagebuff[65536];       //buffer to store frame
extern __IO uint8_t white[];
extern __IO int pixx,pixy;   //pixy=caddr,pixx=raddr
struct jpeg_decompress_struct cinfo;
struct jpeg_error_mgr jerr;

extern __IO uint8_t configured;
uint16_t framesize;
uint16_t frameno=0;
uint16_t movilocation;
uint16_t framelocation;
uint16_t audindex=0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
static void MX_DMA2D_Init(void);
static void MX_JPEG_Init(void);
/* USER CODE BEGIN PFP */
void IRQ_ProcessMonitor( void );
void play_video( void );
void play_image( void );
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
                MATRIX_Printf( FONT_DEFAULT, "Dir: %s\r\n", fno.fname);
                color -= 1024;
                MATRIX_SetTextColor(color);
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                SDResult = scan_files(path);                     /* Enter the directory */
                if (SDResult != FR_OK) break;
                path[i] = 0;
            }
            else
            {                                       /* It is a file. */
                MATRIX_Printf( FONT_DEFAULT, "File: %s/%s\r\n", path, fno.fname);
                color -= 1024;
                MATRIX_SetTextColor(color);
            }
        }
        f_closedir(&dir);
    }
    else
    {
        MATRIX_SetTextColor(0xF800);
        MATRIX_Printf( FONT_DEFAULT, "Error open dir!\n");
        MATRIX_SetTextColor(0xFFFF);
    }
    return SDResult;
}

static void MATRIX_UserApplication( void )
{
    play_image();
    play_video();
    MATRIX_SetTextColor(0xF800);
    MATRIX_FillScreen(0x0);
    MATRIX_SetCursor(0, 5);
    MATRIX_Printf( FONT_DEFAULT, "Stop play dir!\n");
    MATRIX_SetTextColor(0xFFFF);
    HAL_Delay(1000);
    plasma();
    while( 1U );
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

void play_video( void )
{
    uint32_t l_Count = 0UL;
    // char tentimes=0;
    // char fstbufsent;
    // int16_t audsample1[4410];
    // int16_t audsample2[4410];

    // set_Horizental();
    // f_chdir("/movi");
    SDResult=f_opendir(&dir,"/movi");
    jpeg_create_decompress(&cinfo);
    // player_init();
    for(;;)
    {
        SDResult = f_readdir(&dir, &fno);
        if (SDResult != FR_OK || fno.fname[0] == 0)
            break;
        if (fno.fname[0] == '.')
            continue;
        if (fno.fattrib & AM_DIR)
        {
            jpeg_destroy_decompress(&cinfo);
            return;
        }
        SDResult=f_open(&SDFile,fno.fname, FA_OPEN_EXISTING | FA_READ);
        SDResult=f_read(&SDFile,pagebuff,4,(UINT *) &nobytsread);

        while(pagebuff[0]!='m' || pagebuff[1]!='o'|| pagebuff[2]!='v'|| pagebuff[3]!='i')
        {
            gIndex++;
            SDResult=f_lseek(&SDFile,gIndex); // pointer forward
            SDResult=f_read(&SDFile,pagebuff,4,(UINT *) &nobytsread);
        }
        gIndex+=4;
        movilocation=gIndex;       //movi tag location set
        gIndex=fno.fsize;          //point to eof
        while(pagebuff[0]!='i' || pagebuff[1]!='d'|| pagebuff[2]!='x'|| pagebuff[3]!='1')
        {
            gIndex--;
            SDResult=f_lseek(&SDFile,gIndex); // pointer forward
            SDResult=f_read(&SDFile,pagebuff,4,(UINT *) &nobytsread);
        }
nextframe:
        while(pagebuff[0]!='0' || pagebuff[1]!='0'|| pagebuff[2]!='d'|| pagebuff[3]!='c')
        {
            gIndex++;
            SDResult=f_lseek(&SDFile,gIndex); // pointer forward
            SDResult=f_read(&SDFile,pagebuff,4,(UINT *) &nobytsread);
        }
        gIndex+=8;          //4bytes from 00dc---->dwflag +4bytes from dwflag--->dwoffset=frame_loaction
        SDResult=f_lseek(&SDFile,gIndex);
        SDResult=f_read(&SDFile,pagebuff,4,(UINT *) &nobytsread);
        framelocation=movilocation+bufftoint(pagebuff);
        f_lseek(&SDFile,framelocation);//length+data
        SDResult=f_read(&SDFile,pagebuff,4,(UINT *) &nobytsread);
        framesize=bufftoint(pagebuff);
        framelocation+=4;  //only data length removed.
        f_lseek(&SDFile,framelocation);   //data locating finished readingvideo=1;
        SDResult=f_read(&SDFile,pagebuff,framesize,(UINT *) &nobytsread); // readingvideo=0;
        // send_frame(pagebuff,framesize);

        for( uint16_t y = 0U; y < MATRIX_HEIGHT; y++ )
        {
            for( uint16_t x = 0U; x < MATRIX_WIDTH; x++ )
            {
                MATRIX_WritePixel( x, y, pagebuff[l_Count++] );
            }
        }
        l_Count = 0UL;
        f_lseek(&SDFile,gIndex);

        goto nextframe;
    }
    finished:
        jpeg_destroy_decompress(&cinfo);
}

void play_image( void )
{
    uint8_t l_Count = 1U;
    struct jpeg_decompress_struct l_Cinfo;
    struct jpeg_error_mgr l_Jerr;

    SDResult = f_opendir(&dir,"/test");

    for( l_Count = 1U; l_Count < 10; l_Count++ )
    {
        uint32_t l_Data;
        JSAMPARRAY buffer;

        SDResult = f_readdir(&dir, &fno);
        if (SDResult != FR_OK || fno.fname[0] == 0)
            return;
        if (fno.fattrib & AM_DIR)
        {
            return;
        }

        SDResult=f_open(&SDFile,fno.fname, FA_READ);
        l_Cinfo.err = jpeg_std_error(&l_Jerr);
        jpeg_create_decompress(&l_Cinfo);
        jpeg_stdio_src(&l_Cinfo, (JFILE * ) &SDFile);
        (void) jpeg_read_header(&l_Cinfo, TRUE);
        //l_Cinfo.scale_num=1;
        //l_Cinfo.scale_denom=2;
        (void) jpeg_start_decompress(&l_Cinfo);
        uint32_t row_stride = l_Cinfo.output_width * l_Cinfo.output_components;
        buffer = (*l_Cinfo.mem->alloc_sarray)((j_common_ptr) &l_Cinfo, JPOOL_IMAGE, row_stride, 1);

        while(l_Cinfo.output_scanline < l_Cinfo.output_height)
        {
            (void) jpeg_read_scanlines(&l_Cinfo, buffer, 1);
            //do something
            for( int i=0;i<l_Cinfo.output_width;i++)
            {
                l_Data = (buffer[0][i*3]<<16) | (buffer[0][i*3+1]<<8) | buffer[0][i*3+2];
                MATRIX_WritePixel( i , l_Cinfo.output_scanline - 1, l_Data );
            }
        }
        (void) jpeg_finish_decompress(&l_Cinfo);
        jpeg_destroy_decompress(&l_Cinfo);

        retSD = f_close(&SDFile);
        HAL_Delay(5000);
    }
}
/* USER CODE END 0 */

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
  MX_RTC_Init();
  MX_SDMMC1_SD_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_FATFS_Init();
//   MX_DMA2D_Init();
//   MX_JPEG_Init();
//   MX_LIBJPEG_Init();
  /* USER CODE BEGIN 2 */
    MATRIX_Init( 100 );

    HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
    HAL_TIM_Base_Start_IT(&htim4);
    __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,MATRIX_getBrightness());

    MATRIX_SetTextSize(1);
    MATRIX_SetCursor(0, 5);
    MATRIX_SetTextColor(0xFFFF);

    // MATRIX_FillCircle( 128, 31, 30, 0x7E0 );
    MATRIX_disImage(gImage_Anime, 256, 128);
    HAL_Delay(5000);
    // MATRIX_disImage(gImage_Nature, 256, 128);
    // HAL_Delay(4000);
    // MATRIX_FillScreen(0xF81F);
    // HAL_Delay(2000);
    // MATRIX_FillScreen(0x0);
    // plasma();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if 0
    if( MSD_ERROR != BSP_SD_Init() )
    {
        MATRIX_SetTextColor(0xF81F);
        MATRIX_FillScreen(0x0);
        MATRIX_SetCursor(0, 5);
        BSP_SD_GetCardInfo(&CardInfo);
        MATRIX_Printf( FONT_DEFAULT, "-----------CardInfo-----------\n");
        MATRIX_Printf( FONT_DEFAULT, " - Type:  %7u\n - Ver:   %7u\n - Class: %7u\n - Add:   %7u\n - BNbr:  %7u\n - Bsize: %7u\n - LNbr:  %7u\n - LSize: %7u\n", CardInfo.CardType, CardInfo.CardVersion, CardInfo.Class, CardInfo.RelCardAdd, CardInfo.BlockNbr, CardInfo.BlockSize, CardInfo.LogBlockNbr, CardInfo.LogBlockSize);
        MATRIX_SetTextColor(0x07E0);
        HAL_Delay(2000);
        // MATRIX_FillScreen(0x0);
        // MATRIX_SetCursor(0, 5);
        if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK)
        {
            MATRIX_Printf( FONT_DEFAULT, "MicroSD Mounted! \n");
            /* Check free space */
            f_getfree("", &fre_clust, &pfs);
            MATRIX_SetTextColor(0x1F1F);
            MATRIX_Printf( FONT_DEFAULT, "Total Size: \t%luMb\n",(uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5)/1024);
            MATRIX_SetTextColor(0xF1F1);
            MATRIX_Printf( FONT_DEFAULT, "Free Space: \t%luMb\n",(uint32_t)(fre_clust * pfs->csize * 0.5)/1024);
            MATRIX_SetTextColor(0xFFFF);
            HAL_Delay(1000);
                MATRIX_SetTextColor(0x1F1F);
                MATRIX_FillScreen(0x0);
                MATRIX_SetCursor(0, 5);
                scan_files(SDPath);

                    HAL_Delay(2000);
                    // MATRIX_SetCursor(0, 5);
                    // MATRIX_FillScreen(0x0);
                    // MATRIX_SetTextColor(0x7FF);
                    // MATRIX_Printf( FONT_DEFAULT, "---------Test RWX Files---------\n");
                    // MATRIX_SetTextColor(0xFFFF);
                    // /*##-4- Create and Open a new text file object with write access #####*/
                    // if(f_open(&SDFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
                    // {
                    //     /* 'STM32.TXT' file Open for write Error */
                    //     MATRIX_Printf( FONT_DEFAULT, "Create file error!\n");
                    // }
                    // else
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, "Created file *STM32.TXT*\n");
                    //     /*##-5- Write data to the text file ################################*/
                    //     SDResult = f_write(&SDFile, wtext, sizeof(wtext), (void *)&byteswritten);

                    //     if((byteswritten == 0) || (SDResult != FR_OK))
                    //     {
                    //         /* 'STM32.TXT' file Write or EOF Error */
                    //         MATRIX_Printf( FONT_DEFAULT, "Write file error!\n");
                    //     }
                    //     else
                    //     {
                    //         MATRIX_Printf( FONT_DEFAULT, "Wrote file *STM32.TXT*\n");
                    //         /*##-6- Close the open text file #################################*/
                    //         f_close(&SDFile);

                    //         /*##-7- Open the text file object with read access ###############*/
                    //         if(f_open(&SDFile, "STM32.TXT", FA_READ) != FR_OK)
                    //         {
                    //             /* 'STM32.TXT' file Open for read Error */
                    //             MATRIX_Printf( FONT_DEFAULT, "Open file error!\n");
                    //         }
                    //         else
                    //         {
                    //             MATRIX_Printf( FONT_DEFAULT, "Opened file *STM32.TXT*\n");
                    //             /*##-8- Read data from the text file ###########################*/
                    //             SDResult = f_read(&SDFile, rtext, sizeof(rtext), (UINT*)&bytesread);

                    //             if((bytesread == 0) || (SDResult != FR_OK)) /* EOF or Error */
                    //             {
                    //                 /* 'STM32.TXT' file Read or EOF Error */
                    //                 MATRIX_Printf( FONT_DEFAULT, "Read file error!\n");
                    //             }
                    //             else
                    //             {
                    //                 MATRIX_Printf( FONT_DEFAULT, "Read file *STM32.TXT*\n");
                    //                 /*##-9- Close the open text file #############################*/
                    //                 f_close(&SDFile);

                    //                 /*##-10- Compare read data with the expected data ############*/
                    //                 if ((bytesread != byteswritten))
                    //                 {
                    //                     /* Read data is different from the expected data */
                    //                     MATRIX_Printf( FONT_DEFAULT, "Data Different!\n");
                    //                 }
                    //                 else
                    //                 {
                    //                     /* Success of the demo: no error occurrence */
                    //                     MATRIX_SetTextColor(0x07E0);
                    //                     MATRIX_Printf( FONT_DEFAULT, "--> Test Card Sucessfully!\n");
                    //                     MATRIX_SetTextColor(0x1F);
                    //                 }
                    //             }
                    //         }
                    //     }
                    // }
                HAL_SD_GetCardStatus(&hsd1, &pState);
                MATRIX_Printf( FONT_DEFAULT, "State: %i, Code: %i\n",hsd1.State, hsd1.ErrorCode);
                MATRIX_SetTextColor(0xF81F);
                MATRIX_Printf( FONT_DEFAULT, "-> Execute user application in 1s!\n");
                HAL_Delay(2000);
                MATRIX_FillScreen(0x0);
                MATRIX_SetCursor(0, 0);
                MATRIX_SetTextColor(0x7FF);

                if( f_opendir(&dir,"/test") == FR_OK )
                {
                    SDResult = f_readdir(&dir, &fno);
                    if( FR_OK == f_open(&SDFile,fno.fname, FA_READ))
                    {
                        struct jpeg_decompress_struct _Cinfo;
                        struct jpeg_error_mgr _Jerr;

                        MATRIX_Printf( FONT_DEFAULT, "-> Opened %s! \n", fno.fname);

                        _Cinfo.err = jpeg_std_error(&_Jerr);
                        MATRIX_Printf( FONT_DEFAULT, "-> jpeg_std_error done! \n");
                        HAL_Delay(1000);
                        jpeg_create_decompress(&_Cinfo);
                        MATRIX_Printf( FONT_DEFAULT, "-> jpeg_create_decompress done! \n");
                        HAL_Delay(1000);
                        jpeg_stdio_src(&_Cinfo, (FILE * ) &SDFile);
                        MATRIX_Printf( FONT_DEFAULT, "-> jpeg_stdio_src done! \n");
                        HAL_Delay(1000);
                        (void) jpeg_read_header(&_Cinfo, TRUE);
                        // MATRIX_Printf( FONT_DEFAULT, "-> jpeg_read_header done! \n");
                        // (void) jpeg_start_decompress(&_Cinfo);
                        // MATRIX_Printf( FONT_DEFAULT, "-> jpeg_start_decompress done! \n");
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, "-> Open %s error! \n", fno.fname);
                    }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, "-> Open /test error! \n");
                }
                while(1);
                MATRIX_UserApplication();
        }
    }
    HAL_Delay(1000);
#endif
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 240;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 20;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_SDMMC;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enables the Clock Security System
  */
  HAL_RCCEx_EnableLSECSS();
}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_CSS_420;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief JPEG Initialization Function
  * @param None
  * @retval None
  */
static void MX_JPEG_Init(void)
{

  /* USER CODE BEGIN JPEG_Init 0 */

  /* USER CODE END JPEG_Init 0 */

  /* USER CODE BEGIN JPEG_Init 1 */

  /* USER CODE END JPEG_Init 1 */
  hjpeg.Instance = JPEG;
  if (HAL_JPEG_Init(&hjpeg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN JPEG_Init 2 */

  /* USER CODE END JPEG_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 8;
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 60;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LA_Pin|LB_Pin|LC_Pin|LD_Pin
                          |LE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LAT_Pin|CLK_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, B4_Pin|R4_Pin|G4_Pin|B2_Pin
                          |R2_Pin|R3_Pin|G3_Pin|B3_Pin
                          |R1_Pin|G1_Pin|B1_Pin|G2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LA_Pin LC_Pin LD_Pin LE_Pin */
  GPIO_InitStruct.Pin = LA_Pin|LC_Pin|LD_Pin|LE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LB_Pin */
  GPIO_InitStruct.Pin = LB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LB_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LAT_Pin CLK_Pin */
  GPIO_InitStruct.Pin = LAT_Pin|CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : B4_Pin R4_Pin G4_Pin B2_Pin
                           R2_Pin R3_Pin G3_Pin B3_Pin
                           R1_Pin G1_Pin B1_Pin G2_Pin */
  GPIO_InitStruct.Pin = B4_Pin|R4_Pin|G4_Pin|B2_Pin
                          |R2_Pin|R3_Pin|G3_Pin|B3_Pin
                          |R1_Pin|G1_Pin|B1_Pin|G2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
