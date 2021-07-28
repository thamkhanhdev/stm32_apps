/*================================================================================================*/
/**
*   @file       clock.c
*   @author     Khanh Tham
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      clock program body.
*   @details    Reserved.
*
*   This file contains sample code only. It is not part of the production code deliverables.
*
*   @addtogroup RESERVED.
*   @{
*/
/*==================================================================================================
*   Project              : Reserved.
*   Platform             : Reserved
*
*   SW Version           : 1.0.0
*   Build Version        : Reserved
*
*   Copyright 2020 , Inc.
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
#include "clock.h"

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

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
  *             2. Only use the one data array gBuff[MATRIX_WIDTH * MATRIX_HEIGHT]. In this case, only prefer use the color code are RGB565, RGB555, RGB444...
  *                 Each interrupt event occured, we want to print out each bit possition like as below algorithm. This exemple shall be declare RGB565 encode
  *                 | R4 R3 R2 R1 R0 | G5 G4 G3 G2 G1 G0 | B4 B3 B2 B1 B0 |
  *                 u16TempDat = gBuff[ y * MATRIX_WIDTH + x ];
  *                 DAT_P->ODR = gBuff[ y * MATRIX_WIDTH + x ];
  * @param  None.
  * @retval None.
  */
/*================================================================================================*/

#ifdef __cplusplus
}
#endif

/** @} */
