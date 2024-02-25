/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

extern TIM_HandleTypeDef htim3;

extern UART_HandleTypeDef huart6;

extern I2S_HandleTypeDef hi2s2;
extern DMA_HandleTypeDef hdma_spi2_tx;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */


/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LEDN_Pin GPIO_PIN_13
#define LEDN_GPIO_Port GPIOC
#define MF_DECODER_ADC_Pin GPIO_PIN_0
#define MF_DECODER_ADC_GPIO_Port GPIOA
#define DTMF0_Pin GPIO_PIN_3
#define DTMF0_GPIO_Port GPIOA
#define DTMF1_Pin GPIO_PIN_4
#define DTMF1_GPIO_Port GPIOA
#define DTMF2_Pin GPIO_PIN_5
#define DTMF2_GPIO_Port GPIOA
#define DTMF3_Pin GPIO_PIN_6
#define DTMF3_GPIO_Port GPIOA
#define ADC_SAMPLE_FREQ_Pin GPIO_PIN_7
#define ADC_SAMPLE_FREQ_GPIO_Port GPIOA
#define DTMF_STB2_Pin GPIO_PIN_15
#define DTMF_STB2_GPIO_Port GPIOA
#define DTMF_STB1_Pin GPIO_PIN_4
#define DTMF_STB1_GPIO_Port GPIOB
#define INMUX_IN_Pin GPIO_PIN_5
#define INMUX_IN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
