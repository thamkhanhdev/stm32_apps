#include "stm32f2xx.h"
#include "Std_Types.h"
#include "stdint.h"
#include "string.h"

/* 1024 Bytes */
#define FLASH_USER_ADDRESS      ((uint32_t)0x0803FF00UL)

void FLASH_Lock(void);
StdReturnType FLASH_Unlock(void);
StdReturnType FLASH_Erase(uint32_t u32Address);
StdReturnType FLASH_Write(uint16_t u16Address, uint8_t u8Data);
uint8_t FLASH_Read(uint16_t u16Address);

#define FLASH_KEY_CODE      (0U)
#define FLASH_MIN_BRIGHTNESS    (19U)
#define FLASH_MAX_BRIGHTNESS    (20U)