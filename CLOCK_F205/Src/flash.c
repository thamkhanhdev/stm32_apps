#include "flash.h"

#define FLASH_TIMEOUT   1000U;

inline void FLASH_Lock(void)
{
    /* Set the LOCK Bit to lock the FLASH Registers access */
    FLASH->CR |= FLASH_CR_LOCK;
}

inline StdReturnType FLASH_Unlock(void)
{
    StdReturnType nReturnValue = E_OK;

    /* If flash is locked, then unlock it, otherwise do nothing */
    if( FLASH->CR & FLASH_CR_LOCK )
    {
        /* Authorize the FLASH Registers access */
        FLASH->KEYR =  0x45670123UL;
        FLASH->KEYR = 0xCDEF89ABUL;

        /* Verify Flash is unlocked */
        if( FLASH->CR & FLASH_CR_LOCK )
        {
            nReturnValue = E_NOT_OK;
        }
    }

    return nReturnValue;
}

inline StdReturnType FLASH_Erase(uint32_t u32Address)
{
    StdReturnType nReturnValue = E_NOT_OK;
    uint16_t u16Timeout = FLASH_TIMEOUT;

    nReturnValue = FLASH_Unlock();

    /* If flash is unlocked */
    if( E_OK == nReturnValue )
    {
        /* Wait for the busy bit is reset or timeout occurs */
        while( (FLASH->SR & FLASH_SR_BSY) && (u16Timeout--) );

        /* In case flash has already been idle */
        if( u16Timeout )
        {
            /* Reset the timeout value to use at the next checking time */
            u16Timeout = FLASH_TIMEOUT;

            /* Enable sector erase */
            FLASH->CR |= FLASH_CR_SER;
            /* It need choosing the sector numbers from 0b000 -> 0b1011 */
            FLASH->CR |= (uint32_t) (0x00 << 3UL);
            /* Start erase flash */
            FLASH->CR |= FLASH_CR_STRT;

            /* Wait for the busy bit is reset or timeout occurs */
            while( (FLASH->SR & FLASH_SR_BSY) && (u16Timeout--) );

            /* In case flash has already been idle */
            if( !u16Timeout )
            {
                FLASH->CR &= ~FLASH_SR_BSY;
                FLASH->CR &= ~FLASH_CR_SER;
            }
            else
            {
                nReturnValue = E_NOT_OK;
            }
        }
        else
        {
            nReturnValue = E_NOT_OK;
        }
    }

    return nReturnValue;
}

inline uint8_t FLASH_Read(uint16_t u16Address)
{
    return (uint8_t) (*((uint8_t *) (FLASH_USER_ADDRESS + u16Address)));
}

inline StdReturnType FLASH_Write(uint16_t u16Address, uint8_t u8Data)
{
    StdReturnType nReturnValue = E_NOT_OK;
    uint32_t u32Addr = 0UL;

    nReturnValue = FLASH_Unlock();

    if( E_OK == nReturnValue )
    {
        u32Addr = FLASH_USER_ADDRESS + u16Address;
        FLASH->CR |= FLASH_CR_PG;
        while( 0U != (FLASH->SR & FLASH_SR_BSY) );
        *(__IO uint8_t *) u32Addr = u8Data;
        while( 0U != (FLASH->SR & FLASH_SR_BSY));
        FLASH->CR &= ~FLASH_CR_PG;
        FLASH_Lock();
    }

    return nReturnValue;
}



