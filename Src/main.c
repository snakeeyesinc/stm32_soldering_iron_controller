
/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"
#include "ssd1306.h"
#include "buzzer.h"
#include "rotary_encoder.h"
#include "adc_global.h"
#include "tempsensors.h"
#include "voltagesensors.h"
#include "screen.h"
#include "gui.h"
#include "debug_screen.h"
#include "pid.h"
#include "settings.h"
#include "iron.h"


/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;
IWDG_HandleTypeDef hiwdg;
TIM_HandleTypeDef tim3_pwm;
TIM_HandleTypeDef tim4_temp_measure;
UART_HandleTypeDef huart3;
TIM_OC_InitTypeDef sConfigOC;


volatile uint32_t adc = 0;
volatile uint32_t dma = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_IWDG_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM3_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
RE_State_t RE1_Data;

static uint8_t activity = 1;
static uint32_t pwmStoppedSince = 0;
static uint32_t startOfNoActivityTime = 0;
static uint8_t startOfNoActivity = 0;
#define ADC_MEASURE_DELAY	0
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC2_Init(&hadc2);
  MX_ADC1_Init(&hadc1);
  buzzer_init();
  if(HAL_ADCEx_Calibration_Start(&hadc2) != HAL_OK)
	  buzzer_alarm_start();
  else if(HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
	  buzzer_alarm_start();
  else
	  buzzer_short_beep();

  ironInit(&tim3_pwm);
  MX_IWDG_Init();
  MX_TIM4_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  /* Enable ADC slave */
  if (HAL_ADC_Start(&hadc2) != HAL_OK)
  {
	  buzzer_alarm_start();
	  Error_Handler();
  }
  HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t*) adc_measures, sizeof(adc_measures)/ sizeof(uint32_t));

  HAL_TIM_Base_Start_IT(&tim4_temp_measure);
  restoreSettings();

  DWT_Delay_Init(); // Important for I2C communication
  UG_GUI gui;
  ssd1306_init();
  UG_Init(&gui, pset, 128, 64);
  guiInit();
  oled_init();
  oled_draw();
  UG_Update();
  update_display();

  HAL_IWDG_Start(&hiwdg);
  RE_Init(&RE1_Data, ROT_ENC_L_GPIO_Port, ROT_ENC_L_Pin, ROT_ENC_R_GPIO_Port, ROT_ENC_R_Pin, ROT_ENC_BUTTON_GPIO_Port, ROT_ENC_BUTTON_GPIO_Pin);

  uint32_t lastTimeDisplay = HAL_GetTick();

  setPWM_tim(&tim3_pwm);
  iron_temp_measure_state = iron_temp_measure_idle;

  setContrast(systemSettings.contrast);
  currentBoostSettings = systemSettings.boost;
  currentSleepSettings = systemSettings.sleep;
  applyBoostSettings();
  applySleepSettings();
  setCurrentTip(systemSettings.currentTip);
  setupPIDFromStruct();


  if(HAL_GPIO_ReadPin(WAKE_GPIO_Port, WAKE_Pin) == GPIO_PIN_RESET) {
	  activity = 0;
	  setCurrentMode(mode_sleep);
  }

  while (1)
  {
	  HAL_IWDG_Refresh(&hiwdg);
	  if(iron_temp_measure_state == iron_temp_measure_ready) {
		  readTipTemperatureCompensated(1);

		  if(HAL_GPIO_ReadPin(WAKE_GPIO_Port, WAKE_Pin) == GPIO_PIN_RESET) {
			  if(startOfNoActivity == 0)
				  startOfNoActivityTime = HAL_GetTick();
			  startOfNoActivity = 1;
			  if((HAL_GetTick() - startOfNoActivityTime) > 500)
				  activity = 0;
		  }
		  else {
			  activity = 1;
			  startOfNoActivity = 0;
		  }

		  handleIron(activity);
		  iron_temp_measure_state = iron_temp_measure_idle;
	  }

	  if(HAL_GetTick() - lastTimeDisplay > 50) {
		  handle_buzzer();
		  RE_Rotation_t r = RE_Get(&RE1_Data);
		  oled_update();
		  oled_processInput(r, &RE1_Data);
		  oled_draw();
		  lastTimeDisplay = HAL_GetTick();
	  }
  }

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim != &tim4_temp_measure)
		return;
	if(iron_temp_measure_state == iron_temp_measure_idle) {
		iron_temp_measure_state = iron_temp_measure_requested;
		HAL_TIM_PWM_Stop(&tim3_pwm, TIM_CHANNEL_3);  // don't use turnIronOff();
		iron_temp_measure_state = iron_temp_measure_pwm_stopped;
		pwmStoppedSince = HAL_GetTick();		
	}
}
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc){
	if(hadc != &hadc1)
		return;
//	if(iron_temp_measure_state == iron_temp_measure_requested) {
//		HAL_TIM_PWM_Stop(&tim3_pwm, TIM_CHANNEL_3);  // don't use turnIronOff();
//		iron_temp_measure_state = iron_temp_measure_pwm_stopped;
//		pwmStoppedSince = HAL_GetTick();
//	}
}

#define FIFO_LEN 			(sizeof(adc_measures)/sizeof(adc_measures[0]))
#define ROLLING_AVG_LEN 	(sizeof(ironTempADCRollingAverage)/sizeof(ironTempADCRollingAverage[0]))
int aleliuja;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	static uint16_t ironTempADCRollingAverage[10] = {0};
	static int rindex = 0;
	static int doOnce = 0;
	uint32_t acc = 0;
	uint16_t max = 0;
	uint16_t min = 0xFFFF;
	uint16_t temp;
	if(hadc != &hadc1)
		return;
	if((iron_temp_measure_state == iron_temp_measure_pwm_stopped) && (HAL_GetTick() - pwmStoppedSince > ADC_MEASURE_DELAY)) {
		iron_temp_measure_state = iron_temp_measure_started;
		return;
	} else if(iron_temp_measure_state == iron_temp_measure_started) {
		for(int x = 0; x < sizeof(adc_measures)/sizeof(adc_measures[0]); ++x) {
			temp = adc_measures[x].iron;
			acc += temp;
			if(temp > max)
				max = temp;
			if(temp < min)
				min = temp;
		}
//		acc = acc - min - max;
//		uint16_t last = acc / ((sizeof(adc_measures)/sizeof(adc_measures[0])) -2);
		uint16_t last = UINT_DIV( (acc - min - max) , FIFO_LEN-2);
		ironTempADCRollingAverage[rindex] = last;
		if(doOnce) {
			for(uint8_t x = 0; x < sizeof(ironTempADCRollingAverage)/sizeof(ironTempADCRollingAverage[0]); ++x) {
				ironTempADCRollingAverage[x] = last;
			}
			doOnce =0;
		}
		rindex = (rindex + 1) % ROLLING_AVG_LEN;
		acc = 0;
		for(uint8_t x = 0; x < sizeof(ironTempADCRollingAverage)/sizeof(ironTempADCRollingAverage[0]); ++x) {
			acc += ironTempADCRollingAverage[x];
		}
		iron_temp_adc_avg = UINT_DIV(acc, ROLLING_AVG_LEN); //minimize divide error from +-1 to +- 0.5
		iron_temp_measure_state = iron_temp_measure_ready;
		if(getIronOn())
			HAL_TIM_PWM_Start(&tim3_pwm, TIM_CHANNEL_3);  // don't use turnIronOn();
		}
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if((GPIO_Pin == ROT_ENC_BUTTON_GPIO_Pin) || (GPIO_Pin == ROT_ENC_R_Pin) || (GPIO_Pin == ROT_ENC_L_Pin)) {
		  RE_Process(&RE1_Data);

	}
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}



/* IWDG init function
 * Frequency 40Khz / 32 = 1.25KHz <=>  0.8ms period
 * timeout every 0.8ms x 4095 = 3.276S */
static void MX_IWDG_Init(void)
{

  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM3 init function
Timer used to interrupt power to the iron in order
to measure Termocouple ADC
Clock rate 48MHz/ 6000 = 8000Hz = 125uS period
interrupt on every 125uS x 10 = 1.25ms */
static void MX_TIM4_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  tim4_temp_measure.Instance = TIM4;
  tim4_temp_measure.Init.Prescaler = 6000;
  tim4_temp_measure.Init.CounterMode = TIM_COUNTERMODE_UP;
  tim4_temp_measure.Init.Period = 10;
  tim4_temp_measure.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//CHECK
  if (HAL_TIM_Base_Init(&tim4_temp_measure) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&tim4_temp_measure, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&tim4_temp_measure, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM4 init function
 * Timer used for PWM generation
 * Clock rate 48MHz / 16 = 3MHz
 * PWM Frequency = 3MHz / 1500 = 2KHz*/

static void MX_TIM3_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;

  tim3_pwm.Instance = TIM3;
  tim3_pwm.Init.Prescaler = 16;
  tim3_pwm.Init.CounterMode = TIM_COUNTERMODE_UP;
  tim3_pwm.Init.Period = 1500;
  tim3_pwm.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&tim3_pwm) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&tim3_pwm, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_Init(&tim3_pwm) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 750;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&tim3_pwm, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&tim3_pwm);

}

/* USART3 init function */
static void MX_USART3_UART_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
#ifndef HW_VER2_1S
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, OLED_CS_Pin|OLED_DC_Pin|OLED_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BUZZER_Pin|PWM_CONTROL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : OLED_CS_Pin OLED_DC_Pin OLED_RST_Pin */
  GPIO_InitStruct.Pin = OLED_CS_Pin|OLED_DC_Pin|OLED_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : ROT_ENC_L_Pin ROT_ENC_R_Pin */
  GPIO_InitStruct.Pin = ROT_ENC_L_Pin| ROT_ENC_R_Pin | ROT_ENC_BUTTON_GPIO_Pin | WAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}
#else
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(T12PWM_GPIO_Port, T12PWM_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, OLED_SCL_Pin|OLED_SDA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, EEPROM_SCL_Pin|EEPROM_SDA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PBPin PBPin */
  GPIO_InitStruct.Pin = OLED_SCL_Pin|OLED_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PAPin PAPin */
  GPIO_InitStruct.Pin = EEPROM_SCL_Pin|EEPROM_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : ROT_ENC_L_Pin ROT_ENC_R_Pin */
  GPIO_InitStruct.Pin = ROT_ENC_L_Pin| ROT_ENC_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ROT_ENC_BUTTON_GPIO_Pin WAKE_Pin */
  GPIO_InitStruct.Pin = ROT_ENC_BUTTON_GPIO_Pin | WAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
#endif
/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
