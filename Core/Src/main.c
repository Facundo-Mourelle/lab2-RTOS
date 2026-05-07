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
#include "cmsis_os.h"
#include "gpio.h"
#include "stm32f4xx_hal_gpio.h"
#include "usb_host.h"
#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define QUEUE_VELOCIDAD_LEN 3
#define QUEUE_MODO_LEN 3

#define LED1 GPIO_PIN_12
#define LED2 GPIO_PIN_13
#define LED3 GPIO_PIN_14
#define LED4 GPIO_PIN_15
#define LED_PORT GPIOD

#define BTN_PIN GPIO_PIN_0
#define BTN_PORT GPIOA
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static QueueHandle_t Cola_Velocidad;
static QueueHandle_t Cola_Modo;
static QueueSetHandle_t xQueueSet;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void vSetLeds(uint16_t value) {
  HAL_GPIO_WritePin(LED_PORT, LED1,
                    (value & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_PORT, LED2,
                    (value & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_PORT, LED3,
                    (value & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_PORT, LED4,
                    (value & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void vVelocidadTask(void *pvParameters) {
  uint16_t velocidad;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    // Generar valor entre 100 y 300 ms
    velocidad = 100 + (rand() % 201);
    xQueueSend(Cola_Velocidad, &velocidad, 0);
  }
}

void vModoTask(void *pvParameters) {
  GPIO_PinState estadoAnterior = GPIO_PIN_RESET;
  char modo_actual = 'D';

  for (;;) {
    GPIO_PinState estadoActual = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);
    if (estadoActual == GPIO_PIN_SET && estadoAnterior == GPIO_PIN_RESET) {
      vTaskDelay(pdMS_TO_TICKS(50));
      estadoActual = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

      if (estadoActual == GPIO_PIN_SET) {
        modo_actual = (modo_actual == 'D') ? 'I' : 'D';
        xQueueSend(Cola_Modo, &modo_actual, pdMS_TO_TICKS(10));
      }
    }
    estadoAnterior = estadoActual;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void vProcessTask(void *pvParameters) {
  uint16_t current_speed = 200;
  char current_mode = 'D';
  uint8_t led_state = 0x01; // Secuencia inicial (LED0 encendido)
  QueueSetMemberHandle_t xActivatedMember;

  for (;;) {
    // Bloquear en el Queue Set. El timeout se usa como delay
    xActivatedMember =
        xQueueSelectFromSet(xQueueSet, pdMS_TO_TICKS(current_speed));

    if (xActivatedMember == Cola_Velocidad) {
      xQueueReceive(Cola_Velocidad, &current_speed, 0);
    } else if (xActivatedMember == Cola_Modo) {
      xQueueReceive(Cola_Modo, &current_mode, 0);
    } else if (xActivatedMember == NULL) {
      // El timeout expiró => avanzar el frame de la secuencia LED
      if (current_mode == 'D') {
        led_state = (led_state << 1) | ((led_state & 0x08) << 3);
      } else {
        led_state = (led_state >> 1) | ((led_state & 0x01) >> 3);
      }
    }
    vSetLeds(led_state);
  }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU
   * Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the
   * Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  Cola_Velocidad = xQueueCreate(QUEUE_VELOCIDAD_LEN, sizeof(uint16_t));
  Cola_Modo = xQueueCreate(QUEUE_MODO_LEN, sizeof(char));
  xQueueSet = xQueueCreateSet(QUEUE_VELOCIDAD_LEN + QUEUE_MODO_LEN);

  if (xQueueSet != NULL && Cola_Velocidad != NULL && Cola_Modo != NULL) {
    xQueueAddToSet(Cola_Velocidad, xQueueSet);
    xQueueAddToSet(Cola_Modo, xQueueSet);

    xTaskCreate(vVelocidadTask, "Velocidad", 128, NULL, 1, NULL);
    xTaskCreate(vModoTask, "Modo", 128, NULL, 1, NULL);
    xTaskCreate(vProcessTask, "Procesadora", 128, NULL, 2, NULL);
  }
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
     file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
