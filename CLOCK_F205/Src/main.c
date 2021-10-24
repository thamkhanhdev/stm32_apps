/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyu16RightPos (c) 2020 STMicroelectronics.
  * All u16RightPoss reserved.</center></h2>
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
#include "clock.h"
#include "amlich.h"
#include "avistream.h"
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

/* u8Seconds accuracy */
enum DS3231AlarmOneControl
{
   /* bit order:  A1M4  DY/DT  A1M3  A1M2  A1M1 */
    DS3231AlarmOneControl_Hoursu8Minutesu8SecondsDayOfMonthMatch = 0x00,
    DS3231AlarmOneControl_OncePeru8Second = 0x17,
    DS3231AlarmOneControl_u8SecondsMatch = 0x16,
    DS3231AlarmOneControl_u8Minutesu8SecondsMatch = 0x14,
    DS3231AlarmOneControl_Hoursu8Minutesu8SecondsMatch = 0x10,
    DS3231AlarmOneControl_Hoursu8Minutesu8SecondsDayOfWeekMatch = 0x08,
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

uint8_t u8Second,u8Minute,u8Hour,u8Day,u8Date,u8Month,u8Year;
uint8_t u8LastMin=255U;
uint32_t u32CursorTick = 0UL;
uint32_t u32BackgroundTick = 0UL;
uint32_t u32TextTick = 0UL;
uint32_t u32ColorTick = 0UL;
static volatile uint8_t gUpdateTimeFlag = 1U;
static int8_t gH_X = 0U, gH_Y = 0U, gM_X = 0, gM_Y = 0U, gS_X = 0U, gS_Y = 0U;
static uint8_t u8LrDate = 0U;
static uint8_t u8LrMoth = 0U;
static uint8_t gOldHour = 25U, gOldMin = 60U, gOldSec = 60U, gOldDate = 99U, gOldDay = 0U, gOldMon = 99U, gOldYear = 99U;
static uint8_t u8IsFirstDraw = 1U;
MATRIX_MonitorTypes gCurrentMonitor = MONITOR_DISPLAY_MON1;
uint8_t receive_data[7],send_data[7];

FRESULT SDResult = FR_OK;
FILINFO fno = { 0 };
DIR dir;

/* Max brightness in daylight */
static uint8_t u8MaxBrightness = 90U;
/* Min brightness in nightlight */
static uint8_t u8MinBrightness = 2U;
/* State of blinking led */
static uint8_t u8PixelOff = 0U;
/* The current position of cursor, used in display mode */
static uint8_t u8CurPos = 0U;
/* Store the random time value in each monitor state */
static uint16_t u16ColorCnt = 0U;
/* Store the current brightness setting */
static CLOCK_Brightness_Type nBrightness = CLOCK_STATIC_BRIGHTNESS;
/* Day of week */
static const char * cDayOfWeek[8U] =
{
    "Rsv",
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};
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

static inline void CLOCK_SetTimes(void);
static inline void CLOCK_SetFonts(void);
static inline void CLOCK_SetLights(void);
static inline void CLOCK_SetTexts(void);
static inline void CLOCK_SetColor(void);
static inline void MEDIA_ExplorePictures(void);
static inline void MEDIA_PlayAudio(void);
static inline void MEDIA_PlayVideo(void);

static inline void MATRIX_DisplayMonitor0(void);
static inline void MATRIX_DisplayMonitor1(void);
static inline void MATRIX_DisplayMonitor2(void);
static inline void MATRIX_DisplayMonitor3(void);
static inline void MATRIX_AnalogClockMode0(void);

void MEDIA_DisplayJpeg(char * pPath);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
typedef void (*pFunction_t) ();

typedef struct MATRIX_ObjectInfo
{
    char sName[10];

} MATRIX_ObjectInfo_TagType;

static pFunction_t MATRIX_pSetupFuncs[8U] =
{
    &CLOCK_SetTimes,
    &CLOCK_SetFonts,
    &CLOCK_SetLights,
    &CLOCK_SetTexts,
    &CLOCK_SetColor,
    &MEDIA_PlayAudio,
    &MEDIA_ExplorePictures,
    &MEDIA_PlayVideo
};

static pFunction_t MATRIX_DisplayFuncs[MONITOR_DISPLAY_RANDOM] =
{
    &MATRIX_DisplayMonitor0,
    &MATRIX_DisplayMonitor1,
    &MATRIX_DisplayMonitor2,
    &MATRIX_DisplayMonitor3,
    &MATRIX_AnalogClockMode0
};

/* Reset all tick values and clear the template times data */
static inline void CLOCK_ResetDefaultState(void)
{
    gOldSec = gOldMin = gOldHour = gOldDate = gOldMon = gOldYear = 99U;
    gOldDay = 0U;
    u8LrDate = 0U;
    u8LrMoth = 0U;
    u8IsFirstDraw = 1U;
    u32BackgroundTick = 0UL;
    u32CursorTick = 0UL;
    u32TextTick = 0UL;
    u32ColorTick = 0UL;
    u8PixelOff = 0U;
}

/* These values are depending on the fonts type */
#define MON_SET_TIME_X_HOUR (10U)
#define MON_SET_TIME_Y_HOUR (35U)
#define MON_SET_TIME_X_DATE (7U)
#define MON_SET_TIME_Y_DATE (59U)
static inline void CLOCK_SetTimes(void)
{
    /* Store the base time to blink the cursor */
    uint32_t u32CursorTick = 0U;
    /* The started positions of each setup data */
    uint8_t const u8Point[7U][2U] =
    {
        { MON_SET_TIME_X_HOUR, MON_SET_TIME_Y_HOUR }, /* Hour */
        { MON_SET_TIME_X_HOUR + 62U, MON_SET_TIME_Y_HOUR }, /* Minute */
        { MON_SET_TIME_X_DATE, MON_SET_TIME_Y_DATE }, /* Date */
        { MON_SET_TIME_X_DATE + 32U, MON_SET_TIME_Y_DATE }, /* Month */
        { MON_SET_TIME_X_DATE + 64U, MON_SET_TIME_Y_DATE }, /* Year */
        { MON_SET_TIME_X_HOUR + 50U, MON_SET_TIME_Y_HOUR - 34U }, /* Day */
        { 74U,  7U }, /* Second */
    };
    /* The current id in setup data */
    int8_t u8CurrentPoint = 0U;
    /* Get the data from DS3231, also store the template data in order to write to DS3231 */
    uint8_t u8TempDat[8U];

    /* Read data to parse time information */
    HAL_I2C_Mem_Read(&hi2c2,DS3231_ADDRESS,0,I2C_MEMADD_SIZE_8BIT,u8TempDat,7,1000);

    /* Parse data, re-sort data following to the new positionss */
    u8TempDat[0U] = BCD2DEC(u8TempDat[2U]); /* Hour */
    u8TempDat[1U] = BCD2DEC(u8TempDat[1U]); /* Min */
    u8TempDat[2U] = BCD2DEC(u8TempDat[4U]); /* Date */
    u8TempDat[7U] = u8TempDat[3U]; /* Backup Data */
    u8TempDat[3U] = BCD2DEC(u8TempDat[5U]); /* Mon */
    u8TempDat[4U] = BCD2DEC(u8TempDat[6U]); /* Year */
    u8TempDat[5U] = BCD2DEC(u8TempDat[7U]); /* Day */
    u8TempDat[6U] = BCD2DEC(u8TempDat[0U]); /* Sec */

    /* Update the current monitor state */
    gCurrentMonitor = MONITOR_SET_TIME;

    /* Write the first template */
    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON_SET_TIME_X_HOUR, MON_SET_TIME_Y_HOUR, 0xFFFF, "%02u", u8TempDat[0]);
    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON_SET_TIME_X_HOUR + 48U, MON_SET_TIME_Y_HOUR, 0xFFFF, ":");
    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON_SET_TIME_X_HOUR + 62U, MON_SET_TIME_Y_HOUR, 0xFFFF, "%02u", u8TempDat[1]);

    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, MON_SET_TIME_X_DATE, MON_SET_TIME_Y_DATE, 0xFFFF, "%02u", u8TempDat[2]);
    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, MON_SET_TIME_X_DATE + 24U, MON_SET_TIME_Y_DATE, 0xFFFF, "/");
    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, MON_SET_TIME_X_DATE + 32U, MON_SET_TIME_Y_DATE, 0xFFFF, "%02u", u8TempDat[3]);
    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, MON_SET_TIME_X_DATE + 56U, MON_SET_TIME_Y_DATE, 0xFFFF, "/");
    MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, MON_SET_TIME_X_DATE + 64U, MON_SET_TIME_Y_DATE, 0xFFFF, "20%02u", u8TempDat[4]);

    MATRIX_Printf( FONT_DEFAULT, 1U, u8Point[5U][0U], u8Point[5U][1U], 0xFFFF, "%s", cDayOfWeek[u8TempDat[5U]]);

    while( MONITOR_SET_TIME == gCurrentMonitor )
    {
        /* Always get the key code to execute the command immediately */
        switch( IR1838_GetCurrentKey() )
        {
            /* Abort the current setup, move back the general setting monitor */
            case IR_KEY_MENU:
            case IR_KEY_EXIT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                gCurrentMonitor = MONITOR_SETTING;
                break;

            /* After all values are set, write data to DS3231 and move back to the general setting monitor */
            case IR_KEY_OK:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                gCurrentMonitor = MONITOR_SETTING;

                /* Re-sort data following to the origin positionss */
                u8TempDat[1U] = DEC2BCD(u8TempDat[1U]);
                u8TempDat[7U] = u8TempDat[5U]; /* Backup Data */
                u8TempDat[5U] = DEC2BCD(u8TempDat[3U]);
                u8TempDat[6U] = DEC2BCD(u8TempDat[4U]);
                u8TempDat[4U] = DEC2BCD(u8TempDat[2U]);
                u8TempDat[2U] = DEC2BCD(u8TempDat[0U]);
                u8TempDat[0U] = DEC2BCD(u8TempDat[5U]);
                u8TempDat[3U] = DEC2BCD(u8TempDat[7U]);

                /* Write new data to the current counters */
                HAL_I2C_Mem_Write(&hi2c2,DS3231_ADDRESS,0,I2C_MEMADD_SIZE_8BIT,u8TempDat,7,1000);
                break;

            /* Increase the setup value to 1 */
            case IR_KEY_UP:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );

                /* Need clearing the previous values before updating the new values to the monitor */
                if( !u8PixelOff )
                {
                    MATRIX_Printf( (5U == u8CurrentPoint)?(FONT_DEFAULT):(FONR_FREESERIFBOLDITALIC12PT7B), (2U > u8CurrentPoint)?(2U):(1U),
                                    u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1],
                                    0x0U, (4U == u8CurrentPoint)?("20%02u"):((5U == u8CurrentPoint)?("%s"):("%02u")),
                                    (5U == u8CurrentPoint) ? (cDayOfWeek[u8TempDat[5U]]) : (u8TempDat[u8CurrentPoint])
                                 );
                }
                switch( u8CurrentPoint )
                {
                    case 0U: /* Hour */
                        (23U <= u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 0U):(u8TempDat[u8CurrentPoint]++);
                        break;
                    case 1U: /* Minute */
                        (59U <= u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 0U):(u8TempDat[u8CurrentPoint]++);
                        break;
                    case 2U: /* Day */
                        (31U <= u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 1U):(u8TempDat[u8CurrentPoint]++);
                        break;
                    case 3U: /* Month */
                        (12U <= u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 1U):(u8TempDat[u8CurrentPoint]++);
                        break;
                    case 4U: /* Year */
                        (99U <= u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 0U):(u8TempDat[u8CurrentPoint]++);
                        break;
                    case 5U: /* Day */
                        (7U <= u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 1U):(u8TempDat[u8CurrentPoint]++);
                        break;
                    default:
                        break;
                }
                break;

            /* Downcrease the setup value to 1 */
            case IR_KEY_DOWN:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );

                /* Need clearing the previous values before updating the new values to the monitor */
                if( !u8PixelOff )
                {
                    MATRIX_Printf( (5U == u8CurrentPoint)?(FONT_DEFAULT):(FONR_FREESERIFBOLDITALIC12PT7B), (2U > u8CurrentPoint)?(2U):(1U),
                                    u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1],
                                    0x0U, (4U == u8CurrentPoint)?("20%02u"):((5U == u8CurrentPoint)?("%s"):("%02u")),
                                    (5U == u8CurrentPoint) ? (cDayOfWeek[u8TempDat[5U]]) : (u8TempDat[u8CurrentPoint])
                                 );
                }
                switch( u8CurrentPoint )
                {
                    case 0U: /* Hour */
                        (0U == u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 23U):(u8TempDat[u8CurrentPoint]--);
                        break;
                    case 1U: /* Minute */
                        (0U == u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 59U):(u8TempDat[u8CurrentPoint]--);
                        break;
                    case 2U: /* Day */
                        (0U == u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 31U):(u8TempDat[u8CurrentPoint]--);
                        break;
                    case 3U: /* Month */
                        (0U == u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 12U):(u8TempDat[u8CurrentPoint]--);
                        break;
                    case 4U: /* Year */
                        (0U == u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 99U):(u8TempDat[u8CurrentPoint]--);
                        break;
                    case 5U: /* Day */
                        (1U == u8TempDat[u8CurrentPoint])?(u8TempDat[u8CurrentPoint] = 7U):(u8TempDat[u8CurrentPoint]--);
                        break;
                    default:
                        break;
                }
                break;

            /* Move to the next left setup field */
            case IR_KEY_LEFT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                /* Ensure that the previous state is "pixel on" */
                if( u8PixelOff )
                {
                    MATRIX_Printf( (5U == u8CurrentPoint)?(FONT_DEFAULT):(FONR_FREESERIFBOLDITALIC12PT7B), (2U > u8CurrentPoint)?(2U):(1U),
                                    u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1],
                                    0xFFFFU, (4U == u8CurrentPoint)?("20%02u"):((5U == u8CurrentPoint)?("%s"):("%02u")),
                                    (5U == u8CurrentPoint) ? (cDayOfWeek[u8TempDat[5U]]) : (u8TempDat[u8CurrentPoint])
                                 );
                }
                u8CurrentPoint = (0U >= u8CurrentPoint) ? (5U) : (u8CurrentPoint - 1U);
                break;

            /* Move to the next right setup field */
            case IR_KEY_RIGHT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                /* Ensure that the previous state is "pixel on" */
                if( u8PixelOff )
                {
                    MATRIX_Printf( (5U == u8CurrentPoint)?(FONT_DEFAULT):(FONR_FREESERIFBOLDITALIC12PT7B), (2U > u8CurrentPoint)?(2U):(1U),
                                    u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1],
                                    0xFFFFU, (4U == u8CurrentPoint)?("20%02u"):((5U == u8CurrentPoint)?("%s"):("%02u")),
                                    (5U == u8CurrentPoint) ? (cDayOfWeek[u8TempDat[5U]]) : (u8TempDat[u8CurrentPoint])
                                 );
                }
                u8CurrentPoint = (5U <= u8CurrentPoint) ? (0U) : (u8CurrentPoint + 1U);
                break;
            default:
                break;
        }

        /* Blinking the current cursor with the duty cycle is 500ms */
        if( (250U < HAL_GetTick() - u32CursorTick) && (MONITOR_SET_TIME == gCurrentMonitor))
        {
            u32CursorTick = HAL_GetTick();
            u8PixelOff = !u8PixelOff;
            MATRIX_Printf( (5U == u8CurrentPoint)?(FONT_DEFAULT):(FONR_FREESERIFBOLDITALIC12PT7B), (2U > u8CurrentPoint)?(2U):(1U),
                           u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1],
                           (u8PixelOff?0U:0xFFFFU), (4U == u8CurrentPoint)?("20%02u"):((5U == u8CurrentPoint)?("%s"):("%02u")),
                           (5U == u8CurrentPoint) ? (cDayOfWeek[u8TempDat[5U]]) : (u8TempDat[u8CurrentPoint])
                         );
        }
    }

    /* Reset all tick values and clear the template times data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

static inline void CLOCK_SetFonts(void)
{
    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "To be Defined! \n");
    HAL_Delay(1000U);

    /* Reset all tick values and clear the template time data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

static inline void CLOCK_SetLights(void)
{
    uint8_t u8PreBrightness = MATRIX_getBrightness();
    uint8_t u8TempBri = u8PreBrightness;

    /* Update the current monitor state */
    gCurrentMonitor = MONITOR_SET_LIGHT;

    /* Prepare somethings */
    MATRIX_DrawRect( 5U, 5U, 102U, 7U, 0xFFFFU );
    MATRIX_FillRect( 6U, 6U, u8PreBrightness, 5U, 0x1245U );

    while( MONITOR_SET_LIGHT == gCurrentMonitor )
    {
        /* Always get the key code to execute the command immediately */
        switch( IR1838_GetCurrentKey() )
        {
            /* Abort the current setup, move back the general setting monitor */
            case IR_KEY_MENU:
            case IR_KEY_EXIT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                /* Restore the previous brightness */
                MATRIX_setBrightness(u8PreBrightness);
                gCurrentMonitor = MONITOR_SETTING;
                break;
            /* If OK key is pressed, basically only need clearing key id and move back to the general setting monitor */
            case IR_KEY_OK:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                /* Store the value to the flash memory */
                FLASH_Write( (uint16_t) FLASH_MAX_BRIGHTNESS, (uint8_t) u8TempBri );
                gCurrentMonitor = MONITOR_SETTING;
            case IR_KEY_LEFT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                MATRIX_DrawFastVLine( 5U + u8TempBri, 6U , 5U, 0x0U);
                u8TempBri = (1U >= u8TempBri)?(100U):(--u8TempBri);
                if( 100U == u8TempBri )
                {
                    MATRIX_FillRect( 6U, 6U, 100U, 5U, 0x1245U );
                }
                MATRIX_setBrightness(u8TempBri);
                break;
            case IR_KEY_RIGHT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                u8TempBri = (100U <= u8TempBri)?(1U):(++u8TempBri);
                if( 1U == u8TempBri )
                {
                    MATRIX_FillRect( u8TempBri + 6U, 6U, 100U - u8TempBri, 5U, 0x0U );
                }
                else
                {
                    MATRIX_DrawFastVLine( 5U + u8TempBri, 6U , 5U, 0x1245U);
                }
                MATRIX_setBrightness(u8TempBri);
                break;
            default:
                break;
        }
    }

    /* Reset all tick values and clear the template times data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

static inline void CLOCK_SetTexts(void)
{
    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "To be Defined! \n");
    HAL_Delay(1000U);

    /* Reset all tick values and clear the template time data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

static inline void CLOCK_SetColor(void)
{
    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "To be Defined! \n");
    HAL_Delay(1000U);

    /* Reset all tick values and clear the template time data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

static inline void MEDIA_ExplorePictures(void)
{
    // MEDIA_DisplayJpeg("img/natural.jpg");
    // HAL_Delay(1000);
    // MEDIA_DisplayJpeg("img/anime.jpg");
    // HAL_Delay(1000);
    // MEDIA_DisplayJpeg("img/phung.jpg");
    // HAL_Delay(1000);

    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "To be Defined! \n");
    HAL_Delay(1000U);

    /* Reset all tick values and clear the template time data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

static inline void MEDIA_PlayAudio(void)
{
    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "To be Defined! \n");
    HAL_Delay(1000U);

    /* Reset all tick values and clear the template time data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

#define MON0_X_HOUR (7U)
#define MON0_Y_HOUR (33U)
static inline void MATRIX_DisplayMonitor0(void)
{
    /* Randome color for the screen border */
    uint32_t u16Random = 0UL;
    /* constant positions of the second cursor */
    uint8_t const u8CursorPos[16U][2U] =
    {
        { MON0_X_HOUR + 50U, MON0_Y_HOUR -  4U },
        { MON0_X_HOUR + 51U, MON0_Y_HOUR -  7U },
        { MON0_X_HOUR + 52U, MON0_Y_HOUR - 10U },
        { MON0_X_HOUR + 53U, MON0_Y_HOUR - 13U },
        /* Reserved */
        { MON0_X_HOUR + 53U, MON0_Y_HOUR - 21U },
        { MON0_X_HOUR + 54U, MON0_Y_HOUR - 24U },
        { MON0_X_HOUR + 55U, MON0_Y_HOUR - 27U },
        { MON0_X_HOUR + 56U, MON0_Y_HOUR - 30U },
        { MON0_X_HOUR + 55U, MON0_Y_HOUR - 27U },
        { MON0_X_HOUR + 54U, MON0_Y_HOUR - 24U },
        { MON0_X_HOUR + 53U, MON0_Y_HOUR - 21U },
        /* Reserved */
        { MON0_X_HOUR + 53U, MON0_Y_HOUR - 13U },
        { MON0_X_HOUR + 52U, MON0_Y_HOUR - 10U },
        { MON0_X_HOUR + 51U, MON0_Y_HOUR -  7U },
        { MON0_X_HOUR + 50U, MON0_Y_HOUR -  4U }
    };

    if( gUpdateTimeFlag )
    {
        /* Only update the screen data in case detect the changes */
        gUpdateTimeFlag = 0U;

        if( gOldSec != u8Second )
        {
            /* Get the draw the random color for monitor borders */
            gOldSec = u8Second;
            u16Random = rand() >> 15U;
            MATRIX_DrawRect( 0U, 0U, 128U, 64U, (uint16_t) u16Random );
            MATRIX_DrawFastHLine( 0U, MON0_Y_HOUR + 7U, 128U, (uint16_t) u16Random );
        }

        /* It should draw only once time in order to get data regions */
        if( u8IsFirstDraw)
        {
            u8CurPos = 0U;
            u16ColorCnt = 500U;
            u8IsFirstDraw = 0U;

            /* Draw the template data */
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON0_X_HOUR + 50U, MON0_Y_HOUR, 0x7BEFU, ":");
            /* Calibrate positions */
            MATRIX_MoveRegion( MON0_X_HOUR + 56U, MON0_Y_HOUR - 20U, MON0_X_HOUR + 53U, MON0_Y_HOUR - 21U, 8U, 8U );

            /* Draw the rainbow border lines */
            for( uint8_t x = 0U; x < 128U; x++ )
            {
                for( uint8_t y = 1U; y < 2U; y++ )
                {
                    uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( u16xTemp, y, MATRIX_Hsv2Rgb( x * 12U, 255U, 150U, 1U), 0U );
                }

                for( uint8_t y = (MON0_Y_HOUR + 6U); y < (MON0_Y_HOUR + 7U); y++ )
                {
                    uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( u16xTemp, y, MATRIX_Hsv2Rgb( x * 12U, 255U, 150U, 1U), 0U );
                }
            }
        }

        /* Update hour values if detect the changes */
        if( gOldHour != u8Hour )
        {
            /* 2147483647 */
            uint16_t u16Random = u8Hour << 5U + u8Minute << 4U;

            /* Clear the old value on the monitor */
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON0_X_HOUR, MON0_Y_HOUR, 0x0U, "%02i", gOldHour);
            gOldHour = u8Hour;
            /* Update the new value */
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON0_X_HOUR, MON0_Y_HOUR, 0x7FF, "%02i", u8Hour);
            /* Overide the rainbow color of this element */
            MATRIX_FillRainbowColorToRegion( MON0_X_HOUR, MON0_Y_HOUR - 30U, MON0_X_HOUR + 48U, MON0_Y_HOUR + 3U,
                                             u16Random, u16Random + 400U, 0.2, 0U
                                           );
        }

        /* Update minute values if detect the changes */
        if( gOldMin != u8Minute )
        {
            /* 2147483647 */
            uint16_t u16Random = u8Minute << 7U;

            /* clear the old value */
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON0_X_HOUR + 66U, MON0_Y_HOUR, 0x0U, "%02i", gOldMin);
            gOldMin = u8Minute;
            /* Update the new value */
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 2U, MON0_X_HOUR + 66U, MON0_Y_HOUR, 0xFFFF, "%02i", u8Minute);
            /* Overide the rainbow color of this element */
            MATRIX_FillRainbowColorToRegion( MON0_X_HOUR + 65U, MON0_Y_HOUR - 30U, MON0_X_HOUR + 118U, MON0_Y_HOUR + 3U,
                                             u16Random, u16Random + 400, 0.2, 0U
                                           );
        }

        /* Update date values if detect the changes */
        if( gOldDate != u8Date )
        {
            /* clear the old value */
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, MON0_X_HOUR, MON0_Y_HOUR + 25U, 0x0U,
                            "%02i/%02i/20%02i", gOldDate, gOldMon, gOldYear
                         );
            gOldDate = u8Date;
            /* Update the new value */
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, MON0_X_HOUR, MON0_Y_HOUR + 25U, 0x7FF,
                            "%02i/%02i/20%02i", u8Date, u8Month, u8Year
                         );
        }
    }

    /* Blink the cursor in each 166 miliu8Second ~ 6 frames/sec */
    if( 166U <= (HAL_GetTick() - u32CursorTick) )
    {
        u32CursorTick = HAL_GetTick();
        if( 14U == u8CurPos )
        {
            u8CurPos = 0U;
        }
        MATRIX_MoveRegion( u8CursorPos[((3U==u8CurPos)||(10U==u8CurPos))?(++u8CurPos):(u8CurPos)][0U],
                           u8CursorPos[u8CurPos][1U], u8CursorPos[++u8CurPos][0U],
                           u8CursorPos[u8CurPos][1U], 8U, 8U
                         );
    }

    /* Shift left/right the rainbow border lines */
    if( 50U < (HAL_GetTick() - u32BackgroundTick ) )
    {
        u32BackgroundTick = HAL_GetTick();
        MATRIX_ShiftRight( 1U, 1U, 126U, 1U );
        MATRIX_ShiftLeft( 1U, MON0_Y_HOUR + 6U, 126U, 1U );
    }

    /* Sift left/right the lower region of the monitor, show date, string, etc... */
    if( 120U <= (HAL_GetTick() - u32TextTick) )
    {
        u32TextTick = HAL_GetTick();
        MATRIX_FlutterLeftRight( 1U, MON0_Y_HOUR + 8U, 126U, 20U );
    }

    /* Calculate and overide the color of the lower region */
    if( 150U < (HAL_GetTick() - u32ColorTick) )
    {
        u32ColorTick = HAL_GetTick();
        u16ColorCnt+=20;
        if( 15360U <= u16ColorCnt)
        {
            u16ColorCnt = 500U;
        }
        MATRIX_FillRainbowColorToRegion( 1U, MON0_Y_HOUR + 8U, 126U, 62U, u16ColorCnt - 500U, u16ColorCnt, 0U, 0U );
    }
}

static inline uint16_t MATRIX_GenerateColor565(uint8_t u8Value)
{
    uint16_t u16Color = 0U;

    if( 8U > u8Value )
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) (((31U - u8Value) << 11U ) | ((u8Value) << 6U) | (0U));
    }
    else if( 16U > u8Value )
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) ((0U) | ((31U - u8Value) << 6U) | (u8Value));
    }
    else if( 24U > u8Value )
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) (((u8Value) << 11U ) | ((0U) << 6U) | (31U - u8Value));
    }
    else if( 32U > u8Value )
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) (((u8Value) << 11U ) | ((u8Value) << 6U) | (31U - u8Value));
    }
    else if( 40U > u8Value )
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) (((0U) << 11U ) | ((u8Value) << 6U) | (31U - u8Value));
    }
    else if( 48U > u8Value )
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) (((31U - u8Value) << 11U ) | ((u8Value) << 6U) | (u8Value));
    }
    else if( 56U > u8Value )
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) (((31U - u8Value) << 11U ) | ((0U) << 6U) | (u8Value));
    }
    else
    {
        u8Value = (u8Value & 0x7U) * 4U + 3U;
        u16Color = (uint16_t) (((u8Value) << 11U ) | ((31U - u8Value) << 6U) | (u8Value));
    }

    return u16Color;
}

#define MON1_X_HOUR (3U)
#define MON1_Y_HOUR (3U)
static inline void MATRIX_DisplayMonitor1(void)
{
    /* Randome color for the screen border */
    uint32_t u16Random = 0UL;

    if( gUpdateTimeFlag )
    {
        /* Only update the screen data in case detect the changes */
        gUpdateTimeFlag = 0U;

        if( gOldSec != u8Second )
        {
            /* Get the draw the random color for monitor borders */
            u16Random = rand() >> 15U;
            MATRIX_DrawRect( 0U, 0U, 128U, 64U, (uint16_t) u16Random );

            /* 2147483647 */
            uint16_t u16Random = u8Second << 8U;
            // MATRIX_DrawFastHLine( 0U, MON0_Y_HOUR + 7U, 128U, (uint16_t) u16Random );

            /* clear the old value */
            MATRIX_Printf( FONT_DEFAULT, 2U, MON1_X_HOUR + 100U, MON1_Y_HOUR + 0U, 0x0U, "%02i", gOldSec);
            gOldSec = u8Second;
            /* Update the new value */
            MATRIX_Printf( FONT_DEFAULT, 2U, MON1_X_HOUR + 100U, MON1_Y_HOUR + 0U, MATRIX_GenerateColor565(u8Second), "%02i", u8Second);
        }

        /* It should draw only once time in order to get data regions */
        if( u8IsFirstDraw)
        {
            u16ColorCnt = 500U;
            u8IsFirstDraw = 0U;

            /* Draw the template data */
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR + 37U, MON1_Y_HOUR, 0x7BEFU, ":");
            MATRIX_Printf( FONT_DEFAULT, 3U, MON1_X_HOUR + 90U, MON1_Y_HOUR - 3U, 0x7BEFU, ":");

            /* Draw the rainbow border lines */
            for( uint8_t x = 0U; x < 128U; x++ )
            {
                for( uint8_t y = 1U; y < 2U; y++ )
                {
                    uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( u16xTemp, y, MATRIX_Hsv2Rgb( x * 12U, 255U, 150U, 1U), 0U );
                }

                for( uint8_t y = 62; y < 63; y++ )
                {
                    uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( u16xTemp, y, MATRIX_Hsv2Rgb( x * 12U, 255U, 150U, 1U), 0U );
                }
            }
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, MON1_X_HOUR + 2U, MON1_Y_HOUR + 57U, 0x7FF,
                            "Make Yourself!");
            MATRIX_FillRainbowColorToRegion( 1U, MON1_Y_HOUR + 44, 126U, 60U, 0U, 1535 * 2U, 0U, 0U );
        }

        /* Update hour values if detect the changes */
        if( gOldHour != u8Hour )
        {
            /* Clear the old value on the monitor */
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR, MON1_Y_HOUR, 0x0U, "%01i", gOldHour/10U);
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR + 23U, MON1_Y_HOUR, 0x0U, "%01i", gOldHour%10U);
            gOldHour = u8Hour;
            /* Update the new value */
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR, MON1_Y_HOUR, MATRIX_GenerateColor565(u8Hour * 2U), "%01i", u8Hour/10U);
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR + 23U, MON1_Y_HOUR, (uint16_t)((~MATRIX_GenerateColor565(u8Hour * 2U)) & 0xFFFFU), "%01i", u8Hour%10U);
        }

        /* Update minute values if detect the changes */
        if( gOldMin != u8Minute )
        {
            /* clear the old value */
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR + 51U, MON1_Y_HOUR, 0x0U, "%01i", gOldMin/10U);
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR + 74U, MON1_Y_HOUR, 0x0U, "%01i", gOldMin%10U);
            gOldMin = u8Minute;
            /* Update the new value */
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR + 51U, MON1_Y_HOUR, (uint16_t)((~MATRIX_GenerateColor565(u8Minute)) & 0xFFFFU), "%01i", u8Minute/10U);
            MATRIX_Printf( FONT_DEFAULT, 4U, MON1_X_HOUR + 74U, MON1_Y_HOUR, MATRIX_GenerateColor565(u8Minute), "%01i", u8Minute%10U);
        }

        /* Update date values if detect the changes */
        if( gOldDate != u8Date )
        {

            /* clear the old value */
            MATRIX_Printf( FONT_DEFAULT, 2U, MON1_X_HOUR + 2U, MON1_Y_HOUR + 30U, 0x0U,
                            "%02i/%02i/20%02i", gOldDate, gOldMon, gOldYear
                         );
            MATRIX_Printf( FONT_DEFAULT, 1U, MON1_X_HOUR + 102U, MON1_Y_HOUR + 18U, 0x0U,
                            "%s", cDayOfWeek[gOldDay]);
            gOldDate = u8Date;
            gOldDay = u8Day;
            /* Update the new value */
            MATRIX_Printf( FONT_DEFAULT, 1U, MON1_X_HOUR + 102U, MON1_Y_HOUR + 18U, 0x7FF,
                            "%s", cDayOfWeek[u8Day]);
            MATRIX_Printf( FONT_DEFAULT, 2U, MON1_X_HOUR + 2U, MON1_Y_HOUR + 30U, MATRIX_GenerateColor565(u8Date * 2U),
                            "%02i/%02i/20%02i", u8Date, u8Month, u8Year
                         );
        }
    }

    /* Shift left/right the rainbow border lines */
    if( 50U < (HAL_GetTick() - u32BackgroundTick ) )
    {
        u32BackgroundTick = HAL_GetTick();
        MATRIX_ShiftRight( 1U, 1U, 126U, 1U );
        MATRIX_ShiftLeft( 1U, 62, 126U, 1U );
    }

    /* Sift left/right the lower region of the monitor, show date, string, etc... */
    if( 120U <= (HAL_GetTick() - u32TextTick) )
    {
        u32TextTick = HAL_GetTick();
        MATRIX_FlutterLeftRight( 1U, MON1_Y_HOUR + 44, 126U, 14U );
    }
}

static inline void MATRIX_DisplayMonitor2(void)
{
    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "Monitor 2! \n");
    HAL_Delay(500U);
}

static inline void MATRIX_DisplayMonitor3(void)
{
    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "Monitor 3! \n");
    HAL_Delay(500U);
}

#define CLOCK_CENTER_X 31U
#define CLOCK_CENTER_Y 31U
#define CLOCK_RADIOUS  31U
#define CLOCK_LENGHT_HOUR   10
#define CLOCK_LENGHT_MIN    15
#define CLOCK_LENGHT_SEC    20

#define CLOCK_DIGITAL_HM_X    66U
#define CLOCK_DIGITAL_HM_Y    20U
#define CLOCK_DIGITAL_DATE_X    65U
#define CLOCK_DIGITAL_DATE_Y    26U
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

            MATRIX_FillRect(0U, 0U, 32U, 64U, 0xFFFFU);
            MATRIX_FillRainbowColorToRegion( 0U, 0U, 31U, 31U, 1000U, 2000U, 1.0, 0U);
            MATRIX_FillRainbowColorToRegion( 0U, 32U, 31U, 63U, 2000U, 4000U, -1.0, 0U);

            /* Draw new basic graphic */
            MATRIX_FillCircle( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIOUS, 0xFFFFU );
            MATRIX_FillCircle( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIOUS - 2U, 0x0U );

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

            MATRIX_DrawFastHLine(35U, 0U, 93U, 0xFFFFU);
            MATRIX_DrawFastHLine(28U, 63U, 100U, 0xFFFFU);
            MATRIX_DrawFastVLine(127U, 0U, 64U, 0xFFFFU);

            /* Draw the upper rainbow border lines */
            for( uint8_t x = 41U; x < 127U; x++ )
            {
                for( uint8_t y = 1U; y < 2U; y++ )
                {
                    // uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( x, y, MATRIX_Hsv2Rgb( (x - 41U) * 17U, 255U, 150U, 1U), 0U );
                }
            }
            for( uint8_t x = 44U; x < 127U; x++ )
            {
                for( uint8_t y = 2U; y < 3U; y++ )
                {
                    // uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( x, y, MATRIX_Hsv2Rgb( (x - 44U) * 18U, 255U, 150U, 1U), 0U );
                }
            }
            /* Draw the lower rainbow border lines */
            for( uint8_t x = 35U; x < 127U; x++ )
            {
                for( uint8_t y = 62U; y < 63U; y++ )
                {
                    // uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( x, y, MATRIX_Hsv2Rgb( (x - 35U) * 16U, 255U, 150U, 1U), 0U );
                }
            }
            for( uint8_t x = 41U; x < 127U; x++ )
            {
                for( uint8_t y = 61U; y < 62U; y++ )
                {
                    // uint16_t u16xTemp = ((x + (64U - y))>=128U) ? (x + (64U - y) - 128U) : (x + (64U - y));
                    MATRIX_WritePixel( x, y, MATRIX_Hsv2Rgb( (x - 41U) * 17U, 255U, 150U, 1U), 0U );
                }
            }
        }

        gH_X = l_HX = CLOCK_LENGHT_HOUR * sin( (u8Hour + u8Minute/60.0) / 6.0 * _PI );
        gH_Y = l_HY = -CLOCK_LENGHT_HOUR * cos( (u8Hour + u8Minute/60.0) / 6.0 * _PI );
        gM_X = l_MX = CLOCK_LENGHT_MIN * sin( u8Minute / 30.0 * _PI );
        gM_Y = l_MY = -CLOCK_LENGHT_MIN * cos( u8Minute / 30.0 * _PI );
        gS_X = l_SX = CLOCK_LENGHT_SEC * sin( u8Second / 30.0 * _PI );
        gS_Y = l_SY = -CLOCK_LENGHT_SEC * cos( u8Second / 30.0 * _PI );

        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + l_MX, CLOCK_CENTER_Y + l_MY, 0x7E0 );
        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + l_HX, CLOCK_CENTER_Y + l_HY, 0xFFE0 );
        MATRIX_DrawLine( CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_CENTER_X + l_SX, CLOCK_CENTER_Y + l_SY, 0xF8E0 );
        MATRIX_FillCircle( CLOCK_CENTER_X, CLOCK_CENTER_Y, 1, 0xFFFF );

        if( gOldHour != u8Hour )
        {
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X, CLOCK_DIGITAL_HM_Y, 0x0U, "%02i", gOldHour);
            gOldHour = u8Hour;
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X, CLOCK_DIGITAL_HM_Y, 0xFFFFU, "%02i", u8Hour);
        }
        if( gOldMin != u8Minute )
        {
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X + 34U, CLOCK_DIGITAL_HM_Y, 0x0U, "%02i", gOldMin);
            gOldMin = u8Minute;
            MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X + 34U, CLOCK_DIGITAL_HM_Y, 0xFFFFU, "%02i", u8Minute);
        }
        if( gOldDate != u8Date )
        {
            MATRIX_Printf( FONT_TOMTHUMB, 2U, CLOCK_DIGITAL_DATE_X, CLOCK_DIGITAL_DATE_Y + 9U, 0x0U, "AL:%02i/%02i", u8LrDate, u8LrMoth);
            MATRIX_Printf( FONT_DEFAULT, 2U, CLOCK_DIGITAL_DATE_X - 4U, CLOCK_DIGITAL_DATE_Y + 20U, 0x0U, "%s", cDayOfWeek[gOldDay]);
            MATRIX_Printf( FONT_DEFAULT, 1U, CLOCK_DIGITAL_DATE_X + 11U, CLOCK_DIGITAL_DATE_Y + 11U, 0x0U, "DL:%02i/%02i", gOldDate, gOldMon);
            gOldDate = u8Date;
            gOldDay = u8Day;
            am_lich( u8Date, u8Month, u8Year, &u8LrDate, &u8LrMoth);
            MATRIX_Printf( FONT_TOMTHUMB, 2U, CLOCK_DIGITAL_DATE_X, CLOCK_DIGITAL_DATE_Y + 9U, 0xFFFFU, "AL:%02i/%02i", u8LrDate, u8LrMoth);
            MATRIX_Printf( FONT_DEFAULT, 2U, CLOCK_DIGITAL_DATE_X - 4U, CLOCK_DIGITAL_DATE_Y + 20U, 0xFFFFU, "%s", cDayOfWeek[u8Day]);
            MATRIX_Printf( FONT_DEFAULT, 1U, CLOCK_DIGITAL_DATE_X + 11U, CLOCK_DIGITAL_DATE_Y + 11U, 0xFFFFU, "DL:%02i/%02i", u8Date, u8Month );
        }
        if( gOldYear != u8Year )
        {
            MATRIX_Printf( FONT_DEFAULT, 1U, CLOCK_DIGITAL_DATE_X + 34U, CLOCK_DIGITAL_DATE_Y + 20U, 0x0U, "20%02i", gOldYear);
            gOldYear = u8Year;
            MATRIX_Printf( FONT_DEFAULT, 1U, CLOCK_DIGITAL_DATE_X + 34U, CLOCK_DIGITAL_DATE_Y + 20U, 0xFFFFU, "20%02i", u8Year );
        }
    }

    /* Blink the cursor in each 249 miliu8Second ~ 2 frames/sec */
    if( 249U <= (HAL_GetTick() - u32CursorTick) )
    {
        u32CursorTick = HAL_GetTick();
        u8PixelOff = !u8PixelOff;
        MATRIX_Printf( FONR_FREESERIFBOLDITALIC12PT7B, 1U, CLOCK_DIGITAL_HM_X + 25U, CLOCK_DIGITAL_HM_Y - 2U,
                       (u8PixelOff) ? (0x0U) : (0x7E0U), ":");
    }

    /* Shift left/right the rainbow border lines */
    if( 80U < (HAL_GetTick() - u32BackgroundTick ) )
    {
        u32BackgroundTick = HAL_GetTick();
        MATRIX_ShiftRight( 41U, 1U, 86U, 1U );
        MATRIX_ShiftRight( 44U, 2U, 83U, 1U );
        MATRIX_ShiftLeft( 35U, 62U, 92U, 1U );
        MATRIX_ShiftLeft( 41U, 61U, 86U, 1U );
    }
}

/* Fow now ignore this feature */
static inline void CLOCK_UpdateKeyCode( void )
{
    /* Backup the previous monitor in order to restore it when configure successfully */
    MATRIX_MonitorTypes nPreviousMonitor = gCurrentMonitor;

    IR_KeysLists nCurKey = IR_KEY_RESERVED;
    uint32_t u32CursorTick = 0U;
    uint32_t u32StrTick = 0U;
    uint8_t u8Logic = 0;
    uint8_t u8xCurPos = 0U;
    uint8_t u8yCurPos = 0U;

    /* Check whether the button for updating key code is turned on */
    /* Note: This pin shall active at low level */
    if( !(LL_GPIO_ReadInputPort( UPD_KEY_GPIO_Port ) & UPD_KEY_Pin) )
    {
        LL_mDelay(20);
        while( !(LL_GPIO_ReadInputPort( UPD_KEY_GPIO_Port ) & UPD_KEY_Pin) );
        gCurrentMonitor = MONITOR_SET_KEY;
        MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );

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

        /* Draw u16LeftPos button */
        MATRIX_FillCircle( 45U, 42U, 8U, 0x33U );
        MATRIX_FillTriangle( 47U, 47U, 47U, 37U, 39U, 42U, 0xFFFFU );

        /* Draw u16RightPos button */
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

        u32CursorTick = HAL_GetTick();
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
            if( (150U < HAL_GetTick() - u32CursorTick) )
            {
                u32CursorTick = HAL_GetTick();
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
        MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
        CLOCK_ResetDefaultState();

        gCurrentMonitor = nPreviousMonitor;
    }
}

static inline void MATRIX_DisplayMonitorRandom(void)
{
    gCurrentMonitor = MONITOR_DISPLAY_RANDOM;
}

static inline void CLOCK_SwitchMonitor(void)
{
    int8_t s8CurrentPos = (uint8_t) gCurrentMonitor;

    /* At this moment, only digital monitors 0, 2 and analog monitor 0 are activated
     * In case other monitors activated, enum MATRIX_MonitorTypes shall be updated.
     */

    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
    CLOCK_ResetDefaultState();

    switch( IR1838_GetCurrentKey())
    {
        case IR_KEY_LEFT:
            IR1838_SetCurrentKey( IR_KEY_RESERVED );
            if( (uint8_t) MONITOR_DISPLAY_MON0 >= s8CurrentPos )
            {
                s8CurrentPos = (uint8_t) MONITOR_DISPLAY_ANALOG0;
            }
            else
            {
                s8CurrentPos--;
            }
            break;
        case IR_KEY_RIGHT:
            IR1838_SetCurrentKey( IR_KEY_RESERVED );
            if( MONITOR_DISPLAY_ANALOG0 < (MATRIX_MonitorTypes) (++s8CurrentPos) )
            {
                s8CurrentPos = 0U;
            }
            break;
        default:
            break;
    }

    gCurrentMonitor = (MATRIX_MonitorTypes) s8CurrentPos;
}

static inline void CLOCK_ChangesBrightness(void)
{
    (void) gCurrentMonitor;
}
static inline void CLOCK_BasicMonitor(void)
{
    IR_KeysLists nCurKey = IR_KEY_RESERVED;

    switch( gCurrentMonitor )
    {
        case MONITOR_DISPLAY_ANALOG0:
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
}

static inline void CLOCK_EnableAllIrq(void)
{
    LL_TIM_EnableCounter(TIM6);
    LL_TIM_EnableCounter(TIM_OE);
    LL_TIM_EnableCounter(TIM_DAT);
    LL_TIM_EnableCounter( TIM_IR );
}

void DS3231_UpdateData(void)
{
    /* Read data to parse time informations */
    HAL_I2C_Mem_Read(&hi2c2,DS3231_ADDRESS,0,I2C_MEMADD_SIZE_8BIT,receive_data,7,1000);

    /* Raise global flag immediately to avoid the missing data at data cache */
    gUpdateTimeFlag = 1;

    /* Parse data */
    u8Second=BCD2DEC(receive_data[0]);
    u8Minute=BCD2DEC(receive_data[1]);
    u8Hour=BCD2DEC(receive_data[2]);

    u8Day=BCD2DEC(receive_data[3]);
    u8Date=BCD2DEC(receive_data[4]);
    u8Month=BCD2DEC(receive_data[5]);
    u8Year= BCD2DEC(receive_data[6]);
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
    /* Backup the previous monitor in order to restore it when configure successfully */
    MATRIX_MonitorTypes nPreviousMonitor = gCurrentMonitor;

    uint32_t u32CursorTick = 0UL;
    uint8_t const u8Point[8U][2U] =
    {
        {15U, 9U},
        {15U, 24U},
        {15U, 39U},
        {15U, 54U},
        {74U, 9U},
        {74U, 24U},
        {74U, 39U},
        {74U, 54U},
    };
    int8_t u8CurrentPoint = 0U;
    uint8_t u8Logic = 0;

    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
    CLOCK_ResetDefaultState();
    gCurrentMonitor = MONITOR_SETTING;

    while( MONITOR_SETTING == gCurrentMonitor )
    {
        if( u8IsFirstDraw )
        {
            u8IsFirstDraw = 0U;

            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 15U, 0xFF00, "1. Time \n");
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 30U, 0x5505, "2. Font\n");
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 45U, 0x00FF, "3. Light\n");
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 59U, 0xF00F, "4. Text\n");
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 62U, 15U, 0x07E0, "5. Color\n");
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 62U, 30U, 0x7A1F, "6. Audio\n");
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 62U, 45U, 0x0FF0, "7. Image\n");
            MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 62U, 59U, 0x5505, "8. Video\n");

            for( uint8_t x = 0U; x < 128; x++ )
            {
                for( uint8_t y = 62U; y < 64U; y++ )
                {
                    uint16_t u16xTemp = ((x + 0.5*(64 - y))>=128) ? (x + 0.5*(64 - y) - 128) : (x + 0.5*(64 - y));
                    MATRIX_WritePixel( u16xTemp, y, MATRIX_Hsv2Rgb( x * 12, 255, 150, 1), 0 );
                }

                for( uint8_t y = 0U; y < 2U; y++ )
                {
                    uint16_t u16xTemp = ((x + 0.5*(64 - y))>=128) ? (x + 0.5*(64 - y) - 128) : (x + 0.5*(64 - y));
                    MATRIX_WritePixel( u16xTemp, y, MATRIX_Hsv2Rgb( x * 12, 255, 150, 1), 0 );
                }
            }
        }

        switch( IR1838_GetCurrentKey() )
        {
            case IR_KEY_MENU:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                if( u8Logic )
                {
                    MATRIX_DrawCursor( u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1], 0x0, CURSOR_RIGHT );
                }
                break;
            case IR_KEY_EXIT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                if( u8Logic )
                {
                    MATRIX_DrawCursor( u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1], 0x0, CURSOR_RIGHT );
                }
                gCurrentMonitor = nPreviousMonitor;
                break;
            case IR_KEY_UP:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                if( u8Logic )
                {
                    MATRIX_DrawCursor( u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1], 0x0, CURSOR_RIGHT );
                }
                u8CurrentPoint = (0U >= u8CurrentPoint) ? (7U) : (u8CurrentPoint - 1U);
                break;
            case IR_KEY_DOWN:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                if( u8Logic )
                {
                    MATRIX_DrawCursor( u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1], 0x0, CURSOR_RIGHT );
                }
                u8CurrentPoint = (7U <= u8CurrentPoint) ? (0U) : (u8CurrentPoint + 1U);
                break;
            case IR_KEY_LEFT:
            case IR_KEY_RIGHT:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                if( u8Logic )
                {
                    MATRIX_DrawCursor( u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1], 0x0, CURSOR_RIGHT );
                }
                u8CurrentPoint += (4U > u8CurrentPoint) ? (4U) : (-4U);
                break;
            case IR_KEY_OK:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
                MATRIX_pSetupFuncs[u8CurrentPoint]();
                break;
            default:
                break;
        }

        if( (150U < HAL_GetTick() - u32CursorTick) && (MONITOR_SETTING == gCurrentMonitor))
        {
            u32CursorTick = HAL_GetTick();
            u8Logic = !u8Logic;
            MATRIX_DrawCursor( u8Point[u8CurrentPoint][0], u8Point[u8CurrentPoint][1], (u8Logic?0xFFFFU:0U), CURSOR_RIGHT );
        }
        if( (50U < (HAL_GetTick() - u32BackgroundTick )) && (MONITOR_SETTING == gCurrentMonitor) )
        {
            u32BackgroundTick = HAL_GetTick();
            MATRIX_ShiftLeft( 0U, 62U, 128U, 2U );
            MATRIX_ShiftRight( 0U, 0U, 128U, 2U );
        }
    }

    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}

uint8_t u8Index = 1U;
FRESULT SD_ScanFiles (char* pat)
{
    // UINT i;
    // char path[20];
    // sprintf (path, "%s",pat);

    // SDResult = f_opendir(&dir, path);                       /* Open the directory */
    // if (SDResult == FR_OK)
    // {
    //     for (;;)
    //     {
    //         SDResult = f_readdir(&dir, &fno);                   /* Read a directory item */
    //         // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "R:%i %s %s \r\n", SDResult, path, fno.fname);

    //         if (SDResult != FR_OK || fno.fname[0] == 0)
    //             break;  /* Break on error or end of dir */

    //         if (fno.fattrib & AM_DIR)     /* It is a directory */
    //         {
    //             if (!(strcmp ("SYSTEM~1", fno.fname)))
    //                 continue;
    //             MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "D:%s\r\n", fno.fname);
    //             i = strlen(path);
    //             sprintf(&path[i], "%s", fno.fname);
    //             SDResult = SD_ScanFiles(path);                     /* Enter the directory */
    //             // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "Sub Folder! %i %s", SDResult, path);
    //             if (SDResult != FR_OK)
    //                 break;
    //             path[i] = 0;
    //             // MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "R:%i %s\r\n", SDResult, path);
    //         }
    //         else
    //         {
    //             /* It is a file. */
    //             MATRIX_Printf( FONT_TOMTHUMB, 1U, 0U, (u8Index++) * 6, 0x1485, "F:%s/%s\r\n", path, fno.fname);
    //         }
    //     }
    //     f_closedir(&dir);
    // }
    // else
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, (u8Index++) * 6, 0xF800, "Error open dir!\n");
    // }
    // return SDResult;
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
int row_stride; /* physical row width in output buffer */
JSAMPROW nRowBuff[1]; /* Output row buffer */

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

#if 1
void MEDIA_DisplayJpeg(char * pPath)
{
    // uint8_t r, g, b;
    // uint16_t u16Color;
    // uint8_t u8Index;


    // if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1U) == FR_OK)
    // {
    //     // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "SdCard Mounted!\n");
    //     // HAL_Delay(1000);
    //     // MATRIX_FillScreen(0x0);
    // }
    // else
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "Mount Failed!\n");
    //     HAL_Delay(1000);
    //     MATRIX_FillScreen(0x0);
    // }

    // if( FR_OK==f_open(&SDFile, pPath, FA_OPEN_EXISTING | FA_READ) )
    // {
    //     /* Step 1: allocate and initialize JPEG decompression object */
    //     /* We set up the normal JPEG error routines, then override error_exit. */
    //     cinfo.err = jpeg_std_error(&jerr.pub);
    //     jerr.pub.error_exit = my_error_exit;

    //     /* Establish the setjmp return context for my_error_exit to use. */
    //     if (setjmp(jerr.setjmp_buffer))
    //     {
    //         /* If we get here, the JPEG code has signaled an error.
    //          * We need to clean up the JPEG object, close the input file, and return.
    //          */
    //         jpeg_destroy_decompress(&cinfo);
    //         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "Destroy Compression!\n");
    //         HAL_Delay(1000);
    //         MATRIX_FillScreen(0x0);
    //         f_close(&SDFile);
    //     }
    //     else
    //     {
    //         /* Now we can initialize the JPEG decompression object. */
    //         jpeg_create_decompress(&cinfo);
    //         /* Step 2: specify data source (eg, a file) */
    //         jpeg_stdio_src(&cinfo, &SDFile);
    //         /* Step 3: read file parameters with jpeg_read_header() */
    //         (void) jpeg_read_header(&cinfo, TRUE);
    //         /* Step 4: set parameters for decompression */
    //         /* In this example, we don't need to change any of the defaults set by
    //          * jpeg_read_header(), so we do nothing here.
    //          */
    //         /* Step 5: Start decompressor */
    //         (void) jpeg_start_decompress(&cinfo);
    //         /* We may need to do some setup of our own at this point before reading
    //          * the data.  After jpeg_start_decompress() we have the correct scaled
    //          * output image dimensions available, as well as the output colormap
    //          * if we asked for color quantization.
    //          * In this example, we need to make an output work buffer of the u16RightPos size.
    //          */
    //         /* JSAMPLEs per row in output buffer */
    //         /* row_stride = cinfo.output_width * cinfo.output_components; */
    //         /* Make a one-row-high sample array that will go away when done with image */
    //         /* nRowBuff[0] = (unsigned char *) malloc( row_stride ); */
    //         nRowBuff[0] = (unsigned char *) Flame_Buffer;
    //         /* Step 6: while (scan lines remain to be read) */
    //         /*           jpeg_read_scanlines(...); */
    //         while( cinfo.output_scanline < cinfo.output_height)
    //         {
    //             /* jpeg_read_scanlines expects an array of pointers to scanlines.
    //              * Here the array is only one element long, but you could ask for
    //              * more than one scanline at a time if that's more convenient.
    //              */
    //             (void) jpeg_read_scanlines(&cinfo, nRowBuff, 1);
    //             for( u8Index = 0U; u8Index < MATRIX_WIDTH; u8Index++ )
    //             {
    //                 r = (uint8_t) nRowBuff[0][ u8Index * 3 + 0 ];
    //                 g = (uint8_t) nRowBuff[0][ u8Index * 3 + 1 ];
    //                 b = (uint8_t) nRowBuff[0][ u8Index * 3 + 2 ];
    //                 r = pgm_read_byte(&gamma_table[(r * 255) >> 8]); // Gamma correction table maps
    //                 g = pgm_read_byte(&gamma_table[(g * 255) >> 8]); // 8-bit input to 4-bit output
    //                 b = pgm_read_byte(&gamma_table[(b * 255) >> 8]);

    //                 u16Color =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
    //                             (g <<  7) | ((g & 0xC) << 3) |
    //                             (b <<  1) | ( b        >> 3);
    //                 MATRIX_WritePixel( u8Index, cinfo.output_scanline - 1U, u16Color, 0 );
    //             }
    //         }
    //         /* Step 7: Finish decompression */
    //         (void) jpeg_finish_decompress(&cinfo);
    //             /* Step 8: Release JPEG decompression object */
    //         /* This is an important step since it will release a good deal of memory. */
    //         jpeg_destroy_decompress(&cinfo);
    //         /* After finish_decompress, we can close the input file. */
    //         f_close(&SDFile);
    //         /**
    //          * In case use the static buffer as a local pointer, it must not be free caused from it shall re-use another behavior
    //          * free(nRowBuff[0]);
    //          */
    //     }
    // }
    // else
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "can't open !\n");
    // }
    // f_mount(NULL, (TCHAR const*)"", 1U);
}

inline static void MEDIA_InitData(void)
{
    (void) 1U;
    /* To be define. */
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    uint8_t u8IsRandomMonitor = 1U; /* Default use random monitors */
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
    /* u8Reg &= ~( DS3231_AIEMASK | DS3231_BBSQW ); */
    /* clear INTCN to enable clock SQW */
    u8Reg &= ~DS3231_INTCN;
    /* set enable int/sqw while in battery backup flag */
    u8Reg |= DS3231_BBSQW;

    HAL_I2C_Mem_Write( &hi2c2, DS3231_ADDRESS, DS3231_REG_CONTROL, I2C_MEMADD_SIZE_8BIT, &u8Reg, 1, 1000);

    // LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
    // LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);
    // pop_noise_reduction();

    MEDIA_InitData();
    u8MaxBrightness = FLASH_Read(FLASH_MAX_BRIGHTNESS);
    u8MinBrightness = FLASH_Read(FLASH_MIN_BRIGHTNESS);

    if( 0xFFU == u8MaxBrightness)
    {
        u8MaxBrightness = 90U;
        FLASH_Write( FLASH_MAX_BRIGHTNESS, u8MaxBrightness );
    }

    if( 0xFFU == u8MinBrightness)
    {
        u8MinBrightness = 2U;
        FLASH_Write( FLASH_MIN_BRIGHTNESS, u8MinBrightness );
    }

    LL_TIM_EnableIT_UPDATE(TIM6);
    LL_TIM_EnableCounter(TIM6);

    MATRIX_setBrightness( u8MaxBrightness );

    LL_TIM_CC_EnableChannel(TIM_OE, LL_TIM_CHANNEL_CH3);
    LL_TIM_OC_SetCompareCH3(TIM_OE, MATRIX_getBrightness() );
    LL_TIM_EnableCounter(TIM_OE);

    LL_TIM_EnableIT_UPDATE(TIM_DAT);
    LL_TIM_EnableCounter(TIM_DAT);

    LL_TIM_CC_EnableChannel( TIM_IR, LL_TIM_CHANNEL_CH1 );
    LL_TIM_EnableIT_CC1( TIM_IR );
    LL_TIM_EnableCounter( TIM_IR );

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    // if( MSD_ERROR != BSP_SD_Init() )
    if( 1U )
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x1485, "SdCard Init!\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
        // CLOCK_DisableAllIrq();
        // if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1) == FR_OK)
        // {
        //     CLOCK_EnableAllIrq();

        //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "SdCard Mounted!\n");
        //     HAL_Delay(1000);
        //     MATRIX_FillScreen(0x0);
        //     // SD_ScanFiles(SDPath);
        //     // SDResult = f_readdir(&dir, &fno);                   /* Read a directory item */
        //     // HAL_Delay(1000);
        //     // MATRIX_FillScreen(0x0);
        // }
        // else
        // {
        //     CLOCK_EnableAllIrq();
        //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "Mount Failed!\n");
        //     HAL_Delay(1000);
        //     MATRIX_FillScreen(0x0);
        // }
    }
    else
    {
        CLOCK_EnableAllIrq();
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0xF800, "Init SD Failed!\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
    }

    while( 1 )
    {
        switch( IR1838_GetCurrentKey() )
        {
            case IR_KEY_MENU:
                IR1838_SetCurrentKey( IR_KEY_RESERVED );
                CLOCK_Setting();
                break;
            case IR_KEY_UP:
            case IR_KEY_DOWN:
                CLOCK_ChangesBrightness();
            case IR_KEY_LEFT:
            case IR_KEY_RIGHT:
                CLOCK_SwitchMonitor();
            case IR_KEY_0:
            case IR_KEY_1:
            case IR_KEY_2:
            case IR_KEY_3:
            case IR_KEY_4:
            case IR_KEY_5:
            case IR_KEY_6:
            case IR_KEY_7:
            case IR_KEY_8:
            case IR_KEY_9:
                CLOCK_UpdateKeyCode();
                break;
        default:
            CLOCK_BasicMonitor();
            /* Auto switch monitors */
            if( u8IsRandomMonitor )
            {
                /* Do something */
                if( (0U == (u8Minute % 3U)) && (0U == u8Second) )
                {
                    IR1838_SetCurrentKey( IR_KEY_LEFT );
                }
            }
            if( (21U == u8Hour) && (0U == u8Minute) && (10U == u8Second))
            {
                MATRIX_setBrightness(u8MinBrightness);
            }
            else if( (5U == u8Hour) && (0U == u8Minute) && (10U == u8Second))
            {
                MATRIX_setBrightness(u8MaxBrightness);
            }
            break;
        }

        /**
          * At this moment, skip get new IR codes
          * CLOCK_UpdateKeyCode();
          */

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
  NVIC_SetPriority(TIM6_DAC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
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
  NVIC_SetPriority(TIM3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),2, 0));
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

// #define S(o,n) r[t[int(h[0])/60*3+o]+o-2] = (n+h[2]-c/2)*255;
// void C(float*h,int*r)
// {
//     float g=2*h[2]-1, c=(g<0?1+g:1-g)*h[1], a=int(h[0])%120/60.f-1;
//     int t[]={2,2,2,3,1,2,3,3,0,4,2,0,4,1,1,2,3,1};
//     S(0,c) S(1,c*(a<0?1+a:1-a)) S(2,0)
// }

// The meaning of the variables:
// c - chroma
// x - intermediate value used for computing r,g,b
// m - intermediate value used for creating the r,g,b baseline
// r,g,b - the components of the RGB model (red, green, blue)
// h,s,l - the components of HSL model (hue, saturation, lightness)

// The components of the RGB model (r,g,b), saturation (s)  and intensity (i) should have values in the range [0,1], while the hue (h) should have values in the rnage [0,360].

// The prototype of the function is:
/*
 * Description:
 *  Creates a RgbFColor structure from HSL components
 * Parameters:
 *  h,s,l - the components of an HSL model expressed
 *          as real numbers
 * Returns:
 *  A pointer to the RgbFColor is the parameters are
 * correct. Otherwise returns NULL.
 */
// RgbFColor* RgbF_CreateFromHsl(double h, double s, double l);
// The function can be implemented as:
// RgbFColor* RgbF_CreateFromHsl(double h, double s, double l)
// {
//     RgbFColor* color = NULL;
//     double c = 0.0, m = 0.0, x = 0.0;
//     if (Hsl_IsValid(h, s, l) == true)
//     {
//         c = (1.0 - fabs(2 * l - 1.0)) * s;
//         m = 1.0 * (l - 0.5 * c);
//         x = c * (1.0 - fabs(fmod(h / 60.0, 2) - 1.0));
//         if (h >= 0.0 && h < (HUE_UPPER_LIMIT / 6.0))
//         {
//             color = RgbF_Create(c + m, x + m, m);
//         }
//         else if (h >= (HUE_UPPER_LIMIT / 6.0) && h < (HUE_UPPER_LIMIT / 3.0))
//         {
//             color = RgbF_Create(x + m, c + m, m);
//         }
//         else if (h < (HUE_UPPER_LIMIT / 3.0) && h < (HUE_UPPER_LIMIT / 2.0))
//         {
//             color = RgbF_Create(m, c + m, x + m);
//         }
//         else if (h >= (HUE_UPPER_LIMIT / 2.0)
//                 && h < (2.0f * HUE_UPPER_LIMIT / 3.0))
//         {
//             color = RgbF_Create(m, x + m, c + m);
//         }
//         else if (h >= (2.0 * HUE_UPPER_LIMIT / 3.0)
//                 && h < (5.0 * HUE_UPPER_LIMIT / 6.0))
//         {
//             color = RgbF_Create(x + m, m, c + m);
//         }
//         else if (h >= (5.0 * HUE_UPPER_LIMIT / 6.0) && h < HUE_UPPER_LIMIT)
//         {
//             color = RgbF_Create(c + m, m, x + m);
//         }
//         else
//         {
//             color = RgbF_Create(m, m, m);
//         }
//     }
//     return color;
// }

// static inline uint16_t MATRIX_ConvertHslToRGB( float )
// {

// }

// unction calcColor(min, max, val)
// {
//     var minHue = 240, maxHue=0;
//     var curPercent = (val - min) / (max-min);
//     var colString = "hsl(" + ((curPercent * (maxHue-minHue) ) + minHue) + ",100%,50%)";
//     return colString;
// }

void SD_PlayMotionJpeg(char * pPath)
{
    if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1U) == FR_OK)
    {
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "SdCard Mounted!\n");
        // HAL_Delay(1000);
        // MATRIX_FillScreen(0x0);
    }
    else
    {
        MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "Mount Failed!\n");
        HAL_Delay(1000);
        MATRIX_FillScreen(0x0);
    }

    if( FR_OK==f_open(&SDFile, pPath, FA_OPEN_EXISTING | FA_READ) )
    {
        // OFF_LED();
        // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "%s opened!\n", pPath);
        /* Step 1: allocate and initialize JPEG decompression object */
        // HAL_Delay(1000);
        // MATRIX_FillScreen(0x0);

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
             * In this example, we need to make an output work buffer of the u16RightPos size.
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
                    MATRIX_WritePixel( u8Index, cinfo.output_scanline - 1U, u16Color, 0 );
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

    f_mount(NULL, (TCHAR const*)"", 1U);
}
#endif

#if 1


// #define CHUNKHEADER_SIZE 8
// #define RIFF_CHUNK_SIZE (4 + CHUNKHEADER_SIZE)
// #define Media_WaveFormatEx_TagType_CHUNK_SIZE (18 + CHUNKHEADER_SIZE)



// typedef struct RIFF
// {
//     Media_ChunkHeader_TagType nHeader;
//     uint8_t u8Format[4];
// } RIFF;

/*
typedef struct Media_Fourcc_TagType {
  uint32_t fcc;
} Media_Fourcc_TagType;
*/

#define FCC(ch4) ((((uint32_t)(ch4) & 0xFF) << 24) |     \
                  (((uint32_t)(ch4) & 0xFF00) << 8) |    \
                  (((uint32_t)(ch4) & 0xFF0000) >> 8) |  \
                  (((uint32_t)(ch4) & 0xFF000000) >> 24))

#define RIFFROUND(cb) ((cb) + ((cb)&1))
#define RIFFNEXT(pChunk) (Media_LpRiffChunk_TagType)((uint8_t *)(pChunk) + sizeof(Media_RiffChunk_TagType) + RIFFROUND(((Media_LpRiffChunk_TagType)pChunk)->cb))

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

void MEDIA_PlayVideo(void)
{

    MATRIX_Printf( FONT_FREESERIF9PT7B, 1U, 2U, 31U, 0xF800U, "To be Defined! \n");
    HAL_Delay(1000U);

    /* Reset all tick values and clear the template time data */
    CLOCK_ResetDefaultState();
    MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );

    // Media_PlayInfo_TagType *playlist;

    // // unsigned char playlist_count = 0;
    // unsigned char track_count = 0;
    // unsigned char all_track_count = 0;
    // unsigned int frame_count = 0;
    // char playlist_filename[_MAX_LFN] = "list0.csv";
    // uint32_t u32PreTickFps = 0UL;
    // float fCurrkFps = 0;

    // uint32_t u32AfterDelay = 0UL;
    // uint32_t u32PreTick = 0UL;

    // // float flame_rate;

    // unsigned char loop_status = LOOP_ALL;
    // MEDIA_FileResult_Type nResult = FILE_OK;
    // // unsigned char ledpanel_bu16RightPosness = 100;
    // // unsigned char volume_value = 5;

    // // unsigned int previous_sw_value = 0;

    // // char display_text_buffer[DISPLAY_TEXT_MAX];
    // // unsigned short display_text_flame;
    // // unsigned short display_text_flame_count = 0;
    // // char status_text[DISPLAY_TEXT_MAX] = {0};

    // // char movie_time[20] = {0};
    // // unsigned char movie_total_time_min, movie_total_time_sec;
    // // unsigned char movie_current_time_min, movie_current_time_sec;

    // // char mediainfo_char_buffer[DISPLAY_TEXT_MAX];

    // // unsigned char display_OSD = SET;
    // // unsigned char display_status = SET;
    // // unsigned char display_mediainfo = RESET;

    // // unsigned short tim7_count_value;
    // // unsigned int tim7_count_add = 0;
    // // unsigned char display_fps_average_count = 0;

    // LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
    // LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);
    // pop_noise_reduction();

    // if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 1U) == FR_OK)
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0x07E0, "SdCard Mounted!\n");
    //     HAL_Delay(500U);
    //     MATRIX_FillScreen(0x0);
    // }
    // else
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 10U, 0xF800, "Mount Failed!\n");
    //     HAL_Delay(500U);
    //     MATRIX_FillScreen(0x0);
    // }

    // do
    // {
    //     IR1838_SetCurrentKey( IR_KEY_RESERVED );
    //     CLOCK_DisableAllIrq();
    //     nResult = MEDIA_GetPlayList(playlist_filename, &playlist, &all_track_count);
    //     CLOCK_EnableAllIrq();
    //     if( FILE_OK != nResult )
    //     {
    //         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 30U, 0xFFFFU, "GetPlayList?\nOK or EXIT?");
    //     }
    //     while( (FILE_OK != nResult) && (IR_KEY_EXIT != IR1838_GetCurrentKey()) && (IR_KEY_OK != IR1838_GetCurrentKey() ));
    //     if( IR_KEY_OK == IR1838_GetCurrentKey())
    //     {
    //         MATRIX_Printf( FONT_DEFAULT, 1U, 5U, 10U, 0xFFFFU, "OK!");
    //         IR1838_SetCurrentKey( IR_KEY_RESERVED );
    //     }
    //     else if( IR_KEY_EXIT == IR1838_GetCurrentKey())
    //     {
    //         MATRIX_Printf( FONT_DEFAULT, 1U, 5U, 10U, 0xF800U, "EXIT!");
    //     }
    //     HAL_Delay(500U);
    //     MATRIX_FillScreen(0x0);
    // } while( (FILE_OK != nResult) && (IR_KEY_EXIT != IR1838_GetCurrentKey()) );

    // if( FILE_OK == nResult )
    // {
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 0U, 0x07E0, "Get Playlist done!\r\nTrackCount: %i", all_track_count );
    //     HAL_Delay(500);
    //     MATRIX_FillScreen(0x0);
    //     MEDIA_SetCurrentState( MEDIA_PLAY );

    //     do
    //     {
    //         IR1838_SetCurrentKey( IR_KEY_RESERVED );
    //         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xFFFFU, "Starting...\n");
    //         HAL_Delay(500U);
    //         CLOCK_DisableAllIrq();
    //         nResult = MEDIA_ReadAviStream(&playlist[0], 0);
    //         CLOCK_EnableAllIrq();

    //         MATRIX_Printf( FONT_DEFAULT, 1U, 0U, 20U, 0xFFFFU, "ReadAviStream0?\nOK or EXIT?");
    //         while( (FILE_OK != nResult) && (IR_KEY_EXIT != IR1838_GetCurrentKey()) && (IR_KEY_OK != IR1838_GetCurrentKey() ));
    //         if( IR_KEY_OK == IR1838_GetCurrentKey())
    //         {
    //             MATRIX_Printf( FONT_DEFAULT, 1U, 20U, 10U, 0xFFFFU, "OK!");
    //             IR1838_SetCurrentKey( IR_KEY_RESERVED );
    //         }
    //         else if( IR_KEY_EXIT == IR1838_GetCurrentKey())
    //         {
    //             MATRIX_Printf( FONT_DEFAULT, 1U, 20U, 10U, 0xF800U, "EXIT!");
    //         }
    //         HAL_Delay(500U);
    //         MATRIX_FillScreen(0x0);
    //     } while( (FILE_OK != nResult) && (IR_KEY_EXIT != IR1838_GetCurrentKey()) );
    //     // display_text_flame_count = 0;
    // }

    // while((HAL_GPIO_ReadPin(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) == GPIO_PIN_RESET) && (MEDIA_PLAY == MEDIA_GetCurrentState()) && (IR_KEY_EXIT != IR1838_GetCurrentKey() && (FILE_OK == nResult)))
    // {
    //     // flame_rate = playlist[track_count].nAviFileInfo.fVideoFrameRate;
    //     // display_text_flame = (unsigned short)(flame_rate * 1000) / DISPLAY_TEXT_TIME;
    //     u32PreTick = HAL_GetTick();
    //     MEDIA_ReadAviStream(&playlist[track_count], frame_count);

    //     if(MEDIA_PLAY == MEDIA_GetCurrentState())
    //     {
    //         if(frame_count < playlist[track_count].nAviFileInfo.u32VideoLength)
    //         {
    //             frame_count++;
    //         }
    //         else
    //         {
    //             frame_count = 0;
    //             nVideoEndFlag = FLAG_SET;
    //             nAudioEndFlag = FLAG_SET;
    //         }
    //     } else if( MEDIA_PAUSE == MEDIA_GetCurrentState())
    //     {
    //         (void) 1U;
    //     }
    //     else
    //     {
    //         frame_count = 0;
    //     }

    //     if((FLAG_SET == nVideoEndFlag ) && (FLAG_SET == nAudioEndFlag))
    //     {
    //         switch(loop_status)
    //         {
    //         case LOOP_NO:
    //             MEDIA_SetCurrentState( MEDIA_STOP );
    //             // display_text_flame_count = 0;
    //             // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "Su16TopPos");
    //             break;
    //         case LOOP_SINGLE:
    //             // display_text_flame_count = 0;
    //             // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "%02u:%s", track_count, playlist[track_count].nFileName);
    //             break;
    //         case LOOP_ALL:
    //             if(track_count < (all_track_count - 1))
    //             {
    //                 track_count++;
    //             }
    //             else
    //             {
    //                 /* Reset all tick values and clear the template time data */
    //                 CLOCK_ResetDefaultState();
    //                 MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
    //                 track_count = 0;
    //             }
    //             // display_text_flame_count = 0;
    //             // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "%02u:%s", track_count, playlist[track_count].nFileName);
    //             break;
    //         default:
    //             MEDIA_SetCurrentState( MEDIA_STOP );
    //             // display_text_flame_count = 0;
    //             // snprintf(display_text_buffer, DISPLAY_TEXT_MAX, "MEDIA_STOP");
    //             break;
    //         }

    //         nVideoEndFlag = FLAG_RESET;
    //         nAudioEndFlag = FLAG_RESET;
    //     }
    //     MEDIA_UpdateStream();
    //     MATRIX_Printf( FONT_DEFAULT, 1U, 85, 56, 0xFFFF, "%i.%ifPs", (uint32_t) fCurrkFps, (uint32_t) ((fCurrkFps - (uint32_t) fCurrkFps) * 10) );
    //     MATRIX_UpdateScreen( 85U, 56U, 128U, 64U );
    //     // ON_LED();
    //     while( FLAG_RESET == MEDIA_GetAudioFlameFlag() );
    //     MEDIA_SetAudioFlameFlag( FLAG_RESET );
    //     u32AfterDelay = HAL_GetTick();

    //     if( u32AfterDelay - u32PreTickFps > 200)
    //     {
    //         u32PreTickFps = u32AfterDelay;
    //         fCurrkFps = 1000.0/(u32AfterDelay - u32PreTick);
    //     }
    //     OFF_LED();
    // }

    // /* Reset all tick values and clear the template time data */
    // IR1838_SetCurrentKey(IR_KEY_RESERVED);
    // /* Disconnection */
    // f_mount(NULL, (TCHAR const*)"", 1U);
    // CLOCK_ResetDefaultState();
    // MATRIX_TransitionEffect( 0U, 0U, 128U, 64U, TRANS_LEFT, EFFECT_FADE_OUT );
}
#endif
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

/************************ (C) COPYu16RIGHTPos STMicroelectronics *****END OF FILE****/
