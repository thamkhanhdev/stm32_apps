#include "stm32f2xx.h"
#include "Std_Types.h"
#include "stdint.h"
#include "string.h"

/* 1024 Bytes */
#define FLASH_USER_ADDRESS      ((uint32_t)0x0803FF20UL)

void FLASH_Lock(void);
StdReturnType FLASH_Unlock(void);
StdReturnType FLASH_Erase(uint32_t u32Address);
StdReturnType FLASH_Write(uint16_t u16Address, uint8_t u8Data);
uint8_t FLASH_Read(uint16_t u16Address);

//so do bo nho EEPROM
#define ADD_BUTTON_TANG_DO_SANG_EEP   0
#define ADD_BUTTON_GIAM_DO_SANG_EEP   1
#define ADD_BUTTON_MENU_EEP           2
#define ADD_BUTTON_UP_EEP             3
#define ADD_BUTTON_DOWN_EEP           4
#define ADD_BUTTON_THOAT_EEP          5
#define ADD_dosang                    42