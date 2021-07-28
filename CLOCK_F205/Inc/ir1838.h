/*==================================================================================================
*   @file       IR1838.h
*   @author     Khanh Tham Duy
*   @contact    thamkhanhdev@gmail.com
*   @version    1.0.0
*
*   @brief      Reserved - API header.
*   @details    Contains declaration of the IR1838 communication.
*
*   @addtogroup BUILD_ENV_COMPONENT
*   @{
*/
/*==================================================================================================
*   Project              : Reserved.
*   Platform             : Reserved.
*
*   SW Version           : 1.0.0
*   Build Version        : Reserved
*
*   Copyright 2020, Inc.
*   All Rights Reserved.
==================================================================================================*/
/**
 * Note: Reserved.
 */
#ifndef IR1838_H
#define IR1838_H

#ifdef __cplusplus
extern "C"{
#endif


/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "main.h"
#include "Std_Types.h"

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
#define TIM_IR  TIM3
/*==================================================================================================
*                                             ENUMS
==================================================================================================*/
typedef enum
{
    IR_LEADING_PULSE_FIELD,
    IR_DAT_FIELD,
    IR_RESERVED_FIELD
} IR_FieldTypes;

typedef enum
{
    IR_KEY_RESERVED,
    IR_KEY_MENU,
    IR_KEY_EXIT,
    IR_KEY_UP,
    IR_KEY_DOWN,
    IR_KEY_LEFT,
    IR_KEY_RIGHT,
    IR_KEY_OK,
    IR_KEY_0,
    IR_KEY_1,
    IR_KEY_2,
    IR_KEY_3,
    IR_KEY_4,
    IR_KEY_5,
    IR_KEY_6,
    IR_KEY_7,
    IR_KEY_8,
    IR_KEY_9,
    IR_KEY_DONE,
    IR_TOTAL_KEY
} IR_KeysLists;

typedef enum
{
    IR_NORMAL_MODE,
    IR_SETUP_MODE,
    IR_RESERVED_MODE
} IR_ModeLists;
/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/


/*==================================================================================================
*                                GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/


/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
void IR1838_IRQHandler(void);
IR_KeysLists IR1838_GetCurrentKey(void);
void IR1838_SetCurrentKey( IR_ModeLists nKeySet );
void IR1838_SetKeysCode(IR_KeysLists nKeyPos, uint8_t u8KeyCode);
uint8_t IR1838_GetKeysCode(IR_KeysLists nKeyPos);
void IR1838_EnterSetupMode(void);
void IR1838_ExitSetupMode(void);
IR_ModeLists IR1838_GetCurrentMode(void);
void IR1838_InitData(void);

#ifdef __cplusplus
}
#endif

#endif /* IR1838_H */

/** @} */
