/*==================================================================================================
*   @file       clock.h
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      API header.
*   @details    Contains declaration of the API functions.
*
*   @addtogroup BUILD_ENV_COMPONENT
*   @{
*/
/*==================================================================================================
*   Project              : Reserved.
*   Platform             : Reserved
*
*   SW Version           : 1.0.0
*   Build Version        : Reserved
*
*   Copyright 2020, Inc.
*   All Rights Reserved.
==================================================================================================*/
#ifndef CLOCK_H
#define CLOCK_H

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
// #include <stdbool.h>
// #include <stdlib.h>
// #include "math.h"
// #include <string.h>
// #include <stdarg.h>

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/
/**
 * @brief   Type of pixel color
 */
typedef enum
{
    CLOCK_STATIC_SINGLE_COLOR,
    CLOCK_STATIC_RAINBOW_COLOR,
    CLOCK_RANDOM_SINGLE_COLOR,
    CLOCK_RANDOM_RAINBOW_COLOR,
    CLOCK_RESERVED_COLOR
} CLOCK_Color_Type;

/**
 * @brief   Type of brightness
 */
typedef enum
{
    CLOCK_STATIC_BRIGHTNESS,
    CLOCK_DYNAMIC_BRIGHTNESS
} CLOCK_Brightness_Type;

/**
 * @brief   Store info of each rainbow field in the monitor
 */
typedef struct CLOCK_RainbowInfo
{
    uint16_t u16HueStart;
    uint16_t u16HueEnd;
    uint8_t u8Saturation;
    uint8_t u8Value;
    float fTilt;
} CLOCK_RainbowInfo_TagType;

/**
 * @brief   Store info of each region in the monitor
 */
typedef struct CLOCK_RegionColor
{
    CLOCK_RainbowInfo_TagType nRainbow;
    CLOCK_Color_Type nColor;
    MATRIX_FontTypes nFont;
    uint16_t u16Color;
    uint16_t u16xStart;
    uint16_t u16yStart;
    uint8_t u8Width;
    uint8_t u8Height;
} CLOCK_RegionColor_TagType;

typedef struct CLOCK_MonitorIndex
{
    CLOCK_RegionColor_TagType nHour;
    CLOCK_RegionColor_TagType nMinute;
    CLOCK_RegionColor_TagType nTingTong;
    CLOCK_RegionColor_TagType nDate;
    CLOCK_RegionColor_TagType nText;
} CLOCK_MonitorIndex_TagType;
/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/


/*==================================================================================================
*                                GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_H */

/** @} */
