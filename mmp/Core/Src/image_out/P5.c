#include "P5.h"
#include "math.h"
#include "stdlib.h"
#include "image1.h"
#include "gamma.h"

extern TIM_HandleTypeDef htim5;

extern const  uint8_t gImage_test[];
// unsigned int const timer_clock[8]={36,72,144,288,576}; // he so chia nap vao timer
// unsigned int const timer_clock[8]={60,60,60,60,480}; // he so chia nap vao timer
// unsigned int const timer_clock[8]={30, 60, 120, 240, 480}; // he so chia nap vao timer
// unsigned int const timer_clock[8]={12, 24, 48, 96, 192}; // he so chia nap vao timer  ok with 196x64
// unsigned int const timer_clock[8]={15, 30, 60, 120, 240}; // he so chia nap vao timer  ok with 196x64
unsigned int const timer_clock[8]={10, 20, 40, 80, 160}; // he so chia nap vao timer ok with 256x64 47.9Hz
uint8_t     u8BitPos=0; // kiem tra xem dang <code> den bit nao
unsigned char hang_led;
uint8_t u8OutBuffer[MAX_BIT][MATRIX_W];
unsigned char display[3][MATRIX_H][MATRIX_W];

unsigned char COLOR[2][3]={{0,0,0},{0,0,0}};

int dosang;

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

//---------------------------------Chuong trinh quet LED ma tran --------------------------------------------------------//
static uint16_t Color888( uint8_t r, uint8_t g, uint8_t b, int gflag )
{
    if(gflag) { // Gamma-corrected color?
        r = pgm_read_byte(&gamma_table[r]); // Gamma correction table maps
        g = pgm_read_byte(&gamma_table[g]); // 8-bit input to 4-bit output
        b = pgm_read_byte(&gamma_table[b]);
        return ((uint16_t)r << 12) | ((uint16_t)(r & 0x8) << 8) | // 4/4/4->5/6/5
            ((uint16_t)g <<  7) | ((uint16_t)(g & 0xC) << 3) |
            (          b <<  1) | (           b        >> 3);
    } // else linear (uncorrected) color
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

#if 1
void math(uint16_t hang)
{
    for( uint8_t ts=0;ts<MAX_BIT;ts++)
    {
        for(uint16_t i=0;i<MATRIX_W;i++)
        {
            u8OutBuffer[ts][i]=0;
            if((display[0][0+hang][i] & (1<<ts)) != 0 ){u8OutBuffer[ts][i]|=0x1;}   		/* Write bit 0-1 to R1 */
            if((display[1][0+hang][i] & (1<<ts)) != 0 ){u8OutBuffer[ts][i]|=0x2;}   		/* Write bit 1-2 to G1 */
            if((display[2][0+hang][i] & (1<<ts)) != 0 ){u8OutBuffer[ts][i]|=0x4;}   		/* Write bit 2-4 to B1 */

            if((display[0][SCAN_RATE+hang][i] & (1<<ts)) != 0 ){u8OutBuffer[ts][i]|=0x8;}  	/* Write bit 3-8 to R2 */
            if((display[1][SCAN_RATE+hang][i] & (1<<ts)) != 0 ){u8OutBuffer[ts][i]|=0x10;} 	/* Write bit 4-16 to G2 */
            if((display[2][SCAN_RATE+hang][i] & (1<<ts)) != 0 ){u8OutBuffer[ts][i]|=0x20;} 	/* Write bit 5-32 to B2 */
        }
    }
}
#endif

#define CALLOVERHEAD 60   // Actual value measured = 56
#define LOOPTIME     200  // Actual value measured = 188

static void updateDisplay( void )
{
    uint16_t t, duration;


    /* Pull up OE pin to disable monitor */
    __HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_2,0);
    /* Pull down LAT pin in order to release new data region */
    Control_P->BSRR=0x10000;

    // t = (nRows > 8) ? LOOPTIME : (LOOPTIME * 2);
    // duration = ((t + CALLOVERHEAD * 2) << u8BitPos) - CALLOVERHEAD;

}

void IRQ_ProcessMonitor(void)
{
    /* Load new division value */
    TIM4->PSC = timer_clock[u8BitPos];
    /* Trigger EGR upto 1 in order re-load PSC value */
    TIM4->EGR = 1;
    /* Clear interrupt flag */
    TIM4->SR = 0xFFFFFFFE;

    /* Pull down LAT pin in order to release new data region */
    Control_P->BSRR=(1<<(LAT+16));
    /* Pull up OE pin to disable monitor */
    __HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_2,0);

    /* Release new data region */
    for(int i=0; i<MATRIX_W; i++)
    {
    	data_PORT->ODR=u8OutBuffer[u8BitPos][i];
    	__asm(
    		// "nop\n"
    		"nop\n"
    		"nop\n"
    	);
    	clk_P->BSRR=(1<<clk);
    	__asm(
    		// "nop\n"
    		"nop\n"
    		// "nop\n"
    	);
    }
    // bat sang hang tu 1->SCAN_RATE ( do tam matrix P5 quet 1/SCAN_RATE) (1 thoi diem chi co duy nhat 1 hang duoc sang len)
    GPIOB->ODR = (GPIOB->ODR & ~(mask_bit)) | hang_led;
    /* Latch data when one region was released */
    Control_P->BSRR=(1<<LAT); // chot data
    __HAL_TIM_SET_COMPARE(&htim5,TIM_CHANNEL_2,dosang);

    u8BitPos++;
    if (u8BitPos == MAX_BIT) {
      u8BitPos = 0;
      hang_led++;
      if (hang_led == SCAN_RATE) hang_led = 0;
      math(hang_led);
    }
}

void SET_dosang(uint16_t sang)
{
     dosang=sang;
}
uint16_t GET_dosang(void)
{
     return dosang;
}
//----------------------------------Cac ham de tao ra dong ho kim  analog ------------------------------------//

#define swap(a, b) {int t = a; a = b; b = t; }    // chuong trinh con ( hoan doi gia tri a va b)

void set_px(int x, int y, char cR,char cG,char cB)
{
    if(x<0 || y<0 || x>=MATRIX_W ||y>=MATRIX_H)return;
    display[0][y][x]=cR;
    display[1][y][x]=cG;
    display[2][y][x]=cB;
}
void P5_veduongngang(unsigned int x, unsigned int y,unsigned int dodai,char cR,char cG,char cB)   // ve 1 duong ke ngang
{
  int16_t dem;
  if(dodai>MATRIX_W)dodai=MATRIX_W;
  for(dem=0;dem<dodai;dem++)
    {
         display[0][y][x+dem] = cR;
         display[1][y][x+dem] = cG;
         display[2][y][x+dem] = cB;
    }
}

void P5_veduongdoc(unsigned char x, unsigned char y,unsigned char dodai,char cR,char cG,char cB)      // ve 1 duong ke doc
{
char dem;
  if(dodai>MATRIX_H) dodai=MATRIX_H;
  for(dem=0;dem<dodai;dem++)
    {
         display[0][y+dem][x] = cR;
         display[1][y+dem][x] = cG;
         display[2][y+dem][x] = cB;
    }
}
void P5_FillAllColorPannel( char cR, char cG, char cB )
{
    uint16_t u8H, u8W;
    for( u8H = 0U; u8H < MATRIX_H; u8H++ )
    {
        for( u8W = 0U; u8W < MATRIX_W; u8W++ )
        {
            display[0][u8H][u8W] = cR;
            display[1][u8H][u8W] = cG;
            display[2][u8H][u8W] = cB;
        }
    }
}
void P5_ClearAllColorPannel( void )
{
    uint16_t u8H, u8W;
    for( u8H = 0U; u8H < MATRIX_H; u8H++ )
    {
        for( u8W = 0U; u8W < MATRIX_W; u8W++ )
        {
            display[0][u8H][u8W] = 0;
            display[1][u8H][u8W] = 0;
            display[2][u8H][u8W] = 0;
        }
    }
}
void P5_veduongthang(int x, int y, int x1, int y1, char cR, char cG, char cB)
{
    int dx, dy, err, ystep;
    int steep = abs(y1 - y) > abs(x1 - x);
    int xtd, ytd;
    if (x == x1) {
        ytd = y1 - y;
        ytd = abs(ytd);
        if (y1 > y) P5_veduongdoc(x, y, ytd + 1, cR, cG, cB);
        else P5_veduongdoc(x1, y1, ytd + 1, cR, cG, cB);
        return;
    }

    if (y == y1) {
        xtd = x1 - x;
        xtd = abs(xtd);
        if (x1 > x) P5_veduongngang(x, y, xtd + 1, cR, cG, cB);
        else P5_veduongngang(x1, y1, xtd + 1, cR, cG, cB);
        return;
    }
    if (steep) {
        swap(x, y);
        swap(x1, y1);
    }

    if (x > x1) {
        swap(x, x1);
        swap(y, y1);
    }
    dx = x1 - x;
    dy = abs(y1 - y);
    err = dx / 2;

    if (y < y1) ystep = 1;
    else ystep = -1;

    for (; x <= x1; x++) {
        if (steep) {
        display[0][x][y] = cR;
        display[1][x][y] = cG;
        display[2][x][y] = cB;
        } else {
        display[0][y][x] = cR;
        display[1][y][x] = cG;
        display[2][y][x] = cB;
        }

        err -= dy;
        if (err < 0) {
        y += ystep;
        err += dx;
        }
    }
}
float mapf(float value, float c_min, float c_max, float t_min, float t_max) {
    return (value - c_min) / (c_max - c_min) * (t_max - t_min) + t_min;
}

float bound(float value, float max, float min) {
    return fmaxf(fminf(value, max), min);
}
void LED_plasmaEffect( void )
{
    static float time;
    uint8_t r, g, b;
    float xx, yy;
    float v;
    float delta = 0.025;

    time += 0.025;
    if(time > 12*M_PI) {
        delta *= -1;
    }

    for(uint8_t y = 0; y < MATRIX_H; y++) {
        yy = mapf(y, 0, MATRIX_H-1, 0, 2*M_PI);
        for(uint8_t x = 0; x < MATRIX_W; x++) {
            xx = mapf(x, 0, MATRIX_W-1, 0, 2*M_PI);

            v = sinf(xx + time);
            v += sinf((yy + time) / 2.0);
            v += sinf((xx + yy + time) / 2.0);
            float cx = xx + .5 * sinf(time/5.0);
            float cy = yy + .5 * cosf(time/3.0);
            v += sinf(sqrtf((cx*cx+cy*cy)+1)+time);
            v /= 2.0;
            r = 255 * (sinf(v * M_PI) + 1) / 2;
            g = 255 * (cosf(v * M_PI) + 1) / 2;
            b = 128 * (sinf(v * M_PI + 2*M_PI/3) + 1) / 2;
            // PIXEL(display, x, y).R = r;
            // PIXEL(display, x, y).G = g;
            // PIXEL(display, x, y).B = b;
            display[0][y][x] = r;
            display[1][y][x] = g;
            display[2][y][x] = b;
        }
    }
}

void LED_waveEffect( void )
{
    static float time;
    float xx;
    uint8_t r, g, b;

    if(time > 2*M_PI) {
        time = 0.0;
    }

    for(uint8_t y = 0; y < MATRIX_H; y++)
    {
        for(uint8_t x = 0; x < MATRIX_W; x++)
        {
            xx = mapf(x, 0, MATRIX_W-1, 0, 2*M_PI);
            r = 16 + 100 * (bound(sinf(xx + time + 2*M_PI/3), 0.5, -0.5) + 0.5);
            g = 16 + 100 * (bound(sinf(xx + time - 2*M_PI/3), 0.5, -0.5) + 0.5);
            b = 16 + 100 * (bound(sinf(xx + time         ), 0.5, -0.5) + 0.5);
            display[0][y][x] = g;
            display[1][y][x] = r;
            display[2][y][x] = b;
        }
    }
    time += 0.1;
}

#define GIMAGE_TEST
#if 1
void LED_fillBuffer( void )
{
    uint32_t p1;
    uint8_t u8R, u8G, u8B;
    for(uint8_t row = 0; row < MATRIX_H; row++)
    {
        for(uint8_t col = 0; col < MATRIX_W; col++)
        {
#ifdef GIMAGE_TEST
            p1 = row * MATRIX_W*3;
#if 0
            display[0][row][col] = gImage_test[p1+col*3+2];
            display[1][row][col] = gImage_test[p1+col*3+1];
            display[2][row][col] = gImage_test[p1+col*3+0];
#else
            display[0][row][col] = pgm_read_byte(&gamma_table[(gImage_test[p1+col*3+2])]); // Gamma correction table maps
            display[1][row][col] = pgm_read_byte(&gamma_table[(gImage_test[p1+col*3+1])]); // 8-bit input to 4-bit output
            display[2][row][col] = pgm_read_byte(&gamma_table[(gImage_test[p1+col*3+0])]);
#endif
#endif
#if 0
            p1 = row * MATRIX_W*2;
            // rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
            u8R = ( anhbii_map[p1+col*2 + 1] & 0xF8 ) >> 3;
            u8G = (( anhbii_map[p1+col*2 + 1] & 0x7 ) << 5 ) || (( anhbii_map[p1+col*2 ] & 0xE0 ) >> 5 );
            u8B = ( anhbii_map[p1+col*2 ] & 0x1F );

            display[0][row][col] = pgm_read_byte(&gamma_table[( u8R * 200) >> 5]); // Gamma correction table maps
            display[1][row][col] = pgm_read_byte(&gamma_table[( u8G * 200) >> 5]); // 8-bit input to 4-bit output
            display[2][row][col] = pgm_read_byte(&gamma_table[( u8B * 200) >> 5]);
#endif
#ifdef ORIGIN_IMAGE
            p1 = row * MATRIX_W;
            display[0][row][col] = frame[p1+col*3+2].Red;
            display[1][row][col] = frame[p1+col*3+1].Gre;
            display[2][row][col] = frame[p1+col*3+0].Blu;
#endif
        }
    }
}
#else
void LED_fillBuffer( void )
{
    uint32_t i = 0, p1, p2;
    uint8_t bit, mask;
    for(uint8_t row = 0; row < MATRIX_H; row++) {
        p1 = row * MATRIX_W;
        p2 = p1 + MATRIX_W * MATRIX_H;
        // for(bit = 0; bit < MAX_BIT; bit++) {
        //     mask = 1<<bit;
            for(uint8_t col = 0; col < MATRIX_W; col++)
            {
            display[0][y][x] = frame[p2+col].R;
            display[1][y][x] = frame[p2+col].G;
            display[2][y][x] = frame[p2+col].B;
                // buffer[i] =
                //     ((((gammaR[frame[p2+col].R]) & mask) >> bit) << 5) |
                //     ((((gammaG[frame[p2+col].G]) & mask) >> bit) << 4) |
                //     ((((gammaB[frame[p2+col].B]) & mask) >> bit) << 3) |
                //     ((((gammaR[frame[p1+col].R]) & mask) >> bit) << 2) |
                //     ((((gammaG[frame[p1+col].G]) & mask) >> bit) << 1) |
                //     ((((gammaB[frame[p1+col].B]) & mask) >> bit) << 0);
                i++;
            }
        }
    }
}
#endif
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
    display[0][y][x] = g;
    display[1][y][x] = r;
    display[2][y][x] = b;
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
float       angle1  = 0.0, angle2  = 0.0, angle3  = 0.0, angle4  = 0.0;
long        hueShift= 0;
    while (1)
    {
    	HAL_Delay(120);
        sx1 = (int)(cos(angle1) * radius1 + centerx1);
        sx2 = (int)(cos(angle2) * radius2 + centerx2);
        sx3 = (int)(cos(angle3) * radius3 + centerx3);
        sx4 = (int)(cos(angle4) * radius4 + centerx4);
        y1  = (int)(sin(angle1) * radius1 + centery1);
        y2  = (int)(sin(angle2) * radius2 + centery2);
        y3  = (int)(sin(angle3) * radius3 + centery3);
        y4  = (int)(sin(angle4) * radius4 + centery4);

        for(y=0; y<MATRIX_H; y++)
        {
            x1 = sx1; x2 = sx2; x3 = sx3; x4 = sx4;
            for(x=0; x<MATRIX_W; x++)
            {
                value = hueShift
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x1 * x1 + y1 * y1) >> 2))
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x2 * x2 + y2 * y2) >> 2))
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x3 * x3 + y3 * y3) >> 3))
                    + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x4 * x4 + y4 * y4) >> 3));
                    ColorHSV(x, y, value * 3, 255, 255, 1);
                    // matrix.drawPixel(x, y, ColorHSV(value * 3, 255, 255, 1));
                x1--; x2--; x3--; x4--;
            }
        y1--; y2--; y3--; y4--;
        }

        angle1 += 0.03;
        angle2 -= 0.07;
        angle3 += 0.13;
        angle4 -= 0.15;
        hueShift += 2;
        while(1);
    }

}
