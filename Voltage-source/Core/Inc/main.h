/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f3xx_hal.h"

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
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOF
#define ENC_A_Pin GPIO_PIN_0
#define ENC_A_GPIO_Port GPIOA
#define ENC_B_Pin GPIO_PIN_1
#define ENC_B_GPIO_Port GPIOA
#define VCP_TX_Pin GPIO_PIN_2
#define VCP_TX_GPIO_Port GPIOA
#define ENC_BTN_Pin GPIO_PIN_3
#define ENC_BTN_GPIO_Port GPIOA
#define CH1_OUT_Pin GPIO_PIN_4
#define CH1_OUT_GPIO_Port GPIOA
#define CH2_OUT_Pin GPIO_PIN_5
#define CH2_OUT_GPIO_Port GPIOA
#define CH1_MEAS_V_Pin GPIO_PIN_6
#define CH1_MEAS_V_GPIO_Port GPIOA
#define CH2_MEAS_V_Pin GPIO_PIN_7
#define CH2_MEAS_V_GPIO_Port GPIOA
#define CH1_MEAS_A_Pin GPIO_PIN_0
#define CH1_MEAS_A_GPIO_Port GPIOB
#define CH2_MEAS_A_Pin GPIO_PIN_1
#define CH2_MEAS_A_GPIO_Port GPIOB
#define MODE_BTN_Pin GPIO_PIN_8
#define MODE_BTN_GPIO_Port GPIOA
#define CH_SEL_BTN_Pin GPIO_PIN_9
#define CH_SEL_BTN_GPIO_Port GPIOA
#define CH1_EN_BTN_Pin GPIO_PIN_10
#define CH1_EN_BTN_GPIO_Port GPIOA
#define CH2_EN_BTN_Pin GPIO_PIN_11
#define CH2_EN_BTN_GPIO_Port GPIOA
#define CH2_LED_Pin GPIO_PIN_12
#define CH2_LED_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_15
#define VCP_RX_GPIO_Port GPIOA
#define CH1_LED_Pin GPIO_PIN_3
#define CH1_LED_GPIO_Port GPIOB
#define CH1_REL_Pin GPIO_PIN_4
#define CH1_REL_GPIO_Port GPIOB
#define CH2_REL_Pin GPIO_PIN_5
#define CH2_REL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
