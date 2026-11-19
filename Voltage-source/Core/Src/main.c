/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h> // pro funkci sprintf
#include <string.h>
#include "power_control.h"
#include "liquidcrystal_i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

DAC_HandleTypeDef hdac1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t selected_channel = 1; // 1 = Na displeji a enkodéru ovládáme CH1, 2 = CH2

// Hodnoty kroků napětí v milivoltech (10 mV, 100 mV, 1 V)
int32_t step_sizes_mv[] = {10, 100, 1000};
uint8_t current_step_idx = 1; // Výchozí krok 100 mV
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_DAC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Proměnné pro náš časový plánovač
uint32_t last_1ms_tick = 0;
uint32_t last_20ms_tick = 0;
uint32_t last_100ms_tick = 0;

// Prototypy našich Tasků (napíšeme je níže)
void Task_ControlLoop_1ms(void);
void Task_Inputs_20ms(void);
void Task_UI_100ms(void);


// Přesměrování standardního výstupu printf do USART2 (USB k PC)
int _write(int file, char *ptr, int len)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	setvbuf(stdout, NULL, _IONBF, 0); // Vypne bufferování pro printf
	printf("\r\n--- PROCESOR SE PROBUDIL ---\r\n");
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
  MX_USART2_UART_Init();
  MX_DAC1_Init();
  MX_TIM2_Init();
  MX_ADC2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
	printf("\r\n\r\n--- SYSTEM START ---\r\n");

	printf("Cekam na probuzeni I2C periferii...\r\n");
	HAL_Delay(200); // 200 ms pauza pro hardware!

	// PŘIDÁNO: Hardwarová kalibrace ADC před prvním měřením!
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

	PowerSystem_Init();

	printf("1. Spoustim DAC...\r\n");
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);

	printf("2. Spoustim TIM2 (Enkoder)...\r\n");
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

	printf("VSE OK, vstupuji do while(1) smycky!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		uint32_t current_tick = HAL_GetTick(); // Aktuální čas v milisekundách

		// ==========================================
		// TASK 1: Regulační smyčka (Běží přesně 1000x za vteřinu)
		// ==========================================
		if ((current_tick - last_1ms_tick) >= 1) {
				last_1ms_tick = current_tick;
				Task_ControlLoop_1ms();
		}

		// ==========================================
		// TASK 2: Čtení tlačítek a Enkodéru (Běží 50x za vteřinu)
		// ==========================================
		if ((current_tick - last_20ms_tick) >= 20) {
				last_20ms_tick = current_tick;
				Task_Inputs_20ms();
		}

		// ==========================================
		// TASK 3: Vykreslení Displeje (Běží 10x za vteřinu)
		// ==========================================
		if ((current_tick - last_100ms_tick) >= 100) {
				last_100ms_tick = current_tick;
				Task_UI_100ms();
		}
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC12;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT2 config
  */
  sConfig.DAC_OutputSwitch = DAC_OUTPUTSWITCH_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4095;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CH2_LED_GPIO_Port, CH2_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CH1_LED_Pin|CH1_REL_Pin|CH2_REL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : ENC_BTN_Pin MODE_BTN_Pin CH_SEL_BTN_Pin CH1_EN_BTN_Pin
                           CH2_EN_BTN_Pin */
  GPIO_InitStruct.Pin = ENC_BTN_Pin|MODE_BTN_Pin|CH_SEL_BTN_Pin|CH1_EN_BTN_Pin
                          |CH2_EN_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : CH2_LED_Pin */
  GPIO_InitStruct.Pin = CH2_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CH2_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CH1_LED_Pin CH1_REL_Pin CH2_REL_Pin */
  GPIO_InitStruct.Pin = CH1_LED_Pin|CH1_REL_Pin|CH2_REL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// 1. Pomocná funkce pro rychlé přečtení konkrétního ADC kanálu
uint16_t Read_ADC_Channel(ADC_HandleTypeDef *hadc, uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;

    // Zde vynucujeme Single-Ended režim navzdory výchozímu nastavení!
    sConfig.SingleDiff = ADC_SINGLE_ENDED;

    // Zvolíme delší čas vzorkování (61.5 cyklů) pro nabití vnitřního kondenzátoru
    // To výrazně zlepší přesnost a omezí šum na děličích s velkými odpory (desítky kOhm).
    sConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;

    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    // Aplikujeme konfiguraci
    HAL_ADC_ConfigChannel(hadc, &sConfig);

    // Spustíme převod, počkáme na výsledek a převodník zase zastavíme
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 5); // Max 5 ms timeout
    uint16_t result = HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);

    return result;
}

// 2. HLAVNÍ REGULAČNÍ SMYČKA (Volá se striktně každou 1 ms)
void Task_ControlLoop_1ms(void)
{
    // ==========================================
    // KAN�?L 1
    // ==========================================
    // A) Čtení hardwaru (CH1_MEAS_V = ADC2_IN3, CH1_MEAS_A = ADC1_IN11)
    uint16_t adc_v1 = Read_ADC_Channel(&hadc2, ADC_CHANNEL_3);
    uint16_t adc_a1 = Read_ADC_Channel(&hadc1, ADC_CHANNEL_11);

    // B) Zpracování logiky a regulace
    PowerControl_ProcessADC(&ch1, adc_v1, adc_a1);
    PowerControl_UpdateSOALimit(&ch1);
    PowerControl_ProcessControlLoop(&ch1);

    // C) Zápis vypočteného výsledku do hardwaru
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, ch1.current_dac_val);


    // ==========================================
    // KAN�?L 2
    // ==========================================
    // A) Čtení hardwaru (CH2_MEAS_V = ADC2_IN4, CH2_MEAS_A = ADC1_IN12)
    uint16_t adc_v2 = Read_ADC_Channel(&hadc2, ADC_CHANNEL_4);
    uint16_t adc_a2 = Read_ADC_Channel(&hadc1, ADC_CHANNEL_12);

    // B) Zpracování logiky a regulace
    PowerControl_ProcessADC(&ch2, adc_v2, adc_a2);
    PowerControl_UpdateSOALimit(&ch2);
    PowerControl_ProcessControlLoop(&ch2);

    // C) Zápis vypočteného výsledku do hardwaru
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, ch2.current_dac_val);
}

void Task_Inputs_20ms(void)
{
    // ==========================================
    // 1. OBSLUHA ENKODÉRU (Čtení z HW časovače TIM2)
    // ==========================================
    static uint16_t last_timer_val = 0;
    static int16_t tick_accumulator = 0;

    uint16_t timer_val = __HAL_TIM_GET_COUNTER(&htim2);
    int16_t delta_ticks = (int16_t)timer_val - (int16_t)last_timer_val;

    // Korekce pro přetočení čítače čipu (0 <-> 4095)
    if (delta_ticks > 2047) delta_ticks -= 4096;
    if (delta_ticks < -2048) delta_ticks += 4096;

    last_timer_val = timer_val;
    tick_accumulator += delta_ticks;

    // Pokud došlo k mechanickému "cvaknutí" (4 tiky)
    if (tick_accumulator >= 4 || tick_accumulator <= -4)
    {
        int16_t clicks = tick_accumulator / 4;
        tick_accumulator %= 4;

        int32_t voltage_change = clicks * step_sizes_mv[current_step_idx];

        // Zjistíme, který kanál aktuálně uživatel ovládá
        PowerChannel_t *active_ch = (selected_channel == 1) ? &ch1 : &ch2;

        active_ch->target_voltage_mv += voltage_change;

        // DYNAMICKÝ LIMIT: 3.3V pro čistý DAC, 15V pro výkonový zdroj
				int32_t max_allowed_mv = (active_ch->mode == MODE_DAC_DIRECT) ? 3300 : 16000;

				// Limity nastavitelného rozsahu
				if (active_ch->target_voltage_mv > max_allowed_mv) {
						active_ch->target_voltage_mv = max_allowed_mv;
				}
				if (active_ch->target_voltage_mv < 0) {
						active_ch->target_voltage_mv = 0;
				}
    }

    // ==========================================
    // 2. OBSLUHA 5 TLAČÍTEK (Detekce sestupné hrany)
    // ==========================================
    // Statické proměnné si pamatují stav z minulého cyklu (před 20 ms)
    // Inicializujeme na 1 (GPIO_PULLUP = v klidu je na pinu hodnota 1)
    static uint8_t prev_btn_enc = 1, prev_btn_mode = 1, prev_btn_sel = 1;
    static uint8_t prev_btn_ch1 = 1, prev_btn_ch2 = 1;

    uint8_t curr_btn_enc = HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port, ENC_BTN_Pin);
    uint8_t curr_btn_mode = HAL_GPIO_ReadPin(MODE_BTN_GPIO_Port, MODE_BTN_Pin);
    uint8_t curr_btn_sel = HAL_GPIO_ReadPin(CH_SEL_BTN_GPIO_Port, CH_SEL_BTN_Pin);
    uint8_t curr_btn_ch1 = HAL_GPIO_ReadPin(CH1_EN_BTN_GPIO_Port, CH1_EN_BTN_Pin);
    uint8_t curr_btn_ch2 = HAL_GPIO_ReadPin(CH2_EN_BTN_GPIO_Port, CH2_EN_BTN_Pin);

    // Pointer na aktivní kanál pro snadnější nastavování módů
    PowerChannel_t *active_ch = (selected_channel == 1) ? &ch1 : &ch2;

    // A) Změna kroku (Tlačítko enkodéru) - Reakce POUZE pokud teď je 0 a minule byla 1
    if (curr_btn_enc == 0 && prev_btn_enc == 1) {
        current_step_idx++;
        if (current_step_idx > 2) current_step_idx = 0;
    }

    // B) Přepínání UI mezi CH1 a CH2 (CH_SEL_BTN)
    if (curr_btn_sel == 0 && prev_btn_sel == 1) {
        selected_channel = (selected_channel == 1) ? 2 : 1;
    }

    // C) Změna režimu výstupu (DAC / LM317)
		if (curr_btn_mode == 0 && prev_btn_mode == 1) {
				// Bezpečnost: Vypneme fyzicky kanál, než změníme způsob řízení!
				active_ch->enabled = 0;

				if (active_ch->mode == MODE_POWER_LM317) {
						active_ch->mode = MODE_DAC_DIRECT;

						// BEZPEČNOSTNÍ O�?EZ: Pokud přepínáme do DAC a napětí bylo > 3.3V, srazíme ho
						if (active_ch->target_voltage_mv > 3300) {
								active_ch->target_voltage_mv = 3300;
						}
				} else {
						active_ch->mode = MODE_POWER_LM317;
				}
		}

    // D) Zapnutí/Vypnutí Kanálu 1
    if (curr_btn_ch1 == 0 && prev_btn_ch1 == 1) {
        ch1.enabled = !ch1.enabled;
    }

    // E) Zapnutí/Vypnutí Kanálu 2
    if (curr_btn_ch2 == 0 && prev_btn_ch2 == 1) {
        ch2.enabled = !ch2.enabled;
    }

    // Uložení aktuálních stavů do historie pro další průběh za 20ms
    prev_btn_enc = curr_btn_enc;
    prev_btn_mode = curr_btn_mode;
    prev_btn_sel = curr_btn_sel;
    prev_btn_ch1 = curr_btn_ch1;
    prev_btn_ch2 = curr_btn_ch2;

    // ==========================================
		// 3. Vizuální indikace (Nezávislé LED pro CH1 a CH2)
		// ==========================================
		// LED Kanálu 1 (Pin PB3)
		if (ch1.enabled) {
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
		} else {
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
		}

		// LED Kanálu 2 (Pin PA12)
		if (ch2.enabled) {
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
		} else {
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
		}

    // ==========================================
		// 4. Fyzické spínání výkonových relé
		// ==========================================
		// Relé Kanálu 1 (Pin PB4)
		if (ch1.enabled) {
				HAL_GPIO_WritePin(GPIOB, CH1_REL_Pin, GPIO_PIN_SET);
		} else {
				HAL_GPIO_WritePin(GPIOB, CH1_REL_Pin, GPIO_PIN_RESET);
		}

		// Relé Kanálu 2 (Pin PB5)
		if (ch2.enabled) {
				HAL_GPIO_WritePin(GPIOB, CH2_REL_Pin, GPIO_PIN_SET);
		} else {
				HAL_GPIO_WritePin(GPIOB, CH2_REL_Pin, GPIO_PIN_RESET);
		}
}

void Task_UI_100ms(void)
{
    char buf[25];
    char step_str[5];
    char state_str[4];

    // ==========================================
        // VYKRESLENÍ KAN�?LU 1
        // ==========================================
        // Připravíme si správný znak pro okraj (hexadecimální hodnota 0x01 zavolá náš custom znak)
        char sel1 = (selected_channel == 1) ? '\x01' : ' ';

        strcpy(step_str, "    ");
        if (selected_channel == 1) {
            if (step_sizes_mv[current_step_idx] == 10) strcpy(step_str, " 10m");
            else if (step_sizes_mv[current_step_idx] == 100) strcpy(step_str, "100m");
            else if (step_sizes_mv[current_step_idx] == 1000) strcpy(step_str, "  1V");
        }

        // �?ádek 0 (Hlavička)
        sprintf(buf, "%cCH1 %s %2lu.%02luV %s",
                sel1,
                (ch1.mode == MODE_POWER_LM317) ? "PWR" : "CAL",
                ch1.target_voltage_mv / 1000, (ch1.target_voltage_mv % 1000) / 10,
                step_str);
        printf("L0:%-20s\r\n", buf);

        if (!ch1.enabled) strcpy(state_str, "OFF");
        else if (ch1.is_limited) strcpy(state_str, "LIM");
        else strcpy(state_str, "ON ");

        // �?ádek 1 (Hodnoty - ZDE TAKÉ P�?ID�?N ZNAK sel1)
        if (ch1.mode == MODE_DAC_DIRECT) {
            uint32_t dac_v1 = (ch1.current_dac_val * ch1.calib.adc_v_ref_mv) / 4095;
            sprintf(buf, "%c%s %2lu.%02luV   ---- ", sel1, state_str, dac_v1 / 1000, (dac_v1 % 1000) / 10);
        } else {
            sprintf(buf, "%c%s %2lu.%02luV   %lu.%02luA ", sel1, state_str, ch1.ui_voltage_mv / 1000, (ch1.ui_voltage_mv % 1000) / 10, ch1.ui_current_ma / 1000, (ch1.ui_current_ma % 1000) / 10);
        }
        printf("L1:%-20s\r\n", buf);


        // ==========================================
        // VYKRESLENÍ KAN�?LU 2
        // ==========================================
        char sel2 = (selected_channel == 2) ? '\x01' : ' ';

        strcpy(step_str, "    ");
        if (selected_channel == 2) {
            if (step_sizes_mv[current_step_idx] == 10) strcpy(step_str, " 10m");
            else if (step_sizes_mv[current_step_idx] == 100) strcpy(step_str, "100m");
            else if (step_sizes_mv[current_step_idx] == 1000) strcpy(step_str, "  1V");
        }

        // �?ádek 2 (Hlavička)
        sprintf(buf, "%cCH2 %s %2lu.%02luV %s",
                sel2,
                (ch2.mode == MODE_POWER_LM317) ? "PWR" : "CAL",
                ch2.target_voltage_mv / 1000, (ch2.target_voltage_mv % 1000) / 10,
                step_str);
        printf("L2:%-20s\r\n", buf);

        if (!ch2.enabled) strcpy(state_str, "OFF");
        else if (ch2.is_limited) strcpy(state_str, "LIM");
        else strcpy(state_str, "ON ");

        // �?ádek 3 (Hodnoty - ZDE TAKÉ P�?ID�?N ZNAK sel2)
        if (ch2.mode == MODE_DAC_DIRECT) {
            uint32_t dac_v2 = (ch2.current_dac_val * ch2.calib.adc_v_ref_mv) / 4095;
            sprintf(buf, "%c%s %2lu.%02luV   ---- ", sel2, state_str, dac_v2 / 1000, (dac_v2 % 1000) / 10);
        } else {
            sprintf(buf, "%c%s %2lu.%02luV   %lu.%02luA ", sel2, state_str, ch2.ui_voltage_mv / 1000, (ch2.ui_voltage_mv % 1000) / 10, ch2.ui_current_ma / 1000, (ch2.ui_current_ma % 1000) / 10);
        }
        printf("L3:%-20s\r\n\n", buf);
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
