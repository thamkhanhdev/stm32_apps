#include "ir1838.h"
#include "flash.h"
#include "matrix.h"

static uint8_t u8IRCurrentKey = IR_KEY_RESERVED;
static IR_ModeLists nIRCurrentMode = IR_NORMAL_MODE;

static uint32_t u32CurrData = 0UL;
static uint8_t u8CurDatKey = 0U;
static uint8_t u8CurrentBit = 0U;
static uint8_t u8DuplicatedDat = 0U;
/**
 * The current matrix key codes for the keyboard has 17 buttons
 *  A2  62  E2
 *  22  02  C2
 *  E0  A8  90
 *  68  98  B0
 *      18
 *  10  38  5A
 *      4A
 */
static const uint8_t u8KeyIR[IR_TOTAL_KEY] =
{
    0xFFU,
    0xB0U,
    0x68U,
    0x18U,
    0x4AU,
    0x10U,
    0x5AU,
    0x38U,
    0xA2U,
    0x62U,
    0xE2U,
    0x22U,
    0x02U,
    0xC2U,
    0xE0U,
    0xA8U,
    0x90U,
    0x98U,
    0xFFU
};

IR_KeysLists IR1838_GetCurrentKey(void)
{
    return (IR_KeysLists) u8IRCurrentKey;
}
inline void IR1838_SetKeysCode(IR_KeysLists nKeyPos, uint8_t u8KeyCode)
{
    (void) nKeyPos;
    (void) u8KeyCode;
    /* Skip this behavior until use dynamic codes */
    /* u8KeyIR[nKeyPos] = u8KeyCode; */
}

inline uint8_t IR1838_GetKeysCode(IR_KeysLists nKeyPos)
{
    return u8KeyIR[nKeyPos];
}

inline void IR1838_SetCurrentKey( IR_ModeLists nKeySet )
{
    u8IRCurrentKey = nKeySet;
}

inline IR_ModeLists IR1838_GetCurrentMode(void)
{
    return nIRCurrentMode;
}

inline void IR1838_EnterSetupMode(void)
{
    uint8_t u8ElePos = 0U;

    nIRCurrentMode = IR_SETUP_MODE;
    u8IRCurrentKey = IR_KEY_MENU;
    for( u8ElePos = 0U; u8ElePos < IR_TOTAL_KEY; u8ElePos++ )
    {
        (void) u8ElePos;
        /* Skip this behavior until use dynamic codes */
        /* u8KeyIR[u8ElePos] = 0U; */
    }
}
inline void IR1838_ExitSetupMode(void)
{
    nIRCurrentMode = IR_NORMAL_MODE;
}

/* TIM_IR should be configured input capture mode with falling edge */
void IR1838_IRQHandler(void)
{
    uint16_t u16CapturedValue;
    u16CapturedValue = LL_TIM_IC_GetCaptureCH1( TIM_IR );
    TIM_IR->CNT = 0U;

            /* With default HCLK 120Mhz: The recommented values are 12000 & 14000
             * With default HCLK 150Mhz: The recommented values are 15000 & 17500
             */
    if( (13000 <= u16CapturedValue) && (14000 >= u16CapturedValue) )
    {
        u32CurrData = 0UL;
        u8CurrentBit = 0U;
    }
    else if( 6000U > u16CapturedValue )
    {
        u32CurrData <<= 1UL;
        u8CurrentBit++;
        /* With default HCLK 120Mhz: The recommented values are 2100 & 2400
         * With default HCLK 150Mhz: The recommented values are 2625 & 3000
         * The current HCLK frequency is 150Mhz.
         */
        if( ((2000 <= u16CapturedValue) && (2400 >= u16CapturedValue)) )
        {
            u32CurrData |= 1UL;
        }
        if( 32U == u8CurrentBit )
        {
            if( (((u32CurrData >> 8U) & 0xFF) == ((uint8_t)~((uint8_t)(u32CurrData & 0xFF)))) && \
                (((u32CurrData >> 16U) & 0xFF) == ((uint8_t)~((uint8_t)((u32CurrData >> 24U) & 0xFF)))) )
            {
                u8CurDatKey = ((u32CurrData >> 8U) & 0xFF);
            }
            else
            {
                u8CurDatKey = 0U;
            }

            if( IR_SETUP_MODE == nIRCurrentMode )
            {
                for( uint8_t u8Pos = 0U; u8Pos < IR_KEY_DONE; u8Pos++)
                {
                    if( u8CurDatKey == u8KeyIR[u8Pos] )
                    {
                        u8DuplicatedDat = 1U;
                        break;
                    }
                }
                if( !u8DuplicatedDat )
                {
                    /* Skip this behavior until use dynamic codes */
                    /* u8KeyIR[u8IRCurrentKey++] = u8CurDatKey; */
                    if( IR_KEY_DONE == (IR_KeysLists) u8IRCurrentKey )
                    {
                        nIRCurrentMode = IR_NORMAL_MODE;
                    }
                }
                else
                {
                    u8DuplicatedDat = 0U;
                }
            }
            else if( IR_NORMAL_MODE == nIRCurrentMode )
            {
                for( uint8_t u8CheckPos = 0U; u8CheckPos < IR_KEY_DONE; u8CheckPos++)
                {
                    if( u8CurDatKey == u8KeyIR[u8CheckPos] )
                    {
                        u8IRCurrentKey = u8CheckPos;
                        break;
                    }
                }
            }
            // MATRIX_Printf( FONT_DEFAULT, 1U, 0U, (temp++) * 8, (uint16_t) 0xFFFF,  "0x%02X\n", u8CurDatKey);
        }
    }

    if( LL_TIM_IsActiveFlag_CC1OVR( TIM_IR ) )
    {
        LL_TIM_ClearFlag_CC1OVR( TIM_IR );
    }
    OFF_LED();

}

inline void IR1838_InitData(void)
{
    uint8_t u8KeyPos;
    for( u8KeyPos = IR_KEY_RESERVED; u8KeyPos < IR_TOTAL_KEY; u8KeyPos++ )
    {
        IR1838_SetKeysCode( (IR_KeysLists) u8KeyPos, (uint8_t) FLASH_Read(u8KeyPos));
        // MATRIX_Printf( FONT_DEFAULT, 1U, u8KeyPos * 13, 0, 0xF81F, "%02X ", IR1838_GetKeysCode(u8KeyPos) );
        // MATRIX_Printf( FONT_DEFAULT, 1U, u8KeyPos * 13, 0, 0xF81F, "%02X ", FLASH_Read(u8KeyPos) );
    }
}