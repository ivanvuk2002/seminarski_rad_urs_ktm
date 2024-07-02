
#include "main.h"
#include "i2c-lcd.h"

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);

uint32_t Value_1 = 0;     // Vrijednost prvog tajmera
uint32_t Value_2 = 0;     // Vrijednost drugog tajmera
uint32_t Difference = 0;  // Razlika između prvog i drugog
uint8_t Captured = 0;     // Status uhvata (je li prvi ili drugi)
uint8_t Distance  = 0;     // Izračunata udaljenost u cm
uint32_t last_toggle_time = 0;  // Vrijeme zadnjeg prekidača buzzer-a
uint8_t Buzzer_On = 0;    // Status buzzer-a (uključen ili isključen)
uint32_t Buzzer;         // Period za uključivanje/isključivanje buzzer-a

#define TRIG_PIN GPIO_PIN_9    // Pin za TRIG signal HC-SR04
#define TRIG_PORT GPIOA        // Port za TRIG signal HC-SR04



// Funkcija za obradu uhvata tajmera (poziva se kad se dogodi uhvat)
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        if (Captured == 0)
        {
        	 // Čitanje vrijednosti prvog uhvata tajmera
            Value_1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            Captured = 1;
            // Postavljanje polariteta na padajući rub (sljedeći uhvat će biti na rastućem rubu)
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
        }
        else if (Captured == 1)
        {
        	 // Čitanje vrijednosti drugog uhvata tajmera
            Value_2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            // Izračun razlike između dva uhvata, uzimajući u obzir prelazak preko granice 0xFFFF
            if (Value_2 > Value_1)
            {
                Difference = Value_2 - Value_1;
            }
            else
            {
                Difference = (0xFFFF - Value_1) + Value_2;
            }
            // Pretvaranje razlike u udaljenost (uz pretpostavku brzine zvuka od 343 m/s)
          Distance = (float)Difference * 0.0343/ 2.0;

              Captured = 0;
              // Postavljanje polariteta na rastući rub (sljedeći uhvat će biti na padajućem rubu)
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            // Onemogućavanje prekida za Channel 1 (TIM_IT_CC1)
            __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC1);
        }
    }
}

// Funkcija za kašnjenje
void delay (uint16_t time)
{
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	while (__HAL_TIM_GET_COUNTER (&htim1) < time);
}

// Funkcija za čitanje HC-SR04 senzora
void HCSR04_Read(void)
{
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    delay(10);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC1);
}
// Glavna funkcija
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();

    lcd_init();
    // Pokretanje tajmera 1 u režimu uhvata sa prekidima za Channel 1
    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);
    // Pokretanje PWM signala na tajmeru 2, Channel 2
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    uint32_t current_time = 0;

    while (1)
    {
    	 // Čitanje podataka s HC-SR04 senzora
        HCSR04_Read();
        lcd_clear();
        // Ispisivanje izmjerene udaljenosti na LCD-u
        lcd_print_distance(Distance);
        HAL_Delay(200);

        // Čitanje trenutnog vremena
        current_time = HAL_GetTick();

        // Postavljanje periode za uključivanje/isključivanje buzzer-a na temelju udaljenosti
        if (Distance < 10)
        {
            Buzzer = 100;
        }
        else if (Distance < 20 )
        {
            Buzzer = 250;
        }
        else if (Distance < 30)
        {
            Buzzer = 500;
        }
        else
        {
            Buzzer = 0;
        }

        // Upravljanje buzzer-om na temelju vremena i izmjerene udaljenosti
        if (Buzzer > 0)
        {
            if (current_time - last_toggle_time >= Buzzer)
            {
                if (Buzzer_On)
                {
                	 // Isključivanje PWM signala na tajmeru 2, Channel 2
                    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
                }
                else
                {
                	 // Postavljanje duty ciklusa PWM signala na tajmeru 2, Channel 2 na 50%
                    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500);
                    // Pokretanje PWM signala na tajmeru 2, Channel 2
                    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
                }
                // Promjena stanja buzzer-a
                Buzzer_On = !Buzzer_On;
                // Ažuriranje vremena zadnje promjene stanja buzzer-a
                last_toggle_time = current_time;
            }
        }
        else
        {
        	 // Isključivanje PWM signala na tajmeru 2, Channel 2
                HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
                // Postavljanje stanja buzzer-a na isključeno
                Buzzer_On = 0;

        }
    }
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }


  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }


  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLRCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}


static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

}


static void MX_TIM1_Init(void)
{



  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 180-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0xFFFF;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }


}


static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 90;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 45;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim2);

}


static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


}


void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{

}
#endif
