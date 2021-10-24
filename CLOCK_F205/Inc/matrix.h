/*==================================================================================================
*   @file       matrix.h
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      Project name Full color - Rgb matrix pannel - API header.
*   @details    Contains declaration of the Full color - Rgb matrix pannel API functions.
*
*   @addtogroup BUILD_ENV_COMPONENT
*   @{
*/
/*==================================================================================================
*   Project              : Full color - Rgb matrix pannel
*   Platform             : STM32H750xxx
*
*   SW Version           : 1.0.0
*   Build Version        : Reserved
*
*   Copyright 2020 ST Semiconductor, Inc.
*   All Rights Reserved.
==================================================================================================*/
/**
 * Note configuration for these pannels:
 *
 * Configuration of the project:
 *  - Clock frequency shall be from the range 72Mhz to 250Mhz.
 *  - Enable OE pin as PWM mode to configure the brightness:
 *      - Init.Period = 100, CounterMode = UP, Divison is 1.
 *  - Enable another timer to refresh monitor.
 *      - Init.Period = 60, CounterMode = UP, Divison is 1.
 *  - At the main.c shall be declared some functions like as:
 *      -   void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 *          {
 *              if( htim->Instance==TIMx)
 *                  IRQ_ProcessMonitor();
 *          }
 *      - Initialize Matrix pannels.
 *          - MATRIX_Init( 128, 64, 32, 20, false );
 *          - MATRIX_SetTextSize(1);
 *          - MATRIX_SetCursor(1, 6);
 *          - MATRIX_SetTextColor(0xFFFF);
 *      - Start PWM:
 *          - HAL_TIM_PWM_Start(&htim5,TIM_CHANNEL_2);
 *          - HAL_TIM_Base_Start_IT(&htim4);
 *          - __HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_2,MATRIX_getBrightness());
 *  - To avoid the screen flickering, the priority of TIM matrix must be set a highest priority.
 *  - Step by step for matrix pannel to process the current monitor:
 *      - Calculate the duration or get from the array gTimeCount
 *      - Load new division value into TIM_DAT->PSC = duration;
 *      - Trigger EGR upto 1 in order re-load PSC value TIM_DAT->EGR = 1U;
 *      - Clear interrupt flag TIM_DAT->SR = 0xFFFFFFFE;
 *      - Latch data loaded during *prior* interrupt LAT_P->BSRR = LAT_BIT_SET;
 *      - Disable LED output during gRows/gBitPos switchover TIM_OE->CCRx = 0U;
 *      - Release new data region for loop MATRIX_WIDTH -> CLK LOW -> DAT_P->ODR -> CLK HIGH
 *      - Enable LED output TIM_OE->CCR3 = gBrightness;
 *      - Latch data again LAT_P->BSRR = LAT_BIT_SET;
 *      - Trigger the current row ROW_P->ODR = gRows;
 *      - Calculate the next row, next bit.
 *      !!!!! Note:
 *          - For P4 pannel: LAT_P shall be pull down first, then pull up.
 *          - For P3 pannel: LAT_P shall be pull up first, then pull down.
 */
#ifndef MATRIX_H
#define MATRIX_H

#ifdef __cplusplus
extern "C"{
#endif


/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "stm32f2xx.h"
#include "Std_Types.h"
#include <stdbool.h>
#include <stdlib.h>
#include "math.h"
#include <string.h>
#include <stdarg.h>

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/

#define RGB555
// #define USE2BUS

#define RECORD_TEXT_NO_BACKGROUND
// #define RECORD_LAST_FRAME

#define MATRIX_WIDTH     128U
#define MATRIX_HEIGHT    64U
#define MATRIX_TOLTAL    ((MATRIX_WIDTH)*(MATRIX_HEIGHT))

#ifdef USE2BUS
#define MATRIX_SCANRATE  (MATRIX_HEIGHT/4U)
#else
#define MATRIX_SCANRATE  (MATRIX_HEIGHT/2U)
#endif /* #ifdef USE2BUS */
#define MATRIX_MASKROWS  MATRIX_SCANRATE-1U

#if defined(RGB888)
#define MAX_BIT   8
#elif defined(RGB666) || defined(RGB565)
#define MAX_BIT   6
#elif defined(RGB555)
#define MAX_BIT   5
#elif defined(RGB444)
#define MAX_BIT   4
#elif defined(RGB333)
#define MAX_BIT   3
#endif

#define CLK       0   	//A
#define LAT       1		//A
#define OE        2  	//A

#define CLK_BIT_RESET       (uint32_t) (1<<(CLK+16))
#define CLK_BIT_SET         (uint32_t) (1<<CLK)
#define LAT_BIT_RESET       (uint32_t) (1<<(LAT+16))
#define LAT_BIT_SET         (uint32_t) (1<<LAT)
#define LAT_BIT_SET_MASK    (uint32_t) (~((uint32_t)LAT_BIT_SET))
#define LAT_BIT_RESET_MASK  (uint32_t) (~((uint32_t)LAT_BIT_RESET))
#define CLK_BIT_SET_MASK    (uint32_t) (~((uint32_t)CLK_BIT_SET))
#define CLK_BIT_RESET_MASK  (uint32_t) (~((uint32_t)CLK_BIT_RESET))

#define A         0   //C
#define B         1   //C
#define C         2   //C
#define D         3   //C
#define E         4   //C

#define R1        3
#define B1        4
#define G1        5
#define R2        0
#define G2        1
#define B2        2

#define R1_MASK     (uint16_t) (1 << R1)
#define G1_MASK     (uint16_t) (1 << G1)
#define B1_MASK     (uint16_t) (1 << B1)
#define RGB1_MASK   (uint16_t) (R1_MASK | G1_MASK | B1_MASK)

#define R2_MASK     (uint16_t) (1 << R2)
#define G2_MASK     (uint16_t) (1 << G2)
#define B2_MASK     (uint16_t) (1 << B2)
#define RGB2_MASK   (uint16_t) (R2_MASK | G2_MASK | B2_MASK)
#define BUS1_MASK   (uint16_t) (RGB1_MASK | RGB2_MASK)

#ifdef USE2BUS
#define R3        6
#define G3        7
#define B3        8
#define R4        9
#define B4        12
#define G4        13

#define R3_MASK     (uint16_t) (1 << R3)
#define G3_MASK     (uint16_t) (1 << G3)
#define B3_MASK     (uint16_t) (1 << B3)
#define RGB3_MASK   (uint16_t) (R3_MASK | G3_MASK | B3_MASK)

#define R4_MASK     (uint16_t) (1 << R4)
#define G4_MASK     (uint16_t) (1 << G4)
#define B4_MASK     (uint16_t) (1 << B4)
#define RGB4_MASK   (uint16_t) (R4_MASK | G4_MASK | B4_MASK)
#endif /* #ifdef USE2BUS */

#define CLK_P       GPIOA
#define LAT_P       GPIOA
#define OE_P        GPIOA

#define ROW_P       GPIOC
#define DAT_P       GPIOB

#define TIM_DAT TIM4
#define TIM_OE  TIM5

#define _PI 3.14159265359

#ifndef pgm_read_byte
    #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
    #define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
    #define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...
#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
 #define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
 #define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif /* #if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF) */
/*==================================================================================================
*                                             ENUMS
==================================================================================================*/
typedef enum
{
    FONT_DEFAULT,
    FONT_TOMTHUMB,
    FONT_FREESERIF9PT7B,
    FONR_FREESERIFBOLDITALIC18PT7B,
    FONR_FREESERIFBOLDITALIC12PT7B,
    FONR_FREESAN9PT7B,
    FONT_FREEMONO9PT7B,
    FONT_ORG_01
} MATRIX_FontTypes;

typedef enum
{
    MONITOR_DISPLAY_MON0,
    MONITOR_DISPLAY_MON1,
    MONITOR_DISPLAY_ANALOG0,
    MONITOR_DISPLAY_MON2,
    MONITOR_DISPLAY_MON3,
    MONITOR_DISPLAY_RANDOM,
    MONITOR_SETTING,
    MONITOR_SET_KEY,
    MONITOR_SET_TIME,
    MONITOR_SET_COLOR,
    MONITOR_SET_LIGHT,
    MONITOR_SET_STRING,
    MONITOR_SPE_AUDIO_SPEC,
    MONITOR_SPE_PLAY_VID,
    MONITOR_EXIT,
    MONITOR_RESERVED
} MATRIX_MonitorTypes;

typedef enum
{
    TRANS_RANDOM,
    TRANS_UP,
    TRANS_DOWN,
    TRANS_LEFT,
    TRANS_RIGHT,
    TRANS_RESERVED
} MATRIX_TransitionTypes;

typedef enum
{
    EFFECT_FADE_IN,
    EFFECT_FADE_OUT,
    EFFECT_RESERVED
} MATRIX_EffectTypes;

typedef enum
{
    CURSOR_RESERVED,
    CURSOR_UP,
    CURSOR_DOWN,
    CURSOR_LEFT,
    CURSOR_RIGHT
} MATRIX_DirectionalCursor_Types;

typedef enum
{
    FLUTTER_LEFT_TO_RIGHT,
    FLUTTER_RIGHT_TO_LEFT,
    FLUTTER_RESERVED
} MATRIX_FlutterTypes;
/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/


/*==================================================================================================
*                                GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/
extern MATRIX_MonitorTypes gCurrentMonitor;

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
uint8_t MATRIX_getBrightness( void );

StdReturnType MATRIX_setBrightness( uint8_t bri );

void MATRIX_TransitionEffect(uint16_t u16xStart, uint16_t u16yStart,
                             uint16_t u16Width, uint16_t u16Height,
                             MATRIX_TransitionTypes nTransition, MATRIX_EffectTypes nEffect);
void MATRIX_ShiftLeft(uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height);
void MATRIX_ShiftRight(uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height);
void MATRIX_FlutterLeftRight(uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height);
void MATRIX_MoveRegion(uint16_t u16xStart, uint16_t u16yStart, uint16_t u16xEnd, uint16_t u16yEnd, uint8_t u8Width, uint8_t u8Height);
void MATRIX_DispImage( const uint8_t * pData, uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height);
void MATRIX_DrawCircle( int16_t x0, int16_t y0, int16_t r, uint16_t u16Color );
void MATRIX_WriteLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t u16Color );
void MATRIX_DrawFastVLine( int16_t x, int16_t y, int16_t h, uint16_t u16Color );
void MATRIX_DrawFastHLine( int16_t x, int16_t y, int16_t w, uint16_t u16Color );
void MATRIX_FillRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t u16u16Color );
void MATRIX_FillScreen( uint16_t u16Color );
void MATRIX_DrawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t u16Color );
void MATRIX_DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t u16Color );
void MATRIX_FillCircle( int16_t x0, int16_t y0, int16_t r, uint16_t u16Color );
void MATRIX_FillCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t u16Color );
void MATRIX_DrawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t u16Color );
void MATRIX_DrawRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t u16Color );
void MATRIX_FillRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t u16Color );
void MATRIX_DrawTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t u16Color );
void MATRIX_FillTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t u16Color );
void MATRIX_WritePixel( uint16_t x, uint16_t y, uint16_t c, uint8_t u8LockPos );
void MATRIX_DrawChar( int16_t x, int16_t y, unsigned char c, uint8_t size_x, uint8_t size_y, MATRIX_FontTypes nFontType, uint16_t u16Color );
size_t MATRIX_Write( uint8_t c, MATRIX_FontTypes nFontType, uint16_t u16Color );
void MATRIX_Printf( MATRIX_FontTypes nFontType, uint8_t u8TextSize, uint16_t u16XPos, uint16_t u16YPos, uint16_t u16Color, char *fmt, ... );
void MATRIX_UpdateScreen(uint8_t u8xStart, uint8_t u8yStart, uint8_t u8xEnd, uint8_t u8yEnd);
void MATRIX_DrawCursor(uint8_t u8xPos, uint8_t u8yPos, uint16_t u16Color, MATRIX_DirectionalCursor_Types nCursor);
uint16_t MATRIX_Hsv2Rgb( int16_t s16Hue, uint8_t u8Sat, uint8_t u8Val, char gflag);
uint8_t MATRIX_IsPixelColored(uint16_t u16xPos, uint16_t u16yPos);
void MATRIX_FillRainbowColorToRegion(uint8_t u8xStart, uint8_t u8yStart, uint8_t u8xEnd, uint8_t u8yEnd, uint16_t u16ColorStart, uint16_t u16ColorEnd, float fTilt, uint8_t u8GammaFlag);
void plasma( void );

void IRQ_ProcessMonitor( void );
#ifdef __cplusplus
}
#endif

#endif /* MATRIX_H */

/** @} */
