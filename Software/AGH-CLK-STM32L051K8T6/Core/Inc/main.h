/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef enum
{
	HOUR,
	MIN,
	SEC,
} HAND_T;

typedef struct
{
	HAND_T t;
	uint8_t x1;
	uint8_t x2;
	uint8_t y1;
	uint8_t y2;
} WATCH_HAND_T;

typedef struct
{
	uint8_t r_press;
	uint32_t r_hold;
	uint8_t l_press;
	uint32_t l_hold;
} KEYBOARD_T;


typedef enum
{
	regular,
	reg_bat_low,
	reg_discharged,
	reg_charging,
	reg_bat_full,
	set_hour,
	set_minute,
	set_second,
	set_day,
	set_month,
	set_year
} MODE_T;


/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern KEYBOARD_T keyboard;
extern uint8_t measRequest;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define A_BAT_Pin GPIO_PIN_0
#define A_BAT_GPIO_Port GPIOA
#define BAT_CH_Pin GPIO_PIN_1
#define BAT_CH_GPIO_Port GPIOA
#define A_BAT_EN_Pin GPIO_PIN_2
#define A_BAT_EN_GPIO_Port GPIOA
#define LCD_DC_Pin GPIO_PIN_4
#define LCD_DC_GPIO_Port GPIOA
#define LCD_SCK_Pin GPIO_PIN_5
#define LCD_SCK_GPIO_Port GPIOA
#define LCD_BLK_Pin GPIO_PIN_6
#define LCD_BLK_GPIO_Port GPIOA
#define LCD_SDA_Pin GPIO_PIN_7
#define LCD_SDA_GPIO_Port GPIOA
#define LCD_CS_Pin GPIO_PIN_0
#define LCD_CS_GPIO_Port GPIOB
#define LCD_RST_Pin GPIO_PIN_1
#define LCD_RST_GPIO_Port GPIOB
#define SW2_Pin GPIO_PIN_15
#define SW2_GPIO_Port GPIOA
#define SW1_Pin GPIO_PIN_3
#define SW1_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define VREF					3300	// mV
#define ADC_DIV					4096	// resolution
#define ADC_V_DIV				2		// R/R
#define BATTERY_MEAS_DELAY		10000	// ms
#define BATTERY_LIMIT_GREEN		4000	// mV
#define BATTERY_LIMIT_ORANGE	3750	// mV
#define BATTERY_LIMIT_RED		3700	// mV
#define BATTERY_IS_CHARGING		0
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
