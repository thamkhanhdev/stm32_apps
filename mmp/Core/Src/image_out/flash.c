#include <flash.h>

void Flash_Lock()
{
	HAL_FLASH_Lock();
}

void Flash_Unlock()
{
	HAL_FLASH_Unlock();
}

void Flash_Erase(uint32_t addr)
{
	/*
	 * 	Sector Erase
		To erase a sector, follow the procedure below:
		1. Check that no Flash memory operation is ongoing by checking the BSY bit in the
		FLASH_SR register
		2. Set the SER bit and select the sector out of the 12 sectors (for STM32F405xx/07xx and
		STM32F415xx/17xx) and out of 24 (for STM32F42xxx and STM32F43xxx) in the main
		memory block you wish to erase (SNB) in the FLASH_CR register
		3. Set the STRT bit in the FLASH_CR register
		4. Wait for the BSY bit to be cleared
	 */
  Flash_Unlock();
  while((FLASH->SR&FLASH_SR_BSY));
  FLASH->CR |= FLASH_CR_SER; // Sector Erase Set
  FLASH->CR |= FLASH_CR_SNB_2; // Erase Sector 4
  FLASH->CR |= FLASH_CR_STRT; //Start Page Erase
  while((FLASH->SR&FLASH_SR_BSY));
  FLASH->CR &= ~FLASH_SR_BSY;
  FLASH->CR &= ~FLASH_CR_SER; //Page Erase Clear
  Flash_Lock();
}

void EEP_write(int add, int data)
{
  uint32_t addr;
	uint16_t dat[50]={0};
	for(int i=0;i<50;i++)dat[i]=EEP_read(i); //doc du lieu ra
	Flash_Erase(DATA_START_ADDRESS);
	Flash_Unlock();
	for(int i=0;i<50;i++)
	{
		addr = DATA_START_ADDRESS + (i*2);
		FLASH->CR |= FLASH_CR_PG;				/*!< Programming */
		while((FLASH->SR&FLASH_SR_BSY));
		if(i==add)*(__IO uint16_t*)addr = data;
		else *(__IO uint16_t*)addr = dat[i];
		while((FLASH->SR&FLASH_SR_BSY));
		FLASH->CR &= ~FLASH_CR_PG;
	}
	Flash_Lock();
}

uint16_t EEP_read(int add)
{
	int32_t addr;
	addr = DATA_START_ADDRESS + (add*2);
	uint16_t* val = (uint16_t *)addr;
	return *val;
}

