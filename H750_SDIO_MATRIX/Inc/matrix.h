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
#include "stm32h7xx.h"
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
#define MATRIX_WIDTH    64U
#define MATRIX_HEIGHT   32U
#define MATRIX_SCANRATE  16U
#define MATRIX_MASKROWS MATRIX_SCANRATE-1U
#define BRIGHTNESS 10
#define RGB565
// #define USE2BUS

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

#define LAT       0     //A
#define OE         1     //A
#define CLK       2     //A

#define CLK_BIT_RESET       (uint32_t) (1<<(CLK+16))
#define CLK_BIT_SET         (uint32_t) (1<<CLK)
#define LAT_BIT_RESET       (uint32_t) (1<<(LAT+16))
#define LAT_BIT_SET         (uint32_t) (1<<LAT)
#define LAT_BIT_SET_MASK    (uint32_t) (~((uint32_t)LAT_BIT_SET))
#define LAT_BIT_RESET_MASK  (uint32_t) (~((uint32_t)LAT_BIT_RESET))
#define CLK_BIT_SET_MASK    (uint32_t) (~((uint32_t)CLK_BIT_SET))
#define CLK_BIT_RESET_MASK  (uint32_t) (~((uint32_t)CLK_BIT_RESET))

#define A         0   /*! C */
#define B         1   /*! C */
#define C         2   /*! C */
#define D         3   /*! C */
#define E         4   /*! C */

#define B1        8
#define R1        6
#define G1        7
#define R2        11
#define G2        9
#define B2        10

#define R1_MASK     (uint16_t) (1 << R1)
#define G1_MASK     (uint16_t) (1 << G1)
#define B1_MASK     (uint16_t) (1 << B1)
#define RGB1_MASK   (uint16_t) (R1_MASK | G1_MASK | B1_MASK)

#define R2_MASK     ((uint16_t) 1 << R2)
#define G2_MASK     ((uint16_t) 1 << G2)
#define B2_MASK     ((uint16_t) 1 << B2)
#define RGB2_MASK   ((uint16_t) R2_MASK | G2_MASK | B2_MASK)

#ifdef USE2BUS
#define B3        5
#define R3        3
#define G3        4
#define R4        1
#define G4        2
#define B4        0

#define R3_MASK     (uint16_t) (1 << R3)
#define G3_MASK     (uint16_t) (1 << G3)
#define B3_MASK     (uint16_t) (1 << B3)
#define RGB3_MASK   (uint16_t) (R3_MASK | G3_MASK | B3_MASK)

#define R4_MASK     ((uint16_t) 1 << R4)
#define G4_MASK     ((uint16_t) 1 << G4)
#define B4_MASK     ((uint16_t) 1 << B4)
#define RGB4_MASK   ((uint16_t) R4_MASK | G4_MASK | B4_MASK)
#endif /* USE2BUS */

#define OE_P        GPIOA
#define ROW_P       GPIOC
#define CLK_P       GPIOA
#define LAT_P       GPIOA
#define DAT_P       GPIOB
#define ROW_MASK    0x1F

#define TIM_DAT TIM4
#define TIM_OE  TIM2

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
StdReturnType MATRIX_disImage( const uint8_t arr[], uint16_t nCols, uint16_t nRows);
StdReturnType MATRIX_Init( uint8_t bri );
StdReturnType MATRIX_FillAllColorPannel( uint8_t cR, uint8_t cG, uint8_t cB );

void MATRIX_SwapBuffers( void );
void MATRIX_DrawCircle( int16_t x0, int16_t y0, int16_t r, uint16_t color );
void MATRIX_WriteLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color );
void MATRIX_DrawFastVLine( int16_t x, int16_t y, int16_t h, uint16_t color );
void MATRIX_DrawFastHLine( int16_t x, int16_t y, int16_t w, uint16_t color );
void MATRIX_FillRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color );
void MATRIX_FillScreen( uint16_t color );
void MATRIX_DrawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color );
void MATRIX_DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color );
void MATRIX_FillCircle( int16_t x0, int16_t y0, int16_t r, uint16_t color );
void MATRIX_FillCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color );
void MATRIX_DrawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color );
void MATRIX_DrawRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color );
void MATRIX_FillRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color );
void MATRIX_DrawTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color );
void MATRIX_FillTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color );
void MATRIX_WritePixel( uint16_t x, uint16_t y, uint32_t c );
void MATRIX_DrawChar( int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y, MATRIX_FontTypes fontType );
size_t MATRIX_Write( uint8_t c, MATRIX_FontTypes fontType );
void MATRIX_SetTextSize( uint8_t s );
void MATRIX_SetRotation( uint8_t x );
void MATRIX_SetCursor( int16_t x, int16_t y );
void MATRIX_SetTextColor( uint16_t c );
void MATRIX_Print( uint8_t *s, MATRIX_FontTypes fontType );
void MATRIX_Printf( MATRIX_FontTypes fontType, char *fmt, ... );
void plasma( void );
#ifdef __cplusplus
}
#endif

#endif /* MATRIX_H */

/** @} */
