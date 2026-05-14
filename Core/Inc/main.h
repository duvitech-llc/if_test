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
#include "stm32f4xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BTN_IN_Pin GPIO_PIN_2
#define BTN_IN_GPIO_Port GPIOE
#define LED_EN1_Pin GPIO_PIN_1
#define LED_EN1_GPIO_Port GPIOF
#define LED_EN2_Pin GPIO_PIN_2
#define LED_EN2_GPIO_Port GPIOF
#define LED_EN3_Pin GPIO_PIN_3
#define LED_EN3_GPIO_Port GPIOF
#define EN_DRAWER_Pin GPIO_PIN_4
#define EN_DRAWER_GPIO_Port GPIOF
#define DIR_DRAWER_Pin GPIO_PIN_5
#define DIR_DRAWER_GPIO_Port GPIOF
#define STEP_DRAWER_Pin GPIO_PIN_6
#define STEP_DRAWER_GPIO_Port GPIOF
#define EN_Y_Pin GPIO_PIN_7
#define EN_Y_GPIO_Port GPIOF
#define STEP_Y_Pin GPIO_PIN_8
#define STEP_Y_GPIO_Port GPIOF
#define DIR_Y_Pin GPIO_PIN_9
#define DIR_Y_GPIO_Port GPIOF
#define HEAT_EN_Pin GPIO_PIN_0
#define HEAT_EN_GPIO_Port GPIOC
#define HEAT_SDO_Pin GPIO_PIN_2
#define HEAT_SDO_GPIO_Port GPIOC
#define ENCODER1_A_Pin GPIO_PIN_0
#define ENCODER1_A_GPIO_Port GPIOA
#define ENCODER1_B_Pin GPIO_PIN_1
#define ENCODER1_B_GPIO_Port GPIOA
#define ENCODER2_A_Pin GPIO_PIN_6
#define ENCODER2_A_GPIO_Port GPIOA
#define ENCODER2_B_Pin GPIO_PIN_7
#define ENCODER2_B_GPIO_Port GPIOA
#define PD_CS_Pin GPIO_PIN_12
#define PD_CS_GPIO_Port GPIOF
#define LD_CS_Pin GPIO_PIN_14
#define LD_CS_GPIO_Port GPIOF
#define RESET_Pin GPIO_PIN_15
#define RESET_GPIO_Port GPIOF
#define GPIO_0_Pin GPIO_PIN_0
#define GPIO_0_GPIO_Port GPIOG
#define GPIO_1_Pin GPIO_PIN_1
#define GPIO_1_GPIO_Port GPIOG
#define FAN_PWM0_Pin GPIO_PIN_9
#define FAN_PWM0_GPIO_Port GPIOE
#define FAN_PWM1_Pin GPIO_PIN_11
#define FAN_PWM1_GPIO_Port GPIOE
#define FAN_PWM2_Pin GPIO_PIN_13
#define FAN_PWM2_GPIO_Port GPIOE
#define FAN_PWM3_Pin GPIO_PIN_14
#define FAN_PWM3_GPIO_Port GPIOE
#define TX_Pin GPIO_PIN_10
#define TX_GPIO_Port GPIOB
#define RX_Pin GPIO_PIN_11
#define RX_GPIO_Port GPIOB
#define HEAT_CNVST_Pin GPIO_PIN_12
#define HEAT_CNVST_GPIO_Port GPIOB
#define EN_X_Pin GPIO_PIN_13
#define EN_X_GPIO_Port GPIOB
#define STEP_X_Pin GPIO_PIN_14
#define STEP_X_GPIO_Port GPIOB
#define DIR_X_Pin GPIO_PIN_15
#define DIR_X_GPIO_Port GPIOB
#define FAULT_DRAWER_Pin GPIO_PIN_8
#define FAULT_DRAWER_GPIO_Port GPIOD
#define FAULT_DRAWER_EXTI_IRQn EXTI9_5_IRQn
#define FAULT_HEATPUMP_Pin GPIO_PIN_9
#define FAULT_HEATPUMP_GPIO_Port GPIOD
#define FAULT_HEATPUMP_EXTI_IRQn EXTI9_5_IRQn
#define FAULT_X_Pin GPIO_PIN_10
#define FAULT_X_GPIO_Port GPIOD
#define FAULT_X_EXTI_IRQn EXTI15_10_IRQn
#define FAULT_Y_Pin GPIO_PIN_11
#define FAULT_Y_GPIO_Port GPIOD
#define FAULT_Y_EXTI_IRQn EXTI15_10_IRQn
#define FAN_SENSE0_Pin GPIO_PIN_12
#define FAN_SENSE0_GPIO_Port GPIOD
#define FAN_SENSE1_Pin GPIO_PIN_13
#define FAN_SENSE1_GPIO_Port GPIOD
#define FAN_SENSE2_Pin GPIO_PIN_14
#define FAN_SENSE2_GPIO_Port GPIOD
#define FAN_SENSE3_Pin GPIO_PIN_15
#define FAN_SENSE3_GPIO_Port GPIOD
#define GPIO_2_Pin GPIO_PIN_2
#define GPIO_2_GPIO_Port GPIOG
#define GPIO_3_Pin GPIO_PIN_3
#define GPIO_3_GPIO_Port GPIOG
#define GPIO_4_Pin GPIO_PIN_4
#define GPIO_4_GPIO_Port GPIOG
#define GPIO_5_Pin GPIO_PIN_5
#define GPIO_5_GPIO_Port GPIOG
#define GPIO_6_Pin GPIO_PIN_6
#define GPIO_6_GPIO_Port GPIOG
#define GPIO_7_Pin GPIO_PIN_7
#define GPIO_7_GPIO_Port GPIOG
#define LED_0_Pin GPIO_PIN_8
#define LED_0_GPIO_Port GPIOG
#define HEAT_CLK_Pin GPIO_PIN_7
#define HEAT_CLK_GPIO_Port GPIOC
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define OTG_FS_DP_Pin GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port GPIOA
#define LOAD_CELL_CS_Pin GPIO_PIN_15
#define LOAD_CELL_CS_GPIO_Port GPIOA
#define UART4_TX_Pin GPIO_PIN_10
#define UART4_TX_GPIO_Port GPIOC
#define UART4_RX_Pin GPIO_PIN_11
#define UART4_RX_GPIO_Port GPIOC
#define LIMIT0_Pin GPIO_PIN_0
#define LIMIT0_GPIO_Port GPIOD
#define LIMIT0_EXTI_IRQn EXTI0_IRQn
#define LIMIT1_Pin GPIO_PIN_1
#define LIMIT1_GPIO_Port GPIOD
#define LIMIT1_EXTI_IRQn EXTI1_IRQn
#define LIMIT2_Pin GPIO_PIN_2
#define LIMIT2_GPIO_Port GPIOD
#define LIMIT2_EXTI_IRQn EXTI2_IRQn
#define LIMIT3_Pin GPIO_PIN_3
#define LIMIT3_GPIO_Port GPIOD
#define LIMIT3_EXTI_IRQn EXTI3_IRQn
#define LIMIT4_Pin GPIO_PIN_4
#define LIMIT4_GPIO_Port GPIOD
#define LIMIT4_EXTI_IRQn EXTI4_IRQn
#define LIMIT5_Pin GPIO_PIN_5
#define LIMIT5_GPIO_Port GPIOD
#define LIMIT5_EXTI_IRQn EXTI9_5_IRQn
#define LIMIT6_Pin GPIO_PIN_6
#define LIMIT6_GPIO_Port GPIOD
#define LIMIT6_EXTI_IRQn EXTI9_5_IRQn
#define LIMIT7_Pin GPIO_PIN_7
#define LIMIT7_GPIO_Port GPIOD
#define LIMIT7_EXTI_IRQn EXTI9_5_IRQn
#define LED_1_Pin GPIO_PIN_9
#define LED_1_GPIO_Port GPIOG
#define LDPD_SCLK_Pin GPIO_PIN_11
#define LDPD_SCLK_GPIO_Port GPIOG
#define LDPD_MISO_Pin GPIO_PIN_12
#define LDPD_MISO_GPIO_Port GPIOG
#define LDPD_MOSI_Pin GPIO_PIN_13
#define LDPD_MOSI_GPIO_Port GPIOG
#define LOAD_CELL_CLK_Pin GPIO_PIN_3
#define LOAD_CELL_CLK_GPIO_Port GPIOB
#define LOAD_CELL_MISO_Pin GPIO_PIN_4
#define LOAD_CELL_MISO_GPIO_Port GPIOB
#define LOAD_CELL_MOSI_Pin GPIO_PIN_5
#define LOAD_CELL_MOSI_GPIO_Port GPIOB
#define EN_HEATPUMP_Pin GPIO_PIN_7
#define EN_HEATPUMP_GPIO_Port GPIOB
#define DIR_HEATPUMP_Pin GPIO_PIN_8
#define DIR_HEATPUMP_GPIO_Port GPIOB
#define STEP_HEATPUMP_Pin GPIO_PIN_9
#define STEP_HEATPUMP_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
extern CRC_HandleTypeDef hcrc;
extern TIM_HandleTypeDef htim7;
extern UART_HandleTypeDef huart3;

#define DEBUG_UART huart3
#define UTIL_TIMER htim7

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
