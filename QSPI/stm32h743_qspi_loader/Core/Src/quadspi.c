/**
  ******************************************************************************
  * File Name          : QUADSPI.c
  * Description        : This file provides code for the configuration
  *                      of the QUADSPI instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "quadspi.h"

/* USER CODE BEGIN 0 */
static uint8_t QSPI_WriteEnable(void);
static uint8_t QSPI_AutoPollingMemReady(void);
static uint32_t QSPI_FLASH_ReadStatusReg(uint8_t reg);
static uint32_t QSPI_FLASH_WriteStatusReg(uint8_t reg,uint8_t regvalue);
static uint8_t QSPI_ResetMemory(void);
static uint8_t QSPI_EnterQPI(void);
static void QSPI_Wait_Busy(void);

uint32_t u32DeviceID = 0;
uint32_t u32Id = 0;
QSPI_StatusTypeDef nCurrentMode = QSPI_SPI_MODE;

/* USER CODE END 0 */

QSPI_HandleTypeDef hqspi;

/* QUADSPI init function */
void MX_QUADSPI_Init(void)
{

  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold = 32;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize = 23;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef* qspiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(qspiHandle->Instance==QUADSPI)
  {
  /* USER CODE BEGIN QUADSPI_MspInit 0 */

  /* USER CODE END QUADSPI_MspInit 0 */
    /* QUADSPI clock enable */
    __HAL_RCC_QSPI_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**QUADSPI GPIO Configuration
    PE2     ------> QUADSPI_BK1_IO2
    PB2     ------> QUADSPI_CLK
    PD11     ------> QUADSPI_BK1_IO0
    PD12     ------> QUADSPI_BK1_IO1
    PD13     ------> QUADSPI_BK1_IO3
    PB6     ------> QUADSPI_BK1_NCS
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN QUADSPI_MspInit 1 */

  /* USER CODE END QUADSPI_MspInit 1 */
  }
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef* qspiHandle)
{

  if(qspiHandle->Instance==QUADSPI)
  {
  /* USER CODE BEGIN QUADSPI_MspDeInit 0 */

  /* USER CODE END QUADSPI_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_QSPI_CLK_DISABLE();

    /**QUADSPI GPIO Configuration
    PE2     ------> QUADSPI_BK1_IO2
    PB2     ------> QUADSPI_CLK
    PD11     ------> QUADSPI_BK1_IO0
    PD12     ------> QUADSPI_BK1_IO1
    PD13     ------> QUADSPI_BK1_IO3
    PB6     ------> QUADSPI_BK1_NCS
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13);

  /* USER CODE BEGIN QUADSPI_MspDeInit 1 */

  /* USER CODE END QUADSPI_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @retval None
  */
static uint8_t QSPI_WriteEnable(void)
{
    QSPI_CommandTypeDef     sCommand;
    QSPI_AutoPollingTypeDef sConfig;

    if( QSPI_QPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    }

    sCommand.Instruction       = WRITE_ENABLE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0U;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if( HAL_OK != HAL_QSPI_Command( &hqspi,
                                    &sCommand,
                                    HAL_QPSI_TIMEOUT_DEFAULT_VALUE
                                  ) )
    {
        return QSPI_ERROR;
    }

    sConfig.Match           = MEMORY_FSR_WREN;
    sConfig.Mask            = MEMORY_FSR_WREN;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1U;
    sConfig.Interval        = 0x10U;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    if( QSPI_QPI_MODE == nCurrentMode )
    {
        sCommand.DataMode       = QSPI_DATA_4_LINES;
    }
    else
    {
        sCommand.DataMode       = QSPI_DATA_1_LINE;
    }

    sCommand.Instruction    = READ_STATUS_REG1_CMD;

    if( HAL_OK != HAL_QSPI_AutoPolling( &hqspi,
                                        &sCommand,
                                        &sConfig,
                                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE
                                      ) )
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}


/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @retval None
  */
static uint8_t QSPI_AutoPollingMemReady(void)
{
    QSPI_CommandTypeDef     sCommand;
    QSPI_AutoPollingTypeDef sConfig;

    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        sCommand.DataMode          = QSPI_DATA_1_LINE;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
        sCommand.DataMode          = QSPI_DATA_4_LINES;
    }

    /* Configure automatic polling mode to wait for memory ready */
    sCommand.Instruction       = READ_STATUS_REG1_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DummyCycles       = 0U;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    sConfig.Match           = 0x00;
    sConfig.Mask            = MEMORY_FSR_BUSY;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1;
    sConfig.Interval        = 0x10U;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    if( HAL_OK != HAL_QSPI_AutoPolling( &hqspi,
                                        &sCommand,
                                        &sConfig,
                                        HAL_QPSI_TIMEOUT_DEFAULT_VALUE
                                      ) )
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}


/**
  * @brief  This function reset the QSPI memory.
  * @retval None
  */
static uint8_t QSPI_ResetMemory()
{
    QSPI_CommandTypeDef sCommand;

    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = RESET_ENABLE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    sCommand.Instruction = RESET_MEMORY_CMD;
    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    sCommand.Instruction       = RESET_ENABLE_CMD;
    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    sCommand.Instruction = RESET_MEMORY_CMD;
    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    if (QSPI_AutoPollingMemReady() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}


/**
  * @brief  This function enter the QSPI memory in QPI mode
  * @param  hqspi QSPI handle
  * @retval QSPI status
  */
static uint8_t QSPI_EnterQPI(void)
{
    QSPI_CommandTypeDef sCommand;
    uint8_t stareg2;

    stareg2 = QSPI_FLASH_ReadStatusReg(2U);

    if((stareg2 & 0X02) == 0)
    {
        QSPI_WriteEnable();
        stareg2 |= 1<<1;
        QSPI_FLASH_WriteStatusReg( 2U, stareg2 );
    }

    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = ENTER_QPI_MODE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AddressSize       = QSPI_ADDRESS_8_BITS;
    sCommand.Address           = 0x000000;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.NbData            = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }

    /* Configure automatic polling mode to wait the memory is ready */
    if(QSPI_AutoPollingMemReady() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    nCurrentMode = QSPI_QPI_MODE;

    return QSPI_OK;
}

static void QSPI_Wait_Busy(void)
{
    while((QSPI_FLASH_ReadStatusReg(1)&0x01)==0x01);
}


/**
  * @brief  None.
  * @retval None.
  */
static uint32_t QSPI_FLASH_ReadStatusReg(uint8_t reg)
{
    QSPI_CommandTypeDef sCommand;
    uint32_t Temp = 0;
    uint8_t pData[10];

    if(reg == 1)
    sCommand.Instruction       = READ_STATUS_REG1_CMD;
    else if(reg == 2)
    sCommand.Instruction       = READ_STATUS_REG2_CMD;
    else if(reg == 3)
    sCommand.Instruction       = READ_STATUS_REG3_CMD;

    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        sCommand.DataMode          = QSPI_DATA_1_LINE;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
        sCommand.DataMode          = QSPI_DATA_4_LINES;
    }

    sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize       = QSPI_ADDRESS_NONE;
    sCommand.Address           = 0x000000;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.NbData            = 1;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }
    if (HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }

    Temp = pData[0] ;

    return Temp;
}

/**
  * @brief  None.
  * @retval None.
  */
static uint32_t QSPI_FLASH_WriteStatusReg(uint8_t reg,uint8_t regvalue)
{
    QSPI_CommandTypeDef sCommand;

    if (QSPI_WriteEnable() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    if(reg == 1)
    sCommand.Instruction       = WRITE_STATUS_REG1_CMD;
    else if(reg == 2)
    sCommand.Instruction       = WRITE_STATUS_REG2_CMD;
    else if(reg == 3)
    sCommand.Instruction       = WRITE_STATUS_REG3_CMD;

    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        sCommand.DataMode          = QSPI_DATA_1_LINE;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
        sCommand.DataMode          = QSPI_DATA_4_LINES;
    }

    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AddressSize       = QSPI_ADDRESS_8_BITS;
    sCommand.Address           = 0x000000;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.NbData            = 1;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }
    if (HAL_QSPI_Transmit(&hqspi, &regvalue, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }

    if (QSPI_AutoPollingMemReady() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}

/**
  * @brief  QUADSPI init function.
  * @param  None.
  * @retval None.
  */
uint8_t CSP_QUADSPI_Init(void)
{
    QSPI_CommandTypeDef sCommand;
    uint8_t u8Value;

    HAL_Delay(5);

    if (HAL_QSPI_DeInit(&hqspi) != HAL_OK)
    {
        while(1);
    }

    HAL_Delay(5);

    MX_QUADSPI_Init();

    /* QSPI memory reset */
    if (QSPI_ResetMemory() != QSPI_OK)
    {
        while(1);
    }

    HAL_Delay(5);

    u32DeviceID = QSPI_FLASH_ReadDeviceID();
    if( u32DeviceID != 0xEF17UL )
    {
        while(1);
    }

    u32Id = QSPI_FLASH_ReadID();
    if( u32Id != 0xEF4018UL )
    {
        while(1);
    }


    if (QSPI_AutoPollingMemReady() != HAL_OK)
    {
        while(1);
    }

    /* Enter QSPI memory in QSPI mode */
    if( QSPI_OK != QSPI_EnterQPI() )
    {
        while(1);
    }

    if (QSPI_WriteEnable() != QSPI_OK)
    {
        while(1);
    }

    /* Only use in QPI mode */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    sCommand.Instruction       = SET_READ_PARAM_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AddressSize       = QSPI_ADDRESS_8_BITS;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_4_LINES;
    sCommand.DummyCycles       = 0U;
    sCommand.NbData            = 1U;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* u8Value = (DummyClock(8)/2 -1)<<4 | ((WrapLenth(8)/8 - 1)&0x03); */
    u8Value = 0x30U;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1);
    }

    if (HAL_QSPI_Transmit(&hqspi, &u8Value, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1);
    }

    if (QSPI_AutoPollingMemReady() != QSPI_OK)
    {
        while(1);
    }


    return HAL_OK;
}


/**
  * @brief  None
  * @retval None
  */
uint8_t CSP_QSPI_Erase_Chip(void)
{
    QSPI_CommandTypeDef sCommand;

    if( QSPI_OK!= QSPI_WriteEnable() )
    {
        return QSPI_ERROR;
    }

    QSPI_Wait_Busy();

    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    }

    sCommand.Instruction       = CHIP_ERASE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0U;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    QSPI_Wait_Busy();

    if (QSPI_AutoPollingMemReady() != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK;
}


/**
  * @brief  Configure the QSPI in memory-mapped mode   QPI/SPI && DTR(DDR)/Normal Mode
  * @param  DTRMode: w25qxx_DTRMode DTR mode ,w25qxx_NormalMode Normal mode
  * @retval QSPI memory status
  */
uint8_t CSP_QSPI_EnableMemoryMappedMode(QSPI_StatusTypeDef nDTRMode)
{
    QSPI_CommandTypeDef      sCommand;
    QSPI_MemoryMappedTypeDef sMemMappedCfg;

    if( QSPI_QPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    }

    /* Enable Memory-Mapped mode */
    sCommand.AddressMode        = QSPI_ADDRESS_4_LINES;
    sCommand.Address            = 0U;
    sCommand.AddressSize        = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
    sCommand.AlternateBytes     = 0xEF;
    sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.DataMode           = QSPI_DATA_4_LINES;
    sCommand.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode           = QSPI_SIOO_INST_ONLY_FIRST_CMD;

    if( QSPI_DTR_MODE == nDTRMode )
    {
        sCommand.Instruction     = QUAD_INOUT_FAST_READ_DTR_CMD;
        sCommand.DummyCycles     = DUMMY_CYCLES_READ_QUAD_DTR;
        sCommand.DdrMode         = QSPI_DDR_MODE_ENABLE;
    }
    else
    {
        sCommand.Instruction     = QUAD_INOUT_FAST_READ_CMD_4BYTE;

        if( QSPI_QPI_MODE == nCurrentMode )
        {
            sCommand.DummyCycles   = DUMMY_CYCLES_READ_QUAD;
        }
        else
        {
            sCommand.DummyCycles   = DUMMY_CYCLES_READ_QUAD-2;
        }

        sCommand.DdrMode         = QSPI_DDR_MODE_DISABLE;
    }

    sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    sMemMappedCfg.TimeOutPeriod = 0U;

    /* Configure the memory mapped mode */
    if( HAL_QSPI_MemoryMapped(&hqspi, &sCommand, &sMemMappedCfg) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}


/**
  * @brief  None.
  * @retval None.
  */
uint8_t CSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
    QSPI_CommandTypeDef sCommand;

    if(Size == 0)
    {
        return QSPI_OK;
    }

    if( QSPI_QPI_MODE == nCurrentMode)
    {
        sCommand.Instruction     = QUAD_INOUT_FAST_READ_CMD_4BYTE;
        sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        sCommand.DummyCycles     = DUMMY_CYCLES_READ_QUAD;
    }
    else
    {
        sCommand.Instruction     = QUAD_INOUT_FAST_READ_CMD_4BYTE;
        sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        sCommand.DummyCycles     = DUMMY_CYCLES_READ_QUAD-2;
    }

    sCommand.Address           = ReadAddr;
    sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;

    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
    sCommand.AlternateBytes    = 0xFF;
    sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;

    sCommand.DataMode          = QSPI_DATA_4_LINES;
    sCommand.NbData            = Size;

    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    if (HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  pData Pointer to data to be written
  * @param  WriteAddr Write start address
  * @param  Size Size of data to write.
  * @retval QSPI memory status
  */
uint8_t CSP_QSPI_WriteMemory(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
    QSPI_CommandTypeDef sCommand;
    uint32_t u32EndAddr, u32CurrSize, u32CurrAddr;

    if(Size == 0)
    {
        return QSPI_OK;
    }

    u32CurrAddr = 0;

    while (u32CurrAddr <= WriteAddr)
    {
        u32CurrAddr += MEMORY_PAGE_SIZE;
    }
    u32CurrSize = u32CurrAddr - WriteAddr;

    if (u32CurrSize > Size)
    {
        u32CurrSize = Size;
    }

    u32CurrAddr = WriteAddr;
    u32EndAddr = WriteAddr + Size;

    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.DataMode          = QSPI_DATA_4_LINES;
    sCommand.DummyCycles       = 0U;

    sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.Instruction       = QUAD_INPUT_PAGE_PROG_CMD_4BYTE;
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
        sCommand.Instruction       = INPUT_PAGE_PROG_CMD_4BYTE;
        sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
    }

    do
    {
        sCommand.Address = u32CurrAddr;
        if(u32CurrSize == 0)
        {
        return QSPI_OK;
        }

        sCommand.NbData  = u32CurrSize;

        if (QSPI_WriteEnable() != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return QSPI_ERROR;
        }

        if (HAL_QSPI_Transmit(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return QSPI_ERROR;
        }

        QSPI_Wait_Busy();

        if (QSPI_AutoPollingMemReady() != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        u32CurrAddr += u32CurrSize;
        pData += u32CurrSize;
        u32CurrSize = ((u32CurrAddr + MEMORY_PAGE_SIZE) > u32EndAddr) ? (u32EndAddr - u32CurrAddr) : MEMORY_PAGE_SIZE;
    } while (u32CurrAddr < u32EndAddr);

    return QSPI_OK;
}

/*/**
 * @brief
 *
 */
uint8_t CSP_QSPI_Erase_Block(uint32_t BlockAddress)
{
    QSPI_CommandTypeDef sCommand;

    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
        sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
    }

    sCommand.Instruction       = SECTOR_ERASE_BLOCK_64KB;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.Address           = BlockAddress;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (QSPI_WriteEnable() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    QSPI_Wait_Busy();

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    QSPI_Wait_Busy();

    if (QSPI_AutoPollingMemReady() != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}

/**
  * @brief  None.
  * @retval None.
  */
uint8_t CSP_QSPI_EraseSector(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    QSPI_CommandTypeDef sCommand;

    EraseStartAddress = EraseStartAddress - EraseStartAddress % MEMORY_SECTOR_SIZE;

    /* Erasing Sequence -------------------------------------------------- */
    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
        sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
    }

    sCommand.Instruction        = SECTOR_ERASE_CMD_4BYTE;
    sCommand.AddressSize        = QSPI_ADDRESS_24_BITS;

    sCommand.AlternateBytes     = 0x00;
    sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode           = QSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DdrMode            = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

    while (EraseEndAddress >= EraseStartAddress)
    {
        sCommand.Address = (EraseStartAddress & 0x0FFFFFFF);

        if (QSPI_WriteEnable() != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        QSPI_Wait_Busy();

        if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
                != HAL_OK)
        {
            return QSPI_ERROR;
        }
        EraseStartAddress += MEMORY_SECTOR_SIZE;

        QSPI_Wait_Busy();
    }

    return HAL_OK;
}

/**
  * @brief  None.
  * @retval None.
  */
uint8_t CSP_QSPI_GetStatus(void)
{
    QSPI_CommandTypeDef sCommand;
    uint8_t reg;

    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_STATUS_REG1_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.DummyCycles       = 0;
    sCommand.NbData            = 1;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }
    if (HAL_QSPI_Receive(&hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_ERROR;
    }

    if((reg & MEMORY_FSR_BUSY) != 0)
    {
        return QSPI_BUSY;
    }
    else
    {
        return QSPI_OK;
    }
}

/**
  * @brief  None.
  * @retval None.
  */
uint8_t CSP_QSPI_GetInfo(QSPI_Info* pInfo)
{

    pInfo->FlashSize          = MEMORY_FLASH_SIZE;
    pInfo->EraseSectorSize    = MEMORY_SECTOR_SIZE;
    pInfo->EraseSectorsNumber = (MEMORY_FLASH_SIZE/MEMORY_SECTOR_SIZE);
    pInfo->ProgPageSize       = MEMORY_PAGE_SIZE;
    pInfo->ProgPagesNumber    = (MEMORY_FLASH_SIZE/MEMORY_PAGE_SIZE);

    return QSPI_OK;
}

/**
  * @brief  None.
  * @retval None.
  */
uint32_t QSPI_FLASH_ReadID(void)
{
    QSPI_CommandTypeDef sCommand;
    uint32_t Temp = 0;
    uint8_t pData[3];

    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_JEDEC_ID_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.NbData            = 3;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }
    if (HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }

    Temp = ( pData[2] | pData[1]<<8 )| ( pData[0]<<16 );

    return Temp;
}

/**
  * @brief  None.
  * @retval None.
  */
uint32_t QSPI_FLASH_ReadDeviceID(void)
{
    QSPI_CommandTypeDef sCommand;
    uint32_t Temp = 0;
    uint8_t pData[6];

    if( QSPI_SPI_MODE == nCurrentMode )
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        sCommand.Instruction       = READ_QUAD_ID_CMD;
        sCommand.DummyCycles       = 6;
    }
    else
    {
        sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
        sCommand.Instruction       = READ_ID_CMD;
        sCommand.DummyCycles       = 0;
    }

    sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.Address           = 0x000000;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_4_LINES;
    sCommand.NbData            = 2;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }
    if (HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        while(1)
        {
        }
    }

    Temp = pData[1] |( pData[0]<<8 ) ;

    return Temp;
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
