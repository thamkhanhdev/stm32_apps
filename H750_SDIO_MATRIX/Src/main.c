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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "matrix.h"
#include "string.h"
#include "stdio.h"
#include "anime.h"
#include "nature.h"
#include "phung.h"

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
/* FRESULT SDResult;
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
uint16_t audindex=0; */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */
void play_video( void );
void play_image( void );
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#if 0
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
#endif

static void MATRIX_UserApplication( void )
{
    play_image();
    play_video();
    MATRIX_SetTextColor(0xF800);
    MATRIX_FillScreen(0x0);
    MATRIX_SetCursor(0, 5);
    MATRIX_Printf( FONT_DEFAULT, "Stop play dir!\n");
    MATRIX_SetTextColor(0xFFFF);
    LL_mDelay(1000);
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
    #if 0

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
#endif
}

void play_image( void )
{
#if 0
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
        LL_mDelay(5000);
    }
#endif
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

   /*  HAL_SD_CardInfoTypeDef CardInfo;
    FATFS *pfs;
    DWORD fre_clust;
    HAL_SD_CardStatusTypeDef pState; */
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

  LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);

  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* System interrupt init*/

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
    MATRIX_Init( 90 );

    LL_TIM_CC_EnableChannel(TIM_OE, LL_TIM_CHANNEL_CH2);
    LL_TIM_OC_SetCompareCH2(TIM_OE, MATRIX_getBrightness() );
    LL_TIM_EnableCounter(TIM_OE);

    LL_TIM_EnableIT_UPDATE(TIM_DAT);
    LL_TIM_EnableCounter(TIM_DAT);

    // MATRIX_SetTextSize(1);
    // MATRIX_SetCursor(0, 5);
    // MATRIX_SetTextColor(0xFFFF);

    // MATRIX_FillCircle( 128, 31, 30, 0x7E0 );
    // MATRIX_disImage(gImage_Anime, 256, 128);
    // LL_mDelay(5000);
    // MATRIX_disImage(gImage_Nature, 256, 128);
    // LL_mDelay(4000);
    // MATRIX_FillScreen(0xF81F);
    // LL_mDelay(2000);
    // MATRIX_FillScreen(0x0);
    // plasma();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    MATRIX_FillScreen(0xF81F);
    MATRIX_SetTextColor(0xDDDD);
    MATRIX_SetCursor(15, 5);
    MATRIX_Printf( FONT_DEFAULT, "---CardInfo---\n");
    // MATRIX_SetTextColor(0xDDD);
    // MATRIX_Printf( FONT_DEFAULT, "---CardInfo---\n");
    // MATRIX_Printf( FONT_DEFAULT, "---CardInfo---\n");
    // MATRIX_Printf( FONT_DEFAULT, "---CardInfo---\n");
    // MATRIX_Printf( FONT_DEFAULT, "---CardInfo---\n");
    // MATRIX_Printf( FONT_DEFAULT, "---CardInfo---\n");
    // MATRIX_Printf( FONT_DEFAULT, "---CardInfo---\n");
    /* LL_mDelay(2000); */

  while (1)
  {
    plasma();
    // if( MSD_ERROR != BSP_SD_Init() )
    // {
    //     MATRIX_SetTextColor(0xF81F);
    //     MATRIX_FillScreen(0x0);
    //     MATRIX_SetCursor(0, 5);
    //     BSP_SD_GetCardInfo(&CardInfo);
    //     MATRIX_Printf( FONT_DEFAULT, "-----------CardInfo-----------\n");
    //     MATRIX_Printf( FONT_DEFAULT, " - Type:  %7u\n - Ver:   %7u\n - Class: %7u\n - Add:   %7u\n - BNbr:  %7u\n - Bsize: %7u\n - LNbr:  %7u\n - LSize: %7u\n", CardInfo.CardType, CardInfo.CardVersion, CardInfo.Class, CardInfo.RelCardAdd, CardInfo.BlockNbr, CardInfo.BlockSize, CardInfo.LogBlockNbr, CardInfo.LogBlockSize);
    //     MATRIX_SetTextColor(0x07E0);
    //     LL_mDelay(2000);
    //     // MATRIX_FillScreen(0x0);
    //     // MATRIX_SetCursor(0, 5);
    //     if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK)
    //     {
    //         MATRIX_Printf( FONT_DEFAULT, "MicroSD Mounted! \n");
    //         /* Check free space */
    //         f_getfree("", &fre_clust, &pfs);
    //         MATRIX_SetTextColor(0x1F1F);
    //         MATRIX_Printf( FONT_DEFAULT, "Total Size: \t%luMb\n",(uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5)/1024);
    //         MATRIX_SetTextColor(0xF1F1);
    //         MATRIX_Printf( FONT_DEFAULT, "Free Space: \t%luMb\n",(uint32_t)(fre_clust * pfs->csize * 0.5)/1024);
    //         MATRIX_SetTextColor(0xFFFF);
    //         LL_mDelay(1000);
    //             MATRIX_SetTextColor(0x1F1F);
    //             MATRIX_FillScreen(0x0);
    //             MATRIX_SetCursor(0, 5);
    //             scan_files(SDPath);

    //                 LL_mDelay(2000);
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
    //             HAL_SD_GetCardStatus(&hsd1, &pState);
    //             MATRIX_Printf( FONT_DEFAULT, "State: %i, Code: %i\n",hsd1.State, hsd1.ErrorCode);
    //             MATRIX_SetTextColor(0xF81F);
    //             MATRIX_Printf( FONT_DEFAULT, "-> Execute user application in 1s!\n");
    //             LL_mDelay(2000);
    //             MATRIX_FillScreen(0x0);
    //             MATRIX_SetCursor(0, 0);
    //             MATRIX_SetTextColor(0x7FF);

    //             if( f_opendir(&dir,"/test") == FR_OK )
    //             {
    //                 SDResult = f_readdir(&dir, &fno);
    //                 if( FR_OK == f_open(&SDFile,fno.fname, FA_READ))
    //                 {
    //                     struct jpeg_decompress_struct _Cinfo;
    //                     struct jpeg_error_mgr _Jerr;

    //                     MATRIX_Printf( FONT_DEFAULT, "-> Opened %s! \n", fno.fname);

    //                     _Cinfo.err = jpeg_std_error(&_Jerr);
    //                     MATRIX_Printf( FONT_DEFAULT, "-> jpeg_std_error done! \n");
    //                     LL_mDelay(1000);
    //                     jpeg_create_decompress(&_Cinfo);
    //                     MATRIX_Printf( FONT_DEFAULT, "-> jpeg_create_decompress done! \n");
    //                     LL_mDelay(1000);
    //                     jpeg_stdio_src(&_Cinfo, (FILE * ) &SDFile);
    //                     MATRIX_Printf( FONT_DEFAULT, "-> jpeg_stdio_src done! \n");
    //                     LL_mDelay(1000);
    //                     (void) jpeg_read_header(&_Cinfo, TRUE);
    //                     // MATRIX_Printf( FONT_DEFAULT, "-> jpeg_read_header done! \n");
    //                     // (void) jpeg_start_decompress(&_Cinfo);
    //                     // MATRIX_Printf( FONT_DEFAULT, "-> jpeg_start_decompress done! \n");
    //                 }
    //                 else
    //                 {
    //                     MATRIX_Printf( FONT_DEFAULT, "-> Open %s error! \n", fno.fname);
    //                 }
    //             }
    //             else
    //             {
    //                 MATRIX_Printf( FONT_DEFAULT, "-> Open /test error! \n");
    //             }
    //             while(1);
    //             MATRIX_UserApplication();
    //     }
    // }
    // LL_mDelay(1000);
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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_1)
  {
  }
  LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE3);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSE);
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_4_8);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(5);
  LL_RCC_PLL1_SetN(48);
  LL_RCC_PLL1_SetP(2);
  LL_RCC_PLL1_SetQ(20);
  LL_RCC_PLL1_SetR(2);
  LL_RCC_PLL1_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL1_IsReady() != 1)
  {
  }

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
  LL_RCC_SetSysPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_2);
  LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_2);

  LL_Init1msTick(120000000);

  LL_SetSystemCoreClock(120000000);
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

  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  TIM_InitStruct.Prescaler = 0;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 100;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM2, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM2);
  LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH2);
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = 0;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_LOW;
  LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
  LL_TIM_OC_DisableFast(TIM2, LL_TIM_CHANNEL_CH2);
  LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM2);
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
  /**TIM2 GPIO Configuration
  PA1   ------> TIM2_CH2
  */
  GPIO_InitStruct.Pin = OE_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(OE_GPIO_Port, &GPIO_InitStruct);

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

  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

  /* TIM4 interrupt Init */
  NVIC_SetPriority(TIM4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(TIM4_IRQn);

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  TIM_InitStruct.Prescaler = 0;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 60;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM4, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM4);
  LL_TIM_SetClockSource(TIM4, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_SetTriggerOutput(TIM4, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM4);
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
  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_ResetOutputPin(GPIOC, LA_Pin|LB_Pin|LC_Pin|LD_Pin
                          |LE_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOB, B4_Pin|R4_Pin|G4_Pin|B2_Pin
                          |R2_Pin|R3_Pin|G3_Pin|B3_Pin
                          |R1_Pin|G1_Pin|B1_Pin|G2_Pin);

  /**/
  LL_GPIO_SetOutputPin(GPIOA, LAT_Pin|CLK_Pin);

  /**/
  GPIO_InitStruct.Pin = LA_Pin|LC_Pin|LD_Pin|LE_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LB_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LB_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LAT_Pin|CLK_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = B4_Pin|R4_Pin|G4_Pin|B2_Pin
                          |R2_Pin|R3_Pin|G3_Pin|B3_Pin
                          |R1_Pin|G1_Pin|B1_Pin|G2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE9);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE10);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_9;
  EXTI_InitStruct.Line_32_63 = LL_EXTI_LINE_NONE;
  EXTI_InitStruct.Line_64_95 = LL_EXTI_LINE_NONE;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_10;
  EXTI_InitStruct.Line_32_63 = LL_EXTI_LINE_NONE;
  EXTI_InitStruct.Line_64_95 = LL_EXTI_LINE_NONE;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);

  /**/
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_10, LL_GPIO_PULL_UP);

  /* EXTI interrupt init*/
  NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(EXTI9_5_IRQn);
  NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(EXTI15_10_IRQn);

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
