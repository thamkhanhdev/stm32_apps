/**
  ******************************************************************************
  * File Name          : QUADSPI.h
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __quadspi_H
#define __quadspi_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#define F_25Q128JVSIQ

/* USER CODE END Includes */

extern QSPI_HandleTypeDef hqspi;

/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

void MX_QUADSPI_Init(void);

/* USER CODE BEGIN Prototypes */

/* QSPI Status codes */
typedef enum
{
    QSPI_OK       = 0x00,
    QSPI_ERROR    = 0x01,
    QSPI_BUSY     = 0x02,
    QSPI_TIMEOUT  = 0x03,

    QSPI_QPI_MODE = 0x04,
    QSPI_SPI_MODE = 0x05,

    QSPI_DTR_MODE = 0x06,
    QSPI_NORMAL_MODE = 0x07,

    QSPI_SUSPENDED = 0x08
} QSPI_StatusTypeDef;

/* QSPI Info */
 typedef struct {
   uint32_t FlashSize;
   uint32_t EraseSectorSize;
   uint32_t EraseSectorsNumber;
   uint32_t ProgPageSize;
   uint32_t ProgPagesNumber;
 } QSPI_Info;

/* Private define ------------------------------------------------------------*/

/**
  * @brief  W25Q128JV
  */
#define MEMORY_FLASH_SIZE                  0x1000000 /* 128 MBits => 16MBytes */
#define MEMORY_BLOCK_SIZE                  0x10000   /* 128 blocks of 64KBytes */
#define MEMORY_SECTOR_SIZE                 0x1000    /* 2048 sectors of 4kBytes */
#define MEMORY_PAGE_SIZE                   0x100     /* 32768 pages of 256 bytes */

#define MEMORY_DUMMY_CYCLES_READ           4
#define MEMORY_DUMMY_CYCLES_READ_QUAD      10

#define MEMORY_BULK_ERASE_MAX_TIME         250000
#define MEMORY_SECTOR_ERASE_MAX_TIME       3000
#define MEMORY_SUBSECTOR_ERASE_MAX_TIME    800

/* Write Enable (06h) */
#define WRITE_ENABLE_CMD                     0x06
/* Write Disable (04h) */
#define WRITE_DISABLE_CMD                    0x04
/* Read
 * Status Register-1 (05h)
 * Status Register-2 (35h)
 * Status Register-3 (15h)
 */
#define READ_STATUS_REG1_CMD                  0x05
#define READ_STATUS_REG2_CMD                  0x35
#define READ_STATUS_REG3_CMD                  0x15
/* Write
 * Status Register-1 (01h)
 * Status Register-2 (31h)
 * Status Register-3 (11h)
 */
#define WRITE_STATUS_REG1_CMD                 0x01
#define WRITE_STATUS_REG2_CMD                 0x31
#define WRITE_STATUS_REG3_CMD                 0x11
/* Read Data (03h) */
#define READ_CMD_4BYTE                        0x03
/* Fast Read Quad Output (6Bh) */
#define QUAD_OUT_FAST_READ_CMD                0x6B
/* Fast Read Quad I/O (EBh) */
#define QUAD_INOUT_FAST_READ_CMD_4BYTE        0xEB
/* Input Page Program (2h) */
#define INPUT_PAGE_PROG_CMD_4BYTE             0x02
/* Quad Input Page Program (32h) */
#define QUAD_INPUT_PAGE_PROG_CMD_4BYTE        0x32
/* Sector Erase (20h) */
#define SECTOR_ERASE_CMD_4BYTE                0x20
/* 64KB Block Erase (D8h) */
#define SECTOR_ERASE_BLOCK_64KB               0xD8
/* Chip Erase (C7h / 60h) */
#define CHIP_ERASE_CMD                        0xC7
/* Erase / Program Resume (7Ah) */
#define PROG_ERASE_RESUME_CMD                 0x7A
/* Erase / Program Suspend (75h) */
#define PROG_ERASE_SUSPEND_CMD                0x75
/* Read Manufacturer / Device ID (90h) */
#define READ_ID_CMD                          0x90
/* Read Manufacturer / Device ID (94h) */
#define READ_QUAD_ID_CMD                     0x94
/* Read Manufacturer / Device ID Dual I/O (92h) */
#define DUAL_READ_ID_CMD                     0x92
/* Read Manufacturer / Device ID Quad I/O (94h) */
#define QUAD_READ_ID_CMD                     0x94
/* Read JEDEC ID (9Fh) */
#define READ_JEDEC_ID_CMD                    0x9F
/* Enable Reset (66h) */
#define RESET_ENABLE_CMD                     0x66
/* Reset Device (99h) */
#define RESET_MEMORY_CMD                     0x99

#define MEMORY_FSR_BUSY                    ((uint8_t)0x01)    /*!< busy */
#define MEMORY_FSR_WREN                    ((uint8_t)0x02)    /*!< write enable */
#define MEMORY_FSR_QE                      ((uint8_t)0x02)    /*!< quad enable */

/* Dummy cycles for DTR read mode */
#define DUMMY_CYCLES_READ_QUAD_DTR  4U
#define DUMMY_CYCLES_READ_QUAD      6U

/**
  * @brief  External command for W25Q128JV
  */
#define ENTER_QPI_MODE_CMD                   0x38
#define SET_READ_PARAM_CMD                   0xC0
#define QUAD_INOUT_FAST_READ_DTR_CMD         0xED

uint8_t CSP_QUADSPI_Init(void);
uint8_t CSP_QSPI_EraseSector(uint32_t EraseStartAddress ,uint32_t EraseEndAddress);
uint8_t CSP_QSPI_WriteMemory(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
uint8_t CSP_QSPI_EnableMemoryMappedMode(void);
uint8_t CSP_QSPI_Erase_Chip (void);
uint8_t QSPI_AutoPollingMemReady(void);
uint32_t QSPI_FLASH_ReadDeviceID(void);
uint32_t QSPI_FLASH_ReadID(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ quadspi_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
