/*================================================================================================*/
/**
*   @file       matrix.c
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      Matrix program body.
*   @details    This is a demo application showing the usage of the API.
*
*   This file contains sample code only. It is not part of the production code deliverables.
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
#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "matrix.h"
#include "gamma.h"
#include "gfxfont.h"
#include "TomThumb.h"
#include "FreeSerifBoldItalic18pt7b.h"
#include "FreeSerifBoldItalic12pt7b.h"
#include "FreeSans9pt7b.h"
#include "FreeSerif9pt7b.h"
#include "FreeMono9pt7b.h"
#include "Org_01.h"

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
/*
 * 15 for RGB444 at 144Mhz
 * 10 for RGB555 at 144Mhz
 *
 */
#define CALLOVERHEAD    15U
/*
 * 25 for RGB444 at 144Mhz
 * 20 for RGB555 at 144Mhz
 */
#define LOOPTIME        25U
#define IRQ_TIMEDELAY   100UL

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

// static uint16_t const gTimeCount[8]={10, 20, 40, 80, 160, 240, 320}; // he so chia nap vao timer ok with 256x64 47.9Hz
// static uint16_t const gTimeCount[8]={15, 30, 60, 120, 160, 240, 320}; // he so chia nap vao timer ok with 256x64 47.9Hz
// uint16_t const gTimeCount[8]={20, 40, 80, 160, 320, 640, 480, 600}; // he so chia nap vao timer ok with 256x64 47.9Hz
// uint16_t const gTimeCount[8]={30, 60, 120, 320, 480, 640, 480, 600}; // he so chia nap vao timer ok with 256x64 47.9Hz
// uint16_t const gTimeCount[8]={ 50, 80, 160, 240, 300, 640, 480, 600}; //ok case 1: still glitch
#if defined(RGB444)
uint16_t const gTimeCount[8]={ 50, 80, 150, 230, 300, 400, 480, 600}; // ok case 2: still flicker but lower than case 1 at RGB444 160Mhz or 170Mhz
#elif defined(RGB555)
uint16_t const gTimeCount[8]={8, 16, 32, 64, 128, 256, 512}; // ok case 2: still flicker but lower than case 1 at RGB555 160Mhz or 170Mhz
#elif defined(RGB565)
uint16_t const gTimeCount[8]={ 33, 60, 110, 180, 250, 320, 480, 600}; // ok case 3: still flicker at RGB565 160Mhz or 170Mhz
#endif /* #if defined(RGB565) */

static const int8_t sinetab[256] =
{
     0,   2,   5,   8,  11,  15,  18,  21,
    24,  27,  30,  33,  36,  39,  42,  45,
    48,  51,  54,  56,  59,  62,  65,  67,
    70,  72,  75,  77,  80,  82,  85,  87,
    89,  91,  93,  96,  98, 100, 101, 103,
   105, 107, 108, 110, 111, 113, 114, 116,
   117, 118, 119, 120, 121, 122, 123, 123,
   124, 125, 125, 126, 126, 126, 126, 126,
   127, 126, 126, 126, 126, 126, 125, 125,
   124, 123, 123, 122, 121, 120, 119, 118,
   117, 116, 114, 113, 111, 110, 108, 107,
   105, 103, 101, 100,  98,  96,  93,  91,
    89,  87,  85,  82,  80,  77,  75,  72,
    70,  67,  65,  62,  59,  56,  54,  51,
    48,  45,  42,  39,  36,  33,  30,  27,
    24,  21,  18,  15,  11,   8,   5,   2,
     0,  -3,  -6,  -9, -12, -16, -19, -22,
   -25, -28, -31, -34, -37, -40, -43, -46,
   -49, -52, -55, -57, -60, -63, -66, -68,
   -71, -73, -76, -78, -81, -83, -86, -88,
   -90, -92, -94, -97, -99,-101,-102,-104,
  -106,-108,-109,-111,-112,-114,-115,-117,
  -118,-119,-120,-121,-122,-123,-124,-124,
  -125,-126,-126,-127,-127,-127,-127,-127,
  -128,-127,-127,-127,-127,-127,-126,-126,
  -125,-124,-124,-123,-122,-121,-120,-119,
  -118,-117,-115,-114,-112,-111,-109,-108,
  -106,-104,-102,-101, -99, -97, -94, -92,
   -90, -88, -86, -83, -81, -78, -76, -73,
   -71, -68, -66, -63, -60, -57, -55, -52,
   -49, -46, -43, -40, -37, -34, -31, -28,
   -25, -22, -19, -16, -12,  -9,  -6,  -3
};

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
/**
 * @brief Number of multiplexed rows; actual MATRIX_HEIGHT is 2X this.
 */
static uint8_t gRows = 0U;
// static uint8_t gRows = 0;

/**
 * @brief The actual postion bit is handling.
 */
static uint8_t gBitPos = 0U;
// static uint8_t gBitPos = 0;

/**
 * @brief The current brightness value
 */
static uint8_t gBrightness;
static bool gCp437 = false;
#if defined(RECORD_LAST_FRAME) || defined(RECORD_TEXT_NO_BACKGROUND)
#if 16U==MATRIX_SCANRATE
static uint16_t gRecordFrame[MATRIX_WIDTH/MATRIX_SCANRATE][MATRIX_HEIGHT][2] = {0UL};
#elif 32U==MATRIX_SCANRATE
static uint32_t gRecordFrame[MATRIX_WIDTH/MATRIX_SCANRATE][MATRIX_HEIGHT][2] = {0UL};
#endif /* 16U==MATRIX_SCANRATE */
static uint8_t  gCurrFrame = 0U;
#if 16U==MATRIX_SCANRATE
static const uint16_t gBitMask[MATRIX_SCANRATE] =
#elif 32U==MATRIX_SCANRATE
static const uint32_t gBitMask[MATRIX_SCANRATE] =
#endif /* 16U==MATRIX_SCANRATE */
{
    0x1,
    0x2,
    0x4,
    0x8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000,
#if 32U==MATRIX_SCANRATE
    0x10000,
    0x20000,
    0x40000,
    0x80000,
    0x100000,
    0x200000,
    0x400000,
    0x800000,
    0x1000000,
    0x2000000,
    0x4000000,
    0x8000000,
    0x10000000,
    0x20000000,
    0x40000000,
    0x80000000,
#endif /* #if 32U==MATRIX_SCANRATE */
};
#endif /* #if defined(RECORD_LAST_FRAME) || defined(RECORD_TEXT_NO_BACKGROUND) */
static uint16_t gCursorX = 0U;
static uint16_t gCursorY = 0U;
static uint8_t  gTextSizeX = 1U;
static uint8_t  gTextSizeY = 1U;
static MATRIX_FlutterTypes gFlutterState = FLUTTER_RIGHT_TO_LEFT;
// static GFXfont *gfxFont = &TomThumb;       ///< Pointer to special font
static const GFXfont *gfxFont = NULL;       ///< Pointer to special font
static volatile uint32_t gCountBit = 0U;
uint16_t gIsrBuff[MATRIX_WIDTH*MAX_BIT];
TYPEDEF_BUFF gMainBuff[MATRIX_WIDTH][MATRIX_HEIGHT];
/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * The C library function int vsprintf(char *str, const char *format, va_list arg)
 * sends formatted output to a string using an argument list passed to it.
 */
int vsprintf(char *str, const char *format, va_list arg);

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/
static inline uint8_t * pgm_read_bitmap_ptr( const GFXfont *gfxFont )
{
    return  pgm_read_pointer( &gfxFont->bitmap );
}

static inline GFXglyph * pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c)
{
    return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
}

static inline void MATRIX_Print( char *s, MATRIX_FontTypes nFontType, uint16_t u16Color )
{
    while( *s )
    {
        MATRIX_Write( *s++, nFontType, u16Color );
    }
}

static inline void MATRIX_ExecuteTransitionLeft(uint16_t u16xStart, uint16_t u16yStart,
                                                uint16_t u16Width, uint16_t u16Height,
                                                MATRIX_EffectTypes nEffect)
{
    uint16_t const u16yEnd = u16yStart + u16Height;
    uint8_t u8xPos, u8yPos, u8Count = u16Width;
    uint16_t u16yCurrPos;

    if( EFFECT_FADE_IN == nEffect )
    {

    }
    else
    {
        while( u8Count--)
        {
            for( u8yPos = u16yStart; u8yPos < u16yEnd; u8yPos++ )
            {
                for( u8xPos = u16xStart; u8xPos < u8Count; u8xPos++ )
                {
                    gMainBuff[u8xPos][u8yPos] = gMainBuff[u8xPos + 1U][u8yPos];
                }
                gMainBuff[u8xPos][u8yPos] = 0U;
            }

            /* Reset all scan frame in order to refresh new data *MATRIX_TransitionEffect/
            gBitPos = 0U;
            gRows = 0U; */
            /* Previous states */
            // for( u8yPos = u16yStart; u8yPos < MATRIX_SCANRATE; u8yPos++ )
            // {
            //     for( u8xPos = u16xStart; u8xPos < u8Count; u8xPos++ )
            //     {
            //         u16yCurrPos = u8yPos * MATRIX_WIDTH;
            //         gMainBuff[u16yCurrPos + u8xPos] = gMainBuff[u16yCurrPos + u8xPos + 1U];
            //         gMainBuff[ MATRIX_TOLTAL -  MATRIX_WIDTH - u16yCurrPos + u8xPos] = gMainBuff[ MATRIX_TOLTAL -  MATRIX_WIDTH - u16yCurrPos + u8xPos + 1U];
            //     }
            //     gMainBuff[u8yPos * MATRIX_WIDTH + u8xPos] = 0U;
            //     gMainBuff[ MATRIX_TOLTAL -  (1 + u8yPos) * MATRIX_WIDTH + u8xPos] = 0U;
            // }

            /* Delay in few times */
            HAL_Delay( 8U );
        }
    }
}

static inline void MATRIX_ExecuteTransitionRight(MATRIX_EffectTypes nEffect)
{
    uint8_t u8xPos, u8yPos, u8Count = MATRIX_WIDTH;
    (void) nEffect;

    if( EFFECT_FADE_IN == nEffect )
    {

    }
    else
    {
        while( u8Count-- )
        {
            for( u8yPos = 1U; u8yPos < MATRIX_HEIGHT; u8yPos++ )
            {
                for( u8xPos = 0U; u8xPos < MATRIX_WIDTH - 1U; u8xPos++ )
                {
                    gMainBuff[u8xPos][u8yPos] = gMainBuff[u8xPos + 1U][u8yPos];
                }
                gMainBuff[u8xPos][u8yPos] = 0U;
            }
            HAL_Delay( 50 );
        }
    }
}

/**
 * @brief   Write a pixel, overwrite in subclasses if startWrite is defined!
 * @details
 *          Write a pixel, overwrite in subclasses if startWrite is defined,
 *          For example scan with 5bit per color per pixel
 *          For each | w | instance, they store MATRIX_HEIGHT pixels.
 *          |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |
 *          | b0 | b1 | b2 | b3 | b4 | b0 | b1 | b2 | b3 | b4 | b0 | b1 | b2 | b3 | b4 |
 *          |---------- r0 ----------|---------- r1 ----------|---------- r2 ----------|
 *          Total elements are MATRIX_WIDTH * MATRIX_HEIGHT * MAX_BIT. Actual only need to scan a half
 *          part of one matrix.
 *
 * @param   x   x coordinate
 * @param   y   y coordinate
 * @param   color 16-bit 5-6-5 Color to fill with
 *
 * @retval  StdReturnType
 *          E_OK: Write a pexel successfully.
 *          E_NOT_OK Otherwise.
 */
 static inline void MATRIX_DrawPixel(uint16_t x, uint16_t y, uint16_t u16Color)
{
    if( (x < 0) || (x >= MATRIX_WIDTH) || (y < 0) || (y >= MATRIX_HEIGHT) )
    {
        return;
    }

    gMainBuff[x][y] = (TYPEDEF_BUFF) u16Color;
}

static void MATRIX_ProcessNextRow(void)
{
    uint32_t _curPos, color;
    uint16_t u16Idx;
    uint16_t bit, limit;
    uint8_t r, g, b;

    for( u16Idx = 0U; u16Idx < MATRIX_WIDTH; u16Idx++ )
    {
        /* Process the first part */
        color = gMainBuff[u16Idx][gRows];
        r = (color >>  R_COLOR_SHIFT) & R_COLOR_MASK;
        g = (color >>  G_COLOR_SHIFT) & G_COLOR_MASK;
        b = (color >>  B_COLOR_SHIFT) & B_COLOR_MASK;

        bit   = 1;
        limit = 1 << MAX_BIT;
        _curPos = u16Idx;

        while( bit < limit )
        {
           gIsrBuff[_curPos] &= ~RGB1_MASK;                   /* 00000111b Mask out R,G,B in one op */
           if( r & bit ) gIsrBuff[_curPos] |= R1_MASK;     /* Plane N R: bit 6 0000.0100.0000b     */
           if( g & bit ) gIsrBuff[_curPos] |= G1_MASK;     /* Plane N G: bit 7 0000.1000.0000b     */
           if( b & bit ) gIsrBuff[_curPos] |= B1_MASK;    /* Plane N B: bit 8 0001.0000.0000b     */
            _curPos  += MATRIX_WIDTH;                /* Advance to next bit plane */
            bit <<= 1 ;
        }

        /* Process the second part */
        color = gMainBuff[u16Idx][gRows | MATRIX_SCANRATE];
        r = (color >>  R_COLOR_SHIFT) & R_COLOR_MASK;
        g = (color >>  G_COLOR_SHIFT) & G_COLOR_MASK;
        b = (color >>  B_COLOR_SHIFT) & B_COLOR_MASK;

        bit   = 1;
        limit = 1 << MAX_BIT;
        _curPos = u16Idx;

        while( bit < limit )
        {
           gIsrBuff[_curPos] &= ~RGB2_MASK;                   /* 00000111b Mask out R,G,B in one op */
           if( r & bit ) gIsrBuff[_curPos] |= R2_MASK;     /* Plane N R: bit 6 0000.0100.0000b     */
           if( g & bit ) gIsrBuff[_curPos] |= G2_MASK;     /* Plane N G: bit 7 0000.1000.0000b     */
           if( b & bit ) gIsrBuff[_curPos] |= B2_MASK;    /* Plane N B: bit 8 0001.0000.0000b     */
            _curPos  += MATRIX_WIDTH;                /* Advance to next bit plane */
            bit <<= 1 ;
        }

#ifdef USE2BUS
        /* Process the first part */
        color = gMainBuff[u16Idx][gRows | (MATRIX_SCANRATE << 1)];
        r = (color >>  R_COLOR_SHIFT) & R_COLOR_MASK;
        g = (color >>  G_COLOR_SHIFT) & G_COLOR_MASK;
        b = (color >>  B_COLOR_SHIFT) & B_COLOR_MASK;

        bit   = 1;
        limit = 1 << MAX_BIT;
        _curPos = u16Idx;

        while( bit < limit )
        {
           gIsrBuff[_curPos] &= ~RGB3_MASK;                   /* 00000111b Mask out R,G,B in one op */
           if( r & bit ) gIsrBuff[_curPos] |= R3_MASK;     /* Plane N R: bit 6 0000.0100.0000b     */
           if( g & bit ) gIsrBuff[_curPos] |= G3_MASK;     /* Plane N G: bit 7 0000.1000.0000b     */
           if( b & bit ) gIsrBuff[_curPos] |= B3_MASK;    /* Plane N B: bit 8 0001.0000.0000b     */
            _curPos  += MATRIX_WIDTH;                /* Advance to next bit plane */
            bit <<= 1 ;
        }

        /* Process the second part */
        color = gMainBuff[u16Idx][gRows | MATRIX_SCANRATE | (MATRIX_SCANRATE << 1)];
        r = (color >>  R_COLOR_SHIFT) & R_COLOR_MASK;
        g = (color >>  G_COLOR_SHIFT) & G_COLOR_MASK;
        b = (color >>  B_COLOR_SHIFT) & B_COLOR_MASK;

        bit   = 1;
        limit = 1 << MAX_BIT;
        _curPos = u16Idx;

        while( bit < limit )
        {
           gIsrBuff[_curPos] &= ~RGB4_MASK;                   /* 00000111b Mask out R,G,B in one op */
           if( r & bit ) gIsrBuff[_curPos] |= R4_MASK;     /* Plane N R: bit 6 0000.0100.0000b     */
           if( g & bit ) gIsrBuff[_curPos] |= G4_MASK;     /* Plane N G: bit 7 0000.1000.0000b     */
           if( b & bit ) gIsrBuff[_curPos] |= B4_MASK;    /* Plane N B: bit 8 0001.0000.0000b     */
            _curPos  += MATRIX_WIDTH;                /* Advance to next bit plane */
            bit <<= 1 ;
        }
#endif /* #ifdef USE_BUS_2 */
    }
}
/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
/**
  * @brief   Resfresh function for active display.
  * @details
  *             We have some plans for scan each bit matrix. it shall be listed below. Each case shall use the difference sources, timing..
  *             1. Decode pixel to the actual possition, IRQ only need move data to peripherals.
  *                 For example scan with 5bit per color per pixel
  *                 For each | w | instance, they store MATRIX_HEIGHT pixels.
  *                 |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |  w |
  *                 | b0 | b1 | b2 | b3 | b4 | b0 | b1 | b2 | b3 | b4 | b0 | b1 | b2 | b3 | b4 |
  *                 |---------- r0 ----------|---------- r1 ----------|---------- r2 ----------|
  *                 Total elements are MATRIX_WIDTH * MATRIX_HEIGHT * MAX_BIT. Actual only need to scan a half
  *                 part of one matrix.
  *             2. Only use the one data array gMainBuff[MATRIX_WIDTH * MATRIX_HEIGHT]. In this case, only prefer use the color code are RGB565, RGB555, RGB444...
  *                 Each interrupt event occured, we want to print out each bit possition like as below algorithm. This exemple shall be declare RGB565 encode
  *                 | R4 R3 R2 R1 R0 | G5 G4 G3 G2 G1 G0 | B4 B3 B2 B1 B0 |
  *                 u16TempDat = gMainBuff[ y * MATRIX_WIDTH + x ];
  *                 DAT_P->ODR = gMainBuff[ y * MATRIX_WIDTH + x ];
  * @param  None.
  * @retval None.
  */
void IRQ_ProcessMonitor( void )
{
    uint16_t u16Color = 0U;
    uint16_t u16Count = MATRIX_WIDTH;
    uint16_t t, duration;
    uint8_t u8Timeout = IRQ_TIMEDELAY;

    TIM_OE->CCR3 = 0U;

    /*! Step 1: Clock in the data for the current row one bit at a time */
    while( u16Count-- )
    {
        DAT_P->ODR = gIsrBuff[ gCountBit ] & BUS1_MASK;
        CLK_P->BSRR=CLK_OFF;
        gCountBit++;
        CLK_P->BSRR=CLK_ON;
    }

    /*! Step 2:
     *  Pull the latch and output enable pins high. This enables the latch,
     *  allowing the row data to reach the output driver but it also disables
     *  the output so that no LEDs are lit while we're switching rows.
     */
    TIM_DAT->PSC = gTimeCount[gBitPos];
    /* Trigger EGR upto 1 in order re-load PSC value */
    TIM_DAT->EGR = 1U;
    /* Clear interrupt flag */
    TIM_DAT->SR = 0xFFFFFFFE;

    /* Latch data loaded during *prior* interrupt */
    LAT_P->BSRR = LAT_ON;

    /*! Step 3: Switch rows by driving the appropriate row select lines */
    ROW_P->ODR = gRows;

    /*! Step 4:
     *  Pull the latch and output enable pins low again,
     *  enabling the output and closing the latch so we
     *  can clock in the next row of data.
     */
    LAT_P->BSRR = LAT_OFF;
    TIM_OE->CCR3 = gBrightness;
    // TIM_OE->CCR3 = (2 * (1 << gBitPos));

    if( ++gBitPos >= MAX_BIT )
    {
        gBitPos = 0U;
        if( ++gRows >= MATRIX_SCANRATE )
        {
            gRows = 0U;
        }
        gCountBit = 0UL;
        MATRIX_ProcessNextRow();
    }
}

/**
 * Note: It shall only move the data region in the same bank cause from the difference bit possition between two banks.
 */
inline void MATRIX_MoveRegion(uint16_t u16xStart, uint16_t u16yStart, uint16_t u16xEnd, uint16_t u16yEnd, uint8_t u8Width, uint8_t u8Height)
{
    uint16_t u16xCurPos, u16yCurPos;

    if( u16xStart < u16xEnd )
    {
        if( u16yStart < u16yEnd )
        {
            /**
             * Center -> 4h
             */
            for( u16yCurPos = u8Height; u16yCurPos > 0; u16yCurPos-- )
            {
                for( u16xCurPos = u8Width; u16xCurPos > 0; u16xCurPos-- )
                {
                    gMainBuff[u16xCurPos - 1U + u16xEnd][u16yCurPos - 1U + u16yEnd] = gMainBuff[u16xCurPos - 1U + u16xStart][u16yCurPos - 1U + u16yStart];
                    gMainBuff[u16xCurPos - 1U + u16xStart][u16yCurPos - 1U + u16yStart] = 0U;
                }
            }
        }
        else
        {
            /**
             * Center -> 2h
             */
            for( u16yCurPos = 0U; u16yCurPos < u8Height; u16yCurPos++ )
            {
                for( u16xCurPos = u8Width; u16xCurPos > 0; u16xCurPos-- )
                {
                    gMainBuff[u16xCurPos - 1U + u16xEnd][u16yCurPos + u16yEnd] = gMainBuff[u16xCurPos - 1U + u16xStart][u16yCurPos + u16yStart];
                    gMainBuff[u16xCurPos - 1U + u16xStart][u16yCurPos  + u16yStart] = 0U;
                }
            }
        }
    }
    else
    {
        if( u16yStart < u16yEnd )
        {
            /**
             * Center -> 8h
             */
            for( u16yCurPos = u8Height; u16yCurPos > 0; u16yCurPos-- )
            {
                for( u16xCurPos = 0U; u16xCurPos < u8Width; u16xCurPos++ )
                {
                    gMainBuff[u16xCurPos + u16xEnd][u16yCurPos - 1U + u16yEnd] = gMainBuff[u16xCurPos + u16xStart][u16yCurPos - 1U + u16yStart];
                    gMainBuff[u16xCurPos + u16xStart][u16yCurPos - 1U + u16yStart] = 0U;
                }
            }
        }
        else
        {
            /**
             * Center -> 10h
             */
            for( u16yCurPos = 0U; u16yCurPos < u8Height; u16yCurPos++ )
            {
                for( u16xCurPos = 0U; u16xCurPos < u8Width; u16xCurPos++ )
                {
                    gMainBuff[u16xCurPos + u16xEnd][u16yCurPos + u16yEnd] = gMainBuff[u16xCurPos + u16xStart][u16yCurPos + u16yStart];
                    gMainBuff[u16xCurPos + u16xStart][u16yCurPos + u16yStart] = 0U;
                }
            }
        }
    }
}

inline void MATRIX_DispImage( const uint8_t * pData, uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height)
{
    uint8_t u8xPos, u8yPos;
    uint8_t r, g, b;
    uint32_t uColor;

    (void) pData;

    /* Horizon scan */
    for( u8yPos = u16yStartedPos; u8yPos < (u16yStartedPos + u8Height); u8yPos++ )
    {
        for( u8xPos = u16xStartedPos; u8xPos < (u16xStartedPos + u8Width); u8xPos++ )
        {

            r = pData[ u8yPos * u8Width*3 + u8xPos * 3 + 2];
            g = pData[ u8yPos * u8Width*3 + u8xPos * 3 + 1];
            b = pData[ u8yPos * u8Width*3 + u8xPos * 3 + 0];

            // r = (uColor >> 11U) & 0x1F;
            // g = (uColor >>  5U) & 0x3F;
            // b = (uColor >>  0U) & 0x1F;

            r = pgm_read_byte(&gamma_table[(r * 255) >> 8]); // Gamma correction table maps
            g = pgm_read_byte(&gamma_table[(g * 255) >> 8]); // 8-bit input to 4-bit output
            b = pgm_read_byte(&gamma_table[(b * 255) >> 8]);

            uColor =  (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                        (g <<  7) | ((g & 0xC) << 3) |
                        (b <<  1) | ( b        >> 3);

            gMainBuff[u8xPos][u8yPos] = (TYPEDEF_BUFF) uColor;
        }
    }
}

inline void MATRIX_DrawCursor(uint8_t u8xPos, uint8_t u8yPos, uint16_t u16Color, MATRIX_DirectionalCursor_Types nCursor)
{
    if( CURSOR_LEFT == nCursor )
    {
        /* Right to left */
        MATRIX_FillRect( u8xPos + 1U, u8yPos - 1U, 6U, 3U, u16Color);
        MATRIX_DrawLine( u8xPos, u8yPos, u8xPos + 2U, u8yPos - 2U, u16Color);
        MATRIX_DrawLine( u8xPos, u8yPos, u8xPos + 2U, u8yPos + 2U, u16Color);
    }
    else if( CURSOR_RIGHT == nCursor )
    {
        /* Left to right */
        MATRIX_FillRect( u8xPos, u8yPos - 1U, 6U, 3U, u16Color);
        MATRIX_DrawLine( u8xPos + 6U, u8yPos, u8xPos + 4U, u8yPos - 2U, u16Color);
        MATRIX_DrawLine( u8xPos + 6U, u8yPos, u8xPos + 4U, u8yPos + 2U, u16Color);
    }
    else if( CURSOR_UP == nCursor )
    {
        /* Down to up */
        MATRIX_FillRect( u8xPos - 1U, u8yPos - 6U, 3U, 6U, u16Color);
        MATRIX_DrawLine( u8xPos, u8yPos - 7U, u8xPos - 2U, u8yPos - 5U, u16Color);
        MATRIX_DrawLine( u8xPos, u8yPos - 7U, u8xPos + 2U, u8yPos - 5U, u16Color);
    }
    else if( CURSOR_DOWN == nCursor )
    {
        /* up to down */
        MATRIX_FillRect( u8xPos - 1U, u8yPos, 3U, 6U, u16Color);
        MATRIX_DrawLine( u8xPos, u8yPos + 6U, u8xPos + 2U, u8yPos + 4U, u16Color);
        MATRIX_DrawLine( u8xPos, u8yPos + 6U, u8xPos - 2U, u8yPos + 4U, u16Color);
    }
}

inline void MATRIX_FlutterLeftRight(uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height)
{
    uint16_t u16ColorBackup;
    uint8_t u8xPos, u8yPos;

    (void) u16xStartedPos;

    if( FLUTTER_RIGHT_TO_LEFT == gFlutterState )
    {
        for( u8yPos = u16yStartedPos; u8yPos < (u16yStartedPos + u8Height); u8yPos++ )
        {
            if( gMainBuff[u16xStartedPos][u8yPos] )
            {
                gFlutterState = FLUTTER_LEFT_TO_RIGHT;
                break;
            }
        }
    }
    else if( FLUTTER_LEFT_TO_RIGHT == gFlutterState )
    {
        for( u8yPos = u16yStartedPos; u8yPos < (u16yStartedPos + u8Height - 1U); u8yPos++ )
        {
            if( gMainBuff[u16xStartedPos + u8Width - 1U][u8yPos] )
            {
                gFlutterState = FLUTTER_RIGHT_TO_LEFT;
                break;
            }
        }
    }

    if( FLUTTER_RIGHT_TO_LEFT == gFlutterState )
    {
        for( u8yPos = u16yStartedPos; u8yPos < (u16yStartedPos + u8Height); u8yPos++ )
        {
            u16ColorBackup = gMainBuff[u16xStartedPos][u8yPos];
            for( u8xPos = u16xStartedPos; u8xPos < (u16xStartedPos + u8Width - 1U ); u8xPos++ )
            {
                gMainBuff[u8xPos][u8yPos] = gMainBuff[u8xPos + 1U][u8yPos];
            }
            gMainBuff[u8xPos][u8yPos] = u16ColorBackup;
        }
    }
    else if( FLUTTER_LEFT_TO_RIGHT == gFlutterState )
    {
        for( u8yPos = u16yStartedPos; u8yPos < (u16yStartedPos + u8Height); u8yPos++ )
        {
            u16ColorBackup = gMainBuff[u16xStartedPos + u8Width - 1U][u8yPos];
            for( u8xPos = (u16xStartedPos + u8Width - 2U ); u8xPos >= u16xStartedPos; u8xPos-- )
            {
                gMainBuff[u8xPos + 1U][u8yPos] = gMainBuff[u8xPos][u8yPos];
            }
            gMainBuff[u16xStartedPos][u8yPos] = u16ColorBackup;
        }
    }
}

inline void MATRIX_ShiftLeft(uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height)
{
    uint16_t u16ColorBackup;
    uint8_t u8xPos, u8yPos;

    for( u8yPos = u16yStartedPos; u8yPos < (u16yStartedPos + u8Height); u8yPos++ )
    {
        u16ColorBackup = gMainBuff[u16xStartedPos][u8yPos];
        for( u8xPos = u16xStartedPos; u8xPos < (u16xStartedPos + u8Width - 1U ); u8xPos++ )
        {
            gMainBuff[u8xPos][u8yPos] = gMainBuff[u8xPos + 1U][u8yPos];
        }
        gMainBuff[u8xPos][u8yPos] = u16ColorBackup;
    }
}

inline void MATRIX_ShiftRight(uint16_t u16xStartedPos, uint16_t u16yStartedPos, uint8_t u8Width, uint8_t u8Height)
{
    uint16_t u16ColorBackup;
    uint16_t const u16xEnd = u16xStartedPos + u8Width - 1U;
    uint16_t const u16yEnd = u16yStartedPos + u8Height;
    uint16_t u16yCurrPos = 0U;
    uint8_t u8xPos, u8yPos;

    for( u8yPos = u16yStartedPos; u8yPos < u16yEnd; u8yPos++ )
    {
        u16ColorBackup = gMainBuff[u16xEnd][u8yPos];
        for( u8xPos = u16xEnd; u8xPos > u16xStartedPos; u8xPos-- )
        {
            gMainBuff[u8xPos][u8yPos] = gMainBuff[u8xPos - 1U][u8yPos];
        }
        gMainBuff[u8xPos][u8yPos] = u16ColorBackup;
    }
}

inline void MATRIX_TransitionEffect(uint16_t u16xStart, uint16_t u16yStart,
                             uint16_t u16Width, uint16_t u16Height,
                             MATRIX_TransitionTypes nTransition, MATRIX_EffectTypes nEffect)
{

    switch( nTransition )
    {
        case TRANS_UP:
            break;
        case TRANS_DOWN:
            break;
        case TRANS_LEFT:
            MATRIX_ExecuteTransitionLeft( u16xStart, u16yStart, u16Width, u16Height, nEffect );
            break;
        case TRANS_RIGHT:
            MATRIX_ExecuteTransitionRight( nEffect );
            break;
        case TRANS_RANDOM:
            break;
        default:
            break;
    }
}

void MATRIX_UpdateScreen(uint8_t u8xStart, uint8_t u8yStart, uint8_t u8xEnd, uint8_t u8yEnd)
{
    uint8_t u8xIdx;
    uint8_t u8yIdx;

    (void) u8xStart;

#ifdef RECORD_LAST_FRAME
    gCurrFrame = !gCurrFrame;
    for( uint8_t u8xIdx = 0; u8xIdx < u8xEnd; u8xIdx++ )
    {
        for( uint8_t u8yIdx = u8yStart; u8yIdx < u8yEnd; u8yIdx++ )
        {
            if( gRecordFrame[u8xIdx/MATRIX_SCANRATE][u8yIdx][gCurrFrame]&gBitMask[u8xIdx%MATRIX_SCANRATE] )
            {
                if( 0 == (gRecordFrame[u8xIdx/MATRIX_SCANRATE][u8yIdx][1 - gCurrFrame]&gBitMask[u8xIdx%MATRIX_SCANRATE]) )
                {
                    MATRIX_DrawPixel(u8xIdx, u8yIdx, 0x0);
                }
                gRecordFrame[u8xIdx/MATRIX_SCANRATE][u8yIdx][gCurrFrame]&=(uint32_t)~gBitMask[u8xIdx%MATRIX_SCANRATE];
            }
        }
    }
#elif defined(RECORD_TEXT_NO_BACKGROUND)
    gCurrFrame = !gCurrFrame;
    for( uint8_t u8xIdx = u8xStart; u8xIdx < u8xEnd; u8xIdx++ )
    {
        for( uint8_t u8yIdx = u8yStart; u8yIdx < u8yEnd; u8yIdx++ )
        {
            if( gRecordFrame[u8xIdx/MATRIX_SCANRATE][u8yIdx][gCurrFrame]&gBitMask[u8xIdx%MATRIX_SCANRATE] )
            {
                gRecordFrame[u8xIdx/MATRIX_SCANRATE][u8yIdx][gCurrFrame]&=(uint32_t)~gBitMask[u8xIdx%MATRIX_SCANRATE];
            }
        }
    }
#endif /* #ifdef RECORD_LAST_FRAME */
}

/**
 * @brief   Thís function fill rainbow color affect to a region pannel.
 * @details
 *          Calculate the actual color according to the current pixels and overide the old data.
 *
 * @param[in]   u8xStart The X start of the region (Top left).
 *              u8yStart The Y start of the region (Top left).
 *              u8xEnd The X end of the region (Button Right).
 *              u8yEnd The Y end of the region (Button Right).
 *              u16ColorStart The start of hue color.
 *              u16ColorEnd The end of hue color.
 *              fTilt The tilt of the actual color region
 *              u8GammaFlag calibrate the RGB color? use gamma table.
 *
 * @retval  void
 */
inline void MATRIX_FillRainbowColorToRegion(uint8_t u8xStart, uint8_t u8yStart, uint8_t u8xEnd, uint8_t u8yEnd,
                            uint16_t u16ColorStart, uint16_t u16ColorEnd, float fTilt, uint8_t u8GammaFlag)
{
    uint8_t const u8Muliple = (u16ColorEnd - u16ColorStart)/(u8xEnd - u8xStart) - 1U;
    uint8_t const u8Height = u8yEnd - u8yStart + 1U;
    uint8_t const u8Width = u8xEnd - u8xStart + 1U;
    uint8_t u8xCurPos = 0U;
    uint8_t u8yCurPos = 0U;
    uint8_t u8xTemp = 0U;

    for( u8yCurPos = u8yStart; u8yCurPos <= u8yEnd; u8yCurPos++ )
    {
        for( u8xCurPos = u8xStart; u8xCurPos <= u8xEnd; u8xCurPos++ )
        {
            /**
             *  |-----------------------------|
             * 0 |     *                  *    |
             * 1 | *                  *        |
             * 2 |    *(1)        *(2)     *   |
             * 3 |            *        *       |
             * 4 |        *        *           |
             * 5 |    *        *               |
             *   |_____________________________|
             *
             */
            float fCal = fTilt * (u8Height - (u8yCurPos - u8yStart));
            u8xTemp = ((u8xCurPos - (uint8_t) fCal) < u8xStart) ?
                    (u8xCurPos - (uint8_t) fCal + u8Width) :
                    (u8xCurPos - (uint8_t) fCal);

            /* Check whether the expected pixel has data */
            if( gMainBuff[u8xCurPos][u8yCurPos] )
            {
                /* Overide the rainbow color to the current pixel */
                MATRIX_WritePixel( u8xCurPos, u8yCurPos,
                                   MATRIX_Hsv2Rgb( (u8xTemp - u8xStart) * u8Muliple + u16ColorStart,
                                                    255U, 150U, u8GammaFlag
                                                 ), 0U
                                 );
            }
        }
    }
}


/**
  * @brief  None.
  *
  * @param  None.
  * @retval None.
  */
uint8_t MATRIX_getBrightness( void )
{
    return gBrightness;
}

/**
  * @brief  None.
  *
  * @param  None.
  * @retval None.
  */
StdReturnType MATRIX_setBrightness( uint8_t bri )
{
    gBrightness = bri;
    return E_OK;
}


inline void MATRIX_WritePixel( uint16_t x, uint16_t y, uint16_t c, uint8_t u8LockPos )
{
    uint32_t u32Temp = 0UL;

    u32Temp = x >> 16U;
    u32Temp = u32Temp >> 16U;
    if( ( 0U == u8LockPos) || ((1U == u8LockPos) && (0 == (gRecordFrame[x/MATRIX_SCANRATE][y][1 - gCurrFrame]&gBitMask[x%MATRIX_SCANRATE]))) )
    {
        MATRIX_DrawPixel( x, y, c );
    }
}

uint16_t MATRIX_Hsv2Rgb( int16_t s16Hue, uint8_t u8Sat, uint8_t u8Val, char gflag)
{
    uint8_t  r, g, b, lo;
    uint16_t s1, v1;

    // s16Hue
    s16Hue %= 1536;             // -1535 to +1535
    if(s16Hue < 0)
        s16Hue += 1536; //     0 to +1535
    lo = s16Hue & 255;          // Low byte  = primary/secondary color mix
    switch(s16Hue >> 8)
    {       // High byte = sextant of colorwheel
        case 0 : r = 255     ; g =  lo     ; b =   0     ; break; // R to Y
        case 1 : r = 255 - lo; g = 255     ; b =   0     ; break; // Y to G
        case 2 : r =   0     ; g = 255     ; b =  lo     ; break; // G to C
        case 3 : r =   0     ; g = 255 - lo; b = 255     ; break; // C to B
        case 4 : r =  lo     ; g =   0     ; b = 255     ; break; // B to M
        default: r = 255     ; g =   0     ; b = 255 - lo; break; // M to R
    }

    // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
    // on the result rather than a costly divide, while the type upgrade to int
    // avoids repeated type conversions in both directions.
    s1 = u8Sat + 1;
    r  = 255 - (((255 - r) * s1) >> 8);
    g  = 255 - (((255 - g) * s1) >> 8);
    b  = 255 - (((255 - b) * s1) >> 8);

    // Value (brightness) & 16-bit color reduction: similar to above, add 1
    // to allow shifts, and upgrade to int makes other conversions implicit.
    v1 = u8Val + 1;
    if(gflag)
    { // Gamma-corrected color?
        r = pgm_read_byte(&gamma_table[(r * v1) >> 8]); // Gamma correction table maps
        g = pgm_read_byte(&gamma_table[(g * v1) >> 8]); // 8-bit input to 4-bit output
        b = pgm_read_byte(&gamma_table[(b * v1) >> 8]);
    }
    else
    { // linear (uncorrected) color
        r = (r * v1) >> 12; // 4-bit results
        g = (g * v1) >> 12;
        b = (b * v1) >> 12;
    }
            // rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
    return (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
            (g <<  7) | ((g & 0xC) << 3) |
            (b <<  1) | ( b        >> 3);
}

void plasma( void )
{
    int           x1, x2, x3, x4, y1, y2, y3, y4, sx1, sx2, sx3, sx4;
    unsigned int x, y;
    long          value;
const float radius1 =16.3, radius2 =23.0, radius3 =40.8, radius4 =44.2,
            centerx1=16.1, centerx2=11.6, centerx3=23.4, centerx4= 4.1,
            centery1= 8.7, centery2= 6.5, centery3=14.0, centery4=-2.9;
double       angle1  = 0.0, angle2  = 0.0, angle3  = 0.0, angle4  = 0.0;
long        hueShift= 0;
        // uint16_t xReserved;
        // uint8_t j, i;
    while (1)
    {
        // HAL_Delay(100);
        // HAL_Delay(800);
        // for( j = 0; j < 7; j++ )
        // {
        //     xReserved = gMainBuff[j * 128 ];
        //     for( i = 0; i < 127; i++ )
        //     {
        //         gMainBuff[j * 128 + i] = gMainBuff[j * 128 + i +1 ];
        //     }
        //     gMainBuff[j * 128 + i] = xReserved;
        // }
        sx1 = (int)(cos(angle1) * radius1 + centerx1);
        sx2 = (int)(cos(angle2) * radius2 + centerx2);
        sx3 = (int)(cos(angle3) * radius3 + centerx3);
        sx4 = (int)(cos(angle4) * radius4 + centerx4);
        y1  = (int)(sin(angle1) * radius1 + centery1);
        y2  = (int)(sin(angle2) * radius2 + centery2);
        y3  = (int)(sin(angle3) * radius3 + centery3);
        y4  = (int)(sin(angle4) * radius4 + centery4);

        for(y=0; y<MATRIX_HEIGHT; y++)
        {
            x1 = sx1; x2 = sx2; x3 = sx3; x4 = sx4;
            for(x=0; x<MATRIX_WIDTH; x++)
            {
                value = hueShift
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x1 * x1 + y1 * y1) >> 5))
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x2 * x2 + y2 * y2) >> 5))
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x3 * x3 + y3 * y3) >> 6))
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x4 * x4 + y4 * y4) >> 6));
#ifdef RECORD_LAST_FRAME
                    if( (gRecordFrame[x/MATRIX_SCANRATE][y][1 - gCurrFrame]&gBitMask[x%MATRIX_SCANRATE]) )
#endif /* #ifdef RECORD_LAST_FRAME */
                    {
                        MATRIX_DrawPixel(x, y, MATRIX_Hsv2Rgb( value * 5, 255, 150, 1) );
                    }
                    // matrix.drawPixel(x, y, MATRIX_Hsv2Rgb(value * 3, 255, 255, 1));
                x1--; x2--; x3--; x4--;
            }
        y1--; y2--; y3--; y4--;
        }
        return ;
        // MATRIX_SwapBuffers();

        angle1 += 0.03;
        angle2 -= 0.07;
        angle3 += 0.13;
        angle4 -= 0.15;
        hueShift += 0.02;
    }
}


// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

/**
 *  @brief   Draw a single character
 *
 *  @param    x   Bottom left corner x coordinate
 *  @param    y   Bottom left corner y coordinate
 *  @param    c   The 8-bit font-indexed character (likely ascii)
 *  @param    color 16-bit 5-6-5 Color to draw chraracter with
 *  @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
 *  @param    size_x  Font magnification level in X-axis, 1 is 'original' size
 *  @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
void MATRIX_DrawChar( int16_t x, int16_t y, unsigned char c, uint8_t size_x, uint8_t size_y, MATRIX_FontTypes nFontType, uint16_t u16Color )
{
    if( FONT_DEFAULT == nFontType )
    { // 'Classic' built-in font
    // if(1) { // 'Classic' built-in font

        if((x >= MATRIX_WIDTH)            || // Clip right
           (y >= MATRIX_HEIGHT)           || // Clip bottom
           ((x + 6 * size_x - 1) < 0) || // Clip left
           ((y + 8 * size_y - 1) < 0))   // Clip top
            return;

        if(!gCp437 && (c >= 176)) c++; // Handle 'classic' charset behavior
        // MATRIX_FillRect(x, y, 5*size_x, 8*size_y, 0x0);
        for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
            uint8_t line = pgm_read_byte(&font[c * 5 + i]);
            for(int8_t j=0; j<8; j++, line >>= 1) {
                if(line & 1) {
                    if(size_x == 1 && size_y == 1)
                    {
#ifdef RECORD_LAST_FRAME
                        if( !u16Color )
                        {
                            gRecordFrame[(x+i)/MATRIX_SCANRATE][y+j][gCurrFrame] |= gBitMask[(x+i)%MATRIX_SCANRATE];
                        }
                        else
#endif /* #ifdef RECORD_LAST_FRAME */
                        {
#ifdef RECORD_TEXT_NO_BACKGROUND
                            gRecordFrame[(x+i)/MATRIX_SCANRATE][y+j][gCurrFrame] |= gBitMask[(x+i)%MATRIX_SCANRATE];
#endif /* #ifdef RECORD_TEXT_NO_BACKGROUND */
                            MATRIX_DrawPixel(x+i, y+j, u16Color);
                        }
                    }
                    else
                    {
                        MATRIX_FillRect(x+i*size_x, y+j*size_y, size_x, size_y, u16Color);
                    }
                }
            }
        }
        // if(bg != color) { // If opaque, draw vertical line for last column
        //     if(size_x == 1 && size_y == 1) MATRIX_DrawFastVLine(x+5, y, 8, bg);
        //     else          MATRIX_FillRect(x+5*size_x, y, size_x, 8*size_y, bg);
        // }

    } else { // Custom font

        // Character is assumed previously filtered by write() to eliminate
        // newlines, returns, non-printable characters, etc.  Calling
        // drawChar() directly with 'bad' characters of font may cause mayhem!

        c -= (uint8_t)pgm_read_byte(&gfxFont->first);
        GFXglyph *glyph  = pgm_read_glyph_ptr(gfxFont, c);
        uint8_t  *bitmap = pgm_read_bitmap_ptr(gfxFont);

        uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
        uint8_t  w  = pgm_read_byte(&glyph->width),
                 h  = pgm_read_byte(&glyph->height);
        int8_t   xo = pgm_read_byte(&glyph->xOffset),
                 yo = pgm_read_byte(&glyph->yOffset);
        uint8_t  xx, yy, bits = 0, bit = 0;
        int16_t  xo16 = 0, yo16 = 0;

        if(size_x > 1 || size_y > 1) {
            xo16 = xo;
            yo16 = yo;
        }

        // Todo: Add character clipping here

        // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
        // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
        // has typically been used with the 'classic' font to overwrite old
        // screen contents with new data.  This ONLY works because the
        // characters are a uniform size; it's not a sensible thing to do with
        // proportionally-spaced fonts with glyphs of varying sizes (and that
        // may overlap).  To replace previously-drawn text when using a custom
        // font, use the getTextBounds() function to determine the smallest
        // rectangle encompassing a string, erase the area with fillRect(),
        // then draw new text.  This WILL infortunately 'blink' the text, but
        // is unavoidable.  Drawing 'background' pixels will NOT fix this,
        // only creates a new set of problems.  Have an idea to work around
        // this (a canvas object type for MCUs that can afford the RAM and
        // displays supporting setAddrWindow() and pushColors()), but haven't
        // implemented this yet.
        // MATRIX_FillRect(x, y-h, w/2, h, 0x0);
        for(yy=0; yy<h; yy++) {
            for(xx=0; xx<w; xx++) {
                if(!(bit++ & 7)) {
                    bits = pgm_read_byte(&bitmap[bo++]);
                }
                if(bits & 0x80)
                {
                    if(size_x == 1 && size_y == 1)
                    {
#ifdef RECORD_LAST_FRAME
                        if( !u16Color )
                        {
                            gRecordFrame[(x+xo+xx)/MATRIX_SCANRATE][y+yo+yy][gCurrFrame] |= gBitMask[(x+xo+xx)%MATRIX_SCANRATE];
                        }
                        else
#endif /* #ifdef RECORD_LAST_FRAME */
                        {
                            MATRIX_DrawPixel(x+xo+xx, y+yo+yy, u16Color);
                        }
                    }
                    else
                    {
                        MATRIX_FillRect( x+(xo16+xx)*size_x, y+(yo16+yy)*size_y,
                          size_x, size_y, u16Color );
                    }
                }
                bits <<= 1;
            }
        }

    } // End classic vs custom font
}

/**
 *  @brief  Print one byte/character of data, used to support print()
 *
 *  @param  c  The 8-bit ascii character to write
 */
size_t MATRIX_Write( uint8_t c, MATRIX_FontTypes nFontType, uint16_t u16Color )
{
    if( FONT_DEFAULT == nFontType ) // 'Classic' built-in font
    {
        if(c == '\n') {                        // Newline?
            gCursorX  = 0;                     // Reset x to zero,
            gCursorY += gTextSizeY * 8;        // advance y one line
        } else if(c != '\r') {                 // Ignore carriage returns
            if(((gCursorX + gTextSizeX * 6) > MATRIX_WIDTH)) { // Off right?
                gCursorX  = 0;                 // Reset x to zero,
                gCursorY += gTextSizeY * 8;    // advance y one line
            }
            MATRIX_DrawChar(gCursorX, gCursorY, c, gTextSizeX, gTextSizeY, nFontType, u16Color );
            gCursorX += gTextSizeX * 6;          // Advance x one char
        }
    } else
    {
        if(c == '\n') {
            gCursorX  = 0;
            gCursorY += (int16_t)gTextSizeY *
                        (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        }
        else if(c != '\r')
        {
            uint8_t first = pgm_read_byte(&gfxFont->first);
            if( (c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last)) )
            {
                GFXglyph *glyph  = pgm_read_glyph_ptr(gfxFont, c - first);
                uint8_t   w     = pgm_read_byte(&glyph->width),
                          h     = pgm_read_byte(&glyph->height);
                if((w > 0) && (h > 0))
                {
                    int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset);

                    if(((gCursorX + gTextSizeX * (xo + w/2)) > MATRIX_WIDTH))
                    {
                        gCursorX  = 0;
                        gCursorY += (int16_t)gTextSizeY * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                        if( gCursorY > MATRIX_HEIGHT)
                        {
                            gCursorY = (int16_t)gTextSizeY * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                        }
                    }
                    MATRIX_DrawChar(gCursorX, gCursorY, c, gTextSizeX, gTextSizeY, nFontType, u16Color );
                }
                gCursorX += (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)gTextSizeX;
            }
        }
    }
    return 1;
}

/**
 *   @brief      Set rotation setting for display
 *
 *   @param  x   0 thru 3 corresponding to 4 cardinal rotations
 */
void MATRIX_SetRotation( uint8_t x )
{
    (void) x;
    // switch(x & 3) {
    //     case 0:
    //     case 2:
    //         _width  = WIDTH;
    //         _height = HEIGHT;
    //         break;
    //     case 1:
    //     case 3:
    //         _width  = HEIGHT;
    //         _height = WIDTH;
    //         break;
    // }
}

void MATRIX_Printf( MATRIX_FontTypes nFontType, uint8_t u8TextSize,
                    uint16_t u16XPos, uint16_t u16YPos, uint16_t u16Color, char *fmt, ... )
{
    va_list argp;
    char string[200];

    va_start(argp, fmt);

    if( 0U < vsprintf( string, fmt, argp ) )
    {
        gTextSizeX = (u8TextSize > 0) ? u8TextSize : 1;
        gTextSizeY = (u8TextSize > 0) ? u8TextSize : 1;

        if( (0xFFFFU != gCursorX) && (0xFFFFU != gCursorY) )
        {
            gCursorX = u16XPos;
            gCursorY = u16YPos;
        }

        if( FONT_TOMTHUMB == nFontType)
        {
            gfxFont = &TomThumb;
        }
        else if( FONT_FREESERIF9PT7B == nFontType )
        {
            gfxFont = &FreeSerif9pt7b;
        }
        else if ( FONR_FREESERIFBOLDITALIC18PT7B == nFontType )
        {
            gfxFont = &FreeSerifBoldItalic18pt7b;
        }
        else if ( FONR_FREESERIFBOLDITALIC12PT7B == nFontType )
        {
            gfxFont = &FreeSerifBoldItalic12pt7b;
        }
        else if ( FONT_FREEMONO9PT7B == nFontType )
        {
            gfxFont = &FreeMono9pt7b;
        }
        else if ( FONR_FREESAN9PT7B == nFontType )
        {
            gfxFont = &FreeSans9pt7b;
        }
        else if ( FONT_ORG_01 == nFontType )
        {
            gfxFont = &Org_01;
        }

        MATRIX_Print( string, nFontType, u16Color );
    }

    va_end(argp);
}




/**
 *  @brief    Fill the screen completely with one color. Update in subclasses if desired!
 *
 *  @param    color 16-bit 5-6-5 Color to fill with
 */
inline void MATRIX_FillScreen(uint16_t u16Color )
{
    for( uint8_t u8xPos = 0U; u8xPos < MATRIX_WIDTH; u8xPos++ )
    {
        for( uint8_t u8yPos = 0U; u8yPos < MATRIX_WIDTH; u8yPos++ )
        {
            MATRIX_DrawPixel( u8xPos, u8yPos, u16Color );
        }
    }
}

/**
 *  @brief   Draw a rectangle with no fill color
 *
 *  @param    x   Top left corner x coordinate
 *  @param    y   Top left corner y coordinate
 *  @param    w   Width in pixels
 *  @param    h   Height in pixels
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
inline void MATRIX_DrawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t u16Color  )
{
    MATRIX_DrawFastHLine( x, y, w, u16Color );
    MATRIX_DrawFastHLine( x, y+h-1, w, u16Color );
    MATRIX_DrawFastVLine( x, y, h, u16Color );
    MATRIX_DrawFastVLine( x+w-1, y, h, u16Color );
}

/**
 *   @brief    Draw a perfectly horizontal line (this is often optimized in a subclass!)
 *
 *   @param    x   Left-most x coordinate
 *   @param    y   Left-most y coordinate
 *   @param    w   Width in pixels
 *   @param    color 16-bit 5-6-5 Color to fill with
 */
inline void MATRIX_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t u16Color )
{
    MATRIX_WriteLine( x, y, x+w-1, y, u16Color );
}

/**
 *   @brief    Draw a perfectly vertical line (this is often optimized in a subclass!)
 *
 *   @param    x   Top-most x coordinate
 *   @param    y   Top-most y coordinate
 *   @param    h   Height in pixels
 *   @param    color 16-bit 5-6-5 Color to fill with
 */
inline void MATRIX_DrawFastVLine( int16_t x, int16_t y, int16_t h, uint16_t u16Color )
{
    MATRIX_WriteLine(x, y, x, y+h-1, u16Color );
}


/**
 *   @brief    Write a line.  Bresenham's algorithm - thx wikpedia
 *
 *   @param    x0  Start point x coordinate
 *   @param    y0  Start point y coordinate
 *   @param    x1  End point x coordinate
 *   @param    y1  End point y coordinate
 *   @param    color 16-bit 5-6-5 Color to draw with
 */
inline void MATRIX_WriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t u16Color )
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++)
    {
        if( steep )
        {
#ifdef RECORD_LAST_FRAME
            if( !u16Color )
            {
                gRecordFrame[y0/MATRIX_SCANRATE][x0][gCurrFrame] |= gBitMask[y0%MATRIX_SCANRATE];
            }
            else
#endif /* #ifdef RECORD_LAST_FRAME */
            {
                MATRIX_DrawPixel(y0, x0, u16Color );
            }
        }
        else
        {
#ifdef RECORD_LAST_FRAME
            if( !u16Color )
            {
                gRecordFrame[x0/MATRIX_SCANRATE][y0][gCurrFrame] |= gBitMask[x0%MATRIX_SCANRATE];
            }
            else
#endif /* #ifdef RECORD_LAST_FRAME */
            {
                MATRIX_DrawPixel( x0, y0, u16Color );
            }
        }
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 *
 *  @brief    Draw a circle outline
 *
 *  @param    x0   Center-point x coordinate
 *  @param    y0   Center-point y coordinate
 *  @param    r   Radius of circle
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
inline void MATRIX_DrawCircle( int16_t x0, int16_t y0, int16_t r, uint16_t u16Color )
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    MATRIX_DrawPixel( x0  , y0+r, u16Color );
    MATRIX_DrawPixel( x0  , y0-r, u16Color );
    MATRIX_DrawPixel( x0+r, y0  , u16Color );
    MATRIX_DrawPixel( x0-r, y0  , u16Color );

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        MATRIX_DrawPixel( x0 + x, y0 + y, u16Color );
        MATRIX_DrawPixel( x0 - x, y0 + y, u16Color );
        MATRIX_DrawPixel( x0 + x, y0 - y, u16Color );
        MATRIX_DrawPixel( x0 - x, y0 - y, u16Color );
        MATRIX_DrawPixel( x0 + y, y0 + x, u16Color );
        MATRIX_DrawPixel( x0 - y, y0 + x, u16Color );
        MATRIX_DrawPixel( x0 + y, y0 - x, u16Color );
        MATRIX_DrawPixel( x0 - y, y0 - x, u16Color );
    }
}

/**
 *  @brief    Draw a circle with filled color
 *
 *  @param    x0   Center-point x coordinate
 *  @param    y0   Center-point y coordinate
 *  @param    r   Radius of circle
 *  @param    color 16-bit 5-6-5 Color to fill with
 */
inline void MATRIX_FillCircle( int16_t x0, int16_t y0, int16_t r, uint16_t u16Color  )
{
    MATRIX_DrawFastVLine( x0, y0-r, 2*r+1, u16Color );
    MATRIX_FillCircleHelper( x0, y0, r, 3, 0, u16Color );
}


/**
 *  @brief  Quarter-circle drawer with fill, used for circles and roundrects
 *
 *  @param  x0       Center-point x coordinate
 *  @param  y0       Center-point y coordinate
 *  @param  r        Radius of circle
 *  @param  corners  Mask bits indicating which quarters we're doing
 *  @param  delta    Offset from center-point, used for round-rects
 *  @param  color    16-bit 5-6-5 Color to fill with
 */
inline void MATRIX_FillCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t u16Color  )
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;
    int16_t px    = x;
    int16_t py    = y;

    delta++; // Avoid some +1's in the loop

    while(x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if(x < (y + 1)) {
            if(corners & 1) MATRIX_DrawFastVLine( x0+x, y0-y, 2*y+delta, u16Color );
            if(corners & 2) MATRIX_DrawFastVLine( x0-x, y0-y, 2*y+delta, u16Color );
        }
        if(y != py) {
            if(corners & 1) MATRIX_DrawFastVLine( x0+py, y0-px, 2*px+delta, u16Color );
            if(corners & 2) MATRIX_DrawFastVLine( x0-py, y0-px, 2*px+delta, u16Color );
            py = y;
        }
        px = x;
    }
}

/**
 *  @brief    Draw a line
 *  @param    x0  Start point x coordinate
 *  @param    y0  Start point y coordinate
 *  @param    x1  End point x coordinate
 *  @param    y1  End point y coordinate
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
inline void MATRIX_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t u16Color )
{
    // Update in subclasses if desired!
    if(x0 == x1){
        if(y0 > y1) _swap_int16_t( y0, y1 );
        MATRIX_DrawFastVLine( x0, y0, y1 - y0 + 1, u16Color );
    } else if(y0 == y1){
        if( x0 > x1) _swap_int16_t( x0, x1 );
        MATRIX_DrawFastHLine( x0, y0, x1 - x0 + 1, u16Color );
    } else {
        MATRIX_WriteLine( x0, y0, x1, y1, u16Color );
    }
}

/*!
 *  @brief    Fill a rectangle completely with one color. Update in subclasses if desired!
 *
 *  @param    x   Top left corner x coordinate
 *  @param    y   Top left corner y coordinate
 *  @param    w   Width in pixels
 *  @param    h   Height in pixels
 *  @param    color 16-bit 5-6-5 Color to fill with
*/
inline void MATRIX_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t u16Color)
{
    for( int16_t i=x; i<x+w; i++)
    {
        MATRIX_DrawFastVLine(i, y, h, u16Color);
    }
}


/**
 *   @brief     Draw a triangle with color-fill
 *
 *   @param    x0  Vertex #0 x coordinate
 *   @param    y0  Vertex #0 y coordinate
 *   @param    x1  Vertex #1 x coordinate
 *   @param    y1  Vertex #1 y coordinate
 *   @param    x2  Vertex #2 x coordinate
 *   @param    y2  Vertex #2 y coordinate
 *   @param    color 16-bit 5-6-5 Color to fill/draw with
 */
inline void MATRIX_FillTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t u16Color  )
{

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1); _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }

    if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)      a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a)      a = x2;
        else if(x2 > b) b = x2;
        MATRIX_DrawFastHLine( a, y0, b-a+1, u16Color );
        return;
    }

    int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    int32_t
    sa   = 0,
    sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it

    for(y=y0; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        MATRIX_DrawFastHLine( a, y, b-a+1, u16Color );
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for(; y<=y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        MATRIX_DrawFastHLine( a, y, b-a+1, u16Color );
    }
}

/**
 *  @brief   Draw a rounded rectangle with no fill color
 *
 *  @param    x   Top left corner x coordinate
 *  @param    y   Top left corner y coordinate
 *  @param    w   Width in pixels
 *  @param    h   Height in pixels
 *  @param    r   Radius of corner rounding
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
inline void MATRIX_DrawRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t u16Color  )
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    MATRIX_DrawFastHLine( x+r  , y    , w-2*r, u16Color ); // Top
    MATRIX_DrawFastHLine( x+r  , y+h-1, w-2*r, u16Color ); // Bottom
    MATRIX_DrawFastVLine( x    , y+r  , h-2*r, u16Color ); // Left
    MATRIX_DrawFastVLine( x+w-1, y+r  , h-2*r, u16Color ); // Right
    // draw four corners
    MATRIX_DrawCircleHelper( x+r    , y+r    , r, 1, u16Color );
    MATRIX_DrawCircleHelper( x+w-r-1, y+r    , r, 2, u16Color );
    MATRIX_DrawCircleHelper( x+w-r-1, y+h-r-1, r, 4, u16Color );
    MATRIX_DrawCircleHelper( x+r    , y+h-r-1, r, 8, u16Color );
}

/**
 *  @brief    Quarter-circle drawer, used to do circles and roundrects
 *
 *  @param    x0   Center-point x coordinate
 *  @param    y0   Center-point y coordinate
 *  @param    r   Radius of circle
 *  @param    cornername  Mask bit #1 or bit #2 to indicate which quarters of the circle we're doing
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
inline void MATRIX_DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t u16Color )
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {
            MATRIX_DrawPixel( x0 + x, y0 + y, u16Color );
            MATRIX_DrawPixel( x0 + y, y0 + x, u16Color );
        }
        if (cornername & 0x2) {
            MATRIX_DrawPixel( x0 + x, y0 - y, u16Color );
            MATRIX_DrawPixel( x0 + y, y0 - x, u16Color );
        }
        if (cornername & 0x8) {
            MATRIX_DrawPixel( x0 - y, y0 + x, u16Color );
            MATRIX_DrawPixel( x0 - x, y0 + y, u16Color );
        }
        if (cornername & 0x1) {
            MATRIX_DrawPixel( x0 - y, y0 - x, u16Color );
            MATRIX_DrawPixel( x0 - x, y0 - y, u16Color );
        }
    }
}

/**
 *  @brief   Draw a rounded rectangle with fill color
 *
 *  @param    x   Top left corner x coordinate
 *  @param    y   Top left corner y coordinate
 *  @param    w   Width in pixels
 *  @param    h   Height in pixels
 *  @param    r   Radius of corner rounding
 *  @param    color 16-bit 5-6-5 Color to draw/fill with
 */
void MATRIX_FillRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t u16Color  )
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    MATRIX_FillRect( x+r, y, w-2*r, h, u16Color );
    // draw four corners
    MATRIX_FillCircleHelper( x+w-r-1, y+r, r, 1, h-2*r-1, u16Color );
    MATRIX_FillCircleHelper( x+r    , y+r, r, 2, h-2*r-1, u16Color );
}

/**
 *  @brief   Draw a triangle with no fill color
 *
 *  @param    x0  Vertex #0 x coordinate
 *  @param    y0  Vertex #0 y coordinate
 *  @param    x1  Vertex #1 x coordinate
 *  @param    y1  Vertex #1 y coordinate
 *  @param    x2  Vertex #2 x coordinate
 *  @param    y2  Vertex #2 y coordinate
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
void MATRIX_DrawTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t u16Color  )
{
    MATRIX_DrawLine( x0, y0, x1, y1, u16Color );
    MATRIX_DrawLine( x1, y1, x2, y2, u16Color );
    MATRIX_DrawLine( x2, y2, x0, y0, u16Color );
}

/*================================================================================================*/

#ifdef __cplusplus
}
#endif

/** @} */
