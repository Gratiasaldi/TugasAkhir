/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body — DAC-based gated square wave, Vpp = 4V
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "dac.h"
#include "tim.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* USER CODE BEGIN PV */
/* ============================================================
 * KONFIGURASI TARGET Vpp
 * Ubah nilai ini untuk mengganti amplitudo output (dalam Volt).
 * Range valid berdasarkan spek: 1.00 V sampai 8.00 V.
 * ============================================================ */
#define TARGET_VPP  1.0f

/* Konstanta rangkaian analog dari skematik:
 *   PULSE_OUT = 5V - 3 × MCU_PULSE_OUT
 * Gelombang PULSE_OUT diasumsikan simetris di sekitar 0V,
 * jadi V_high = +Vpp/2 dan V_low = -Vpp/2.
 *
 * Rumus mundur untuk nilai DAC yang perlu dikeluarkan di PA5:
 *   MCU_high = (5 - (+Vpp/2)) / 3
 *   MCU_low  = (5 - (-Vpp/2)) / 3
 *   MCU_mid  = 5 / 3  (menghasilkan PULSE_OUT = 0V saat idle)
 *
 * Nilai DAC 12-bit dihitung: DAC_value = MCU_voltage × 4095 / 3.3
 */
#define DAC_VREF         3.3f
#define DAC_MAX_COUNT    4095U
#define OPAMP_OFFSET     5.0f
#define OPAMP_GAIN       3.0f

/* Kalkulasi nilai DAC untuk 3 kondisi:
 *  - dac_high : DAC level saat "high" (PULSE_OUT = +Vpp/2)
 *  - dac_low  : DAC level saat "low"  (PULSE_OUT = -Vpp/2)
 *  - dac_mid  : DAC level saat idle   (PULSE_OUT = 0V)
 * Dihitung sekali di startup, disimpan di variable global.
 */
volatile uint32_t dac_high = 0;
volatile uint32_t dac_low  = 0;
volatile uint32_t dac_mid  = 0;

/* State runtime:
 *  - gate_enabled : 1 saat trigger HIGH (rising..falling), 0 saat idle
 *  - dac_state    : 0 = keluarkan dac_low, 1 = keluarkan dac_high
 *                   (di-toggle setiap TIM3 interrupt saat gate_enabled)
 */
volatile uint8_t  gate_enabled = 0;
volatile uint8_t  dac_state    = 0;

/* Debug counter (opsional, bisa dipantau di Keil Watch window) */
volatile uint32_t gate_rise_count = 0;
volatile uint32_t gate_fall_count = 0;
volatile uint32_t tim3_tick_count = 0;
/* USER CODE END PV */

void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* Hitung nilai DAC dari target Vpp, hasil disimpan ke variable global. */
static void ComputeDACLevels(float vpp)
{
    float v_high_out = +vpp / 2.0f;     /* target PULSE_OUT saat high */
    float v_low_out  = -vpp / 2.0f;     /* target PULSE_OUT saat low  */

    float mcu_high = (OPAMP_OFFSET - v_high_out) / OPAMP_GAIN;
    float mcu_low  = (OPAMP_OFFSET - v_low_out)  / OPAMP_GAIN;
    float mcu_mid  =  OPAMP_OFFSET / OPAMP_GAIN;   /* untuk PULSE_OUT = 0V */

    dac_high = (uint32_t)((mcu_high / DAC_VREF) * (float)DAC_MAX_COUNT);
    dac_low  = (uint32_t)((mcu_low  / DAC_VREF) * (float)DAC_MAX_COUNT);
    dac_mid  = (uint32_t)((mcu_mid  / DAC_VREF) * (float)DAC_MAX_COUNT);

    /* Clamp untuk keamanan (jangan melewati rentang 0..4095) */
    if (dac_high > DAC_MAX_COUNT) dac_high = DAC_MAX_COUNT;
    if (dac_low  > DAC_MAX_COUNT) dac_low  = DAC_MAX_COUNT;
    if (dac_mid  > DAC_MAX_COUNT) dac_mid  = DAC_MAX_COUNT;
}
/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM2_Init();      /* tidak dipakai lagi — sisa dari config lama */
  MX_DAC_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */

  /* Hitung nilai DAC untuk target Vpp yang diminta */
  ComputeDACLevels(TARGET_VPP);

  /* Start DAC channel 2 (PA5) */
  HAL_DAC_Start(&hdac, DAC_CHANNEL_2);

  /* Set output awal ke "mid" ? PULSE_OUT idle di 0V */
  HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_mid);

  /* Start TIM3 dengan interrupt aktif — ini yang akan toggle DAC di 40 kHz.
   * Meski gate_enabled=0, TIM3 tetap jalan; ISR akan cek flag sebelum toggle.
   */
  HAL_TIM_Base_Start_IT(&htim3);

  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Idle — semua kerja dilakukan di ISR (EXTI PA6 + TIM3 update) */
  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

/* USER CODE BEGIN 4 */

/* EXTI callback — dipanggil saat PA6 (trigger input) berubah level.
 * Konfigurasi CubeMX: PA6 = rising + falling edge.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_6)
    {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET)
        {
            /* Rising edge ? mulai gelombang kotak */
            gate_rise_count++;
            dac_state = 0;                /* mulai dari low, tick pertama akan toggle ke high */
            gate_enabled = 1;
        }
        else
        {
            /* Falling edge ? berhenti, DAC balik ke mid (PULSE_OUT = 0V) */
            gate_fall_count++;
            gate_enabled = 0;
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_mid);
        }
    }
}

/* TIM3 update callback — dipanggil setiap 25 µs (frekuensi 40 kHz).
 * Kalau gate aktif, toggle DAC antara high dan low ? hasilkan 20 kHz square wave.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        tim3_tick_count++;

        if (gate_enabled)
        {
            dac_state ^= 1;   /* toggle 0 <-> 1 */
            uint32_t out = (dac_state) ? dac_high : dac_low;
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, out);
        }
        /* kalau gate tidak aktif, DAC tidak diubah — tetap di dac_mid dari EXTI falling */
    }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1) { }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif
