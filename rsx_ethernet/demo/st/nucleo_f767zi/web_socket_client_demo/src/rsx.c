#include "rsx.h"
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include "debug.h"


extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;
extern uint16_t batt_voltage_mv[NUMCELLS];
extern measure_t adc_measure;
TaskHandle_t  rsx_task_handle; //plz don't move

//LED states (1 is on, 0 is off)
bool_t led_red_on = 0;
bool_t led_green_on = 0;
bool_t led_blue_on = 0;

// Latched on boot: true only if the previous reset was a real power cycle
// (POR/BOR), not a soft reset, NRST press, or watchdog reset.
bool_t power_cycle_detected = 0;


float vref_val = 3.3f;
float vref_val_mv = 3300.0f;

extern I2C_HandleTypeDef hi2c1;
/**
 * @brief Initializes all GPIO pins for the RSX system.
 * This must be called in main() BEFORE starting the RTOS kernel.
 */
void RSX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 0. Latch reset cause BEFORE clearing the RCC reset flags.
     PORRSTF / BORRSTF are only set when the chip actually lost power
     (or browned out). They are NOT cleared by hardware on subsequent
     resets, so we must clear them ourselves; otherwise every reset
     after the first power-up would look like a power cycle. */
  power_cycle_detected =
      (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET) ||
      (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET);
  __HAL_RCC_CLEAR_RESET_FLAGS();

  if (power_cycle_detected) {
    TRACE_INFO("Reset cause: POWER CYCLE (POR/BOR) -- will auto power_sequence()\r\n");
  } else {
    TRACE_INFO("Reset cause: soft/NRST/watchdog -- skipping auto power_sequence()\r\n");
  }

  /* 1. Enable Peripheral Clocks for all used Ports */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* 2. Configure Digital Output Pins (Control)
     Set to Push-Pull, No Pull, Low Speed for safety */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  // Motor and Arm Enable
  GPIO_InitStruct.Pin = MOTOR_EN_PIN;
  HAL_GPIO_Init(MOTOR_EN_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ARM_EN_PIN;
  HAL_GPIO_Init(ARM_EN_PORT, &GPIO_InitStruct);

  // Power Buses
  GPIO_InitStruct.Pin = BUS_5V_PIN;
  HAL_GPIO_Init(BUS_5V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BUS_12V_PIN;
  HAL_GPIO_Init(BUS_12V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BUS_24V_PIN;
  HAL_GPIO_Init(BUS_24V_PORT, &GPIO_InitStruct);

  //TODO: enable after changing to antohre pin
//  GPIO_InitStruct.Pin = BUS_55V_PIN;
//  HAL_GPIO_Init(BUS_55V_PORT, &GPIO_InitStruct);

  // E-Stop Control
  GPIO_InitStruct.Pin = ESTOP_PIN;
  HAL_GPIO_Init(ESTOP_PORT, &GPIO_InitStruct);
  // LED's
  GPIO_InitStruct.Pin = LED_R_PIN;
  HAL_GPIO_Init(LED_R_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED_G_PIN;
  HAL_GPIO_Init(LED_GB_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED_B_PIN;
  HAL_GPIO_Init(LED_GB_PORT, &GPIO_InitStruct);
  /* 3. Configure Analog Input Pins (Measurement)
     Set to Analog mode to disable digital input buffers for better ADC accuracy
   */
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  // Voltage Measurements
  GPIO_InitStruct.Pin = MEASURE_5V_PIN;
  HAL_GPIO_Init(MEASURE_5V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MEASURE_12V_PIN;
  HAL_GPIO_Init(MEASURE_12V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MEASURE_19V_PIN;
  HAL_GPIO_Init(MEASURE_19V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MEASURE_24V_PIN;
  HAL_GPIO_Init(MEASURE_24V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MEASURE_55V_PIN;
  HAL_GPIO_Init(MEASURE_55V_PORT, &GPIO_InitStruct);

  // Battery and Current Measurements
  GPIO_InitStruct.Pin = MEASURE_BATT_V_PIN;
  HAL_GPIO_Init(MEASURE_BATT_V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MEASURE_MOTOR_A_PIN;
  HAL_GPIO_Init(MEASURE_MOTOR_A_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MEASURE_CHARGER_A_PIN;
  HAL_GPIO_Init(MEASURE_CHARGER_A_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MEASURE_BATT_A_PIN;
  HAL_GPIO_Init(MEASURE_BATT_A_PORT, &GPIO_InitStruct);



  // enable adc clokc
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_ADC3_CLK_ENABLE();
  // adc initialization
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  // hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  // hadc1.Init.NbrOfConversion = 4;  // Number of channels

  // adc initialization
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  // hadc3.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  // hadc3.Init.NbrOfConversion = 4;  // Number of channels
  /* 3. Apply the config */

  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;

  hadc3.Init.ScanConvMode = DISABLE;
  hadc3.Init.NbrOfConversion = 1;

  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    TRACE_ERROR("ADC Init Error\r\n");
  }
  if (HAL_ADC_Init(&hadc3) != HAL_OK) {
    TRACE_ERROR("ADC Init Error\r\n");
  }

  // if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
  //   TRACE_ERROR("ADC1 calibration failed\r\n");
  // }

  // if (HAL_ADCEx_Calibration_Start(&hadc3) != HAL_OK) {
  //   TRACE_ERROR("ADC3 calibration failed\r\n");
  // }

  /* 4. Set initial safe state (All OFF) */
  shutoff_sequence();

  // I2C1 Init for ADS1015
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;  // PB8=SCL, PB9=SDA
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x20404768;  // 100kHz at 216MHz
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(&hi2c1);
}

void Estop_toggle(void) {
  TRACE_INFO("Enable estop\n");
  HAL_GPIO_WritePin(ESTOP_PORT, ESTOP_PIN, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(ESTOP_PORT, ESTOP_PIN, GPIO_PIN_RESET);
}

void arm_on(void) { HAL_GPIO_WritePin(ARM_EN_PORT, ARM_EN_PIN, GPIO_PIN_SET); }

void arm_off(void) {
  HAL_GPIO_WritePin(ARM_EN_PORT, ARM_EN_PIN, GPIO_PIN_RESET);
}

void motor_on(void) {
	TRACE_INFO("motor_on\n");
  HAL_GPIO_WritePin(MOTOR_EN_PORT, MOTOR_EN_PIN, GPIO_PIN_SET);
}

void motor_off(void) {
  HAL_GPIO_WritePin(MOTOR_EN_PORT, MOTOR_EN_PIN, GPIO_PIN_RESET);
}

void bus_5v_on(void) {
  HAL_GPIO_WritePin(BUS_5V_PORT, BUS_5V_PIN, GPIO_PIN_SET);
}

void bus_5v_off(void) {
  HAL_GPIO_WritePin(BUS_5V_PORT, BUS_5V_PIN, GPIO_PIN_RESET);
}

void bus_12v_on(void) {
  HAL_GPIO_WritePin(BUS_12V_PORT, BUS_12V_PIN, GPIO_PIN_SET);
}

void bus_12v_off(void) {
  HAL_GPIO_WritePin(BUS_12V_PORT, BUS_12V_PIN, GPIO_PIN_RESET);
}

void bus_24v_on(void) {
  HAL_GPIO_WritePin(BUS_24V_PORT, BUS_24V_PIN, GPIO_PIN_SET);
}

void bus_24v_off(void) {
  HAL_GPIO_WritePin(BUS_24V_PORT, BUS_24V_PIN, GPIO_PIN_RESET);
}

void bus_55v_on(void) {
  HAL_GPIO_WritePin(BUS_55V_PORT, BUS_55V_PIN, GPIO_PIN_SET);
}

void bus_55v_off(void) {
  HAL_GPIO_WritePin(BUS_55V_PORT, BUS_55V_PIN, GPIO_PIN_RESET);
}

void LED_G_on() { HAL_GPIO_WritePin(LED_GB_PORT, LED_G_PIN, GPIO_PIN_SET); }

void LED_G_off() { HAL_GPIO_WritePin(LED_GB_PORT, LED_G_PIN, GPIO_PIN_RESET); }

void LED_B_on() { HAL_GPIO_WritePin(LED_GB_PORT, LED_B_PIN, GPIO_PIN_SET); }

void LED_B_off() { HAL_GPIO_WritePin(LED_GB_PORT, LED_B_PIN, GPIO_PIN_RESET); }

void LED_R_on() { HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_SET); }

void LED_R_off() { HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_RESET); }

void calibrate_adc(void) {
  TRACE_INFO("Starting ADC Calibration using VREFINT...\r\n");

  // Read the internal 1.2V reference on hadc1
  uint32_t vref_raw = adc_read(&hadc1, ADC_CHANNEL_VREFINT);

  if (vref_raw == 0) {
    TRACE_ERROR("Calibration failed: VREFINT read 0.\r\n");
    return;
  }

  // THE MATH:
  // In a perfect 12-bit ADC, Raw = (Input_Voltage / VDDA) * 4095
  // Therefore: VDDA = (Input_Voltage * 4095) / Raw
  // We know Input_Voltage is exactly 1.2V (VINTREF)
  
  vref_val = (1.2f * 4095.0f) / (float)vref_raw;

  TRACE_INFO("Raw VREFINT: %lu\r\n", vref_raw);
  TRACE_INFO("Calibrated VDDA is now: %.2f V\r\n", vref_val);
}

// Ruth
static uint32_t adc_read(ADC_HandleTypeDef *hadc, uint32_t channel) {
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

  HAL_ADC_ConfigChannel(hadc,
                        &sConfig);  // Use the passed-in ADC (hadc1 or hadc3)
  HAL_ADC_Start(hadc);

  HAL_ADC_PollForConversion(hadc, 10);
  uint32_t value = HAL_ADC_GetValue(hadc);
  HAL_ADC_Stop(hadc);

  return value;
}

static uint16_t adc_to_voltage_mv(uint32_t adc) { return (adc *vref_val_mv) / ADC_MAX; }

void measure_v(measure_t* arr) {
  calibrate_adc();
  uint32_t adc_12v = adc_read(&hadc1, ADC_CHANNEL_12);    // ADC123_IN12
  uint32_t adc_battv = adc_read(&hadc1, ADC_CHANNEL_10);  // ADC123_IN10
  //uint32_t adc_19v = adc_read(&hadc1, ADC_CHANNEL_3);     // ADC123_IN3
  uint32_t adc_24v = adc_read(&hadc3, ADC_CHANNEL_14);    // ADC3_IN14
  uint32_t adc_55v = adc_read(&hadc3, ADC_CHANNEL_8);     // ADC3_IN8

  arr->mv_12v = adc_to_voltage_mv(adc_12v) * DIV_12V;
  arr->mv_24v = adc_to_voltage_mv(adc_24v) * DIV_24V;
  arr->mv_55v = adc_to_voltage_mv(adc_55v) * DIV_55V;
  arr->mv_batt_adc = adc_to_voltage_mv(adc_battv) * DIV_17V;

}

// Andrew
void measure_batt(measure_t* adc_arr, uint16_t* mv_cells) {
  // ADC Measure
  uint32_t adc_battv = adc_read(&hadc1, ADC_CHANNEL_10);  // ADC123_IN10
  adc_arr->mv_batt_adc = adc_to_voltage_mv(adc_battv) * DIV_17V;

  // BMS Measure
  measure_batt_bms(mv_cells, 1);
  adc_arr->mv_batt_bms = 0;
  for (int i = 0; i < NUMCELLS; i++){
	  adc_arr->mv_batt_bms += mv_cells[i];
  }
}

static float adc_to_current(uint16_t adc, float sensitivity) {
  float pin_v = (adc * VREF) / ADC_MAX;
  float centred_v = pin_v - VREF / 2.0f;
  return centred_v / sensitivity;
}

// Tanmay
void measure_a(measure_t* arr) {
  uint16_t adc_arm_motor = adc_read(&hadc1, ADC_CHANNEL_13);
  uint16_t adc_charger = adc_read(&hadc3, ADC_CHANNEL_9);
  uint16_t adc_batt = adc_read(&hadc3, ADC_CHANNEL_15);

  arr->a_arm_motor = adc_to_current(adc_arm_motor, 0.0133f);  // placeholder sensitivity
  arr->a_charger = adc_to_current(adc_charger, 0.0660f);          // placeholder sensitivity
  arr->a_batt = adc_to_current(adc_batt, 0.0110f);  // placeholder sensitivity

  TRACE_INFO("Currents - Arm+Motor: %.1fA, Charger: %.1fA, Batt: %.1fA\r\n",
		  arr->a_arm_motor, arr->a_charger, arr->a_batt);
}

// Turn off buses, arm, and motor
void shutoff_sequence(void) {
  // Turn off motor and arm

}

void power_sequence(void) {
  // 5V bus test
  bus_5v_on();  // works
  bus_12v_on();  // works
  bus_24v_on();  // works
  bus_55v_on();  // works
  // Turn on arm
  arm_on();  // doesn't work
  // Turn on motor
  motor_on();
}



int parse_int(char_t *received_cmd, int *out) {
  char *end;
  long val = strtol(received_cmd, &end, 10);

  if (end == received_cmd) {
    TRACE_INFO("parse_int end == received_cmd pointer");
    return 1;  // no digits found
  }
  // allow trailing whitespace / CRLF
  while (*end == ' ' || *end == '\r' || *end == '\n' || *end == '\t') end++;
  if (*end != '\0') {
    TRACE_INFO("parse_int failed *end != '\\0' ");
    return 1;  // extra junk in buffer
  }
  if (val < INT_MIN || val > INT_MAX) {
    TRACE_INFO("parse_int failed val < INT_MIN || val > INT_MAX ");
    return 1;  // overflow for int
  }

  *out = (int)val;
  // send the command to rsx_task
  // TODO: enable multiple commands at once
  TRACE_INFO("parsed int = %d\n", (int)val);
  xTaskNotify(rsx_task_handle, (1U << val), eSetBits);
  return 0;  // success
}

void rsxTask(void *param) {
	// set initial states:
	// Only auto-run power_sequence() if the chip actually power-cycled.
	// Soft resets / NRST presses / watchdog resets leave buses, arm, and
	// motor OFF so a developer reflash doesn't suddenly energize loads.
	if (power_cycle_detected) {
		TRACE_INFO("Power cycle detected -- running power_sequence()\r\n");
		power_sequence();
	}
	led_red_on = 0;
	led_green_on = 0;
	led_blue_on = 0;

  uint32_t rsx_task_received;
  for (;;) {
    xTaskNotifyWait(0, 0xFFFFFFFFUL, &rsx_task_received, portMAX_DELAY);
    TRACE_INFO("rsx_task_received notify bits = 0x%lx\r\n", rsx_task_received);
    ///*
    if (rsx_task_received & ESTOP_CMD) {
    	Estop_toggle();
    }
    if (rsx_task_received & MEASURE_V_CMD) {
    	measure_v(&adc_measure);
    	measureVFlag = 1;
    }
    if (rsx_task_received & MEASURE_B_CMD){
    	for (int i = 0; i < sizeof(batt_voltage_mv); i++){
    		batt_voltage_mv[i] = 0;
    	}
        measure_batt_bms(batt_voltage_mv, 1);
        measureBFlag = 1;
    }
    if (rsx_task_received & MEASURE_A_CMD) {
    	measure_a(&adc_measure);
    	measureIFlag = 1;
    }
    if (rsx_task_received & MOTOR_ON_CMD) {
    	motor_on();
    	printStatusFlag[0] = 1;
    }
    if (rsx_task_received & MOTOR_OFF_CMD) {
    	motor_off();
    	printStatusFlag[1] = 1;
    }
    if (rsx_task_received & ARM_ON_CMD) {
    	arm_on();
    	printStatusFlag[2] = 1;
    }
    if (rsx_task_received & ARM_OFF_CMD) {
    	arm_off();
    	printStatusFlag[3] = 1;
    }
    if (rsx_task_received & ON_5V_CMD) {
    	bus_5v_on();
    	printStatusFlag[4] = 1;
    }
    if (rsx_task_received & OFF_5V_CMD) {
    	bus_5v_off();
    	printStatusFlag[5] = 1;
    }
    if (rsx_task_received & ON_12V_CMD) {
    	bus_12v_on();
    	printStatusFlag[6] = 1;
    }
    if (rsx_task_received & OFF_12V_CMD) {
    	bus_12v_off();
    	printStatusFlag[7] = 1;
    }
    if (rsx_task_received & ON_24V_CMD) {
    	bus_24v_on();
    	printStatusFlag[8] = 1;
    }
    if (rsx_task_received & OFF_24V_CMD) {
    	bus_24v_off();
    	printStatusFlag[9] = 1;
    }
    if (rsx_task_received & ON_55V_CMD) {
    	bus_55v_on();
    	printStatusFlag[10] = 1;
    }
    if (rsx_task_received & OFF_55V_CMD) {
    	bus_55v_off();
    	printStatusFlag[11] = 1;
    }
    if (rsx_task_received & R_LED_TOGGLE) {
    	if(led_red_on){
    		LED_R_off();
    		led_red_on = 0;
    		printStatusFlag[12] = 1;
    	} else {
    		LED_R_on();
    		led_red_on = 1;
    		printStatusFlag[13] = 1;
    	}
    }
    if (rsx_task_received & G_LED_TOGGLE) {
    	if(led_green_on){
    		LED_G_off();
    		led_green_on = 0;
    		printStatusFlag[14] = 1;
    	} else {
    		LED_G_on();
    		led_green_on = 1;
    		printStatusFlag[15] = 1;
    	}
    }
    if (rsx_task_received & B_LED_TOGGLE) {
    	if(led_blue_on){
    		LED_B_off();
    		led_blue_on = 0;
    		printStatusFlag[16] = 1;
    	} else {
    		LED_B_on();
    		led_blue_on = 1;
    		printStatusFlag[17] = 1;
    	}
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void bmsUVProtectionTask(void *param) {
	uint16_t total_v;
	error_t error;
	int length;
	char buffer[100];
	int UV_counter = 0;
#ifndef DISABLE_UV_PROTECTION
    for (;;){
    	for (int i = 0; i < sizeof(batt_voltage_mv); i++){
    		batt_voltage_mv[i] = 0;
    	}
        measure_batt_bms(batt_voltage_mv, 0);
        total_v = batt_voltage_mv[0]+batt_voltage_mv[1]+batt_voltage_mv[2]+batt_voltage_mv[3];
       	if (total_v < (UV_THRESHOLD_MV-BMS_NEG_OFFSET_MV) && total_v != 0 ) {
       		TRACE_INFO("UV! UV! UV!\r\n");
       		UV_counter ++;
       		if(UV_counter > UV_SAMPLE_SIZE){
				Estop_toggle();
				uvEventFlag=1;
       		}
       	}
       	else {
       		//reset counter
       		UV_counter = 0;
       	}

       	vTaskDelay(pdMS_TO_TICKS(5000));

   	}
#endif
}

bool_t is_all_zero(bool_t *arr, size_t len){
	for (int i = 0; i < len; i++) {
	    if (arr[i]) {  // shorter: nonzero check
	        return 1;
	    }
	}
	return 0;
}


