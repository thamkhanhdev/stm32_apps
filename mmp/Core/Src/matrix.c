/*================================================================================================*/
/**
*   @file       matrix.c
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      Matrix program body.
*   @details    Reserved.
*
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

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/


/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

// static uint16_t const gTimeCount[8]={10, 20, 40, 80, 160, 240, 320}; // he so chia nap vao timer ok with 256x64 47.9Hz
// static uint16_t const gTimeCount[8]={15, 30, 60, 120, 160, 240, 320}; // he so chia nap vao timer ok with 256x64 47.9Hz
const uint16_t scan_PWM_duty[]={4,8,16,32,64,128,256,512,1024};   //he so chia nạp vao time pwm chan OE
// uint16_t const gTimeCount[8]={14, 28, 56, 112, 224, 448, 480, 600}; // he so chia nap vao timer ok with 256x64 47.9Hz
uint16_t const gTimeCount[8]={ 40, 80, 160, 240, 360, 480, 720, 1200}; // he so chia nap vao timer ok with 256x64 47.9Hz
// uint16_t const gTimeCount[8]={ 160, 240, 360, 480, 600, 1000, 1500}; // he so chia nap vao timer ok with 256x64 47.9Hz

static const int8_t sinetab[256] = {
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
static uint8_t gRows __attribute__((section (".ram_d1_cacheable")));

/**
 * @brief The actual postion bit is handling.
 */
static uint8_t gBitPos __attribute__((section (".ram_d1_cacheable")));

/**
 * @brief The current brightness value
 */
static uint8_t gBrightness __attribute__((section (".ram_d1_cacheable")));
static bool gCp437 __attribute__((section (".ram_d1_cacheable")));
static volatile uint32_t gCountBit __attribute__((section (".ram_d1_cacheable")));
static uint16_t gCursorX __attribute__((section (".ram_d1_cacheable")));
static uint16_t gCursorY __attribute__((section (".ram_d1_cacheable")));
static uint8_t  gTextSizeX __attribute__((section (".ram_d1_cacheable")));
static uint8_t  gTextSizeY __attribute__((section (".ram_d1_cacheable")));
static uint16_t gTextColor __attribute__((section (".ram_d1_cacheable")));
static uint16_t gTextBgColor __attribute__((section (".ram_d1_cacheable")));

static const GFXfont *gfxFont = &TomThumb;       ///< Pointer to special font
#ifdef MAXTRIX_MAX_BUFFER
uint16_t gBuff0[MATRIX_WIDTH*(MAXTRIX_MAX_BUFFER)*MAX_BIT/2] __attribute__((section (".ram_d2_cacheable"))); //;
uint16_t gBuff1[MATRIX_WIDTH*(MATRIX_HEIGHT - MAXTRIX_MAX_BUFFER)*MAX_BIT/2] __attribute__((section (".ram_d2_cacheable")));;
#else
uint16_t gBuff0[MATRIX_WIDTH*(MATRIX_HEIGHT)*MAX_BIT/2] __attribute__((section (".ram_d1_cacheable")));
#endif /* #ifdef MAXTRIX_MAX_BUFFER */
// static const GFXfont *gfxFont = NULL;       ///< Pointer to special font
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
    return (uint8_t *) pgm_read_pointer( &gfxFont->bitmap );
}

static inline GFXglyph * pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c)
{
    return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
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
 static inline void MATRIX_DrawPixel( uint16_t x, uint16_t y, uint32_t c )
{
    uint32_t _curPos;
    uint8_t r, g, b;
    uint16_t bit, limit;

#if defined(RGB888)
    r = (c >> 16) & 0xFF; /* RRRRRRRRggggggggbbbbbbbb */
    g = (c >>  8) & 0xFF; /* rrrrrrrrGGGGGGGGbbbbbbbb */
    b = (c >>  0) & 0xFF; /* rrrrrrrrggggggggBBBBBBBB */
#elif defined(RGB666)
    r =  c >> 12;         /* RRRRRRggggggbbbbbb */
    g = (c >>  6) & 0x3F; /* rrrrrrGGGGGGbbbbbb */
    b = (c >>  0) & 0x3F; /* rrrrrrggggggBBBBBB */
#elif defined(RGB565)
    r =  c >> 11;         /* RRRRRggggggbbbbb */
    g = (c >>  6) & 0x1F; /* rrrrrGGGGGGbbbbb */
    b = (c >>  0) & 0x1F; /* rrrrrggggggBBBBB */
#elif defined(RGB555)
    r =  c >> 10;         /* RRRRRgggggbbbbb */
    g = (c >>  5) & 0x1F; /* rrrrrGGGGGbbbbb */
    b = (c >>  0) & 0x1F; /* rrrrrgggggBBBBB */
#elif defined(RGB444)
    r = (c >>  8) & 0xF;  /* RRRRggggbbbb */
    g = (c >>  4) & 0xF;  /* rrrrGGGGbbbb */
    b = (c >>  0) & 0xF;  /* rrrrggggBBBB */
#elif defined(RGB333)
#endif

    bit   = 1;
    limit = 1 << MAX_BIT;

#ifdef USE_BUS_3
    if( y >= MAXTRIX_MAX_BUFFER )
    {
        /* Minus by MAXTRIX_MAX_BUFFER */
        y = y & 0x7FU;

        if( y < MATRIX_SCANRATE )
        {
            /**
             * Data for the upper half of the display is stored in the lower
             * bits of each byte.
             */
            /* Base address of current position of the pixel */
            _curPos = (y << MATRIX_WIDTH_BIT) * MAX_BIT + x;

            /**
             * The remaining three image planes are more normal-ish.
             * Data is stored in the high 6 bits so it can be quickly
             * copied to the DATAPORT register w/6 output lines.
             */
            while( bit < limit )
            {
                gBuff1[_curPos] &= ~RGB5_MASK;                   /* 00000111b Mask out R,G,B in one op */
                if( r & bit ) gBuff1[_curPos] |= R5_MASK;     /* Plane N R: bit 6 0000.0100.0000b     */
                if( g & bit ) gBuff1[_curPos] |= G5_MASK;     /* Plane N G: bit 7 0000.1000.0000b     */
                if( b & bit ) gBuff1[_curPos] |= B5_MASK;    /* Plane N B: bit 8 0001.0000.0000b     */
                _curPos  += MATRIX_WIDTH;                /* Advance to next bit plane */
                bit <<= 1 ;
            }
        }
        else
        {
            /* Data for the lower half of the display is stored in the upper bits */
            _curPos = ((y - MATRIX_SCANRATE)<<MATRIX_WIDTH_BIT) * MAX_BIT + x;

            while( bit < limit )
            {
                gBuff1[_curPos] &= ~RGB6_MASK;              /* 00111000b Mask out R,G,B in one op */
                if(r & bit) gBuff1[_curPos] |= R6_MASK;    /* Plane N R: bit 11 1000.0000.0000b    */
                if(g & bit) gBuff1[_curPos] |= G6_MASK;   /* Plane N G: bit 9  0010.0000.0000b    */
                if(b & bit) gBuff1[_curPos] |= B6_MASK;   /* Plane N B: bit 10 0100.0000.0000b    */
                _curPos  += MATRIX_WIDTH;             /* Advance to next bit plane */
                bit <<= 1 ;
            }
        }
    }
    else
#endif /* #ifdef USE_BUS_3 */

    /* Fill data to the second bus if used */
#ifdef USE_BUS_2
    if( y >= 64U )
    {
        /* Minus by MATRIX_HEIGHT/2 */
        y = y & 0xBFU ;

        if( y < MATRIX_SCANRATE )
        {
            /**
             * Data for the upper half of the display is stored in the lower
             * bits of each byte.
             */
            /* Base address of current position of the pixel */
            _curPos = (y << MATRIX_WIDTH_BIT) * MAX_BIT + x;

            /**
             * The remaining three image planes are more normal-ish.
             * Data is stored in the high 6 bits so it can be quickly
             * copied to the DATAPORT register w/6 output lines.
             */
            while( bit < limit )
            {
                gBuff0[_curPos] &= ~RGB3_MASK;                   /* 00000111b Mask out R,G,B in one op */
                if( r & bit ) gBuff0[_curPos] |= R3_MASK;     /* Plane N R: bit 6 0000.0100.0000b     */
                if( g & bit ) gBuff0[_curPos] |= G3_MASK;     /* Plane N G: bit 7 0000.1000.0000b     */
                if( b & bit ) gBuff0[_curPos] |= B3_MASK;    /* Plane N B: bit 8 0001.0000.0000b     */
                _curPos  += MATRIX_WIDTH;                /* Advance to next bit plane */
                bit <<= 1 ;
            }
        }
        else
        {
            /* Data for the lower half of the display is stored in the upper bits */
            _curPos = ((y - MATRIX_SCANRATE)<<MATRIX_WIDTH_BIT) * MAX_BIT + x;

            while( bit < limit )
            {
                gBuff0[_curPos] &= ~RGB4_MASK;              /* 00111000b Mask out R,G,B in one op */
                if(r & bit) gBuff0[_curPos] |= R4_MASK;    /* Plane N R: bit 11 1000.0000.0000b    */
                if(g & bit) gBuff0[_curPos] |= G4_MASK;   /* Plane N G: bit 9  0010.0000.0000b    */
                if(b & bit) gBuff0[_curPos] |= B4_MASK;   /* Plane N B: bit 10 0100.0000.0000b    */
                _curPos  += MATRIX_WIDTH;             /* Advance to next bit plane */
                bit <<= 1 ;
            }
        }
    }
    else
#endif /* #ifdef USE_BUS_2 */

#ifdef USE_BUS_1
    /* Fill data to the first bus at default mode */
    if( y < MATRIX_SCANRATE)
    {
        /**
         * Data for the upper half of the display is stored in the lower
         * bits of each byte.
         */
        /* Base address of current position of the pixel */
        _curPos = (y << MATRIX_WIDTH_BIT) * MAX_BIT + x;

        /**
         * The remaining three image planes are more normal-ish.
         * Data is stored in the high 6 bits so it can be quickly
         * copied to the DATAPORT register w/6 output lines.
         */
        while( bit < limit )
        {
           gBuff0[_curPos] &= ~RGB1_MASK;                   /* 00000111b Mask out R,G,B in one op */
           if( r & bit ) gBuff0[_curPos] |= R1_MASK;     /* Plane N R: bit 6 0000.0100.0000b     */
           if( g & bit ) gBuff0[_curPos] |= G1_MASK;     /* Plane N G: bit 7 0000.1000.0000b     */
           if( b & bit ) gBuff0[_curPos] |= B1_MASK;    /* Plane N B: bit 8 0001.0000.0000b     */
            _curPos  += MATRIX_WIDTH;                /* Advance to next bit plane */
            bit <<= 1 ;
        }
    }
   else
   {
       /* Data for the lower half of the display is stored in the upper bits */
       _curPos = ((y - MATRIX_SCANRATE)<<MATRIX_WIDTH_BIT) * MAX_BIT + x;

       while( bit < limit )
       {
           gBuff0[_curPos] &= ~RGB2_MASK;              /* 00111000b Mask out R,G,B in one op */
           if(r & bit) gBuff0[_curPos] |= R2_MASK;    /* Plane N R: bit 11 1000.0000.0000b    */
           if(g & bit) gBuff0[_curPos] |= G2_MASK;   /* Plane N G: bit 9  0010.0000.0000b    */
           if(b & bit) gBuff0[_curPos] |= B2_MASK;   /* Plane N B: bit 10 0100.0000.0000b    */
           _curPos  += MATRIX_WIDTH;             /* Advance to next bit plane */
           bit <<= 1 ;
       }
   }
#endif /* #ifdef USE_BUS_1 */
}

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
/**
  * @brief  None.
  *
  * @param  None.
  * @retval None.
  */
StdReturnType MATRIX_Init( uint8_t bri )
{
    gBrightness = bri;
    gRows = 0U;
    gBitPos = 0U;
    gCp437 = false;
    gCountBit = 0U;
    gCursorX = 0U;
    gCursorY = 0U;
    gTextSizeX = 1U;
    gTextSizeY = 1U;
    gTextColor = 0xFFFFU;
    gTextBgColor = 0x0U;

    return E_OK;
}

/**
  * @brief  Resfresh function for active display.
  *
  * @param  None.
  * @retval None.
  */
void IRQ_ProcessMonitor( void )
{
    uint16_t u16Count = MATRIX_WIDTH;

    /* Disable LED output during gRows/gBitPos switchover */
    TIM2->CCR2 = 0U;

    /* Release new data region */
    while( u16Count-- )
    {
#if defined(USE_BUS_1) || defined(USE_BUS_2)
        DAT0_P->ODR= gBuff0[ gCountBit ];
#endif /* #if defined(USE_BUS_3) || defined(USE_BUS_4) */
#if defined(USE_BUS_3) || defined(USE_BUS_4)
        DAT1_P->ODR= gBuff1[ gCountBit ];
#endif /* #if defined(USE_BUS_3) || defined(USE_BUS_4) */
        gCountBit++;
        CLK_P->BSRR=CLK_OFF;
        CLK_P->BSRR=CLK_ON;
    }

    TIM2->CCR2 = gBrightness;
    // TIM2->CCR2 = scan_PWM_duty[gBitPos];
    /* Load new division value */
    TIM4->PSC = gTimeCount[gBitPos];
    /* Trigger EGR upto 1 in order re-load PSC value */
    TIM4->EGR = 1U;
    /* Clear interrupt flag */
    TIM4->SR = 0xFFFFFFFE;
    // if( gBitPos == 0U )
    // {
        // gBitPos 0 was loaded on prior interrupt invocation and is about to
        // latch now, so update the row address lines before we do that:
        // ROW_P->ODR = (ROW_P->ODR & MATRIX_MASKROWS) | gRows;
        ROW_P->ODR = gRows;
    // }
    /* Latch data loaded during *prior* interrupt */
    LAT_P->BSRR=LAT_ON;
    LAT_P->BSRR = LAT_OFF;

    if( ++gBitPos >= MAX_BIT )
    {      // Advance gBitPos counter.  Maxed out?
        gBitPos = 0U;                  // Yes, reset to gBitPos 0, and
        if( ++gRows >= MATRIX_SCANRATE )
        {        // advance gRows counter.  Maxed out?
            gRows = 0U;              // Yes, reset gRows counter, then...
            gCountBit = 0UL;
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

inline void MATRIX_SwapBuffers( void )
{
    (void) 1U;
#ifdef USE_DOUBLE_BUFF
    gSwapFlag = true;
    while(gSwapFlag == true);
#endif
}

/**
  * @brief  None.
  *
  * @param  None.
  * @retval None.
  */
StdReturnType MATRIX_disImage( const uint8_t arr[], uint16_t nCols, uint16_t nRows, uint16_t u16XStart, uint16_t u16YStart, MATRIX_ColorTypes nColorType)
{
    uint32_t p1;
    uint32_t c = 0;
    uint16_t row, col;
    uint8_t  r, g, b;

    for( row = 0; row < nRows; row++)
    {
        for( col = 0; col < nCols; col++)
        {
            p1 = row * nCols*3;
#if   defined(RGB888) || defined(RGB666)
            r = arr[p1+col*3+2];
            g = arr[p1+col*3+1];
            b = arr[p1+col*3+0];

#elif defined(RGB565) || defined(RGB555) || defined(RGB444)
            r = pgm_read_byte(&gamma_table[(arr[p1+col*3+2] * 255) >> 8 ]);
            g = pgm_read_byte(&gamma_table[(arr[p1+col*3+1] * 255) >> 8 ]);
            b = pgm_read_byte(&gamma_table[(arr[p1+col*3+0] * 255) >> 8 ]);
#endif

#if   defined(RGB888)
            c = ((r << 16) & 0xFF0000) |
                ((g <<  8) & 0x00FF00) |
                ((b <<  0) & 0x0000FF);
#elif defined(RGB666)
            c = ((r << 10) & 0x3F000) |
                ((g <<  4) & 0x00FC0) |
                ((b >>  2) & 0x0003F);
#elif defined(RGB565)
            c = (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
                (g <<  7) | ((g & 0xC) << 3) |
                (b <<  1) | ( b        >> 3);
#elif defined(RGB555)
            c = (r << 11) | ((r & 0x8) << 7) | // 4/4/4 -> 5/5/5
                (g <<  6) | ((g & 0x8) << 2) |
                (b <<  1) | ( b        >> 3);
#elif defined(RGB444)
            c = ((r << 8)&0xF00) |
                ((g << 4)&0x0F0) |
                ((b << 0)&0x00F);
#elif defined(RGB333)
#endif  /* defined(RGBxxx) */

            /* RGB565 rrrrrggggggbbbbb */
            // c = ((c>>8)&0xF800) | ((c>>5)&0x07E0) | ((c>>3)&0x1F);
            MATRIX_DrawPixel( col + u16XStart, row + u16YStart, c );
        }
    }
    MATRIX_SwapBuffers();
    return E_OK;
}

void MATRIX_WritePixel( uint16_t x, uint16_t y, uint32_t c )
{
    MATRIX_DrawPixel( x, y, c );
}

StdReturnType MATRIX_FillAllColorPannel( uint8_t cR, uint8_t cG, uint8_t cB )
{
    uint16_t y = 0, x = 0;
    uint32_t u32Color = 0UL;
#if defined(RGB666)
    /* Convert RGB888 to RGB666 */
    u32Color = ((cR << 10)&0x3F000) | ((cG<<4)&0xFB0) | ((cB>>2)&0x3F);
#elif defined(RGB565)
    /* Convert RGB888 to RGB565 */
    u32Color = ((cR << 8)&0xF800) | ((cG<<3)&0x07E0) | ((cB>>3)&0x1F);
#elif defined(RGB555)
    /* Convert RGB888 to RGB555 */
    u32Color = ((cR << 7)&0x3E00) | ((cG<<2)&0x03E0) | ((cB>>3)&0x1F);
#endif

    GPIOD->BSRR = 0x20000000UL;
    for( y = 0U; y < MATRIX_HEIGHT; y++ )
    {
        for( x = 0U; x < MATRIX_WIDTH; x++ )
        {
            MATRIX_DrawPixel( x, y, u32Color );
        }
    }
    GPIOD->BSRR = 0x20000UL;
    MATRIX_SwapBuffers();
    return E_OK;
}

uint16_t ColorHSV( uint16_t x, uint16_t y, long hue, uint8_t sat, uint8_t val, char gflag)
{
  uint8_t  r, g, b, lo;
  uint16_t s1, v1;

  // Hue
  hue %= 1536;             // -1535 to +1535
  if(hue < 0) hue += 1536; //     0 to +1535
  lo = hue & 255;          // Low byte  = primary/secondary color mix
  switch(hue >> 8) {       // High byte = sextant of colorwheel
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
  s1 = sat + 1;
  r  = 255 - (((255 - r) * s1) >> 8);
  g  = 255 - (((255 - g) * s1) >> 8);
  b  = 255 - (((255 - b) * s1) >> 8);

  // Value (brightness) & 16-bit color reduction: similar to above, add 1
  // to allow shifts, and upgrade to int makes other conversions implicit.
  v1 = val + 1;
  if(gflag) { // Gamma-corrected color?
    r = pgm_read_byte(&gamma_table[(r * v1) >> 8]); // Gamma correction table maps
    g = pgm_read_byte(&gamma_table[(g * v1) >> 8]); // 8-bit input to 4-bit output
    b = pgm_read_byte(&gamma_table[(b * v1) >> 8]);
  } else { // linear (uncorrected) color
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
    while (1)
    {
        HAL_Delay(10);
        GPIOD->BSRR = (1<<29);
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
                    MATRIX_DrawPixel(x, y, ColorHSV(x, y, value * 3, 255, 255, 1) );
                    // matrix.drawPixel(x, y, ColorHSV(value * 3, 255, 255, 1));
                x1--; x2--; x3--; x4--;
            }
        y1--; y2--; y3--; y4--;
        }

        MATRIX_SwapBuffers();
        GPIOD->BSRR = (1<<13);
        angle1 += 0.03;
        angle2 -= 0.07;
        angle3 += 0.13;
        angle4 -= 0.15;
        hueShift += 2;
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
void MATRIX_DrawCircle( int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    MATRIX_DrawPixel(x0  , y0+r, color);
    MATRIX_DrawPixel(x0  , y0-r, color);
    MATRIX_DrawPixel(x0+r, y0  , color);
    MATRIX_DrawPixel(x0-r, y0  , color);

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        MATRIX_DrawPixel(x0 + x, y0 + y, color);
        MATRIX_DrawPixel(x0 - x, y0 + y, color);
        MATRIX_DrawPixel(x0 + x, y0 - y, color);
        MATRIX_DrawPixel(x0 - x, y0 - y, color);
        MATRIX_DrawPixel(x0 + y, y0 + x, color);
        MATRIX_DrawPixel(x0 - y, y0 + x, color);
        MATRIX_DrawPixel(x0 + y, y0 - x, color);
        MATRIX_DrawPixel(x0 - y, y0 - x, color);
    }
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
void MATRIX_WriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
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

    for (; x0<=x1; x0++) {
        if (steep) {
            MATRIX_DrawPixel(y0, x0, color);
        } else {
            MATRIX_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 *   @brief    Draw a perfectly vertical line (this is often optimized in a subclass!)
 *
 *   @param    x   Top-most x coordinate
 *   @param    y   Top-most y coordinate
 *   @param    h   Height in pixels
 *   @param    color 16-bit 5-6-5 Color to fill with
 */
void MATRIX_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    MATRIX_WriteLine(x, y, x, y+h-1, color);
}

/**
 *   @brief    Draw a perfectly horizontal line (this is often optimized in a subclass!)
 *
 *   @param    x   Left-most x coordinate
 *   @param    y   Left-most y coordinate
 *   @param    w   Width in pixels
 *   @param    color 16-bit 5-6-5 Color to fill with
 */
void MATRIX_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    MATRIX_WriteLine(x, y, x+w-1, y, color);
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
void MATRIX_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    for( int16_t i=x; i<x+w; i++)
    {
        MATRIX_DrawFastVLine(i, y, h, color);
    }
}

/**
 *  @brief    Fill the screen completely with one color. Update in subclasses if desired!
 *
 *  @param    color 16-bit 5-6-5 Color to fill with
 */
void MATRIX_FillScreen(uint16_t color)
{
    MATRIX_FillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, color);
}

/**
 *  @brief    Draw a line
 *  @param    x0  Start point x coordinate
 *  @param    y0  Start point y coordinate
 *  @param    x1  End point x coordinate
 *  @param    y1  End point y coordinate
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
void MATRIX_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    // Update in subclasses if desired!
    if(x0 == x1){
        if(y0 > y1) _swap_int16_t(y0, y1);
        MATRIX_DrawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if(y0 == y1){
        if(x0 > x1) _swap_int16_t(x0, x1);
        MATRIX_DrawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        MATRIX_WriteLine(x0, y0, x1, y1, color);
    }
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
void MATRIX_DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
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
            MATRIX_DrawPixel(x0 + x, y0 + y, color);
            MATRIX_DrawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            MATRIX_DrawPixel(x0 + x, y0 - y, color);
            MATRIX_DrawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            MATRIX_DrawPixel(x0 - y, y0 + x, color);
            MATRIX_DrawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            MATRIX_DrawPixel(x0 - y, y0 - x, color);
            MATRIX_DrawPixel(x0 - x, y0 - y, color);
        }
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
void MATRIX_FillCircle( int16_t x0, int16_t y0, int16_t r, uint16_t color )
{
    MATRIX_DrawFastVLine(x0, y0-r, 2*r+1, color);
    MATRIX_FillCircleHelper(x0, y0, r, 3, 0, color);
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
void MATRIX_FillCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color )
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
            if(corners & 1) MATRIX_DrawFastVLine(x0+x, y0-y, 2*y+delta, color);
            if(corners & 2) MATRIX_DrawFastVLine(x0-x, y0-y, 2*y+delta, color);
        }
        if(y != py) {
            if(corners & 1) MATRIX_DrawFastVLine(x0+py, y0-px, 2*px+delta, color);
            if(corners & 2) MATRIX_DrawFastVLine(x0-py, y0-px, 2*px+delta, color);
            py = y;
        }
        px = x;
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
void MATRIX_DrawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color )
{
    MATRIX_DrawFastHLine(x, y, w, color);
    MATRIX_DrawFastHLine(x, y+h-1, w, color);
    MATRIX_DrawFastVLine(x, y, h, color);
    MATRIX_DrawFastVLine(x+w-1, y, h, color);
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
void MATRIX_DrawRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color )
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    MATRIX_DrawFastHLine(x+r  , y    , w-2*r, color); // Top
    MATRIX_DrawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
    MATRIX_DrawFastVLine(x    , y+r  , h-2*r, color); // Left
    MATRIX_DrawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
    // draw four corners
    MATRIX_DrawCircleHelper(x+r    , y+r    , r, 1, color);
    MATRIX_DrawCircleHelper(x+w-r-1, y+r    , r, 2, color);
    MATRIX_DrawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    MATRIX_DrawCircleHelper(x+r    , y+h-r-1, r, 8, color);
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
void MATRIX_FillRoundRect( int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color )
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    MATRIX_FillRect(x+r, y, w-2*r, h, color);
    // draw four corners
    MATRIX_FillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
    MATRIX_FillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
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
void MATRIX_DrawTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color )
{
    MATRIX_DrawLine(x0, y0, x1, y1, color);
    MATRIX_DrawLine(x1, y1, x2, y2, color);
    MATRIX_DrawLine(x2, y2, x0, y0, color);
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
void MATRIX_FillTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color )
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
        MATRIX_DrawFastHLine(a, y0, b-a+1, color);
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
        MATRIX_DrawFastHLine(a, y, b-a+1, color);
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
        MATRIX_DrawFastHLine(a, y, b-a+1, color);
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
void MATRIX_DrawChar( int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y, MATRIX_FontTypes fontType )
{
    if( FONT_DEFAULT == fontType )
    { // 'Classic' built-in font
    // if(1) { // 'Classic' built-in font

        if((x >= MATRIX_WIDTH)            || // Clip right
           (y >= MATRIX_HEIGHT)           || // Clip bottom
           ((x + 6 * size_x - 1) < 0) || // Clip left
           ((y + 8 * size_y - 1) < 0))   // Clip top
            return;

        if(!gCp437 && (c >= 176)) c++; // Handle 'classic' charset behavior
        MATRIX_FillRect(x, y, 5*size_x, 8*size_y, 0x0);
        for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
            uint8_t line = pgm_read_byte(&font[c * 5 + i]);
            for(int8_t j=0; j<8; j++, line >>= 1) {
                if(line & 1) {
                    if(size_x == 1 && size_y == 1)
                        MATRIX_DrawPixel(x+i, y+j, color);
                    else
                        MATRIX_FillRect(x+i*size_x, y+j*size_y, size_x, size_y, color);
                } else if(bg != color) {
                    if(size_x == 1 && size_y == 1)
                        MATRIX_DrawPixel(x+i, y+j, bg);
                    else
                        MATRIX_FillRect(x+i*size_x, y+j*size_y, size_x, size_y, bg);
                }
            }
        }
        if(bg != color) { // If opaque, draw vertical line for last column
            if(size_x == 1 && size_y == 1) MATRIX_DrawFastVLine(x+5, y, 8, bg);
            else          MATRIX_FillRect(x+5*size_x, y, size_x, 8*size_y, bg);
        }

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
        MATRIX_FillRect(x, y-h, w/2, h, 0x0);
        for(yy=0; yy<h; yy++) {
            for(xx=0; xx<w; xx++) {
                if(!(bit++ & 7)) {
                    bits = pgm_read_byte(&bitmap[bo++]);
                }
                if(bits & 0x80) {
                    if(size_x == 1 && size_y == 1) {
                        MATRIX_DrawPixel(x+xo+xx, y+yo+yy, color);
                    } else {
                        MATRIX_FillRect(x+(xo16+xx)*size_x, y+(yo16+yy)*size_y,
                          size_x, size_y, color);
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
            MATRIX_DrawChar(gCursorX, gCursorY, c, u16Color, gTextBgColor, gTextSizeX, gTextSizeY, nFontType );
            gCursorX += gTextSizeX * 6;          // Advance x one char
        }
    } else
    {
        if(c == '\n') {
            gCursorX  = 0;
            gCursorY += (int16_t)gTextSizeY *
                        (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        } else if(c != '\r') {
            uint8_t first = pgm_read_byte(&gfxFont->first);
            if((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
                GFXglyph *glyph  = pgm_read_glyph_ptr(gfxFont, c - first);
                uint8_t   w     = pgm_read_byte(&glyph->width),
                          h     = pgm_read_byte(&glyph->height);
                if((w > 0) && (h > 0)) { // Is there an associated bitmap?
                    int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
                    if(((gCursorX + gTextSizeX * (xo + w)) > MATRIX_WIDTH)) {
                        gCursorX  = 0;
                        gCursorY += (int16_t)gTextSizeY *
                          (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                        if( gCursorY > MATRIX_HEIGHT)
                        {
                            gCursorY = (int16_t)gTextSizeY *
                          (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                        }
                    }
                    MATRIX_DrawChar(gCursorX, gCursorY, c, u16Color, gTextBgColor, gTextSizeX, gTextSizeY, nFontType );
                }
                gCursorX += (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)gTextSizeX;
            }
        }
    }
    return 1;
}

/**
 *  @brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
 *
 * @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
 */
void MATRIX_SetTextSize( uint8_t s )
{
    gTextSizeX = (s > 0) ? s : 1;
    gTextSizeY = (s > 0) ? s : 1;
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

void MATRIX_Print( char *s, MATRIX_FontTypes nFontType, uint16_t u16Color )
{
    while (*s)
    {
        MATRIX_Write( *s++, nFontType, u16Color );
    }
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

        if( (0xFFFFU != u16XPos) && (0xFFFFU != u16YPos) )
        {
            gCursorX = u16XPos;
            gCursorY = u16YPos;
        }
/*
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
        else if ( FONR_FREESANOBLIQUE9PT7B == nFontType )
        {
            gfxFont = &FreeSansOblique9pt7b;
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
        } */

        MATRIX_Print( string, nFontType, u16Color );
    }

    va_end(argp);
}
/**
 *  @brief  Set text cursor location
 *
 *  @param  x    X coordinate in pixels
 *  @param  y    Y coordinate in pixels
 */
inline void MATRIX_SetCursor( int16_t x, int16_t y )
{
    gCursorX = x;
    gCursorY = y;
}

/**
 *  @brief   Set text font color with transparant background
 *
 *  @param   c   16-bit 5-6-5 Color to draw text with
 *  @note    For 'transparent' background, background and foreground
 *           are set to same color rather than using a separate flag.
 */
void MATRIX_SetTextColor( uint16_t c )
{
    gTextColor = gTextBgColor = c;
}

#if 0
/*!
    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
    @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
void Adafruit_GFX::setTextSize(uint8_t s) {
    setTextSize(s, s);
}

/*!
    @brief Set the font to display when print()ing, either custom or default
    @param  f  The GFXfont object, if NULL use built in 6x8 font
*/
void Adafruit_GFX::setFont(const GFXfont *f) {
    if(f) {            // Font struct pointer passed in?
        if(!gfxFont) { // And no current font struct?
            // Switching from classic to new font behavior.
            // Move cursor pos down 6 pixels so it's on baseline.
            gCursorY += 6;
        }
    } else if(gfxFont) { // NULL passed.  Current font struct defined?
        // Switching from new to classic font behavior.
        // Move cursor pos up 6 pixels so it's at top-left of char.
        gCursorY -= 6;
    }
    gfxFont = (GFXfont *)f;
}


/*!
    @brief    Helper to determine size of a character with current font/size.
       Broke this out as it's used by both the PROGMEM- and RAM-resident getTextBounds() functions.
    @param    c     The ascii character in question
    @param    x     Pointer to x location of character
    @param    y     Pointer to y location of character
    @param    minx  Minimum clipping value for X
    @param    miny  Minimum clipping value for Y
    @param    maxx  Maximum clipping value for X
    @param    maxy  Maximum clipping value for Y
*/
void Adafruit_GFX::charBounds(char c, int16_t *x, int16_t *y,
  int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy) {

    if(gfxFont) {

        if(c == '\n') { // Newline?
            *x  = 0;    // Reset x to zero, advance y by one line
            *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        } else if(c != '\r') { // Not a carriage return; is normal char
            uint8_t first = pgm_read_byte(&gfxFont->first),
                    last  = pgm_read_byte(&gfxFont->last);
            if((c >= first) && (c <= last)) { // Char present in this font?
                GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
                uint8_t gw = pgm_read_byte(&glyph->width),
                        gh = pgm_read_byte(&glyph->height),
                        xa = pgm_read_byte(&glyph->xAdvance);
                int8_t  xo = pgm_read_byte(&glyph->xOffset),
                        yo = pgm_read_byte(&glyph->yOffset);
                if(wrap && ((*x+(((int16_t)xo+gw)*textsize_x)) > _width)) {
                    *x  = 0; // Reset x to zero, advance y by one line
                    *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                }
                int16_t tsx = (int16_t)textsize_x,
                        tsy = (int16_t)textsize_y,
                        x1 = *x + xo * tsx,
                        y1 = *y + yo * tsy,
                        x2 = x1 + gw * tsx - 1,
                        y2 = y1 + gh * tsy - 1;
                if(x1 < *minx) *minx = x1;
                if(y1 < *miny) *miny = y1;
                if(x2 > *maxx) *maxx = x2;
                if(y2 > *maxy) *maxy = y2;
                *x += xa * tsx;
            }
        }

    } else { // Default font

        if(c == '\n') {                     // Newline?
            *x  = 0;                        // Reset x to zero,
            *y += textsize_y * 8;           // advance y one line
            // min/max x/y unchaged -- that waits for next 'normal' character
        } else if(c != '\r') {  // Normal char; ignore carriage returns
            if(wrap && ((*x + textsize_x * 6) > _width)) { // Off right?
                *x  = 0;                    // Reset x to zero,
                *y += textsize_y * 8;       // advance y one line
            }
            int x2 = *x + textsize_x * 6 - 1, // Lower-right pixel of char
                y2 = *y + textsize_y * 8 - 1;
            if(x2 > *maxx) *maxx = x2;      // Track max x, y
            if(y2 > *maxy) *maxy = y2;
            if(*x < *minx) *minx = *x;      // Track min x, y
            if(*y < *miny) *miny = *y;
            *x += textsize_x * 6;             // Advance x one char
        }
    }
}

/*!
    @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
    @param    str     The ascii string to measure
    @param    x       The current cursor X
    @param    y       The current cursor Y
    @param    x1      The boundary X coordinate, set by function
    @param    y1      The boundary Y coordinate, set by function
    @param    w      The boundary width, set by function
    @param    h      The boundary height, set by function
*/
void Adafruit_GFX::getTextBounds(const char *str, int16_t x, int16_t y,
        int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint8_t c; // Current character

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

    while((c = *str++))
        charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}

/*!
    @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
    @param    str    The ascii string to measure (as an arduino String() class)
    @param    x      The current cursor X
    @param    y      The current cursor Y
    @param    x1     The boundary X coordinate, set by function
    @param    y1     The boundary Y coordinate, set by function
    @param    w      The boundary width, set by function
    @param    h      The boundary height, set by function
*/
void Adafruit_GFX::getTextBounds(const String &str, int16_t x, int16_t y,
        int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    if (str.length() != 0) {
        getTextBounds(const_cast<char*>(str.c_str()), x, y, x1, y1, w, h);
    }
}


/*!
    @brief    Helper to determine size of a PROGMEM string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
    @param    str     The flash-memory ascii string to measure
    @param    x       The current cursor X
    @param    y       The current cursor Y
    @param    x1      The boundary X coordinate, set by function
    @param    y1      The boundary Y coordinate, set by function
    @param    w      The boundary width, set by function
    @param    h      The boundary height, set by function
*/
void Adafruit_GFX::getTextBounds(const __FlashStringHelper *str,
        int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint8_t *s = (uint8_t *)str, c;

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

    while((c = pgm_read_byte(s++)))
        charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}

/*!
    @brief      Invert the display (ideally using built-in hardware command)
    @param   i  True if you want to invert, false to make 'normal'
*/
void Adafruit_GFX::invertDisplay(boolean i) {
    // Do nothing, must be subclassed if supported by hardware
}

/***************************************************************************/

/*!
   @brief    Create a simple drawn button UI element
*/
Adafruit_GFX_Button::Adafruit_GFX_Button(void) {
  _gfx = 0;
}

/*!
   @brief    Initialize button with our desired color/size/settings
   @param    gfx     Pointer to our display so we can draw to it!
   @param    x       The X coordinate of the center of the button
   @param    y       The Y coordinate of the center of the button
   @param    w       Width of the buttton
   @param    h       Height of the buttton
   @param    outline  Color of the outline (16-bit 5-6-5 standard)
   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
   @param    label  Ascii string of the text inside the button
   @param    textsize The font magnification of the label text
*/
// Classic initButton() function: pass center & size
void Adafruit_GFX_Button::initButton(
 Adafruit_GFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize)
{
  // Tweak arguments and pass to the newer initButtonUL() function...
  initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill,
    textcolor, label, textsize);
}

/*!
   @brief    Initialize button with our desired color/size/settings
   @param    gfx     Pointer to our display so we can draw to it!
   @param    x       The X coordinate of the center of the button
   @param    y       The Y coordinate of the center of the button
   @param    w       Width of the buttton
   @param    h       Height of the buttton
   @param    outline  Color of the outline (16-bit 5-6-5 standard)
   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
   @param    label  Ascii string of the text inside the button
   @param    textsize_x The font magnification in X-axis of the label text
   @param    textsize_y The font magnification in Y-axis of the label text
*/
// Classic initButton() function: pass center & size
void Adafruit_GFX_Button::initButton(
 Adafruit_GFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize_x, uint8_t textsize_y)
{
  // Tweak arguments and pass to the newer initButtonUL() function...
  initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill,
    textcolor, label, textsize_x, textsize_y);
}

/*!
   @brief    Initialize button with our desired color/size/settings, with upper-left coordinates
   @param    gfx     Pointer to our display so we can draw to it!
   @param    x1       The X coordinate of the Upper-Left corner of the button
   @param    y1       The Y coordinate of the Upper-Left corner of the button
   @param    w       Width of the buttton
   @param    h       Height of the buttton
   @param    outline  Color of the outline (16-bit 5-6-5 standard)
   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
   @param    label  Ascii string of the text inside the button
   @param    textsize The font magnification of the label text
*/
void Adafruit_GFX_Button::initButtonUL(
 Adafruit_GFX *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize)
{
  initButtonUL(gfx, x1, y1, w, h, outline, fill, textcolor, label, textsize, textsize);
}

/*!
   @brief    Initialize button with our desired color/size/settings, with upper-left coordinates
   @param    gfx     Pointer to our display so we can draw to it!
   @param    x1       The X coordinate of the Upper-Left corner of the button
   @param    y1       The Y coordinate of the Upper-Left corner of the button
   @param    w       Width of the buttton
   @param    h       Height of the buttton
   @param    outline  Color of the outline (16-bit 5-6-5 standard)
   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
   @param    label  Ascii string of the text inside the button
   @param    textsize_x The font magnification in X-axis of the label text
   @param    textsize_y The font magnification in Y-axis of the label text
*/
void Adafruit_GFX_Button::initButtonUL(
 Adafruit_GFX *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize_x, uint8_t textsize_y)
{
  _x1           = x1;
  _y1           = y1;
  _w            = w;
  _h            = h;
  _outlinecolor = outline;
  _fillcolor    = fill;
  _textcolor    = textcolor;
  _textsize_x   = textsize_x;
  _textsize_y   = textsize_y;
  _gfx          = gfx;
  strncpy(_label, label, 9);
}

/*!
   @brief    Draw the button on the screen
   @param    inverted Whether to draw with fill/text swapped to indicate 'pressed'
*/
void Adafruit_GFX_Button::drawButton(boolean inverted) {
  uint16_t fill, outline, text;

  if(!inverted) {
    fill    = _fillcolor;
    outline = _outlinecolor;
    text    = _textcolor;
  } else {
    fill    = _textcolor;
    outline = _outlinecolor;
    text    = _fillcolor;
  }

  uint8_t r = min(_w, _h) / 4; // Corner radius
  _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
  _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

  _gfx->setCursor(_x1 + (_w/2) - (strlen(_label) * 3 * _textsize_x),
    _y1 + (_h/2) - (4 * _textsize_y));
  _gfx->setTextColor(text);
  _gfx->setTextSize(_textsize_x, _textsize_y);
  _gfx->print(_label);
}
// BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------
/**
 *  @brief      Draw a PROGMEM-resident 1-bit image at the specified (x,y) position, using the specified foreground color (unset bits are transparent).
 *
 *  @param    x   Top left corner x coordinate
 *  @param    y   Top left corner y coordinate
 *  @param    bitmap  byte array with monochrome bitmap
 *  @param    w   Width of bitmap in pixels
 *  @param    h   Height of bitmap in pixels
 *  @param    color 16-bit 5-6-5 Color to draw with
 */
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if(byte & 0x80) writePixel(x+i, y, color);
        }
    }
}


/*!
   @brief      Draw a PROGMEM-resident 1-bit image at the specified (x,y) position, using the specified foreground (for set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h,
  uint16_t color, uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            writePixel(x+i, y, (byte & 0x80) ? color : bg);
        }
    }
    endWrite();
}

/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position, using the specified foreground color (unset bits are transparent).
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            if(byte & 0x80) writePixel(x+i, y, color);
        }
    }
    endWrite();
}

/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position, using the specified foreground (for set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            writePixel(x+i, y, (byte & 0x80) ? color : bg);
        }
    }
    endWrite();
}

/*!
   @brief      Draw PROGMEM-resident XBitMap Files (*.xbm), exported from GIMP.
   Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
   C Array can be directly used with this function.
   There is no RAM-resident version of this function; if generating bitmaps
   in RAM, use the format defined by drawBitmap() and call that instead.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
*/
void Adafruit_GFX::drawXBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte >>= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            // Nearly identical to drawBitmap(), only the bit order
            // is reversed here (left-to-right = LSB to MSB):
            if(byte & 0x01) writePixel(x+i, y, color);
        }
    }
    endWrite();
}


/*!
   @brief   Draw a PROGMEM-resident 8-bit image (grayscale) at the specified (x,y) pos.
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
        }
    }
    endWrite();
}

/*!
   @brief   Draw a RAM-resident 8-bit image (grayscale) at the specified (x,y) pos.
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, bitmap[j * w + i]);
        }
    }
    endWrite();
}


/*!
   @brief   Draw a PROGMEM-resident 8-bit image (grayscale) with a 1-bit mask
   (set bits = opaque, unset bits = clear) at the specified (x,y) position.
   BOTH buffers (grayscale and mask) must be PROGMEM-resident.
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    mask  byte array with mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&mask[j * bw + i / 8]);
            if(byte & 0x80) {
                writePixel(x+i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
            }
        }
    }
    endWrite();
}

/*!
   @brief   Draw a RAM-resident 8-bit image (grayscale) with a 1-bit mask
   (set bits = opaque, unset bits = clear) at the specified (x,y) position.
   BOTH buffers (grayscale and mask) must be RAM-residentt, no mix-and-match
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    mask  byte array with mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, uint8_t *mask, int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = mask[j * bw + i / 8];
            if(byte & 0x80) {
                writePixel(x+i, y, bitmap[j * w + i]);
            }
        }
    }
    endWrite();
}


/*!
   @brief   Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
   For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y,
  const uint16_t bitmap[], int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, pgm_read_word(&bitmap[j * w + i]));
        }
    }
    endWrite();
}

/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
   For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *bitmap, int16_t w, int16_t h) {
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            writePixel(x+i, y, bitmap[j * w + i]);
        }
    }
    endWrite();
}


/*!
   @brief   Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask (set bits = opaque, unset bits = clear) at the specified (x,y) position. BOTH buffers (color and mask) must be PROGMEM-resident. For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    mask  byte array with monochrome mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y,
  const uint16_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&mask[j * bw + i / 8]);
            if(byte & 0x80) {
                writePixel(x+i, y, pgm_read_word(&bitmap[j * w + i]));
            }
        }
    }
    endWrite();
}

/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask (set bits = opaque, unset bits = clear) at the specified (x,y) position. BOTH buffers (color and mask) must be RAM-resident. For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    mask  byte array with monochrome mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *bitmap, uint8_t *mask, int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = mask[j * bw + i / 8];
            if(byte & 0x80) {
                writePixel(x+i, y, bitmap[j * w + i]);
            }
        }
    }
    endWrite();
}
#endif
/*================================================================================================*/

#ifdef __cplusplus
}
#endif

/** @} */
