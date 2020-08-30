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
#include "gamma.h"
#include "ir1838.h"
#include "flash.h"
#include "string.h"
#include "stdio.h"
#include <setjmp.h>
#include "ff.h"
// #include "mjpeg_decode.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* I2C Slave Address  */
#define DS3231_ADDRESS  0x68<<1

/* S3231 Register Addresses */
#define DS3231_REG_TIMEDATE   0x00
#define DS3231_REG_ALARMONE   0x07
#define DS3231_REG_ALARMTWO   0x0B

#define DS3231_REG_CONTROL    0x0E
#define DS3231_REG_STATUS     0x0F
#define DS3231_REG_AGING      0x10

#define DS3231_REG_TEMP       0x11

/* S3231 Register Data Size if not just 1 */
#define DS3231_REG_TIMEDATE_SIZE  7
#define DS3231_REG_ALARMONE_SIZE  4
#define DS3231_REG_ALARMTWO_SIZE  3

#define DS3231_REG_TEMP_SIZE  2

/* DS3231 Control Register Bits */
#define DS3231_A1IE   1<<0
#define DS3231_A2IE   1<<1
#define DS3231_INTCN  1<<2
#define DS3231_RS1    1<<3
#define DS3231_RS2    1<<4
#define DS3231_CONV   1<<5
#define DS3231_BBSQW  1<<6
#define DS3231_EOSC   1<<7
#define DS3231_AIEMASK    (DS3231_A1IE | DS3231_A2IE)
#define DS3231_RSMASK     (DS3231_RS1 | DS3231_RS2)

/* DS3231 Status Register Bits */
#define DS3231_A1F        1<<0
#define DS3231_A2F        1<<1
#define DS3231_BSY        1<<2
#define DS3231_EN32KHZ    1<<3
#define DS3231_OSF        1<<7
#define DS3231_AIFMASK    (DS3231_A1F | DS3231_A2F)

/* seconds accuracy */
enum DS3231AlarmOneControl
{
   /* bit order:  A1M4  DY/DT  A1M3  A1M2  A1M1 */
    DS3231AlarmOneControl_HoursMinutesSecondsDayOfMonthMatch = 0x00,
    DS3231AlarmOneControl_OncePerSecond = 0x17,
    DS3231AlarmOneControl_SecondsMatch = 0x16,
    DS3231AlarmOneControl_MinutesSecondsMatch = 0x14,
    DS3231AlarmOneControl_HoursMinutesSecondsMatch = 0x10,
    DS3231AlarmOneControl_HoursMinutesSecondsDayOfWeekMatch = 0x08,
};
typedef enum
{
    MODE_NORMAL,
    MODE_SETUPTIME,
    MODE_RESERVED,
} CLOCK_Modes;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c2;

SD_HandleTypeDef hsd;

/* USER CODE BEGIN PV */
uint8_t second,minute,hour,day,date,month,year;
uint8_t u8LastMin=255U;
static volatile uint8_t gUpdateTimeFlag = 1U;
static int8_t gH_X = 0U, gH_Y = 0U, gM_X = 0, gM_Y = 0U, gS_X = 0U, gS_Y = 0U;
static uint8_t gOldHour = 25U, gOldMin = 60U, gOldSec = 60U, gOldDate = 99U, gOldDay = 99U, gOldMon = 99U, gOldYear = 99U;
static uint8_t u8IsFirstDraw = 1U;
MATRIX_MonitorTypes gCurrentMonitor = MONITOR_DISPLAY_ANALOG_CLOCK0;
uint8_t receive_data[7],send_data[7];

FRESULT SDResult = FR_OK;
FILINFO fno = { 0 };
DIR dir;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM3_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
uint8_t BCD2DEC(uint8_t data);
uint8_t DEC2BCD(uint8_t data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static inline void CLOCK_InitData()
{
    for( uint8_t u8KeyPos = IR_KEY_RESERVED; u8KeyPos < IR_TOTAL_KEY; u8KeyPos++ )
    {
        IR1838_SetKeysCode( (IR_KeysLists) u8KeyPos, (uint8_t) FLASH_Read(u8KeyPos));
        // MATRIX_Printf( FONT_DEFAULT, 1U, u8KeyPos * 13, 0, 0xF81F, "%02X ", IR1838_GetKeysCode(u8KeyPos) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, u8KeyPos * 13, 0, 0xF81F, "%02X ", FLASH_Read(u8KeyPos) );
    }
}

static inline void CLOCK_ResetDefaultState(void)
{
    gOldSec = gOldMin = gOldHour = gOldDate = gOldDay = gOldMon = gOldYear = 99U;
    u8IsFirstDraw = 1U;
}

static inline void CLOCK_UpdateKeyCode( void )
{
    IR_KeysLists nCurKey = IR_KEY_RESERVED;
    uint32_t u32PointerTick = 0U;
    uint32_t u32StrTick = 0U;
    uint8_t u8Logic = 0;
    uint8_t u8xCurPos = 0U;
    uint8_t u8yCurPos = 0U;
    // uint8_t l_u8CurrentBri = MATRIX_getBrightness();

    /* Avoid the warnings when it doesn't have any coding lines below */
    (void) 1U;

    /* Check whether the button for updating key code is turned on */
    /* Note: This pin shall active at low level */
    if( !(LL_GPIO_ReadInputPort( UPD_KEY_GPIO_Port ) & UPD_KEY_Pin) )
    {
        LL_mDelay(20);
        while( !(LL_GPIO_ReadInputPort( UPD_KEY_GPIO_Port ) & UPD_KEY_Pin) );
        gCurrentMonitor = MONITOR_SET_KEY;
        MATRIX_TransitionEffect( TRANS_LEFT, EFFECT_FADE_OUT );

        MATRIX_DrawRect( 0U, 0U, 128U,  64U, 0xEF0U );
        MATRIX_DrawFastHLine( 1U, 20U, 126U, 0xEF0U );
        MATRIX_DrawFastVLine( 20U, 1U,  19U, 0xEF0U );
        MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 21U, 14U, 0xF81F, "Update Keys" );

        /* Draw Menu button */
        MATRIX_FillCircle( 15U, 31U, 8U, 0xF800U );
        MATRIX_DrawRect( 11U, 26U, 9U, 2U, 0xFFFFU );
        MATRIX_DrawRect( 11U, 30U, 9U, 2U, 0xFFFFU );
        MATRIX_DrawRect( 11U, 34U, 9U, 2U, 0xFFFFU );

        /* Draw Exit button */
        MATRIX_FillCircle( 15U, 52U, 8U, 0xF800U );
        MATRIX_DrawLine( 11U, 48U, 19U, 56U, 0xFFFFU);
        MATRIX_DrawLine( 11U, 49U, 18U, 56U, 0xFFFFU);
        MATRIX_DrawLine( 10U, 49U, 18U, 57U, 0xFFFFU);
        MATRIX_DrawLine( 11U, 56U, 19U, 48U, 0xFFFFU);
        MATRIX_DrawLine( 10U, 56U, 19U, 47U, 0xFFFFU);
        MATRIX_DrawLine( 11U, 57U, 20U, 48U, 0xFFFFU);

        /* Draw up button */
        MATRIX_FillCircle( 60U, 32U, 8U, 0x33U );
        MATRIX_FillTriangle( 55U, 34U, 65U, 34U, 60U, 26U, 0xFFFFU );

        /* Draw down button */
        MATRIX_FillCircle( 60U, 52U, 8U, 0x33U );
        MATRIX_FillTriangle( 55U, 50U, 65U, 50U, 60U, 58U, 0xFFFFU );

        /* Draw left button */
        MATRIX_FillCircle( 45U, 42U, 8U, 0x33U );
        MATRIX_FillTriangle( 47U, 47U, 47U, 37U, 39U, 42U, 0xFFFFU );

        /* Draw right button */
        MATRIX_FillCircle( 75U, 42U, 8U, 0x33U );
        MATRIX_FillTriangle( 73U, 47U, 73U, 37U, 81U, 42U, 0xFFFFU );

        /* Draw selected button */
        MATRIX_FillCircle( 105U, 32U, 8U, 0xF800 );
        MATRIX_DrawRoundRect( 100U, 29U, 6U, 7U, 6U, 0xFFFFU );
        MATRIX_DrawFastVLine( 107U, 29U, 7U, 0xFFFFU );

        MATRIX_DrawLine( 107U, 32U, 110U, 29U, 0xFFFFU );
        MATRIX_DrawLine( 107U, 32U, 110U, 35U, 0xFFFFU );

        /* Draw Reserved button */
        MATRIX_FillCircle( 105U, 52U, 8U, 0xF800 );

        // for( uint8_t i = 0; i <= l_u8CurrentBri; i++)
        // {
        //     MATRIX_setBrightness(i);
        //     LL_mDelay(80);
        // }

        u32PointerTick = HAL_GetTick();
        u32StrTick = HAL_GetTick();
        IR1838_EnterSetupMode();
        while( IR_NORMAL_MODE != IR1838_GetCurrentMode() )
        {
            nCurKey = IR1838_GetCurrentKey();
            switch( nCurKey )
            {
                case IR_KEY_MENU:
                    u8xCurPos = 25U;
                    u8yCurPos = 31U;
                    break;
                case IR_KEY_EXIT:
                    u8xCurPos = 25U;
                    u8yCurPos = 52U;
                    break;
                case IR_KEY_UP:
                    u8xCurPos = 70U;
                    u8yCurPos = 31U;
                    break;
                case IR_KEY_DOWN:
                    u8xCurPos = 70U;
                    u8yCurPos = 53U;
                    break;
                case IR_KEY_LEFT:
                    u8xCurPos = 55U;
                    u8yCurPos = 42U;
                    break;
                case IR_KEY_RIGHT:
                    u8xCurPos = 85U;
                    u8yCurPos = 42U;
                    break;
                case IR_KEY_OK:
                    u8xCurPos = 115U;
                    u8yCurPos = 32U;
                    break;
                case IR_KEY_DONE:
                    IR1838_ExitSetupMode();
                    break;
                default:
                    break;
            }
            if( (150U < HAL_GetTick() - u32PointerTick) )
            {
                u32PointerTick = HAL_GetTick();
                u8Logic = !u8Logic;
                MATRIX_FillRect( u8xCurPos + 1U, u8yCurPos - 1U, 6U, 3U, (u8Logic?0:0x5C0U) );
                MATRIX_DrawLine( u8xCurPos, u8yCurPos, u8xCurPos + 2U, u8yCurPos - 2U, (u8Logic?0:0x7F0U));
                MATRIX_DrawLine( u8xCurPos, u8yCurPos, u8xCurPos + 2U, u8yCurPos + 2U, (u8Logic?0:0x7F0U));
            }
            if( 200U < HAL_GetTick() - u32StrTick )
            {
                u32StrTick = HAL_GetTick();
                MATRIX_FlutterLeftRight( 22U, 2U, 104U, 17U );
            }
        }
        MATRIX_FillRect(22U, 2U, 104U, 17U, 0x0U );
        MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 21U, 14U, 0xF81F, "Successful!" );

        for( uint8_t u8KeyPos = 1; u8KeyPos < 9; u8KeyPos++ )
        {
            FLASH_Write(u8KeyPos, IR1838_GetKeysCode(u8KeyPos));
        }

        LL_mDelay(1000);
        MATRIX_TransitionEffect( TRANS_LEFT, EFFECT_FADE_OUT );
        CLOCK_ResetDefaultState();

        gCurrentMonitor = MONITOR_DISPLAY_ANALOG_CLOCK0;
    }
}

#define CLOCK_CENTER_X 31U
#define CLOCK_CENTER_Y 31U
#define CLOCK_RADIOUS  31U
#define CLOCK_LENGHT_HOUR   10
#define CLOCK_LENGHT_MIN    15
#define CLOCK_LENGHT_SEC    20

#define CLOCK_DIGITAL_HM_X    67U
#define CLOCK_DIGITAL_HM_Y    25U
#define CLOCK_DIGITAL_DATE_X    67U
#define CLOCK_DIGITAL_DATE_Y    50U

static inline void MATRIX_AnalogClockMode0(void)
{
    int8_t l_HX, l_HY, l_MX, l_MY, l_SX, l_SY;
    if( gUpdateTimeFlag )
    {
        gUpdateTimeFlag = 0U;

        /* Clear old graphic */
        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + gM_X, CLOCK_CENTER_Y + gM_Y, 0x0U );
        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + gH_X, CLOCK_CENTER_Y + gH_Y, 0x0U );
        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + gS_X, CLOCK_CENTER_Y + gS_Y, 0x0U );

        if( u8IsFirstDraw )
        {
            u8IsFirstDraw = 0U;
            /* Draw new basic graphic */
            MATRIX_FillCircle( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIOUS, 0xFFFFU );
            MATRIX_FillCircle( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIOUS - 2U, 0x0U );
#if 0
            MATRIX_Printf( FONT_TOMTHUMB, 1U, 28U,  7U, 0xFF,  "12");
            MATRIX_Printf( FONT_TOMTHUMB, 1U,  2U, 34U, 0xFF,  "9");
            MATRIX_Printf( FONT_TOMTHUMB, 1U, 30U, 61U, 0xFF,  "6");
            MATRIX_Printf( FONT_TOMTHUMB, 1U, 58U, 34U, 0xFF,  "3");
#else
            MATRIX_Printf( FONT_DEFAULT, 1U, 25U,  3U, 0xF81FU,  "12");
            MATRIX_Printf( FONT_DEFAULT, 1U,  3U, 28U, 0xF81FU,  "9");
            MATRIX_Printf( FONT_DEFAULT, 1U, 29U, 53U, 0xF81FU,  "6");
            MATRIX_Printf( FONT_DEFAULT, 1U, 55U, 28U, 0xF81FU,  "3");

            MATRIX_Printf( FONT_ORG_01,  1U, 44U, 12U, 0xFFFFU,  "1");
            MATRIX_Printf( FONT_ORG_01,  1U, 50U, 21U, 0xFFFFU,  "2");

            MATRIX_Printf( FONT_ORG_01,  1U, 51U, 45U, 0xFFFFU,  "4");
            MATRIX_Printf( FONT_ORG_01,  1U, 42U, 54U, 0xFFFFU,  "5");

            MATRIX_Printf( FONT_ORG_01,  1U, 16U, 54U, 0xFFFFU,  "7");
            MATRIX_Printf( FONT_ORG_01,  1U,  7U, 45U, 0xFFFFU,  "8");

            MATRIX_Printf( FONT_ORG_01,  1U,  7U, 21U, 0xFFFFU,  "10");
            MATRIX_Printf( FONT_ORG_01,  1U, 16U, 12U, 0xFFFFU,  "11");
#endif
        }

        gH_X = l_HX = CLOCK_LENGHT_HOUR * sin( (hour + minute/60.0) / 6.0 * _PI );
        gH_Y = l_HY = -CLOCK_LENGHT_HOUR * cos( (hour + minute/60.0) / 6.0 * _PI );
        gM_X = l_MX = CLOCK_LENGHT_MIN * sin( minute / 30.0 * _PI );
        gM_Y = l_MY = -CLOCK_LENGHT_MIN * cos( minute / 30.0 * _PI );
        gS_X = l_SX = CLOCK_LENGHT_SEC * sin( second / 30.0 * _PI );
        gS_Y = l_SY = -CLOCK_LENGHT_SEC * cos( second / 30.0 * _PI );

        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + l_MX, CLOCK_CENTER_Y + l_MY, 0xF800 );
        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + l_HX, CLOCK_CENTER_Y + l_HY, 0x7E0 );
        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + l_SX, CLOCK_CENTER_Y + l_SY, 0x1F );
        MATRIX_FillCircle( CLOCK_CENTER_X, CLOCK_CENTER_Y, 1, 0xFFFF );

        if( gOldHour != hour )
        {
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X, CLOCK_DIGITAL_HM_Y, 0x0U, "%02i", gOldHour);
            gOldHour = hour;
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X, CLOCK_DIGITAL_HM_Y, 0x7FF, "%02i", hour);
        }
        MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X + 25U, CLOCK_DIGITAL_HM_Y, 0x7FF, ":");
        if( gOldMin != minute )
        {
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X + 34U, CLOCK_DIGITAL_HM_Y, 0x0U, "%02i", gOldMin);
            gOldMin = minute;
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X + 34U, CLOCK_DIGITAL_HM_Y, 0x7FF, "%02i", minute);
        }
        if( gOldDay != day )
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, CLOCK_DIGITAL_DATE_X, CLOCK_DIGITAL_DATE_Y, 0x0U, "%02i/%02i/20%02i", gOldDay, gOldMon, gOldYear);
            gOldDay = day;
            MATRIX_Printf( FONT_DEFAULT, 1U, CLOCK_DIGITAL_DATE_X, CLOCK_DIGITAL_DATE_Y, 0x7FF, "%02i/%02i/20%02i", day, month, year );
        }
    }
}

static inline void MATRIX_DisplayMonitor0(void)
{
    (void) gCurrentMonitor;

    if( gUpdateTimeFlag )
    {
        gUpdateTimeFlag = 0;

    }
}

static inline void MATRIX_DisplayMonitor1(void)
{
    (void) gCurrentMonitor;
}

static inline void MATRIX_DisplayMonitor2(void)
{
    (void) gCurrentMonitor;
}

static inline void MATRIX_DisplayMonitor3(void)
{
    (void) gCurrentMonitor;
}

static inline void MATRIX_DisplayMonitorRandom(void)
{
    (void) gCurrentMonitor;
}

static inline void CLOCK_BasicMonitor(void)
{
    IR_KeysLists nCurKey = IR_KEY_RESERVED;

    /* Avoid the warnings when it doesn't have any coding lines below */
    (void) nCurKey;

    switch( gCurrentMonitor )
    {
        case MONITOR_DISPLAY_ANALOG_CLOCK0:
            MATRIX_AnalogClockMode0();
            break;
        case MONITOR_DISPLAY_MON0:
            MATRIX_DisplayMonitor0();
            break;
        case MONITOR_DISPLAY_MON1:
            MATRIX_DisplayMonitor1();
            break;
        case MONITOR_DISPLAY_MON2:
            MATRIX_DisplayMonitor2();
            break;
        case MONITOR_DISPLAY_MON3:
            MATRIX_DisplayMonitor3();
            break;
        case MONITOR_DISPLAY_RANDOM:
            MATRIX_DisplayMonitorRandom();
            break;
        default:
            break;
    }
}

static inline void CLOCK_DisableAllIrq(void)
{
    LL_TIM_DisableCounter(TIM6);
    LL_TIM_DisableCounter(TIM_OE);
    LL_TIM_DisableCounter(TIM_DAT);
    LL_TIM_DisableCounter( TIM_IR );

    // NVIC_DisableIRQ(TIM3_IRQn);
    // NVIC_DisableIRQ(TIM4_IRQn);
    // NVIC_DisableIRQ(EXTI9_5_IRQn);
}

static inline void CLOCK_EnableAllIrq(void)
{
    LL_TIM_EnableCounter(TIM6);
    LL_TIM_EnableCounter(TIM_OE);
    LL_TIM_EnableCounter(TIM_DAT);
    LL_TIM_EnableCounter( TIM_IR );
    // NVIC_EnableIRQ(TIM3_IRQn);
    // NVIC_EnableIRQ(TIM4_IRQn);
    // NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void DS3231_UpdateData( void )
{
    /* Read data to parse time informations */
    HAL_I2C_Mem_Read(&hi2c2,DS3231_ADDRESS,0,I2C_MEMADD_SIZE_8BIT,receive_data,7,1000);

    /* Raise global flag immediately to avoid the missing data at data cache */
    gUpdateTimeFlag = 1;

    /* Parse data */
    second=BCD2DEC(receive_data[0]);
    minute=BCD2DEC(receive_data[1]);
    hour=BCD2DEC(receive_data[2]);

    day=BCD2DEC(receive_data[3]);
    date=BCD2DEC(receive_data[4]);
    month=BCD2DEC(receive_data[5]);
    year= BCD2DEC(receive_data[6]);
}

uint8_t BCD2DEC(uint8_t data)
{
    return (data>>4)*10 + (data&0x0f);
}

uint8_t DEC2BCD(uint8_t data)
{
    return (data/10)<<4|(data%10);
}

void CLOCK_Setting( void )
{
    // extern const unsigned char gImage_ico_clock_64x64[8192];

    /* Avoid the warnings when it doesn't have any coding lines below */
    (void) 1U;
    MATRIX_TransitionEffect( TRANS_LEFT, EFFECT_FADE_OUT );
    CLOCK_ResetDefaultState();
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x1485, "1. Time \n2. Fonts\n3. Lighting\n4. Strings\n5. Audio\n6. Video\n7. Exit");
    while (1)
    {
        /* code */
    }
}

    uint8_t u8Index = 1U;
FRESULT SD_ScanFiles (char* pat)
{
    UINT i;
    char path[20];
    sprintf (path, "%s",pat);

    SDResult = f_opendir(&dir, path);                       /* Open the directory */
    if (SDResult == FR_OK)
    {
        for (;;)
        {
            SDResult = f_readdir(&dir, &fno);                   /* Read a directory item */
            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "R:%i %s %s \r\n", SDResult, path, fno.fname);

            if (SDResult != FR_OK || fno.fname[0] == 0)
                break;  /* Break on error or end of dir */

            if (fno.fattrib & AM_DIR)     /* It is a directory */
            {
                if (!(strcmp ("SYSTEM~1", fno.fname)))
                    continue;
                MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "D:%s\r\n", fno.fname);
                i = strlen(path);
                sprintf(&path[i], "%s", fno.fname);
                SDResult = SD_ScanFiles(path);                     /* Enter the directory */
                // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "Sub Folder! %i %s", SDResult, path);
                if (SDResult != FR_OK)
                    break;
                path[i] = 0;
                // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "R:%i %s\r\n", SDResult, path);
            }
            else
            {
                /* It is a file. */
                MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "F:%s/%s\r\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, (u8Index++) * 6, 0xF800, "Error open dir!\n");
    }
    return SDResult;
}

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
int row_stride;		/* physical row width in output buffer */
JSAMPROW nRowBuff[1];		/* Output row buffer */

/*
 * Here's the routine that will replace the standard error_exit method:
 */

void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

#if 0
void SD_DisplayJpeg(char * pPath)
{
    ON_LED();
    if( FR_OK==f_open(&SDFile, pPath, FA_OPEN_EXISTING | FA_READ) )
    {
        OFF_LED();
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "natural.jpg opened!\n");
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
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "Destroy Compression!\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            f_close(&SDFile);
            free(nRowBuff[0]);
        }
        else
        {
            // ON_LED();
            /* Now we can initialize the JPEG decompression object. */
            jpeg_create_decompress(&cinfo);

            /* Step 2: specify data source (eg, a file) */
            jpeg_stdio_src(&cinfo, &SDFile);

            /* Step 3: read file parameters with jpeg_read_header() */
            (void) jpeg_read_header(&cinfo, TRUE);

            /* Step 4: set parameters for decompression */
            /* In this example, we don't need to change any of the defaults set by
             * jpeg_read_header(), so we do nothing here.
             */

            /* Step 5: Start decompressor */
            (void) jpeg_start_decompress(&cinfo);

            /* We may need to do some setup of our own at this point before reading
             * the data.  After jpeg_start_decompress() we have the correct scaled
             * output image dimensions available, as well as the output colormap
             * if we asked for color quantization.
             * In this example, we need to make an output work buffer of the right size.
             */
            /* JSAMPLEs per row in output buffer */
            row_stride = cinfo.output_width * cinfo.output_components;

            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "Set!\n");
            HAL_Delay(1000);
            /* Make a one-row-high sample array that will go away when done with image */
            // nRowBuff[0] = (*cinfo.mem->alloc_sarray)
            //         ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
            nRowBuff[0] = (unsigned char *) malloc( row_stride );

            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "Malloc!\n");
            /* Step 6: while (scan lines remain to be read) */
            /*           jpeg_read_scanlines(...); */
            uint8_t u8Heigh = 0U;
            uint8_t r, g, b;
            uint16_t u16Color;

            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "W:%i, H:%i.\nS:%i, N:%i.\nT:%i, C:%i",
            //                 cinfo.output_width, cinfo.output_height, sizeof(buffer), cinfo.output_components, row_stride,
            //                 cinfo.out_color_space
            //              );

            while( cinfo.output_scanline < cinfo.output_height)
            {
                /* jpeg_read_scanlines expects an array of pointers to scanlines.
                 * Here the array is only one element long, but you could ask for
                 * more than one scanline at a time if that's more convenient.
                 */
                (void) jpeg_read_scanlines(&cinfo, nRowBuff, 1);
                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, cinfo.output_scanline * 10, 0x07E0, "CurrL:%i ", cinfo.output_scanline);
                (void) u8Heigh;
                // HAL_Delay(4000);
                for( uint8_t u8Index = 0U; u8Index < MATRIX_WIDTH; u8Index++ )
                {
                    r = (uint8_t) nRowBuff[0][ u8Index * 3 + 0 ];
                    g = (uint8_t) nRowBuff[0][ u8Index * 3 + 1 ];
                    b = (uint8_t) nRowBuff[0][ u8Index * 3 + 2 ];
#if 0
                    u16Color =  ((uint16_t) ((r << 8U) & 0xF800U)) |
                                ((uint16_t) ((g << 3U) & 0x07E0U)) |
                                ((uint16_t) ((b >> 3U) & 0x001FU));
#else
                    r = pgm_read_byte(&gamma_table[(r * 255) >> 8]); // Gamma correction table maps
                    g = pgm_read_byte(&gamma_table[(g * 255) >> 8]); // 8-bit input to 4-bit output
                    b = pgm_read_byte(&gamma_table[(b * 255) >> 8]);

                    u16Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                                (g <<  7) | ((g & 0xC) << 3) |
                                (b <<  1) | ( b        >> 3);

                    // u16Color =  ((uint16_t) ((r << 8U) & 0xF800U)) |
                    //             ((uint16_t) ((g << 3U) & 0x07E0U)) |
                    //             ((uint16_t) ((b >> 3U) & 0x001FU));
#endif
                    MATRIX_WritePixel( u8Index, cinfo.output_scanline - 1U, u16Color );
                }
                /* Assume put_scanline_someplace wants a pointer and sample count. */
                // MATRIX_DispImage( (const uint8_t *) buffer, 0U, cinfo.output_scanline, cinfo.output_width, 1U );
            }

            /* Step 7: Finish decompression */
            (void) jpeg_finish_decompress(&cinfo);

                /* Step 8: Release JPEG decompression object */
            /* This is an important step since it will release a good deal of memory. */
            jpeg_destroy_decompress(&cinfo);

            /* After finish_decompress, we can close the input file.
             * Here we postpone it until after no more JPEG errors are possible,
             * so as to simplify the setjmp error logic above.  (Actually, I don't
             * think that jpeg_destroy can do an error exit, but why assume anything...)
             */
            f_close(&SDFile);
            free(nRowBuff[0]);
            // OFF_LED();
        }
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "can't open !\n");
    }
}

void SD_PlayMotionJpeg(char * pPath)
{
    ON_LED();
    if( FR_OK==f_open(&SDFile, pPath, FA_OPEN_EXISTING | FA_READ) )
    {
        OFF_LED();
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "%s opened!\n", pPath);
        /* Step 1: allocate and initialize JPEG decompression object */
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);

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
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "Destroy Compression!\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            f_close(&SDFile);
        }
        else
        {

            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "Starting..!\n");
            HAL_Delay(1000);

            /* Now we can initialize the JPEG decompression object. */
            jpeg_create_decompress(&cinfo);

            /* Step 2: specify data source (eg, a file) */
            jpeg_stdio_src(&cinfo, &SDFile);
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0x07E0, "Source..!\n");

            /* Step 3: read file parameters with jpeg_read_header() */
            (void) jpeg_read_header(&cinfo, TRUE);
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0x07E0, "Header..!\n");

            // jinit_write_ppm(&cinfo);

            /* Step 4: set parameters for decompression */
            /* In this example, we don't need to change any of the defaults set by
             * jpeg_read_header(), so we do nothing here.
             */

            /* Step 5: Start decompressor */
            (void) jpeg_start_decompress(&cinfo);

            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0x07E0, "Decomp..!\n");

            /* We may need to do some setup of our own at this point before reading
             * the data.  After jpeg_start_decompress() we have the correct scaled
             * output image dimensions available, as well as the output colormap
             * if we asked for color quantization.
             * In this example, we need to make an output work buffer of the right size.
             */
            /* JSAMPLEs per row in output buffer */
            row_stride = cinfo.output_width * cinfo.output_components;

            /* Make a one-row-high sample array that will go away when done with image */
            // nRowBuff[0] = (*cinfo.mem->alloc_sarray)
            //         ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
            nRowBuff[0] = (unsigned char *) malloc( row_stride );

            /* Step 6: while (scan lines remain to be read) */
            /*           jpeg_read_scanlines(...); */
            uint8_t u8Heigh = 0U;
            uint8_t r, g, b;
            uint16_t u16Color;

            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "W:%i, H:%i.\nS:%i, N:%i.\nT:%i, C:%i",
            //                 cinfo.output_width, cinfo.output_height, sizeof(buffer), cinfo.output_components, row_stride,
            //                 cinfo.out_color_space
            //              );

            while( cinfo.output_scanline < cinfo.output_height)
            {
                /* jpeg_read_scanlines expects an array of pointers to scanlines.
                 * Here the array is only one element long, but you could ask for
                 * more than one scanline at a time if that's more convenient.
                 */
                (void) jpeg_read_scanlines(&cinfo, nRowBuff, 1);
                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, cinfo.output_scanline * 10, 0x07E0, "CurrL:%i ", cinfo.output_scanline);
                (void) u8Heigh;
                // HAL_Delay(4000);
                for( uint8_t u8Index = 0U; u8Index < MATRIX_WIDTH; u8Index++ )
                {
                    r = (uint8_t) nRowBuff[0][ u8Index * 3 + 0 ];
                    g = (uint8_t) nRowBuff[0][ u8Index * 3 + 1 ];
                    b = (uint8_t) nRowBuff[0][ u8Index * 3 + 2 ];
#if 0
                    u16Color =  ((uint16_t) ((r << 8U) & 0xF800U)) |
                                ((uint16_t) ((g << 3U) & 0x07E0U)) |
                                ((uint16_t) ((b >> 3U) & 0x001FU));
#else
                    r = pgm_read_byte(&gamma_table[(r * 220) >> 8]); // Gamma correction table maps
                    g = pgm_read_byte(&gamma_table[(g * 220) >> 8]); // 8-bit input to 4-bit output
                    b = pgm_read_byte(&gamma_table[(b * 220) >> 8]);

                    u16Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                                (g <<  7) | ((g & 0xC) << 3) |
                                (b <<  1) | ( b        >> 3);

                    // u16Color =  ((uint16_t) ((r << 8U) & 0xF800U)) |
                    //             ((uint16_t) ((g << 3U) & 0x07E0U)) |
                    //             ((uint16_t) ((b >> 3U) & 0x001FU));
#endif
                    MATRIX_WritePixel( u8Index, cinfo.output_scanline - 1U, u16Color );
                }
                /* Assume put_scanline_someplace wants a pointer and sample count. */
                // MATRIX_DispImage( (const uint8_t *) buffer, 0U, cinfo.output_scanline, cinfo.output_width, 1U );
            }

            /* Step 7: Finish decompression */
            (void) jpeg_finish_decompress(&cinfo);

                /* Step 8: Release JPEG decompression object */
            /* This is an important step since it will release a good deal of memory. */
            jpeg_destroy_decompress(&cinfo);

            /* After finish_decompress, we can close the input file.
             * Here we postpone it until after no more JPEG errors are possible,
             * so as to simplify the setjmp error logic above.  (Actually, I don't
             * think that jpeg_destroy can do an error exit, but why assume anything...)
             */
            f_close(&SDFile);
        }
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "can't open !\n");
    }
}
#endif
// int bufftoint(char *buff)
// {
//     int x=buff[3];
//     x=x<<8;
//     x=x|buff[2];
//     x=x<<8;
//     x=x|buff[1];
//     x=x<<8;
//     x=x|buff[0];

//     return x;
// }

void SD_PlayVideos(void)
{
    (void) 1;
//     FRESULT fres;
//     FATFS   fs;
//     FIL     fsrc;
//     FILINFO fno;
//     DIR     dir;

//     uint16_t nobytsread;
//     int index=0;
//     extern __IO int audiotxcomplete;
//     extern __IO uint8_t readingvideo;
//     uint8_t pagebuff[65536];       //buffer to store frame
//     extern __IO uint8_t pagebuff2[];
//     extern __IO int pixx,pixy;   //pixy=caddr,pixx=raddr
//     struct jpeg_decompress_struct cinfo;

//     extern __IO uint8_t configured;
//     int framesize;
//     int frameno=0;
//     int movilocation;
//     int framelocation;
//     int audindex=0;

//     char tentimes=0;
//     char fstbufsent;

//     // (void) f_chdir("/mov");
//     fres=f_opendir(&dir,"/tes");
//     jpeg_create_decompress(&cinfo);
//     // int16_t audsample1[4410];
//     // int16_t audsample2[4410];
//     // player_init();
//     for(;;)
//     {
//         fres = f_readdir(&dir, &fno);
//         if (fres != FR_OK || fno.fname[0] == 0)
//             break;
//         if (fno.fname[0] == '.')
//             continue;
//         if (fno.fattrib & AM_DIR)
//         {
//             jpeg_destroy_decompress(&cinfo);
//             return;
//         }

//         fres=f_open(&fsrc,fno.fname, FA_OPEN_EXISTING | FA_READ);
//         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, " %s open !\n", fno.fname);
//         fres=f_read(&fsrc,pagebuff,4,&nobytsread);

//         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, " B: %i, P: %s\n", nobytsread, pagebuff);

//         while(pagebuff[0]!='m' || pagebuff[1]!='o'|| pagebuff[2]!='v'|| pagebuff[3]!='i')
//         {
//             index++;
//             fres=f_lseek(&fsrc,index); // pointer forward
//             fres=f_read(&fsrc,pagebuff,4,&nobytsread);
//             MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, " B: %i, P: %s\n", nobytsread, pagebuff);
//             HAL_Delay(1000);
//         }
//         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, " Index: %i open !\n", index);
//         index+=4;
//         movilocation=index;       //movi tag location set
//         index=fno.fsize;          //point to eof
//         while(pagebuff[0]!='i' || pagebuff[1]!='d'|| pagebuff[2]!='x'|| pagebuff[3]!='1')
//         {
//             index--;
//             fres=f_lseek(&fsrc,index); // pointer forward
//             fres=f_read(&fsrc,pagebuff,4,&nobytsread);
//         }
// nextframe:
//         while(pagebuff[0]!='0' || pagebuff[1]!='0'|| pagebuff[2]!='d'|| pagebuff[3]!='c')
//         {
//             index++;
//             fres=f_lseek(&fsrc,index); // pointer forward
//             fres=f_read(&fsrc,pagebuff,4,&nobytsread);
//         }
//         index+=8;          //4bytes from 00dc---->dwflag +4bytes from dwflag--->dwoffset=frame_loaction
//         fres=f_lseek(&fsrc,index);
//         fres=f_read(&fsrc,pagebuff,4,&nobytsread);
//         framelocation=movilocation+bufftoint(pagebuff);
//         f_lseek(&fsrc,framelocation);//length+data
//         fres=f_read(&fsrc,pagebuff,4,&nobytsread);
//         framesize=bufftoint(pagebuff);
//         framelocation+=4;  //only data length removed.
//         f_lseek(&fsrc,framelocation);//data     locating finished
// //          readingvideo=1;
//         fres=f_read(&fsrc,pagebuff,framesize,&nobytsread);
// //          readingvideo=0;

//         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, " Size: %i open !\n", framesize);
//         HAL_Delay(1000);
//         // send_frame(pagebuff,framesize);

//         f_lseek(&fsrc,index);

//         goto nextframe;
//     }
// // finished:
//     jpeg_destroy_decompress(&cinfo);
}

#if 1
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
    #define AVIF_WASCAPTUREFILE  0x00010000
    #define AVIF_COPYRIGHTED     0x00010000
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
   #define AVISF_VIDEO_PALCHANGES  0x00010000

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

#define MATRIXLED_X_COUNT 128
#define MATRIXLED_Y_COUNT 64

#define MATRIXLED_COLOR_COUNT 2
#define MATRIXLED_PWM_RESOLUTION 256

#define SPI_SEND_COMMAND_COUNT 4

#define SPI_DELAY_TIME_0 200
#define SPI_DELAY_TIME_1 1000

#define FONT_DATA_PIXEL_SIZE_X 5
#define FONT_DATA_PIXEL_SIZE_Y 8
#define FONT_DATA_MAX_CHAR 192
#define DISPLAY_TEXT_TIME 1000
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
#define MAX_TRACK_COUNT 100

#define SKIP_TIME 5

volatile unsigned char Video_End_Flag = RESET;
volatile unsigned char Audio_End_Flag = RESET;

float Volume_Value_float = 1;
unsigned char Status = PLAY;
unsigned int Audio_Flame_Data_Count;
unsigned char Audio_Double_Buffer;
unsigned char Audio_Channnel_Count;

volatile unsigned char Audio_Flame_End_flag = RESET;

signed short Audio_Buffer[2][MAX_AUDIO_SAMPLE_RATE * MAX_AUDIO_CHANNEL / MIN_VIDEO_FLAME_RATE] = {0};

unsigned char Flame_Buffer[MATRIXLED_Y_COUNT][MATRIXLED_X_COUNT][MATRIXLED_COLOR_COUNT];


void IRQ_DAC_ProcessAudio(void)
{
    static uint16_t audio_data_output_count;

    /* Clear interrupt flag */
    TIM6->SR = 0xFFFFFFFE;

    // if(htim->Instance == TIM14)
    // {
        // IR_Receive_Data = 0;
        // IR_Receive_Count = 0;
    // }

    // if(htim->Instance == TIM6)
    {
        if((Audio_End_Flag == RESET) && (Status == PLAY))
        {
            if(Audio_Channnel_Count == 1)
            {
                // HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_L, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count] * Volume_Value_float) + 0x8000) & 0x0000fff0));
                // HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_L, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count] * Volume_Value_float) + 0x8000) & 0x0000fff0));
                LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_1, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count] * Volume_Value_float) + 0x8000) & 0x0000fff0) );
                LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_2, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count] * Volume_Value_float) + 0x8000) & 0x0000fff0) );
                if(audio_data_output_count < ((Audio_Flame_Data_Count >> 1) - 1))
                {
                    audio_data_output_count++;
                }
                else
                {
                    Audio_Flame_End_flag = SET;
                    if(Audio_Double_Buffer == 0)
                    {
                        Audio_Double_Buffer = 1;
                    }
                    else
                    {
                        Audio_Double_Buffer = 0;
                    }
                    audio_data_output_count = 0;
                }
            }
            else if(Audio_Channnel_Count == 2)
            {
                // HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_L, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count + 0] * Volume_Value_float) + 0x8000) & 0x0000fff0));
                // HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_L, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count + 1] * Volume_Value_float) + 0x8000) & 0x0000fff0));
                LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_1, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count + 0] * Volume_Value_float) + 0x8000) & 0x0000fff0) );
                LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_2, ((unsigned int)((Audio_Buffer[Audio_Double_Buffer & 0x01][audio_data_output_count + 0] * Volume_Value_float) + 0x8000) & 0x0000fff0) );
                if(audio_data_output_count < ((Audio_Flame_Data_Count >> 1) - 2))
                {
                    audio_data_output_count += 2;
                }
                else
                {
                    Audio_Flame_End_flag = SET;
                    if(Audio_Double_Buffer == 0)
                    {
                        Audio_Double_Buffer = 1;
                    }else
                    {
                        Audio_Double_Buffer = 0;
                    }
                    audio_data_output_count = 0;
                }
            }
        }
        else
        {
            LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_1, (0x8000 & 0x0000fff0) );
            LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_2, (0x8000 & 0x0000fff0) );
            // HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_L, (0x8000 & 0x0000fff0));
            // HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_L, (0x8000 & 0x0000fff0));
            if(Audio_Channnel_Count == 1)
            {
                if(audio_data_output_count < ((Audio_Flame_Data_Count >> 1) - 1))
                {
                    audio_data_output_count++;
                }
                else
                {
                    Audio_Flame_End_flag = SET;
                    Audio_Double_Buffer = 0;
                    audio_data_output_count = 0;
                }
            }
            else if(Audio_Channnel_Count == 2)
            {
                if(audio_data_output_count < ((Audio_Flame_Data_Count >> 1) - 2))
                {
                    audio_data_output_count += 2;
                }
                else
                {
                    Audio_Flame_End_flag = SET;
                    Audio_Double_Buffer = 0;
                    audio_data_output_count = 0;
                }
            }
        }
    }
}

void pop_noise_reduction(void)
{
    unsigned int loop0, loop1;

    for(loop0 = 0;loop0 <= 0x8000;loop0++)
    {
        LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_1, (loop0 & 0x0000fff0));
        LL_DAC_ConvertData12LeftAligned( DAC1, LL_DAC_CHANNEL_2, (loop0 & 0x0000fff0));
        for(loop1 = 0;loop1 < 250;loop1++)
        {
            asm volatile(
                "nop"
            );
        }
    }
}

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

    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x1F, "Read HD %s..\r\n", play_info->file_name);
    // HAL_Delay(1000);
    // MATRIX_FillScreen(0x0);

    // char path[30] = "2.avi";
    // sprintf(path,"%s", "2.avi");
    // for( uint8_t i = 0; i < 3; i++ )
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0, 10 * i, 0xF800, "%d %d %d %d", play_info->file_name[4*i+0], play_info->file_name[4*i+1], play_info->file_name[4*i+2], play_info->file_name[4*i+3]);
    // }
    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 30, 0x7E0, "%s\r\n", (const TCHAR*) path);

    fatfs_result = f_open(&avi_fileobject, play_info->file_name, FA_READ);
    if(FR_OK != fatfs_result)
    {
        // printf("SD_ReadAviHeader f_open NG fatfs_result=%d\r\n", fatfs_result);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "f_open NG fatfs_result=%d\r\n", fatfs_result);
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        goto FATFS_ERROR_PROCESS;
    }
    else
    {
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x7E0, "f_open %s\n", (char*)(play_info->file_name));
        // HAL_Delay(500);
        // MATRIX_FillScreen(0x0);
        // while(1);
    }

    avi_fileobject.cltbl = linkmap_table;
    linkmap_table[0] = LINKMAP_TABLE_SIZE;
    fatfs_result = f_lseek(&avi_fileobject, CREATE_LINKMAP);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x7E0, "Link error");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("SD_ReadAviHeader create linkmap table NG fatfs_result=%d\r\n", fatfs_result);
        // printf("need linkmap table size %lu\r\n", linkmap_table[0]);
        goto FATFS_ERROR_PROCESS;
    }

    riff_chunk = (RIFFCHUNK *)malloc(sizeof(RIFFCHUNK));
    if(NULL == riff_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Malloc error");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("riff_chunk malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, riff_chunk, sizeof(RIFFCHUNK), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "RIFF f_read error");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("riff_chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    /*
    printf("RIFF�`�����Nｯ���q:%.4s\r\n", riff_chunk->fcc.fcc);
    printf("RIFF�`�����N�T�C�Y:%u\r\n", riff_chunk->cb);
    */

    avi_file_size = riff_chunk->cb + sizeof(RIFFCHUNK);

    if(strncmp((char*)riff_chunk->fcc.fcc, "RIFF", sizeof(FOURCC)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Not RIFF");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("this file is not RIFF format\r\n");
        goto FILE_ERROR_PROCESS;
    }

    four_cc = (FOURCC *)malloc(sizeof(FOURCC));
    if(NULL == four_cc)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "four_cc error");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("four_cc malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, four_cc, sizeof(FOURCC), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "four_cc read error");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("four_cc f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }
    /*
     printf("RIFF�`�����N�t�H�[���^�C�v:%.4s\r\n", four_cc->fcc);
    //*/
    if(strncmp((char*)four_cc->fcc, "AVI ", sizeof(FOURCC)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "RIFF not AVI");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("RIFF form type is not AVI \r\n");
        goto FILE_ERROR_PROCESS;
    }

    //hdrl���X�g����������
    riff_list = (RIFFLIST *)malloc(sizeof(RIFFLIST));
    if(NULL == riff_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "riff_list malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("riff_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, riff_list, sizeof(RIFFLIST), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "riff_list f_read");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
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
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found hdrl");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("could not find hdrl list\r\n");
        goto FILE_ERROR_PROCESS;
    }

    avi_main_header = (AVIMAINHEADER *)malloc(sizeof(AVIMAINHEADER));
    if(NULL == avi_main_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_main_header malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("avi_main_header malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, avi_main_header, sizeof(AVIMAINHEADER), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_main_header read");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("avi_main_header f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U, 0xFFF, "1. %.4s", avi_main_header->fcc.fcc);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U, 0xFFF, "2. %u", avi_main_header->cb );
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U, 0xFFF, "3. %u", avi_main_header->dwMicroSecPerFrame);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U, 0xFFF, "4. %u", avi_main_header->dwMaxBytesPerSec );
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U, 0xFFF, "5. %u", avi_main_header->dwPaddingGranularity);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U, 0xFFF, "6. %i", avi_main_header->dwFlags);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U, 0xFFF, "7. %u", avi_main_header->dwTotalFrames);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U, 0xFFF, "8. %u", avi_main_header->dwInitialFrames);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U, 0xFFF, "9. %u", avi_main_header->dwStreams);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U,  7U, 0xFFF, "1. %u", avi_main_header->dwSuggestedBufferSize);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U, 0xFFF, "2. %u", avi_main_header->dwWidth);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U, 0xFFF, "3. %u", avi_main_header->dwHeight);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 28U, 0xFFF, "4. %u", read_data_byte_result);
    // HAL_Delay(5000);
    // MATRIX_FillScreen(0x0);

    if(strncmp((char*)avi_main_header->fcc.fcc, "avih", sizeof(FOURCC)) != 0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found avi main header");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("could not find avi main header\r\n");
        goto FILE_ERROR_PROCESS;
    }
    if(AVIF_HASINDEX != (avi_main_header->dwFlags & AVIF_HASINDEX))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi not index");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("avi file does not have index\r\n");
        goto FILE_ERROR_PROCESS;
    }
    if((MATRIXLED_X_COUNT != avi_main_header->dwWidth) && (MATRIXLED_Y_COUNT != avi_main_header->dwHeight))
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wrong size");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("wrong size avi file\r\n");
        goto FILE_ERROR_PROCESS;
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
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_list malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("strl_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    avi_stream_header = (AVISTREAMHEADER *)malloc(sizeof(AVISTREAMHEADER));
    if(NULL == avi_stream_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi stream malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("avi_stream_header malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    strf_chunk = (RIFFCHUNK *)malloc(sizeof(RIFFCHUNK));
    if(NULL == strf_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strf_chunk malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("strf_chunk malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    bitmap_info_header = (BITMAPINFOHEADER *)malloc(sizeof(BITMAPINFOHEADER));
    if(NULL == bitmap_info_header)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("bitmap_info_header malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    wave_format_ex = (WAVEFORMATEX *)malloc(sizeof(WAVEFORMATEX));
    if(NULL == wave_format_ex)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wave_format_ex malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("wave_format_ex malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    do
    {
        fatfs_result = f_read(&avi_fileobject, strl_list, sizeof(RIFFLIST), &read_data_byte_result);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_list_f_read");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("strl_list f_read NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        if((strncmp((char*)strl_list->fcc.fcc, "LIST", sizeof(FOURCC)) == 0) && (strncmp((char*)strl_list->fccListType.fcc, "strl", sizeof(FOURCC)) == 0))
        {
            fatfs_result = f_read(&avi_fileobject, avi_stream_header, sizeof(AVISTREAMHEADER), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_stream_header f_read ");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
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
            // HAL_Delay(5000);
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
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_chunk f_read");
                                HAL_Delay(1000);
                                MATRIX_FillScreen(0x0);
                                // printf("strf_chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                            if(strncmp((char*)strf_chunk->fcc.fcc, "strf", sizeof(FOURCC)) == 0)
                            {
                                fatfs_result = f_read(&avi_fileobject, bitmap_info_header, sizeof(BITMAPINFOHEADER), &read_data_byte_result);
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header f_read");
                                    HAL_Delay(1000);
                                    MATRIX_FillScreen(0x0);
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
                                HAL_Delay(5000);
                                MATRIX_FillScreen(0x0); */

                                if((MATRIXLED_X_COUNT == bitmap_info_header->biWidth) && (MATRIXLED_Y_COUNT == bitmap_info_header->biHeight))
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
                                            // MATRIX_Printf( FONT_DEFAULT,  1U, 0U,  0U, 0x7E0, "video_stream read.");
                                            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, 21U,  0xFF, "1. %u\r\n", avi_stream_header->dwRate);
                                            // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, 28U,  0xFF, "2. %u\r\n", avi_stream_header->dwScale);
                                            // float test = 15.8467221615153;
                                            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 35U,  0xFF, "VidRate: %5.4f\n", test );
                                            // HAL_Delay(1000);
                                            // MATRIX_FillScreen(0x0);
                                            //printf("video_strem read success\r\n");
                                        }
                                        else
                                        {
                                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not BI_RGB: %i", bitmap_info_header->biCompression );
                                            HAL_Delay(1000);
                                            MATRIX_FillScreen(0x0);
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
                                        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not 24bpp");
                                        // HAL_Delay(1000);
                                        // MATRIX_FillScreen(0x0);
                                        // printf("video flame is not 24bpp\r\n");
                                    }
                                }
                                else
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wrong size video");
                                    HAL_Delay(1000);
                                    MATRIX_FillScreen(0x0);
                                    // printf("wrong size video stream\r\n");
                                }
                                fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(BITMAPINFOHEADER));
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header f_lseek");
                                    HAL_Delay(1000);
                                    MATRIX_FillScreen(0x0);
                                    // printf("bitmap_info_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                                }
                            }
                            else
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found strl_chunk");
                                HAL_Delay(1000);
                                MATRIX_FillScreen(0x0);
                                // printf("could not find strf_chunk\r\n");
                            }
                            fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(RIFFCHUNK));
                            if(FR_OK != fatfs_result)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_chunk f_lseek");
                                HAL_Delay(1000);
                                MATRIX_FillScreen(0x0);
                                // printf("strf_chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                        }
                        else
                        {
                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "out of flame range");
                            HAL_Delay(1000);
                            MATRIX_FillScreen(0x0);
                            // printf("video flame rate is out of range\r\n");
                        }
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "stream disabled");
                        HAL_Delay(1000);
                        MATRIX_FillScreen(0x0);
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
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strl_chunk f_read");
                                HAL_Delay(1000);
                                MATRIX_FillScreen(0x0);
                                // printf("strf_chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                            if(strncmp((char*)strf_chunk->fcc.fcc, "strf", sizeof(FOURCC)) == 0)
                            {
                                fatfs_result = f_read(&avi_fileobject, wave_format_ex, sizeof(WAVEFORMATEX), &read_data_byte_result);
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wave_format_ex f_read");
                                    HAL_Delay(1000);
                                    MATRIX_FillScreen(0x0);
                                    // printf("wave_format_ex f_read NG fatfs_result=%d\r\n", fatfs_result);
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
                                                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x7E0, "audio read!");
                                                // HAL_Delay(1000);
                                                // MATRIX_FillScreen(0x0);
                                                //printf("audio_strem read success\r\n");
                                            }
                                            else
                                            {
                                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not 16bit audio");
                                                HAL_Delay(1000);
                                                MATRIX_FillScreen(0x0);
                                                // printf("audio bits per sample is not 16bit\r\n");
                                            }
                                        }
                                        else
                                        {
                                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "out of audio range");
                                            HAL_Delay(1000);
                                            MATRIX_FillScreen(0x0);
                                            // printf("audio sample rate is out of range\r\n");
                                        }
                                    }
                                    else
                                    {
                                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio channels out range");
                                        HAL_Delay(1000);
                                        MATRIX_FillScreen(0x0);
                                        // printf("audio channels is out of range\r\n");
                                    }
                                }
                                else
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not PCM");
                                    HAL_Delay(1000);
                                    MATRIX_FillScreen(0x0);
                                    // printf("audio format is not linearPCM\r\n");
                                }

                                fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(WAVEFORMATEX));
                                if(FR_OK != fatfs_result)
                                {
                                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "bitmap_info_header f_lssek");
                                    HAL_Delay(1000);
                                    MATRIX_FillScreen(0x0);
                                    // printf("bitmap_info_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                    goto FATFS_ERROR_PROCESS;
                                }
                            }
                            else
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found strf_chunk");
                                HAL_Delay(1000);
                                MATRIX_FillScreen(0x0);
                                // printf("could not find strf_chunk\r\n");
                            }

                            fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(RIFFCHUNK));
                            if(FR_OK != fatfs_result)
                            {
                                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "strf_chunk f_lseek");
                                HAL_Delay(1000);
                                MATRIX_FillScreen(0x0);
                                // printf("strf_chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                                goto FATFS_ERROR_PROCESS;
                            }
                        }
                        else
                        {
                            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "out range audio sample");
                            HAL_Delay(1000);
                            MATRIX_FillScreen(0x0);
                            // printf("audio sample rate is out of range\r\n");
                        }
                    }
                    else
                    {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "stream disabled");
                        HAL_Delay(1000);
                        MATRIX_FillScreen(0x0);
                        // printf("stream is disabled\r\n");
                    }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not vid or audio");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
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
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio buff size wrong");

                    /* MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFF, "1. %u\r\n", wave_format_ex->nSamplesPerSec);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFF, "2. %u\r\n", wave_format_ex->nChannels);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFF, "3. %u\r\n", video_frame_rate);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFF, "4. %u\r\n", audio_suggest_buffer_size);
                    MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFF, "5. %u\r\n", wave_format_ex->nBlockAlign);
                    HAL_Delay(5000);
                    MATRIX_FillScreen(0x0); */

                    /* We will inrestigate it soon */
                    audio_stream_find_flag = RESET;
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("audio buffer size is wrong\r\n");
                }
            }
            else
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "wrong strh header");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("wrong strh header\r\n");
            }

            fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(AVISTREAMHEADER));
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_header_stream f_lseek");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("avi_stream_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            strl_list_find_loop_count++;
        }
        else
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not found strl list");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("could not find strl list\r\n");
        }

        fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + strl_list->cb - sizeof(FOURCC));
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_header_stream f_lseek");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("avi_stream_header f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x7E0, "avi_header_stream f_lseek done!");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }

        if(f_tell(&avi_fileobject) >= (riff_chunk->cb + sizeof(RIFFCHUNK) - 1))
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not find audio video str head");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("could not find playable audio video stream header\r\n");
            goto FILE_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x7E0, "Find str_header");
            // HAL_Delay(800);
            // MATRIX_FillScreen(0x0);
        }
    } while( (video_stream_find_flag != SET) || (audio_stream_find_flag != SET) );

    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0, 0x7E0, "%s header done!!!", (TCHAR*) play_info->file_name);
    // HAL_Delay(1000);
    // MATRIX_FillScreen(0x0);
    //printf("read video audio header success\r\n");
    //printf("%.4s\r\n", video_data_chunk_name.fcc);
    //printf("%.4s\r\n", audio_data_chunk_name.fcc);

    unknown_list = (RIFFLIST *)malloc(sizeof(RIFFLIST));
    if(NULL == unknown_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unknow malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("unknown_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    //movi���X�g�T��
    do
    {
        fatfs_result = f_read(&avi_fileobject, unknown_list, sizeof(RIFFLIST), &read_data_byte_result);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unkown_list f_read");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("unknown_list f_read NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        if((strncmp((char*)unknown_list->fcc.fcc, "LIST", sizeof(FOURCC)) == 0) && (strncmp((char*)unknown_list->fccListType.fcc, "movi", sizeof(FOURCC)) == 0)){
        fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) - sizeof(RIFFLIST));
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unknow_list f_lseek");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("unknown_list f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }

        }else{
        fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + unknown_list->cb - sizeof(FOURCC));
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "unknown_list f_lseek");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("unknown_list f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        }

        if(f_tell(&avi_fileobject) >= (riff_chunk->cb + sizeof(RIFFCHUNK) - 1))
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "not find movi list");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("could not find movi list\r\n");
            goto FILE_ERROR_PROCESS;
        }

    } while(strncmp((char*)unknown_list->fccListType.fcc, "movi", sizeof(FOURCC)) != 0);
    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "movi list found");
    // HAL_Delay(1000);
    // MATRIX_FillScreen(0x0);
    //printf("movi list found\r\n");

    movi_list = (RIFFLIST *)malloc(sizeof(RIFFLIST));
    if(NULL == movi_list)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "movi_list malloc err");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("movi_list malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    fatfs_result = f_read(&avi_fileobject, movi_list, sizeof(RIFFLIST), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "movi_list f_read");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("movi_list f_read NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    movi_list_position = f_tell(&avi_fileobject) - sizeof(FOURCC);

    fatfs_result = f_lseek(&avi_fileobject, f_tell(&avi_fileobject) + movi_list->cb - sizeof(FOURCC));
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "movi_list f_lseek");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("movi_list f_lseek NG fatfs_result=%d\r\n", fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    idx1_chunk = (RIFFCHUNK *)malloc(sizeof(RIFFCHUNK));
    if(NULL == idx1_chunk)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "idx1_chunk malloc");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // printf("idx1_chunk malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }
    fatfs_result = f_read(&avi_fileobject, idx1_chunk, sizeof(RIFFCHUNK), &read_data_byte_result);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "idx1_chunk f_read");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
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
    //free(avi_old_index);
    return FILE_OK;

    FILE_ERROR_PROCESS:
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "FILE_ERROR_PROCESS\n");
    HAL_Delay(1000);
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
    // printf("\r\n");
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "FATFS_ERROR_PROCESS\n");
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
    //free(avi_old_index);
    return FATFS_ERROR;

    OTHER_ERROR_PROCESS:
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "OTHER_ERROR_PROCESS\n");
    HAL_Delay(1000);
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
    return OTHER_ERROR;
}

READ_FILE_RESULT SD_GetPlayList(char *playlist_filename, PLAY_INFO **playlist, uint8_t *track_count)
{
    FILINFO playlist_fileinfo;
    FRESULT fatfs_result;
    FIL playlist_fileobject;

    uint8_t *token_pointer;
    uint8_t playlist_data[_MAX_LFN];
    uint8_t track_count_temp = 0;

    PLAY_INFO *playlist_temp0;
    PLAY_INFO *playlist_temp1;

    fatfs_result = f_stat(playlist_filename, &playlist_fileinfo);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_stat error %u\r\n", playlist_filename, fatfs_result);
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("%s f_stat error %u\r\n", playlist_filename, fatfs_result);
        //fatfs_result = f_open(&playlist_fileobject, playlist_filename, FA_WRITE|FA_OPEN_ALWAYS);
        //fatfs_result = f_write(&playlist_fileobject, Playlist_Default_Message, 461, NULL);
        //fatfs_result = f_close(&playlist_fileobject);
        goto FATFS_ERROR_PROCESS;
    }
    else
    {
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "%s f_stat done %u\r\n", playlist_filename, fatfs_result);
        // HAL_Delay(500);
        // MATRIX_FillScreen(0x0);
    }

    fatfs_result = f_open(&playlist_fileobject, (TCHAR*)&playlist_fileinfo.fname, FA_READ);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_open error %u\r\n", playlist_filename, fatfs_result);
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("%s f_open error %u\r\n", playlist_filename, fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }
    else
    {
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "%s f_open done %u\r\n", playlist_filename, fatfs_result);
        // HAL_Delay(500);
        // MATRIX_FillScreen(0x0);
    }

    while((f_gets((TCHAR*)playlist_data, (_MAX_LFN + 16), &playlist_fileobject) != NULL) && (track_count_temp <= MAX_TRACK_COUNT))
    {
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, track_count_temp * 9, 0x07E0, "L: %s\r\n", strtok((char*)playlist_data, "\r\n"));
        //printf("%s\r\n", strtok((char*)playlist_data, "\r\n"));
        track_count_temp++;
    }
    // HAL_Delay(500);
    // MATRIX_FillScreen(0x0);
    //printf("\r\n");
    if(0 == track_count_temp)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "1. item doesn't exist in playlist\r\n");
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("item does not exist in playlist\r\n");
        goto FILE_ERROR_PROCESS;
    }

    fatfs_result = f_lseek(&playlist_fileobject, 0);
    if(FR_OK != fatfs_result)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_lseek error %u\r\n", playlist_filename, fatfs_result);
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("%s f_lseek error %u\r\n", playlist_filename, fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    playlist_temp0 = (PLAY_INFO *)malloc(sizeof(PLAY_INFO) * track_count_temp);
    if(NULL == playlist_temp0)
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "malloc error\r\n");
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("malloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    track_count_temp = 0;
    while( (f_gets((TCHAR*)playlist_data, (_MAX_LFN + 16), &playlist_fileobject) != NULL) && (track_count_temp <= MAX_TRACK_COUNT))
    {
        token_pointer = (uint8_t*)strtok((char*)playlist_data, ",\r\n");

        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x7E0, "Read %s, 0x%X\r\n", playlist_data, token_pointer);
        // HAL_Delay(500);
        // MATRIX_FillScreen(0x0);
        if((token_pointer != NULL) && (strncmp((char*)playlist_data, "//", 2) != 0))
        {
            strcpy((char*)playlist_temp0[track_count_temp].file_name, (char*)token_pointer);
            if(FILE_OK == SD_ReadAviHeader(&playlist_temp0[track_count_temp]))
            {
                track_count_temp++;
            }
        }
        else
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Read %s error\r\n", playlist_data);
            HAL_Delay(500);
            MATRIX_FillScreen(0x0);
        }
    }

    if(0 == track_count_temp)
    {
        free(playlist_temp0);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "item does not exist in playlist\r\n");
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("item does not exist in playlist\r\n");
        goto FILE_ERROR_PROCESS;
    }

    playlist_temp1 = (PLAY_INFO *)realloc(playlist_temp0, (sizeof(PLAY_INFO) * track_count_temp));
    if(NULL == playlist_temp1){
        free(playlist_temp0);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "realloc error\r\n");
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("realloc error\r\n");
        goto OTHER_ERROR_PROCESS;
    }

    fatfs_result = f_close(&playlist_fileobject);
    if(FR_OK != fatfs_result){
        free(playlist_temp1);
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "%s f_close error %u\r\n", playlist_filename, fatfs_result);
        HAL_Delay(500);
        MATRIX_FillScreen(0x0);
        // printf("%s f_close error %u\r\n", playlist_filename, fatfs_result);
        goto FATFS_ERROR_PROCESS;
    }

    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Track: %i\r\n", track_count_temp);
    // HAL_Delay(1000);

    //  for(int testloop = 0;testloop < track_count_temp;testloop++)
    //  {
    //     float fCurrentFps = playlist_temp1[testloop].avi_info.video_frame_rate;
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U,  7U,  0xFFF, "%s\r\n", playlist_temp1[testloop].file_name);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 14U,  0xFFF, "%d.%5fFPS\r\n", (uint32_t) fCurrentFps, (uint32_t)((fCurrentFps - (uint32_t) fCurrentFps) * 100000) );
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 21U,  0xFFF, "%lu\r\n", playlist_temp1[testloop].avi_info.video_length);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 28U,  0xFFF, "%.4s\r\n", playlist_temp1[testloop].avi_info.video_data_chunk_name.fcc);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 35U,  0xFFF, "%u\r\n", playlist_temp1[testloop].avi_info.audio_channels);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 42U,  0xFFF, "%luHz\r\n", playlist_temp1[testloop].avi_info.audio_sampling_rate);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 49U,  0xFFF, "%lu\r\n", playlist_temp1[testloop].avi_info.audio_length);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 56U,  0xFFF, "%.4s\r\n", playlist_temp1[testloop].avi_info.audio_data_chunk_name.fcc);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 63U,  0xFFF, "%lu\r\n", playlist_temp1[testloop].avi_info.avi_streams_count);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 14U,  0xFFF, "movi:0x%08lx\r\n", playlist_temp1[testloop].avi_info.movi_list_position);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 21U,  0xFFF, "idx1:0x%08lx\r\n", playlist_temp1[testloop].avi_info.avi_old_index_position);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 28U,  0xFFF, "idx1:0x%08lx\r\n", playlist_temp1[testloop].avi_info.avi_old_index_size);
    //     MATRIX_Printf( FONT_TOMTHUMB, 1U, 42U, 35U,  0xFFF, "AVI:%luB\r\n", playlist_temp1[testloop].avi_info.avi_file_size);
    //     HAL_Delay(4000);
    //     MATRIX_FillScreen(0x0);
    // }

    (*playlist) = playlist_temp1;
    (*track_count) = track_count_temp;
    return FILE_OK;

    FILE_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "FILE_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        (*playlist) = NULL;
        (*track_count) = 0;
    return FILE_ERROR;

    FATFS_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "FATFS_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        (*playlist) = NULL;
        (*track_count) = 0;
    return FATFS_ERROR;

    OTHER_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "OTHER_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        (*playlist) = NULL;
        (*track_count) = 0;
    return OTHER_ERROR;
};

READ_FILE_RESULT SD_ReadAviStream(PLAY_INFO *play_info, uint32_t read_frame_count)
{
    static FIL avi_fileobject;
    FRESULT fatfs_result;
    UINT read_data_byte_result;
    static uint32_t linkmap_table[LINKMAP_TABLE_SIZE];

    aIndex avi_old_index_0, avi_old_index_1;
    CHUNKHEADER stream_chunk_header;

    uint32_t read_frame_count_temp;
    uint32_t idx1_search_loop;

    static int8_t previous_filename[_MAX_LFN];

    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "FN: %.4s %i\n", play_info->file_name, read_frame_count);
    // float fCurrentFps = play_info->avi_info.video_frame_rate;
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 17U,  0xFFF, "%s\r\n", play_info->file_name);
    // MATRIX_Printf( FONT_TOMTHUMB, 1U,  0U, 24U,  0xFFF, "%d.%5fFPS\r\n", (uint32_t) fCurrentFps, (uint32_t)((fCurrentFps - (uint32_t) fCurrentFps) * 100000) );
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
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_avi_stream f_open\r\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("read_avi_stream f_open NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFFU, "read_avi_stream f_open\r\n");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }

        avi_fileobject.cltbl = linkmap_table;
        linkmap_table[0] = LINKMAP_TABLE_SIZE;
        fatfs_result = f_lseek(&avi_fileobject, CREATE_LINKMAP);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_avi_header linkmap\r\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("read_avi_header create linkmap table NG fatfs_result=%d\r\n", fatfs_result);
            // printf("need linkmap table size %lu\r\n", linkmap_table[0]);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFFU, "read_avi_header linkmap\r\n");
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
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_index f_lseek\r\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("read_index f_lseek NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFFU, "read_index f_lseek\r\n");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }
        fatfs_result = f_read(&avi_fileobject, &avi_old_index_0, sizeof(aIndex), &read_data_byte_result);
        if(FR_OK != fatfs_result)
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_old_idx f_read\r\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
            // printf("avi_old_index f_read NG fatfs_result=%d\r\n", fatfs_result);
            goto FATFS_ERROR_PROCESS;
        }
        else
        {
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFFU, "avi_old_idx f_read\r\n");
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);
        }

        if(strncmp((char*)avi_old_index_0.dwChunkId.fcc, (char*)play_info->avi_info.video_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
        {
            fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_0.dwOffset + play_info->avi_info.movi_list_position));
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame chunk f_lseek\r\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("read video flame chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFFU, "read vid flame chunk f_lseek\r\n");
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);
            }
            fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame chunk f_read\r\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("read video flame chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFFU, "read vid flame chunk f_read\r\n");
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
                fatfs_result = f_read(&avi_fileobject, Flame_Buffer, (MATRIXLED_Y_COUNT * MATRIXLED_X_COUNT * MATRIXLED_COLOR_COUNT), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }
            }
            else
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "vid flame chunk wrong\r\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("video flame chunk id is wrong\r\n");
            }
        }

        if(strncmp((char*)avi_old_index_0.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
        {
            Audio_Flame_Data_Count = avi_old_index_0.dwSize;
        }
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "Parse Frame: %i\n", read_frame_count_temp);
        // HAL_Delay(1000);
        // MATRIX_FillScreen(0x0);

        if(0 == read_frame_count_temp)
        {
            if(strncmp((char*)avi_old_index_0.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
            {
                fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_0.dwOffset + play_info->avi_info.movi_list_position));
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio chunk f_lseek\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("read audio data chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read audio chunk f_lseek\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }
                fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio chunk f_read\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("read audio data chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read audio chunk f_read\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }

                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "ChunkID: %.4s\nChunkSize: %i", stream_chunk_header.chunkID, stream_chunk_header.chunkSize );
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);

                if(strncmp((char*)stream_chunk_header.chunkID , (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
                {
                    // fatfs_result = f_read(&avi_fileobject, &Audio_Buffer[0][0], stream_chunk_header.chunkSize, &read_data_byte_result);
                    // if(FR_OK != fatfs_result)
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    //     // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                    //     goto FATFS_ERROR_PROCESS;
                    // }
                    // else
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    // }
                    Audio_Double_Buffer = 0;
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio dat chunk wrong\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                // printf("audio data chunk id is wrong\r\n");
                }
            }

            fatfs_result = f_lseek(&avi_fileobject, (play_info->avi_info.avi_old_index_position + sizeof(aIndex) * (play_info->avi_info.avi_streams_count * (read_frame_count_temp + 1) + idx1_search_loop)));
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_idx f_lseek\r\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("read_index f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read_idx f_lseek\r\n");
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);
            }
            fatfs_result = f_read(&avi_fileobject, &avi_old_index_1, sizeof(aIndex), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_old_idx f_read\r\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("avi_old_index f_read NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            else
            {
                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "avi_old_idx f_read\r\n");
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);
            }

            if(strncmp((char*)avi_old_index_1.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
            {
                fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_1.dwOffset + play_info->avi_info.movi_list_position));
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio dat chunk f_lseek\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("read audio data chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read audio dat chunk f_lseek\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }
                fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio data chunk f_read\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("read audio data chunk f_read NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                else
                {
                    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "Read audio data chunk f_read\r\n");
                    // HAL_Delay(1000);
                    // MATRIX_FillScreen(0x0);
                }

                // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "ChunkID: %.4s\nChunkSize: %i", stream_chunk_header.chunkID, stream_chunk_header.chunkSize );
                // HAL_Delay(1000);
                // MATRIX_FillScreen(0x0);

                if(strncmp((char*)stream_chunk_header.chunkID , (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
                {
                    // fatfs_result = f_read(&avi_fileobject, &Audio_Buffer[1][0], stream_chunk_header.chunkSize, &read_data_byte_result);
                    // if(FR_OK != fatfs_result)
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    //     // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                    //     goto FATFS_ERROR_PROCESS;
                    // }
                    // else
                    // {
                    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xFFFF, "read vid flame f_read\r\n");
                    //     HAL_Delay(1000);
                    //     MATRIX_FillScreen(0x0);
                    // }
                }
                else
                {
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio dat chunk wrong\r\n");
                        HAL_Delay(1000);
                        MATRIX_FillScreen(0x0);
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
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read_idx f_lseek\r\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("read_index f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }
            fatfs_result = f_read(&avi_fileobject, &avi_old_index_1, sizeof(aIndex), &read_data_byte_result);
            if(FR_OK != fatfs_result)
            {
                MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "avi_old_idx f_read 3\r\n");
                HAL_Delay(1000);
                MATRIX_FillScreen(0x0);
                // printf("avi_old_index f_read NG fatfs_result=%d\r\n", fatfs_result);
                goto FATFS_ERROR_PROCESS;
            }

            if(strncmp((char*)avi_old_index_1.dwChunkId.fcc, (char*)play_info->avi_info.audio_data_chunk_name.fcc, sizeof(FOURCC)) == 0)
            {
                fatfs_result = f_lseek(&avi_fileobject, (avi_old_index_1.dwOffset + play_info->avi_info.movi_list_position));
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read audio dat chunk f_lseek as\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("read audio data chunk f_lseek NG fatfs_result=%d\r\n", fatfs_result);
                    goto FATFS_ERROR_PROCESS;
                }
                fatfs_result = f_read(&avi_fileobject, &stream_chunk_header, sizeof(CHUNKHEADER), &read_data_byte_result);
                if(FR_OK != fatfs_result)
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read au dat chunk f_read\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
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
                        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "read vid flame f_read NSS\r\n");
                        HAL_Delay(1000);
                        MATRIX_FillScreen(0x0);
                        // printf("read video flame f_read NG fatfs_result=%d\r\n", fatfs_result);
                        goto FATFS_ERROR_PROCESS;
                    }
                }
                else
                {
                    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "audio dat chunk wrong\r\n");
                    HAL_Delay(1000);
                    MATRIX_FillScreen(0x0);
                    // printf("audio data chunk id is wrong\r\n");
                }
            }
        }
    }

    // __HAL_TIM_ENABLE(&htim6);
    memmove(previous_filename, play_info->file_name, _MAX_LFN);
    return FILE_OK;

    FATFS_ERROR_PROCESS:
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "FATFS_ERROR_PROCESS\r\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // snprintf((char*)previous_filename, _MAX_LFN, "*");
    return FATFS_ERROR;
}


void SD_PlayAviVideo(void)
{
    uint8_t r, g, b;
    uint32_t u32Color;

    PLAY_INFO *playlist;

    // unsigned char playlist_count = 0;
    unsigned char track_count = 0;
    unsigned char all_track_count = 0;
    unsigned int frame_count = 0;
    char playlist_filename[_MAX_LFN] = "list0.csv";

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

    (void) 1U;
    CLOCK_DisableAllIrq();
    SD_GetPlayList(playlist_filename, &playlist, &all_track_count);
    CLOCK_EnableAllIrq();
    MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "Get Playlist done!\r\nTrackCount: %i", all_track_count );
    HAL_Delay(500);
    MATRIX_FillScreen(0x0);

    SD_ReadAviStream(&playlist[0], 0);
    // display_text_flame_count = 0;
    uint32_t u32PreTickFps = 0UL;
    float fCurrkFps = 0;

    uint32_t u32AfterDelay = 0UL;
    uint32_t u32CurrTick = 0UL;
    uint32_t u32PreTick = 0UL;
    while(1)
    {
        flame_rate = playlist[track_count].avi_info.video_frame_rate;
        // display_text_flame = (unsigned short)(flame_rate * 1000) / DISPLAY_TEXT_TIME;
        u32PreTick = HAL_GetTick();
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

            Video_End_Flag = RESET;
            Audio_End_Flag = RESET;
        }
        // MATRIX_DispImage( Flame_Buffer, 0U, 0U, 128U, 64U );
        for( uint8_t u8xIdx = 0U; u8xIdx < MATRIX_WIDTH; u8xIdx++ )
        {
            for( uint8_t u8yIdx = 0U; u8yIdx < MATRIX_HEIGHT; u8yIdx++)
            {
#if defined(RGB444) /* RGB444 */
                r =  (Flame_Buffer[u8yIdx][u8xIdx][1] >> 3U) & 0x1F;
                g = ((Flame_Buffer[u8yIdx][u8xIdx][1] << 2U) & 0x1C) | ((Flame_Buffer[u8yIdx][u8xIdx][0] >> 6U) & 0x3);
                b = Flame_Buffer[u8yIdx][u8xIdx][0] & 0x1F;
                // r = pgm_read_byte(&gamma_table[(r * 150) >> 5]); // Gamma correction table maps
                // g = pgm_read_byte(&gamma_table[(g * 150) >> 5]); // 5-bit input to 4-bit output
                // b = pgm_read_byte(&gamma_table[(b * 150) >> 5]);
                // u32Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                //             (g <<  7) | ((g & 0xC) << 3) |
                //             (b <<  1) | ( b        >> 3);
                u32Color = ((r << 10) & 0x7C00) |
                           ((g <<  4) &  0x3E0) |
                           ((b <<  0) &   0x1F);
#elif defined(RGB555) /* RGB555 */
                r =  (Flame_Buffer[u8yIdx][u8xIdx][1] >> 3U) & 0x1F;
                g = ((Flame_Buffer[u8yIdx][u8xIdx][1] << 2U) & 0x1C) | ((Flame_Buffer[u8yIdx][u8xIdx][0] >> 6U) & 0x3);
                b =   Flame_Buffer[u8yIdx][u8xIdx][0] & 0x1F;

                r = pgm_read_byte(&gamma_table[(r * 220) >> 5]); // Gamma correction table maps
                g = pgm_read_byte(&gamma_table[(g * 220) >> 5]); // 5-bit input to 4-bit output
                b = pgm_read_byte(&gamma_table[(b * 220) >> 5]);
                u32Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                            (g <<  7) | ((g & 0xC) << 3) |
                            (b <<  1) | ( b        >> 3);
#elif defined(RGB565) /* RGB565 */
//                 r =  (Flame_Buffer[u8yIdx][u8xIdx][1] >> 3U) & 0x1F;
//                 g = ((Flame_Buffer[u8yIdx][u8xIdx][1] << 3U) & 0x38) | ((Flame_Buffer[u8yIdx][u8xIdx][0] >> 5U) & 0x7);
//                 b =   Flame_Buffer[u8yIdx][u8xIdx][0] & 0x1F;
                r =  (Flame_Buffer[u8yIdx][u8xIdx][1] >> 3U) & 0x1F;
                g = ((Flame_Buffer[u8yIdx][u8xIdx][1] << 2U) & 0x1C) | ((Flame_Buffer[u8yIdx][u8xIdx][0] >> 6U) & 0x3);
                b =   Flame_Buffer[u8yIdx][u8xIdx][0] & 0x1F;
                r = pgm_read_byte(&gamma_table[(r * 255) >> 5]); // Gamma correction table maps
                g = pgm_read_byte(&gamma_table[(g * 255) >> 5]); // 6-bit input to 4-bit output
                b = pgm_read_byte(&gamma_table[(b * 255) >> 5]);
                u32Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                            (g <<  7) | ((g & 0xC) << 3) |
                            (b <<  1) | ( b        >> 3);
                // u32Color = ((r << 11)&0xF800) |
                //            ((g <<  5)&0x7E0) |
                //            ((b <<  0)&0x1F);
#endif
                MATRIX_WritePixel( u8xIdx, MATRIX_HEIGHT - u8yIdx - 1, u32Color, 1U );
            }
        }
        MATRIX_Printf( FONT_DEFAULT, 1U, 85, 56, 0xFFFF, "%i.%ifPs", (uint32_t) fCurrkFps, (uint32_t) ((fCurrkFps - (uint32_t) fCurrkFps) * 10) );
        MATRIX_UpdateScreen();
        u32CurrTick = HAL_GetTick();
        OFF_LED();
        while(Audio_Flame_End_flag == RESET);
        Audio_Flame_End_flag = RESET;
        // HAL_Delay(1000/flame_rate - (u32CurrTick - u32PreTick) - 1U);
        u32AfterDelay = HAL_GetTick();

        if( u32AfterDelay - u32PreTickFps > 200)
        {
            u32PreTickFps = u32AfterDelay;
            fCurrkFps = 1000.0/(u32AfterDelay - u32PreTick);
        }
        // HAL_Delay(20);
        ON_LED();
        // HAL_Delay(1000/flame_rate);
    }
}
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
    // HAL_SD_CardInfoTypeDef CardInfo;
    // FATFS *pfs;
    // DWORD fre_clust;

  /* USER CODE END 1 */


  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* Use systick as time base source and configure 1ms tick (default clock after Reset is HSI) */
  HAL_InitTick(TICK_INT_PRIORITY);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();
  MX_LIBJPEG_Init();
  MX_SDIO_SD_Init();
  MX_DAC_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

    uint8_t u8Reg = 0U;

    HAL_I2C_Mem_Read( &hi2c2, DS3231_ADDRESS, DS3231_REG_CONTROL, I2C_MEMADD_SIZE_8BIT, &u8Reg, 1, 1000);

    /* clear all relevant bits to a known "off" state */
    u8Reg &= ~( DS3231_AIEMASK | DS3231_BBSQW );
    /* clear INTCN to enable clock SQW */
    u8Reg &= ~DS3231_INTCN;
    /* set enable int/sqw while in battery backup flag */
    u8Reg |= DS3231_BBSQW;

    HAL_I2C_Mem_Write( &hi2c2, DS3231_ADDRESS, DS3231_REG_CONTROL, I2C_MEMADD_SIZE_8BIT, &u8Reg, 1, 1000);

    // HAL_I2C_Mem_Write(&hi2c2, DS3231_ADDRESS, DS3231_REG_TIMEDATE, I2C_MEMADD_SIZE_8BIT, receive_data, 7, 1000);

    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);

    pop_noise_reduction();

    LL_TIM_EnableIT_UPDATE(TIM6);
    LL_TIM_EnableCounter(TIM6);

    MATRIX_setBrightness( 70 );
    LL_TIM_CC_EnableChannel(TIM_OE, LL_TIM_CHANNEL_CH3);
    LL_TIM_OC_SetCompareCH3(TIM_OE, MATRIX_getBrightness());
    LL_TIM_EnableCounter(TIM_OE);

    LL_TIM_EnableIT_UPDATE(TIM_DAT);
    LL_TIM_EnableCounter(TIM_DAT);

    LL_TIM_CC_EnableChannel( TIM_IR, LL_TIM_CHANNEL_CH1 );
    LL_TIM_EnableIT_CC1( TIM_IR );
    LL_TIM_EnableCounter( TIM_IR );

    CLOCK_InitData();
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0, 0, 0xF81F, "%04X ", FLASH_Read(1) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 25, 0, 0xF81F, "%04X ", FLASH_Read(2) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 50, 0, 0xF81F, "%04X ", FLASH_Read(3) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 75, 0, 0xF81F, "%04X ", FLASH_Read(4) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 100, 0, 0xF81F, "%04X ", FLASH_Read(5) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0, 10, 0xF81F, "%04X ", FLASH_Read(6) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 25, 10, 0xF81F, "%04X ", FLASH_Read(7) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 50, 10, 0xF81F, "%04X ", FLASH_Read(8) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 75, 10, 0xF81F, "%04X ", FLASH_Read(9) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 100, 10, 0xF81F, "%04X ", FLASH_Read(10) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0, 20, 0xF81F, "%04X ", FLASH_Read(11) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 25, 20, 0xF81F, "%04X ", FLASH_Read(12) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, 50, 20, 0xF81F, "%04X ", FLASH_Read(13) );
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    // char test[30];
    // sprintf(test, "%.5f", 21.35428151);
    // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 35U,  0xFFFF, "VidRate: %s\n", test);

    if( MSD_ERROR != BSP_SD_Init() )
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x1485, "SdCard Init!\n");
        // HAL_Delay(500);
        // MATRIX_FillScreen(0x0);
        // BSP_SD_GetCardInfo(&CardInfo);
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x1485, " - Type:  %7u\n - Ver:   %7u\n - Class: %7u\n - Add:   %7u\n - BNbr:  %7u\n - Bsize: %7u\n - LNbr:  %7u\n - LSize: %7u\n", CardInfo.CardType, CardInfo.CardVersion, CardInfo.Class, CardInfo.RelCardAdd, CardInfo.BlockNbr, CardInfo.BlockSize, CardInfo.LogBlockNbr, CardInfo.LogBlockSize);
        // HAL_Delay(500);
        // MATRIX_FillScreen(0x0);

        CLOCK_DisableAllIrq();
        if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK)
        {
            CLOCK_EnableAllIrq();

            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "SdCard Mounted!\n");
            HAL_Delay(1000);
            // f_getfree("", &fre_clust, &pfs);
            MATRIX_FillScreen(0x0);
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "Total Size: \t%luMb\n",(uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5)/1024);
            // // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "Free Space: \t%luMb\n",(uint32_t)(fre_clust * pfs->csize * 0.5)/1024);
            // HAL_Delay(500);
            // MATRIX_FillScreen(0x0);
            // SD_ScanFiles(SDPath);
            // SDResult = f_readdir(&dir, &fno);                   /* Read a directory item */
            // HAL_Delay(1000);
            // MATRIX_FillScreen(0x0);

            // SD_DisplayJpeg("img/natural.jpg");
            // HAL_Delay(2000);
            // SD_DisplayJpeg("img/anime.jpg");
            // HAL_Delay(2000);
            // SD_DisplayJpeg("img/phung.jpg");
            // HAL_Delay(2000);
            // SD_DisplayJpeg("img/total.jpg");
            // HAL_Delay(1500);
        }
        else
        {
            CLOCK_EnableAllIrq();
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "Mount Failed!\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
        }
    }
    else
    {
            CLOCK_EnableAllIrq();
            MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Init Failed!\n");
            HAL_Delay(1000);
            MATRIX_FillScreen(0x0);
    }

    // SD_PlayMotionJpeg("vid/2.avi");
    // SD_PlayVideos();
    SD_PlayAviVideo();
    // PlayMotionJpeg( 0U );
    while(1)
    {
        // for( uint16_t u16Index = 1; u16Index < 7328; u16Index++ )
        // {
            // char path[30];
            // sprintf(path, "2/%04i.jpg",u16Index);
            // SD_DisplayJpeg(path);
        // }
        // HAL_Delay(1000);
    };

    while( 1 )
    {
        // ON_LED();
        // // LL_mDelay(500);
        // OFF_LED();
        // LL_mDelay(500);
        CLOCK_BasicMonitor();

        switch( IR1838_GetCurrentKey() )
        {
            case IR_KEY_MENU:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                CLOCK_Setting();
                break;
        default:
            break;
        }
        CLOCK_UpdateKeyCode();
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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3)
  {
  Error_Handler();
  }
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_HSE_EnableCSS();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_16, 320, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_16, 320, LL_RCC_PLLQ_DIV_4);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_SetSystemCoreClock(160000000);
}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  LL_DAC_InitTypeDef DAC_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DAC1);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**DAC GPIO Configuration
  PA4   ------> DAC_OUT1
  PA5   ------> DAC_OUT2
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* DAC interrupt Init */
  NVIC_SetPriority(TIM6_DAC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(TIM6_DAC_IRQn);

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */
  /** DAC channel OUT1 config
  */
  DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
  DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_ENABLE;
  LL_DAC_Init(DAC, LL_DAC_CHANNEL_1, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC, LL_DAC_CHANNEL_1);
  /** DAC channel OUT2 config
  */
  LL_DAC_Init(DAC, LL_DAC_CHANNEL_2, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC, LL_DAC_CHANNEL_2);
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
  hsd.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  /**TIM3 GPIO Configuration
  PC6   ------> TIM3_CH1
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* TIM3 interrupt Init */
  NVIC_SetPriority(TIM3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
  NVIC_EnableIRQ(TIM3_IRQn);

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  TIM_InitStruct.Prescaler = 79;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 65535;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM3, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM3);
  LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM3);
  LL_TIM_IC_SetActiveInput(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_DIRECTTI);
  LL_TIM_IC_SetPrescaler(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_ICPSC_DIV1);
  LL_TIM_IC_SetFilter(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV1);
  LL_TIM_IC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_FALLING);
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
  NVIC_SetPriority(TIM4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),4, 0));
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
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM5);

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  TIM_InitStruct.Prescaler = 1;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 100;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM5, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM5);
  LL_TIM_SetClockSource(TIM5, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_OC_EnablePreload(TIM5, LL_TIM_CHANNEL_CH3);
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = 0;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_LOW;
  LL_TIM_OC_Init(TIM5, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);
  LL_TIM_OC_DisableFast(TIM5, LL_TIM_CHANNEL_CH3);
  LL_TIM_SetTriggerOutput(TIM5, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM5);
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**TIM5 GPIO Configuration
  PA2   ------> TIM5_CH3
  */
  GPIO_InitStruct.Pin = OE_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(OE_GPIO_Port, &GPIO_InitStruct);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);

  /* TIM6 interrupt Init */
  NVIC_SetPriority(TIM6_DAC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),8, 0));
  NVIC_EnableIRQ(TIM6_DAC_IRQn);

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  TIM_InitStruct.Prescaler = 0;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 2448;
  LL_TIM_Init(TIM6, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM6);
  LL_TIM_SetTriggerOutput(TIM6, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM6);
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

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
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);

  /**/
  LL_GPIO_ResetOutputPin(LA_GPIO_Port, LA_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LB_GPIO_Port, LB_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LC_GPIO_Port, LC_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LD_GPIO_Port, LD_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LED_GPIO_Port, LED_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LE_GPIO_Port, LE_Pin);

  /**/
  LL_GPIO_ResetOutputPin(R2_GPIO_Port, R2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(G2_GPIO_Port, G2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(B2_GPIO_Port, B2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(B4_GPIO_Port, B4_Pin);

  /**/
  LL_GPIO_ResetOutputPin(G4_GPIO_Port, G4_Pin);

  /**/
  LL_GPIO_ResetOutputPin(R1_GPIO_Port, R1_Pin);

  /**/
  LL_GPIO_ResetOutputPin(B1_GPIO_Port, B1_Pin);

  /**/
  LL_GPIO_ResetOutputPin(G1_GPIO_Port, G1_Pin);

  /**/
  LL_GPIO_ResetOutputPin(R3_GPIO_Port, R3_Pin);

  /**/
  LL_GPIO_ResetOutputPin(G3_GPIO_Port, G3_Pin);

  /**/
  LL_GPIO_ResetOutputPin(B3_GPIO_Port, B3_Pin);

  /**/
  LL_GPIO_ResetOutputPin(R4_GPIO_Port, R4_Pin);

  /**/
  LL_GPIO_SetOutputPin(CLK_GPIO_Port, CLK_Pin);

  /**/
  LL_GPIO_SetOutputPin(LAT_GPIO_Port, LAT_Pin);

  /**/
  GPIO_InitStruct.Pin = LA_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LA_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LB_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LB_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LC_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LC_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LD_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LD_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = CLK_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(CLK_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LAT_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LAT_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = UPD_KEY_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(UPD_KEY_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LE_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LE_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = R2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(R2_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = G2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(G2_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = B2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(B2_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = B4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(B4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = G4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(G4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = SD_CD_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(SD_CD_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = R1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(R1_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = G1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(G1_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = R3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(R3_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = G3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(G3_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = B3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(B3_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = R4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(R4_GPIO_Port, &GPIO_InitStruct);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE7);

  /**/
  LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_7, LL_GPIO_PULL_NO);

  /**/
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_7, LL_GPIO_MODE_INPUT);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_7;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /* EXTI interrupt init*/
  NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),5, 0));
  NVIC_EnableIRQ(EXTI9_5_IRQn);

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
