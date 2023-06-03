/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Count zero-crossings of analog input signal
  * using STM32F401CCU6 dev board
  * J.Beale 27-May-2023
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FALSE 0
#define TRUE 1

#define ADC_BUFFER_SIZE 8000  // how many uint16 words in ADC buffer; max ~ 20k
#define DECIMATION 200  // average together this many raw samples
#define PING_BUFFER_SIZE (ADC_BUFFER_SIZE / (2 * DECIMATION))

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */

volatile uint16_t adc_buffer[ADC_BUFFER_SIZE];
volatile uint32_t ping_buffer[2][PING_BUFFER_SIZE];

volatile uint8_t pingLoaded[2] = {FALSE, FALSE}; // if ping-pong buffers filled from DMA
volatile uint8_t pingReady[2] = {TRUE, TRUE}; // if buffers processed & ready for next fill
//unsigned int pp = 0;      // ping-pong buffer index: 0 = ping, 1 = pong

// these globals variables should not need to be volatile
float dc_avg = -1;  // very-low-pass-filtered version of signal
float lpval = -1;   // slightly low-pass filtered version
int last_sign = 0;  // previous sign of signal trend (1,0,-1 = up, constant, down)
float lpfilt1 = 0.001;  // fractional LP filter constant for long-term DC average
float lpfilt2 = 0.85;     // LP filter for fast-moving value
unsigned int zero_crossings = 0;  // how many total zero crossings

int loopctr = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_USB_DEVICE_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

  // process_adc_data()
  // find average of values in ADC buffer
  // send it out USB serial device
  // also increment global loop counter

  void process_adc_data(unsigned char pp) {
      // calculate statistics on data in buffer
      char out_buffer[800];  // for sending text to USB serial device


	  long datSum = 0;  // reset our accumulated sum of input values to zero
      int sMax = 0;
      int sMin = 65535;
      long n;            // count of how many readings so far
      uint32_t x;
      double mean,delta,m2,variance,stdev;  // to calculate standard deviation

      n = 0;     // have not made any ADC readings yet
      mean = 0; // start off with running mean at zero
      m2 = 0;

      if (dc_avg < 0) {
    	  dc_avg = ping_buffer[pp][PING_BUFFER_SIZE/2];  // initialize very first time
    	  lpval = dc_avg;
      }

      // from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
      for (int i=0;i<PING_BUFFER_SIZE;i++) {
    	x = ping_buffer[pp][i];
        datSum += x;
        if (x > sMax) sMax = x;
        if (x < sMin) sMin = x;
        n++;
        delta = x - mean;
        mean += delta/n;
        m2 += (delta * (x - mean));
      }
      variance = m2/(n-1);  // (n-1):Sample Variance  (n): Population Variance
      stdev = sqrt(variance);  // Calculate standard deviation

      int sAbove = 0; // count of samples more than 1 stdev above mean
      int sBelow = 0; // count of samples below 1 stdev below mean
      int tHigh = mean + stdev;
      int tLow = mean - stdev;
      for (int i=0;i<PING_BUFFER_SIZE;i++) {
       	x = ping_buffer[pp][i];
      	dc_avg = (x * lpfilt1) + dc_avg * (1.0-lpfilt1);  // rolling average low-pass-filter
      	lpval = (x * lpfilt2) + lpval * (1.0-lpfilt2);    // faster low-pass-filter
      	if (x > tHigh) sAbove++;
      	if (x < tLow) sBelow++;
      	int x_sign = 0;
    	if (lpval > (dc_avg + 3*stdev)) {
    		x_sign = 1;
    	}
    	if (lpval < (dc_avg - 3*stdev)){
    		x_sign = -1;
    	}
		if (abs(x_sign - last_sign) == 2) {
			zero_crossings++;
		}
		if (x_sign != 0) last_sign = x_sign;

      }

      int i=0;
      int bp = 0;
      int ccount;

      do {
		  ccount = sprintf(&out_buffer[bp],"%ld\n", (long int)ping_buffer[pp][i]);
		  bp += ccount;
		  i++;
      } while (i < PING_BUFFER_SIZE);
      // CDC_Transmit_FS((uint8_t*)out_buffer,ccount);


      float sf = (sMax - sMin) / stdev; // ratio of peak-peak noise to stdev
      ccount = sprintf(&out_buffer[bp],"%d, %d, %5.3f, %5.3f, %d,%d, %5.1f, %5.3f,  %d\n",
    		  sMin, sMax, mean, stdev, sAbove, sBelow, sf, dc_avg, zero_crossings);
	  bp += ccount;
      CDC_Transmit_FS((uint8_t*)out_buffer,bp);

      loopctr++;
	  // HAL_Delay(10);  // just for debug convenience

      pingReady[pp] = TRUE;  // signal to DMA it is OK to refill buffer now
      pingLoaded[pp] = FALSE; // data is not fresh

  }




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  for (int i=0;i<2;i++){
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 0);  // GPIO13 LOW => turn on the onboard blue LED
	  HAL_Delay(2000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);  // turn it off
	  HAL_Delay(500);
  }

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE);

  while (1)
  {
      if (pingLoaded[0]) process_adc_data(0);
      if (pingLoaded[1]) process_adc_data(1);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV16;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// Called when first half of buffer is filled
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
	int p=0;
    if (pingReady[p]) {
		// copy over first half of DMA buff
		for(int i=0; i<PING_BUFFER_SIZE; i++) {
			int sum = 0;
			for (int j=0; j<DECIMATION; j++) {
				sum += adc_buffer[i*DECIMATION + j];
			}
			ping_buffer[p][i] = sum;
		}
		pingLoaded[p] = TRUE;  // now contains fresh data
		pingReady[p] = FALSE;  // data buffer not yet processed
	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 0);  // GPIO13 LOW => turn on the onboard blue LED
	}
}

// Called when buffer is completely filled
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	int p = 1;
    if (pingReady[p]) {
		// copy over first half of DMA buff
		for(int i=0; i<PING_BUFFER_SIZE; i++) {
			int sum = 0;
			for (int j=0; j<DECIMATION; j++) {
				sum += adc_buffer[i*DECIMATION + j];
			}
			ping_buffer[p][i] = sum;
		}
		pingLoaded[p] = TRUE;  // this buffer now contains fresh data
		pingReady[p] = FALSE;  // this data buffer not yet processed
	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);  // turn off LED
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
