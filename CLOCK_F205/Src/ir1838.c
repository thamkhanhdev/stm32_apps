#include "ir1838.h"
#include "flash.h"
#include "matrix.h"

static uint8_t u8IRCurrentKey = IR_KEY_RESERVED;
static IR_FieldTypes nIRCurrField = IR_RESERVED_FIELD;
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
static uint8_t u8KeyIR[IR_TOTAL_KEY];

IR_KeysLists IR1838_GetCurrentKey(void)
{
    return (IR_KeysLists) u8IRCurrentKey;
}
inline void IR1838_SetKeysCode(IR_KeysLists nKeyPos, uint8_t u8KeyCode)
{
    u8KeyIR[nKeyPos] = u8KeyCode;
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
        u8KeyIR[u8ElePos] = 0U;
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

    switch( nIRCurrField )
    {
        case IR_RESERVED_FIELD:
            nIRCurrField = IR_LEADING_PULSE_FIELD;
            break;
        case IR_LEADING_PULSE_FIELD:

            /* With default HCLK 120Mhz: The recommented values are 12000 & 14000
             * With default HCLK 150Mhz: The recommented values are 15000 & 17500
             * The current HCLK frequency is 150Mhz.
             */
            if( (15000 <= u16CapturedValue) && (17500 >= u16CapturedValue) )
            {
                nIRCurrField = IR_DAT_FIELD;
                u32CurrData = 0UL;
                u8CurrentBit = 0U;
            }
            else
            {
                nIRCurrField = IR_RESERVED_FIELD;
            }
            break;
        case IR_DAT_FIELD:
            u32CurrData <<= 1UL;
            u8CurrentBit++;
            /* With default HCLK 120Mhz: The recommented values are 2100 & 2400
             * With default HCLK 150Mhz: The recommented values are 2625 & 3000
             * The current HCLK frequency is 150Mhz.
             */
            if( ((2625 <= u16CapturedValue) && (3000 >= u16CapturedValue)) )
            {
                u32CurrData |= 1UL;
            }
            if( 32U == u8CurrentBit )
            {
                nIRCurrField = IR_RESERVED_FIELD;
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
                        u8KeyIR[u8IRCurrentKey++] = u8CurDatKey;
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
                // MATRIX_Printf( FONT_DEFAULT, 1U, 30U, 0U, (uint16_t) u32CurrData,  "0x%02X\n", u8CurDatKey);
            }
            break;
        default:
            nIRCurrField = IR_RESERVED_FIELD;
            break;
    }

    if( LL_TIM_IsActiveFlag_CC1OVR( TIM_IR ) )
    {
        LL_TIM_ClearFlag_CC1OVR( TIM_IR );
    }

}