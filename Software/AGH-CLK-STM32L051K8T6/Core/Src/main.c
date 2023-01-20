/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include <math.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
	UG_COLOR s_pri;
	UG_COLOR s_sec;
	UG_COLOR m_pri;
	UG_COLOR m_sec;
	UG_COLOR h_pri;
	UG_COLOR h_sec;
	UG_COLOR t_dig;
	UG_COLOR d_dig;
} COLOR_T;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi1_rx;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
WATCH_HAND_T watchHand[3][60];
RTC_TimeTypeDef time;
RTC_DateTypeDef date;
COLOR_T color;
uint8_t reloadCol;
KEYBOARD_T keyboard;
uint32_t measBat;
uint8_t measRequest;
MODE_T mode;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

//Display
void DrawHand(WATCH_HAND_T* h, uint16_t col);
void SetHandDisplay(WATCH_HAND_T* h, uint8_t pos, uint8_t cnt, uint16_t primCol, uint16_t secCol);
void RefreshClock();

//ADC
void GetBatteryMeas(uint32_t * meas, uint8_t *cond);

//Keyboard
uint8_t KeyPressed(uint8_t* key);
uint8_t KeyHolded(uint32_t* key, uint32_t time);
uint8_t KeysHolded(uint32_t time);

//RTC
void SetTimeDate();

//MISC
void InitHandsCoords();
void ChangeColor(UG_COLOR s_pri, UG_COLOR s_sec, UG_COLOR m_pri, UG_COLOR m_sec, UG_COLOR h_pri, UG_COLOR h_sec, UG_COLOR t_dig, UG_COLOR d_dig);
void SetColor();
void CheckRegularState();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  measRequest = 1;
  LCD_init();
  HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);
  InitHandsCoords();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	  GetBatteryMeas(&measBat, &measRequest);
	  SetColor();
	  RefreshClock();
	  SetTimeDate();
	  CheckRegularState();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV64;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = ENABLE;
  hadc.Init.LowPowerAutoPowerOff = ENABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 41;
  sTime.Seconds = 55;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_TUESDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 10;
  sDate.Year = 23;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, A_BAT_EN_Pin|LCD_BLK_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BAT_CH_Pin SW2_Pin */
  GPIO_InitStruct.Pin = BAT_CH_Pin|SW2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : A_BAT_EN_Pin LCD_BLK_Pin */
  GPIO_InitStruct.Pin = A_BAT_EN_Pin|LCD_BLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(LCD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_RST_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SW1_Pin */
  GPIO_InitStruct.Pin = SW1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SW1_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void DrawHand(WATCH_HAND_T* h, uint16_t col)
{
	UG_DrawLine(h->x1, h->y1, h->x2, h->y2, col);
}

void SetHandDisplay(WATCH_HAND_T* h, uint8_t pos, uint8_t cnt, uint16_t primCol, uint16_t secCol)
{
	for(uint8_t i=60; i>60-cnt; i--)
	{
		uint16_t col = (pos+i) % 5 == 0 ? primCol : secCol;
		DrawHand(&h[(pos+i)%60], LCD_ColorOpacity(col, (60-i)>>1));
	}
}

void GetBatteryMeas(uint32_t * meas, uint8_t* cond)
{
	if (!*cond)
	{
		return;
	}
	HAL_GPIO_WritePin(A_BAT_EN_GPIO_Port, A_BAT_EN_Pin, 1);
	HAL_Delay(1);	//to avoid adc measure during transient state;
	HAL_ADC_Start(&hadc);
	HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
	*meas = HAL_ADC_GetValue(&hadc) * VREF * ADC_V_DIV / ADC_DIV;
	HAL_GPIO_WritePin(A_BAT_EN_GPIO_Port, A_BAT_EN_Pin, 0);
	*cond = 0;
	return;
}

void InitHandsCoords()
{
	uint8_t i = 0, j = 0, z = 0;
	for(z=0; z<3; z++)
	{
		for(j=0; j<60; j++)
		{
			i = ((j % 5) == 0) ? 0 : 10;
			watchHand[z][j].x1 = 120 + (70+z*25)*sin((j%60)*2*M_PI/60);
			watchHand[z][j].y1 = 120 - (70+z*25)*cos((j%60)*2*M_PI/60);
			watchHand[z][j].x2 = 120 + (50+i+z*25)*sin((j%60)*2*M_PI/60);
			watchHand[z][j].y2 = 120 - (50+i+z*25)*cos((j%60)*2*M_PI/60);
		}
	}
}

void ChangeColor(UG_COLOR s_pri, UG_COLOR s_sec, UG_COLOR m_pri, UG_COLOR m_sec, UG_COLOR h_pri, UG_COLOR h_sec, UG_COLOR t_dig, UG_COLOR d_dig)
{
	color.s_pri = s_pri;
	color.s_sec = s_sec;
	color.m_pri = m_pri;
	color.m_sec = m_sec;
	color.h_pri = h_pri;
	color.h_sec = h_sec;
	color.t_dig = t_dig;
	color.d_dig = d_dig;
}

void SetColor()
{
	switch (mode)
	{
									   //sec_pri |sec_sec |min_pri |min_sec |hour_pri|hour_sec|time_dig|date_dig
		case regular:        ChangeColor(C_WHITE, C_BLUE,  C_WHITE, C_WHITE, C_WHITE, C_BLUE,  C_WHITE, C_WHITE); break;
		case reg_bat_low:    ChangeColor(C_WHITE, C_ORANGE,C_WHITE, C_WHITE, C_WHITE, C_ORANGE,C_WHITE, C_WHITE); break;
		case reg_discharged: ChangeColor(C_WHITE, C_RED,   C_WHITE, C_WHITE, C_WHITE, C_RED,   C_WHITE, C_WHITE); break;
		case reg_charging:   ChangeColor(C_WHITE, C_YELLOW,C_WHITE, C_WHITE, C_WHITE, C_YELLOW,C_WHITE, C_WHITE); break;
		case reg_bat_full:   ChangeColor(C_WHITE, C_GREEN, C_WHITE, C_WHITE, C_WHITE, C_GREEN, C_WHITE, C_WHITE); break;
		case set_hour:       ChangeColor(C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_WHITE, C_WHITE, C_WHITE, C_BLACK); break;
		case set_minute:     ChangeColor(C_BLACK, C_BLACK, C_WHITE, C_WHITE, C_BLACK, C_BLACK, C_WHITE, C_BLACK); break;
		case set_second:     ChangeColor(C_WHITE, C_WHITE, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_WHITE, C_BLACK); break;
		case set_day:        ChangeColor(C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_WHITE); break;
		case set_month:      ChangeColor(C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_WHITE); break;
		case set_year:  	 ChangeColor(C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_BLACK, C_WHITE); break;
	}
}

uint8_t KeyPressed(uint8_t* key)
{
	uint8_t t = *key;
	*key = 0;
	return t;
}

uint8_t KeyHolded(uint32_t* key, uint32_t time)
{
	uint8_t t = *key >= time ? 1 : 0;
	if (t) *key = 0;
	return t;
}

uint8_t KeysHolded(uint32_t time)
{
	static uint8_t t;
	t = 0;
	if(keyboard.l_hold >= time && keyboard.r_hold >= time)
	{
		keyboard.l_hold = 0;
		keyboard.r_hold = 0;
		t = 1;
	}
	return t;
}

void RefreshClock()
{
	static struct
	{
		uint8_t sec;
		uint8_t min;
		uint8_t hour;
		uint8_t day;
		uint8_t mth;
		uint8_t year;
	} change;

	static uint8_t hourPos;
	static RTC_TimeTypeDef time_old;
	static RTC_DateTypeDef date_old;
	static MODE_T mode_old;

	static char dateDisplay[9];
	static char timeDisplay[6];

	change.sec  = (time_old.Seconds == time.Seconds) ? 0 : 1;
	change.min  = (time_old.Minutes == time.Minutes) ? 0 : 1;
	change.hour = (time_old.Hours   == time.Hours)   ? 0 : 1;
	change.day  = (date_old.Date    == date.Date)    ? 0 : 1;
	change.mth  = (date_old.Month   == date.Month)   ? 0 : 1;
	change.year = (date_old.Year    == date.Year)    ? 0 : 1;

	if(mode_old != mode)
	{
		change.sec = 1;
		change.min = 1;
		change.hour = 1;
		change.day = 1;
		change.mth = 1;
		change.year = 1;
		mode_old = mode;
	}

	if (!(change.sec || change.min || change.hour || change.day || change.mth || change.year))
	{
		return;
	}

	hourPos = ((time.Hours%12)*5) + (time.Minutes/12);

	//fast reload - only one watch hand
	if (change.sec)
	{
		SetHandDisplay(watchHand[SEC], time.Seconds, 1, color.s_pri, color.s_sec);
	}
	if (change.min)
	{
		SetHandDisplay(watchHand[MIN], time.Minutes, 1, color.m_pri, color.m_sec);
	}
	if (change.hour)
	{
		SetHandDisplay(watchHand[HOUR], hourPos, 1, color.h_pri, color.h_sec);
	}

	if (change.min || change.hour)
	{
		sprintf(timeDisplay, "%02d:%02d", time.Hours, time.Minutes);
		LCD_PutStr(89, 95, timeDisplay, FONT_arial_25X28, LCD_ColorOpacity(color.t_dig, 7), C_BLACK);
	}
	if (change.day || change.mth || change.year)
	{
		sprintf(dateDisplay, "%02d.%02d.%02d", date.Date, date.Month, date.Year);
		LCD_PutStr(89, 130, dateDisplay, FONT_arial_16X18, LCD_ColorOpacity(color.d_dig, 5), C_BLACK);
	}

	//slow reload - rest of watch hands
	if (change.sec)
	{
		SetHandDisplay(watchHand[SEC], time.Seconds, 60, color.s_pri, color.s_sec);
	}
	if (change.min)
	{
		SetHandDisplay(watchHand[MIN], time.Minutes, 60, color.m_pri, color.m_sec);
	}
	if (change.hour)
	{
		SetHandDisplay(watchHand[HOUR], hourPos, 60, color.h_pri, color.h_sec);
	}

	time_old.Seconds = time.Seconds;
	time_old.Minutes = time.Minutes;
	time_old.Hours = time.Hours;
	date_old.Date = date.Date;
	date_old.Month = date.Month;
	date_old.Year = date.Year;

	return;
}

void SetTimeDate()
{
	static uint8_t* curr_p;
	static MODE_T mode_old;
	static uint8_t mod;
	static uint8_t setTimeFlag;
	static enum
	{
		hour,
		min,
		sec,
		day,
		month,
		year
	} num = hour;

	if (KeysHolded(100))
	{
		setTimeFlag = 1;
	}

	if (!setTimeFlag)
	{
		mode_old = mode;
		return;
	}

	switch(num)
	{
		case hour:  curr_p = &time.Hours;   mod = 24; break;
		case min:   curr_p = &time.Minutes; mod = 60; break;
		case sec:   curr_p = &time.Seconds; mod = 60; break;
		case day:   curr_p = &date.Date;    mod = 31; break;
		case month: curr_p = &date.Month;   mod = 12; break;
		case year:  curr_p = &date.Year;	mod = 99; break;
		default: curr_p = NULL;
	}

	if(*curr_p == 0)
	{
		*curr_p = mod;
	}
	*curr_p = (*curr_p + KeyPressed(&keyboard.l_press) - KeyPressed(&keyboard.r_press)) % mod;

	HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);

	if (!KeysHolded(50))
	{
		mode = set_hour + num;
		return;
	}
	UG_DrawLine(89, 150, 151, 150, C_BLACK);
	if (num == year)
	{
		num = 0;
		setTimeFlag = 0;
		mode = mode_old;
		return;
	}
	else
	{
		num++;
		mode = set_hour + num;
		if (mode == set_day)
		{
			UG_DrawLine(89, 150, 107, 150, C_WHITE);
		}
		else if (mode == set_month)
		{
			UG_DrawLine(111, 150, 129, 150, C_WHITE);
		}
		else if (mode == set_year)
		{
			UG_DrawLine(131, 150, 151, 150, C_WHITE);
		}
		return;
	}
}

void CheckRegularState()
{
	static uint8_t isCharging;

	if (mode > reg_bat_full)
		return;
	isCharging = HAL_GPIO_ReadPin(BAT_CH_GPIO_Port, BAT_CH_Pin);

	if(isCharging == BATTERY_IS_CHARGING && measBat >= BATTERY_LIMIT_GREEN)
	{
		mode = reg_bat_full;
	}
	else if(isCharging == BATTERY_IS_CHARGING && measBat <  BATTERY_LIMIT_GREEN)
	{
		mode = reg_charging;
	}
	else if(isCharging != BATTERY_IS_CHARGING && measBat <  BATTERY_LIMIT_RED)
	{
		mode = reg_discharged;
	}
	else if(isCharging != BATTERY_IS_CHARGING && measBat <  BATTERY_LIMIT_ORANGE)
	{
		mode = reg_bat_low;
	}
	else
	{
		mode = regular;
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
