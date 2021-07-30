/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_crs.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_gpio.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LA_Pin LL_GPIO_PIN_0
#define LA_GPIO_Port GPIOC
#define LB_Pin LL_GPIO_PIN_1
#define LB_GPIO_Port GPIOC
#define LC_Pin LL_GPIO_PIN_2
#define LC_GPIO_Port GPIOC
#define LD_Pin LL_GPIO_PIN_3
#define LD_GPIO_Port GPIOC
#define LAT_Pin LL_GPIO_PIN_0
#define LAT_GPIO_Port GPIOA
#define OE_Pin LL_GPIO_PIN_1
#define OE_GPIO_Port GPIOA
#define CLK_Pin LL_GPIO_PIN_2
#define CLK_GPIO_Port GPIOA
#define LE_Pin LL_GPIO_PIN_4
#define LE_GPIO_Port GPIOC
#define B4_Pin LL_GPIO_PIN_0
#define B4_GPIO_Port GPIOB
#define R4_Pin LL_GPIO_PIN_1
#define R4_GPIO_Port GPIOB
#define G4_Pin LL_GPIO_PIN_2
#define G4_GPIO_Port GPIOB
#define B2_Pin LL_GPIO_PIN_10
#define B2_GPIO_Port GPIOB
#define R2_Pin LL_GPIO_PIN_11
#define R2_GPIO_Port GPIOB
#define R3_Pin LL_GPIO_PIN_3
#define R3_GPIO_Port GPIOB
#define G3_Pin LL_GPIO_PIN_4
#define G3_GPIO_Port GPIOB
#define B3_Pin LL_GPIO_PIN_5
#define B3_GPIO_Port GPIOB
#define R1_Pin LL_GPIO_PIN_6
#define R1_GPIO_Port GPIOB
#define G1_Pin LL_GPIO_PIN_7
#define G1_GPIO_Port GPIOB
#define B1_Pin LL_GPIO_PIN_8
#define B1_GPIO_Port GPIOB
#define G2_Pin LL_GPIO_PIN_9
#define G2_GPIO_Port GPIOB
#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority,
                                                                 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority,
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority,
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority,
                                                                 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority,
                                                                 0 bit  for subpriority */
#endif
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
