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
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32h7xx_hal.h"

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
#define R1_Pin GPIO_PIN_2
#define R1_GPIO_Port GPIOE
#define G1_Pin GPIO_PIN_3
#define G1_GPIO_Port GPIOE
#define B1_Pin GPIO_PIN_4
#define B1_GPIO_Port GPIOE
#define R2_Pin GPIO_PIN_5
#define R2_GPIO_Port GPIOE
#define G2_Pin GPIO_PIN_6
#define G2_GPIO_Port GPIOE
#define LA_Pin GPIO_PIN_0
#define LA_GPIO_Port GPIOC
#define LB_Pin GPIO_PIN_1
#define LB_GPIO_Port GPIOC
#define LC_Pin GPIO_PIN_2
#define LC_GPIO_Port GPIOC
#define LD_Pin GPIO_PIN_3
#define LD_GPIO_Port GPIOC
#define LAT_Pin GPIO_PIN_0
#define LAT_GPIO_Port GPIOA
#define OE_Pin GPIO_PIN_1
#define OE_GPIO_Port GPIOA
#define CLK_Pin GPIO_PIN_2
#define CLK_GPIO_Port GPIOA
#define LE_Pin GPIO_PIN_4
#define LE_GPIO_Port GPIOC
#define R5_Pin GPIO_PIN_0
#define R5_GPIO_Port GPIOB
#define G5_Pin GPIO_PIN_1
#define G5_GPIO_Port GPIOB
#define B5_Pin GPIO_PIN_2
#define B5_GPIO_Port GPIOB
#define B2_Pin GPIO_PIN_7
#define B2_GPIO_Port GPIOE
#define R3_Pin GPIO_PIN_8
#define R3_GPIO_Port GPIOE
#define G3_Pin GPIO_PIN_9
#define G3_GPIO_Port GPIOE
#define B3_Pin GPIO_PIN_10
#define B3_GPIO_Port GPIOE
#define R4_Pin GPIO_PIN_11
#define R4_GPIO_Port GPIOE
#define G4_Pin GPIO_PIN_12
#define G4_GPIO_Port GPIOE
#define B4_Pin GPIO_PIN_13
#define B4_GPIO_Port GPIOE
#define G8_Pin GPIO_PIN_10
#define G8_GPIO_Port GPIOB
#define B8_Pin GPIO_PIN_11
#define B8_GPIO_Port GPIOB
#define BT2_Pin GPIO_PIN_6
#define BT2_GPIO_Port GPIOC
#define BT1_Pin GPIO_PIN_7
#define BT1_GPIO_Port GPIOC
#define SDMMC1_CD_Pin GPIO_PIN_8
#define SDMMC1_CD_GPIO_Port GPIOA
#define RTC_INT_Pin GPIO_PIN_10
#define RTC_INT_GPIO_Port GPIOA
#define RTC_INT_EXTI_IRQn EXTI15_10_IRQn
#define R6_Pin GPIO_PIN_3
#define R6_GPIO_Port GPIOB
#define G6_Pin GPIO_PIN_4
#define G6_GPIO_Port GPIOB
#define B6_Pin GPIO_PIN_5
#define B6_GPIO_Port GPIOB
#define R7_Pin GPIO_PIN_6
#define R7_GPIO_Port GPIOB
#define G7_Pin GPIO_PIN_7
#define G7_GPIO_Port GPIOB
#define B7_Pin GPIO_PIN_8
#define B7_GPIO_Port GPIOB
#define R8_Pin GPIO_PIN_9
#define R8_GPIO_Port GPIOB
#define IR1838_Pin GPIO_PIN_1
#define IR1838_GPIO_Port GPIOE
#define IR1838_EXTI_IRQn EXTI1_IRQn
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
