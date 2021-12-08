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
#include "string.h"
#include "stdio.h"
#include "anime.h"
#include "nature.h"
#include "neu.h"
#include "pic1.h"
#include "phung.h"
#include "256_192.h"
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
__attribute__((aligned(128))) unsigned char pagebuff[0x1000];       //buffer to store frame
extern __IO uint8_t white[];
extern __IO int pixx,pixy;   //pixy=caddr,pixx=raddr
// struct jpeg_decompress_struct cinfo;
// struct jpeg_error_mgr jerr;

extern __IO uint8_t configured;
uint16_t framesize;
uint16_t frameno=0;
uint16_t movilocation;
uint16_t framelocation;
uint16_t audindex=0;


extern JSAMPLE * image_buffer;	/* Points to large array of R,G,B-order data */
extern int image_height;	/* Number of rows in image */
extern int image_width;		/* Number of columns in image */
struct jpeg_decompress_struct cinfo;

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

struct my_error_mgr jerr;
int row_stride; /* physical row width in output buffer */
JSAMPROW nRowBuff[1]; /* Output row buffer */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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

/*
 * Here's the routine that will replace the standard error_exit method:
 */

void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

    MATRIX_Printf( FONT_DEFAULT, "my_error_exit!\n");
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

void MEDIA_DisplayJpeg(char * pPath)
{
    FILINFO pListFileInfo;
    uint8_t r, g, b;
    uint16_t u16Color;
    uint16_t u16Index;

    if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1U) == FR_OK)
    {
        // MATRIX_Printf( FONT_DEFAULT, "SdCard Mounted!\n");
        // HAL_Delay(1000);
        // MATRIX_FillScreen(0x0);

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
                MATRIX_Printf( FONT_DEFAULT, "Destroy Compression!\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                f_close(&SDFile);
            }
            else
            {
                /* Now we can initialize the JPEG decompression object. */
                // MATRIX_Printf( FONT_DEFAULT, "jpeg_create_decompress: %i\n",0);
                jpeg_create_decompress(&cinfo);
                // MATRIX_Printf( FONT_DEFAULT, "jpeg_stdio_src: %i\n",0);
                /* Step 2: specify data source (eg, a file) */
                jpeg_stdio_src(&cinfo, &SDFile);
                // MATRIX_Printf( FONT_DEFAULT, "jpeg_read_header: %i\n",0);
                /* Step 3: read file parameters with jpeg_read_header() */
                (void) jpeg_read_header(&cinfo, TRUE);
                /* Step 4: set parameters for decompression */
                /* In this example, we don't need to change any of the defaults set by
                 * jpeg_read_header(), so we do nothing here.
                 */
                // MATRIX_Printf( FONT_DEFAULT, "jpeg_start_decompress: %i\n",0);
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
                // MATRIX_Printf( FONT_DEFAULT, "print: %i\n",0);
                /* Step 6: while (scan lines remain to be read) */
                /*           jpeg_read_scanlines(...); */
                while( cinfo.output_scanline < cinfo.output_height)
                {
                    /* jpeg_read_scanlines expects an array of pointers to scanlines.
                     * Here the array is only one element long, but you could ask for
                     * more than one scanline at a time if that's more convenient.
                     */
                    // MATRIX_Printf( FONT_DEFAULT, "step 0: %i\n",cinfo.output_height);
                    (void) jpeg_read_scanlines(&cinfo, nRowBuff, 1);
                    // MATRIX_Printf( FONT_DEFAULT, "step 1: %i\n",cinfo.output_height);
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
                // MATRIX_Printf( FONT_DEFAULT, "%s\r\n",  pPath);

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
            MATRIX_Printf( FONT_DEFAULT, "can't open !\n");
        }
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, "Mount Failed!\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
    }
        f_mount(NULL, (TCHAR const*)"", 1U);

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
        {
            break;
        }
        if (fno.fname[0] == '.')
            continue;
        if (fno.fattrib & AM_DIR)
        {
            jpeg_destroy_decompress(&cinfo);
            return;
        }
        SDResult=f_open(&SDFile,fno.fname, FA_OPEN_EXISTING | FA_READ);
        if (SDResult != FR_OK)
        {
            MATRIX_Printf( FONT_DEFAULT, "SDResult1: %i\n",SDResult);
            while(1);
        }
        SDResult=f_read(&SDFile,pagebuff,4,(UINT *) &nobytsread);
        if (SDResult != FR_OK)
        {
            MATRIX_Printf( FONT_DEFAULT, "SDResult2: %i\n",SDResult);
            while(1);
        }

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
        {
            return;
        }
        if (fno.fattrib & AM_DIR)
        {
            return;
        }

        SDResult=f_open(&SDFile,fno.fname, FA_READ);
        {
            return;
        }
        l_Cinfo.err = jpeg_std_error(&l_Jerr);
        jpeg_create_decompress(&l_Cinfo);
        jpeg_stdio_src(&l_Cinfo, (JFILE * ) &SDFile);
        (void) jpeg_read_header(&l_Cinfo, TRUE);

        (void) jpeg_start_decompress(&l_Cinfo);
        uint32_t row_stride = l_Cinfo.output_width * l_Cinfo.output_components;
        buffer = (*l_Cinfo.mem->alloc_sarray)((j_common_ptr) &l_Cinfo, JPOOL_IMAGE, row_stride, 1);

        while(l_Cinfo.output_scanline < l_Cinfo.output_height)
        {
            (void) jpeg_read_scanlines(&l_Cinfo, buffer, 1);
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
  MX_SDMMC1_SD_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_FATFS_Init();
  MX_LIBJPEG_Init();
  MX_DAC1_Init();
  MX_I2C4_Init();
  /* USER CODE BEGIN 2 */
    MATRIX_Init( 40 );

    HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
    HAL_TIM_Base_Start_IT(&htim4);
    __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,MATRIX_getBrightness());

    MATRIX_SetTextSize(1);
    MATRIX_SetCursor(0, 5);
    MATRIX_SetTextColor(0xFFFF);


    // MATRIX_disImage(gImage_Phung, 256, 128, 0, 0, RGB_888);
    // HAL_Delay(3000);
    // MATRIX_FillScreen(0x0);
    // MATRIX_disImage(gImage_Anime, 256, 128, 0, 25, RGB_888);
    // HAL_Delay(3000);
    // MATRIX_FillScreen(0x0);
    // MATRIX_disImage(gImage_Nature, 256, 128, 0, 0, RGB_888);
    // HAL_Delay(3000);
    // MATRIX_FillScreen(0x0);
    // MATRIX_disImage(gImage_256_192, 256, 192, 0, 0, RGB_888);
    // HAL_Delay(5000);
    // MATRIX_FillScreen(0x0);
    // MATRIX_disImage(gImage_NEU, 128, 128, 0, 0, RGB_888);
    // MATRIX_disImage(gImage_NEU, 128, 128, 128, 0, RGB_888);
    // HAL_Delay(3000);
    // MATRIX_disImage(gImage_PIC1, 256, 64, 0, 0, RGB_888);
    // MATRIX_disImage(gImage_PIC1, 256, 64, 0, 64, RGB_888);
    // HAL_Delay(5000);
    // MATRIX_FillCircle( 128, 31, 30, 0x7E0 );
    // MATRIX_FillScreen(0xF800);
    // HAL_Delay(5000);
    // MATRIX_FillScreen(0xCE0);
    // HAL_Delay(5000);
    // MATRIX_FillScreen(0x1F);
    // HAL_Delay(5000);
    // MATRIX_FillScreen(0x0);
    // plasma();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if( MSD_OK == BSP_SD_Init() )
    {
        MATRIX_SetTextColor(0xF81F);
        MATRIX_FillScreen(0x0);
        MATRIX_SetCursor(0, 5);
        BSP_SD_GetCardInfo(&CardInfo);
        MATRIX_Printf( FONT_DEFAULT, "-----------CardInfo-----------\n");
        MATRIX_Printf( FONT_DEFAULT, " - Type:  %7u\n - Ver:   %7u\n - Class: %7u\n - Add:   %7u\n - BNbr:  %7u\n - Bsize: %7u\n - LNbr:  %7u\n - LSize: %7u\n", CardInfo.CardType, CardInfo.CardVersion, CardInfo.Class, CardInfo.RelCardAdd, CardInfo.BlockNbr, CardInfo.BlockSize, CardInfo.LogBlockNbr, CardInfo.LogBlockSize);
        MATRIX_SetTextColor(0x07E0);
        HAL_Delay(1000);
        MATRIX_disImage(gImage_256_192, 256, 192, 0, 0, RGB_888);
        // if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK)
        // {
        //     MATRIX_Printf( FONT_DEFAULT, "MicroSD Mounted! \n");
        //     /* Check free space */
        //     f_getfree("", &fre_clust, &pfs);
        //     MATRIX_SetTextColor(0x1F1F);
        //     MATRIX_Printf( FONT_DEFAULT, "Total Size: \t%luMb\n",(uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5)/1024);
        //     MATRIX_SetTextColor(0xF1F1);
        //     MATRIX_Printf( FONT_DEFAULT, "Free Space: \t%luMb\n",(uint32_t)(fre_clust * pfs->csize * 0.5)/1024);
        //     MATRIX_SetTextColor(0xFFFF);
        //     HAL_Delay(1000);
                // MATRIX_SetTextColor(0x1F1F);
                // MATRIX_FillScreen(0x0);
                // MATRIX_SetCursor(0, 5);
                // scan_files(SDPath);

                //     HAL_Delay(2000);
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
                // HAL_SD_GetCardStatus(&hsd1, &pState);
                // MATRIX_Printf( FONT_DEFAULT, "State: %i, Code: %i\n",hsd1.State, hsd1.ErrorCode);
                // MATRIX_SetTextColor(0xF81F);
                // MATRIX_Printf( FONT_DEFAULT, "-> Execute user application in 1s!\n");
                // HAL_Delay(2000);
                while(1)
                {
                    // MATRIX_FillScreen(0x0);
                    // MATRIX_SetCursor(0, 0);
                    // MATRIX_SetTextColor(0x7FF);
                    // MEDIA_DisplayJpeg("test/1.jpg");
                    // HAL_Delay(1000);
                    // MEDIA_DisplayJpeg("test/2.jpg");
                    // HAL_Delay(1000);
                    // MEDIA_DisplayJpeg("test/3.jpg");
                    // HAL_Delay(1000);
                }
                // if( f_opendir(&dir,"/test") == FR_OK )
                // {
                //     // SDResult = f_readdir(&dir, &fno);
                //     if( FR_OK == f_open(&SDFile,fno.fname, FA_READ))
                //     {
                //         struct jpeg_decompress_struct _Cinfo;
                //         struct jpeg_error_mgr _Jerr;

                //         MATRIX_Printf( FONT_DEFAULT, "-> Opened %s! \n", fno.fname);

                //         _Cinfo.err = jpeg_std_error(&_Jerr);
                //         MATRIX_Printf( FONT_DEFAULT, "-> jpeg_std_error done! \n");
                //         HAL_Delay(1000);
                //         jpeg_create_decompress(&_Cinfo);
                //         MATRIX_Printf( FONT_DEFAULT, "-> jpeg_create_decompress done! \n");
                //         HAL_Delay(1000);
                //         jpeg_stdio_src(&_Cinfo, (FILE * ) &SDFile);
                //         MATRIX_Printf( FONT_DEFAULT, "-> jpeg_stdio_src done! \n");
                //         HAL_Delay(1000);
                //         (void) jpeg_read_header(&_Cinfo, TRUE);
                //         // MATRIX_Printf( FONT_DEFAULT, "-> jpeg_read_header done! \n");
                //         // (void) jpeg_start_decompress(&_Cinfo);
                //         // MATRIX_Printf( FONT_DEFAULT, "-> jpeg_start_decompress done! \n");
                //     }
                //     else
                //     {
                //         MATRIX_Printf( FONT_DEFAULT, "-> Open %s error! \n", fno.fname);
                //     }
                // }
                // else
                // {
                //     MATRIX_Printf( FONT_DEFAULT, "-> Open /test error! \n");
                // }
                // while(1);
                // MATRIX_UserApplication();
        // }
        // else
        // {
        //     // __enable_irq();
        //     MATRIX_Printf( FONT_DEFAULT, "Mount Failed! \n");
        // }
        // HAL_Delay(2000);
    }
    MATRIX_SetTextColor(0xF81F);
    MATRIX_FillScreen(0x0);
    MATRIX_SetCursor(0, 5);
    MATRIX_Printf( FONT_DEFAULT, "-----------No SDCard---------\n");
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

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
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SDMMC|RCC_PERIPHCLK_I2C4;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
  PeriphClkInitStruct.I2c4ClockSelection = RCC_I2C4CLKSOURCE_D3PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
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
