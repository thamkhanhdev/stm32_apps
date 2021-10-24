#include "w25qxx_qspi.h"

#define W25Qxx_TEST              (1)
#define APPLICATION_ADDRESS      QSPI_BASE
#define ISP_ADDRESS              (0x1FF09800UL)
typedef  void (*pFunction)(void);

#if W25Qxx_TEST == (1)
uint8_t read[4096];
uint8_t write[4096];
#endif


uint8_t app_IsReady(uint32_t addr)
{
	if(((*(__IO uint32_t*)addr) & 0x2FF80000 ) == 0x24000000)
		return SUCCESS;
	else if(((*(__IO uint32_t*)addr) & 0x2FF80000 ) == 0x20000000)
		return SUCCESS;
	else
		return ERROR;
}

void app_Jump(uint32_t addr)
{
	pFunction JumpToApplication;
	uint32_t JumpAddress;

	/* Jump to user application */
	JumpAddress = *(__IO uint32_t*) (addr + 4);
	JumpToApplication = (pFunction) JumpAddress;

	/* Initialize user application's Stack Pointer */
	__set_MSP(*(__IO uint32_t*) addr);
	JumpToApplication();
}


	w25qxx_Init();
	w25qxx_EnterQPI();

#if W25Qxx_TEST == (1)
	W25qxx_EraseSector(0);
	for(uint32_t j=0;j<sizeof(write);j+=256)
		for(uint32_t i=0;i<=0xff;i++)
			write[j+i] = i;
	W25qxx_WriteNoCheck(write,0,sizeof(write));
	W25qxx_Read(read,0,sizeof(read));
#endif
	w25qxx_Startup(w25qxx_DTRMode);