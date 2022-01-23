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
#include "stm32h7xx_hal.h"
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
#define USE_SECOND_METHOD

#define MATRIX_WIDTH            256U
#define MATRIX_HEIGHT           192U
#define MATRIX_WIDTH_BIT        ((uint8_t) 8U)

#define RGB565

#if (MATRIX_HEIGHT > 0U)
    #define USE_BUS_1
#endif /* #if (MATRIX_HEIGHT > 0U) */
#if (MATRIX_HEIGHT > 64U)
    #define USE_BUS_2
#endif /* #if (MATRIX_HEIGHT > 64U) */
#if (MATRIX_HEIGHT > 128U)
    #define USE_BUS_3
#endif /* #if (MATRIX_HEIGHT > 128U) */
#if (MATRIX_HEIGHT > 192U)
    #define USE_BUS_4
#endif /* #if (MATRIX_HEIGHT > 192U) */

#if defined(USE_BUS_3) || defined(USE_BUS_4)
    #define USE_DOUBLE_BUFFER
    #define MAXTRIX_MAX_BUFFER      ((uint8_t) (128U))
    #define MAXTRIX_MAX_BUFFER_MASK ((uint8_t) (MAXTRIX_MAX_BUFFER-1U))
    #define MAXTRIX_MAX_BUFFER_BIT  ((uint8_t) (7U))
#endif /* #if defined(USE_BUS_3) || defined(USE_BUS_4) */

#define MATRIX_SCANRATE     32U
#define MATRIX_MASKROWS (uint16_t)(~(MATRIX_SCANRATE-1U))

#if defined(RGB888)
    #define TYPEDEF_BUFF uint32_t

    #define MAX_BIT   ((uint8_t) (8U))

    #define R_COLOR_SHIFT ((uint8_t) (16U))
    #define R_COLOR_MASK  ((uint8_t) (0xFFU))

    #define G_COLOR_SHIFT ((uint8_t) (8U))
    #define G_COLOR_MASK  ((uint8_t) (0xFFU))

    #define B_COLOR_SHIFT ((uint8_t) (0U))
    #define B_COLOR_MASK  ((uint8_t) (0xFFU))
#elif defined(RGB666) || defined(RGB565)
    #define TYPEDEF_BUFF uint16_t

    #define MAX_BIT   ((uint8_t) (6U))

    #define R_COLOR_SHIFT ((uint8_t) (11U))
    #define R_COLOR_MASK  ((uint8_t) (0x1FU))

    #define G_COLOR_SHIFT ((uint8_t) (6U))
    #define G_COLOR_MASK  ((uint8_t) (0x1FU))

    #define B_COLOR_SHIFT ((uint8_t) (0U))
    #define B_COLOR_MASK  ((uint8_t) (0x1FU))
#elif defined(RGB555)
    #define TYPEDEF_BUFF uint16_t

    #define MAX_BIT   ((uint8_t) (5U))

    #define R_COLOR_SHIFT ((uint8_t) (11U))
    #define R_COLOR_MASK  ((uint8_t) (0x1FU))

    #define G_COLOR_SHIFT ((uint8_t) (6U))
    #define G_COLOR_MASK  ((uint8_t) (0x1FU))

    #define B_COLOR_SHIFT ((uint8_t) (0U))
    #define B_COLOR_MASK  ((uint8_t) (0x1FU))
#elif defined(RGB444)
    #define MAX_BIT   ((uint8_t) (4U))

    #define R_COLOR_SHIFT ((uint8_t) (8U))
    #define R_COLOR_MASK  ((uint8_t) (0xFU))

    #define G_COLOR_SHIFT ((uint8_t) (4U))
    #define G_COLOR_MASK  ((uint8_t) (0xFU))

    #define B_COLOR_SHIFT ((uint8_t) (0U))
    #define B_COLOR_MASK  ((uint8_t) (0xFU))
#elif defined(RGB333)
#define MAX_BIT   ((uint8_t) (3U))
#endif

/* Port A */
#define LAT       ((uint8_t) (0U))
#define LAT_OFF   ((uint32_t) ((1<<(LAT+16))))
#define LAT_ON    ((uint32_t) (1<<LAT))
#define OE        ((uint8_t) (1U))
#define OE_OFF    ((uint32_t) ((1<<(OE+16))))
#define OE_ON     ((uint32_t) (1<<OE))
#define CLK       ((uint8_t) (2U))
#define CLK_OFF   ((uint32_t) ((1<<(CLK+16))))
#define CLK_ON    ((uint32_t) (1<<CLK))

/* Port C */
#define A         0
#define B         1
#define C         2
#define D         3
#define E         4

/* Port E */
#define R1        2
#define G1        3
#define B1        4
#define R2        5
#define G2        6
#define B2        7
#define R3        8
#define G3        9
#define B3        10
#define R4        11
#define G4        12
#define B4        13

/* Port B */
#define R5        0
#define G5        1
#define B5        2
#define R6        3
#define G6        4
#define B6        5
#define R7        6
#define G7        7
#define B7        8
#define R8        9
#define G8        10
#define B8        11

#define R1_MASK     ((uint16_t) (1U << R1))
#define G1_MASK     ((uint16_t) (1U << G1))
#define B1_MASK     ((uint16_t) (1U << B1))
#define RGB1_MASK   ((uint16_t) (R1_MASK | G1_MASK | B1_MASK))
#define R2_MASK     ((uint16_t) (1U << R2))
#define G2_MASK     ((uint16_t) (1U << G2))
#define B2_MASK     ((uint16_t) (1U << B2))
#define RGB2_MASK   ((uint16_t) (R2_MASK | G2_MASK | B2_MASK))
#define R3_MASK     ((uint16_t) (1U << R3))
#define G3_MASK     ((uint16_t) (1U << G3))
#define B3_MASK     ((uint16_t) (1U << B3))
#define RGB3_MASK   ((uint16_t) (R3_MASK | G3_MASK | B3_MASK))
#define R4_MASK     ((uint16_t) (1U << R4))
#define G4_MASK     ((uint16_t) (1U << G4))
#define B4_MASK     ((uint16_t) (1U << B4))
#define RGB4_MASK   ((uint16_t) (R4_MASK | G4_MASK | B4_MASK))

#define R5_MASK     ((uint16_t) (1U << R5))
#define G5_MASK     ((uint16_t) (1U << G5))
#define B5_MASK     ((uint16_t) (1U << B5))
#define RGB5_MASK   ((uint16_t) (R5_MASK | G5_MASK | B5_MASK))
#define R6_MASK     ((uint16_t) (1U << R6))
#define G6_MASK     ((uint16_t) (1U << G6))
#define B6_MASK     ((uint16_t) (1U << B6))
#define RGB6_MASK   ((uint16_t) (R6_MASK | G6_MASK | B6_MASK))
#define R7_MASK     ((uint16_t) (1U << R7))
#define G7_MASK     ((uint16_t) (1U << G7))
#define B7_MASK     ((uint16_t) (1U << B7))
#define RGB7_MASK   ((uint16_t) (R7_MASK | G7_MASK | B7_MASK))
#define R8_MASK     ((uint16_t) (1U << R8))
#define G8_MASK     ((uint16_t) (1U << G8))
#define B8_MASK     ((uint16_t) (1U << B8))
#define RGB8_MASK   ((uint16_t) (R8_MASK | G8_MASK | B8_MASK))

#define OE_P        GPIOA
#define CLK_P       GPIOA
#define LAT_P       GPIOA
#define ROW_P       GPIOC
#define DAT0_P      GPIOE
#define DAT1_P      GPIOB

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
#endif
/*==================================================================================================
*                                             ENUMS
==================================================================================================*/
typedef enum
{
    FONT_DEFAULT,
    FONT_TOMTHUMB,
} MATRIX_FontTypes;

typedef enum
{
    RGB_333,
    RGB_444,
    RGB_555,
    RGB_565,
    RGB_666,
    RGB_888
} MATRIX_ColorTypes;
/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/


/*==================================================================================================
*                                GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/


/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
uint8_t MATRIX_getBrightness( void );

StdReturnType MATRIX_setBrightness( uint8_t bri );
StdReturnType MATRIX_disImage( const uint8_t arr[], uint16_t nCols, uint16_t nRows,
                               uint16_t u16XStart, uint16_t u16YStart, MATRIX_ColorTypes nColorType);
StdReturnType MATRIX_Init( uint8_t bri );
StdReturnType MATRIX_FillAllColorPannel( uint8_t cR, uint8_t cG, uint8_t cB );

void MATRIX_SwapBuffers( void );
void MATRIX_DrawCircle( int16_t x0, int16_t y0, int16_t r, uint32_t u32Color );
void MATRIX_WriteLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t u32Color );
void MATRIX_DrawFastVLine( int16_t x, int16_t y, int16_t h, uint32_t u32Color );
void MATRIX_DrawFastHLine( int16_t x, int16_t y, int16_t w, uint32_t u32Color );
void MATRIX_FillRect( int16_t x, int16_t y, int16_t w, int16_t h, uint32_t u32Color );
void MATRIX_FillScreen( uint32_t u32Color );
void MATRIX_DrawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t u32Color );
void MATRIX_DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint32_t u32Color );
void MATRIX_FillCircle( int16_t x0, int16_t y0, int16_t r, uint32_t u32Color );
void MATRIX_FillCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint32_t u32Color );
void MATRIX_DrawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint32_t u32Color );
void MATRIX_DrawRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint32_t u32Color );
void MATRIX_FillRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint32_t u32Color );
void MATRIX_DrawTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t u32Color );
void MATRIX_FillTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t u32Color );
void MATRIX_WritePixel( uint16_t x, uint16_t y, uint32_t c );
void MATRIX_DrawChar( int16_t x, int16_t y, unsigned char c, uint32_t u32Color, uint16_t bg, uint8_t size_x, uint8_t size_y, MATRIX_FontTypes fontType );
size_t MATRIX_Write( uint8_t c, MATRIX_FontTypes nFontType, uint32_t u32Color );
void MATRIX_SetTextSize( uint8_t s );
void MATRIX_SetRotation( uint8_t x );
void MATRIX_SetCursor( int16_t x, int16_t y );
void MATRIX_SetTextColor( uint16_t c );
void MATRIX_Print( char *s, MATRIX_FontTypes nFontType, uint32_t u32Color );
void MATRIX_Printf( MATRIX_FontTypes nFontType, uint8_t u8TextSize,
                    uint16_t u16XPos, uint16_t u16YPos, uint32_t u32Color, char *fmt, ... );
void plasma( void );
#ifdef __cplusplus
}
#endif

#endif /* MATRIX_H */

/** @} */
