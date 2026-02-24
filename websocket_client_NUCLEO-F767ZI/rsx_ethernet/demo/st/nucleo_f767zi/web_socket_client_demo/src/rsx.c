#include "rsx.h"

#include <stdlib.h>
#include <limits.h>
#include "debug.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;
extern uint16_t voltage_mv[NUMCELLS];
/**
 * @brief Initializes all GPIO pins for the RSX system.
 * This must be called in main() BEFORE starting the RTOS kernel.
 */
void RSX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

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

  GPIO_InitStruct.Pin = BUS_19V_PIN;
  HAL_GPIO_Init(BUS_19V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BUS_24V_PIN;
  HAL_GPIO_Init(BUS_24V_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BUS_55V_PIN;
  HAL_GPIO_Init(BUS_55V_PORT, &GPIO_InitStruct);

  // E-Stop Control
  GPIO_InitStruct.Pin = ESTOP_PIN;
  HAL_GPIO_Init(ESTOP_PORT, &GPIO_InitStruct);

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

  //enable adc clokc
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_ADC3_CLK_ENABLE();
  // adc initialization
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;   // Number of channels

  // adc initialization
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 4;  // Number of channels
  /* 3. Apply the config */
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    TRACE_ERROR("ADC Init Error\r\n");
  }
  if (HAL_ADC_Init(&hadc3) != HAL_OK) {
    TRACE_ERROR("ADC Init Error\r\n");
  }

  /* 3.5 set adc channels*/
  ADC_ChannelConfTypeDef sConfig = {0};
//ADC123_IN10
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 2;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//ADC123_IN12
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = 3;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//ADC123_IN3
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = 4;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
 //ADC123_IN13
  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = 1;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  //ADC3_IN9
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 1;
  HAL_ADC_ConfigChannel(&hadc3, &sConfig);
  //ADC3_IN15
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = 2;
  HAL_ADC_ConfigChannel(&hadc3, &sConfig);
  //ADC123_IN13
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = 3;
  HAL_ADC_ConfigChannel(&hadc3, &sConfig);
 //ADC3_IN14,
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = 4;
  HAL_ADC_ConfigChannel(&hadc3, &sConfig);
  /* 4. Set initial safe state (All OFF) */
  shutoff_sequence();
}

void Estop_toggle(void) {
	 TRACE_INFO("Enable estop");
  HAL_GPIO_WritePin(ESTOP_PORT, ESTOP_PIN, GPIO_PIN_SET);
}

void arm_on(void) {
  HAL_GPIO_WritePin(ARM_EN_PORT, ARM_EN_PIN, GPIO_PIN_SET);
}

void arm_off(void) {
  HAL_GPIO_WritePin(ARM_EN_PORT, ARM_EN_PIN, GPIO_PIN_RESET);
}

void motor_on(void) {
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

void bus_19v_on(void) {
  HAL_GPIO_WritePin(BUS_19V_PORT, BUS_19V_PIN, GPIO_PIN_SET);
}

void bus_19v_off(void) {
  HAL_GPIO_WritePin(BUS_19V_PORT, BUS_19V_PIN, GPIO_PIN_RESET);
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

void LED_G_on() {
	HAL_GPIO_WritePin(LED_GB_PORT, LED_G_PIN, GPIO_PIN_SET);
}

void LED_G_off() {
	HAL_GPIO_WritePin(LED_GB_PORT, LED_G_PIN, GPIO_PIN_RESET);
}

void LED_B_on() {
	HAL_GPIO_WritePin(LED_GB_PORT, LED_B_PIN, GPIO_PIN_SET);
}

void LED_B_off() {
	HAL_GPIO_WritePin(LED_GB_PORT, LED_B_PIN, GPIO_PIN_RESET);
}

void LED_R_on() {
	HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_SET);
}

void LED_R_off() {
	HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_RESET);
}

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;

// Ruth
static uint16_t adc_read(ADC_HandleTypeDef *hadc, uint32_t channel) {
  //ADC_ChannelConfTypeDef sConfig = {0};
  //sConfig.Channel = channel;
  //sConfig.Rank = ADC_REGULAR_RANK_1;
  //sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5; //TODO: this was erroring out, coco commented this out

  //HAL_ADC_ConfigChannel(hadc, &sConfig);  // Use the passed-in ADC (hadc1 or hadc3)

  HAL_ADC_Start(hadc);
  HAL_ADC_PollForConversion(hadc, 10);
  uint32_t value = HAL_ADC_GetValue(hadc);
  HAL_ADC_Stop(hadc);

  return value;
}

static float adc_to_voltage(uint32_t adc) { return (adc * VREF) / ADC_MAX; }

void measure_v(void) {
  uint32_t adc_12v = adc_read(&hadc1, ADC_CHANNEL_12); //ADC123_IN12
  uint32_t adc_battv = adc_read(&hadc1, ADC_CHANNEL_10); //ADC123_IN10
  uint32_t adc_19v = adc_read(&hadc1, ADC_CHANNEL_3);  //ADC123_IN3
  uint32_t adc_24v = adc_read(&hadc3, ADC_CHANNEL_14); //ADC3_IN14
  uint32_t adc_55v = adc_read(&hadc3, ADC_CHANNEL_8);  //ADC3_IN8

  float v_12v = adc_to_voltage(adc_12v) * DIV_12V;
  float v_battv = adc_to_voltage(adc_battv) * DIV_17V;
  float v_19v = adc_to_voltage(adc_19v) * DIV_19V;
  float v_24v = adc_to_voltage(adc_24v) * DIV_24V;
  float v_55v = adc_to_voltage(adc_55v) * DIV_55V;

  TRACE_INFO("adc_12V: %lu\r\n", adc_12v);
  TRACE_INFO("adc_24V: %lu\r\n", adc_24v);
  TRACE_INFO("adc_55V: %lu\r\n", adc_55v);
  TRACE_INFO("adc_battV: %lu\r\n", adc_battv);
  //TRACE_INFO("adc: %.2f \r\n", adc_to_voltage(adc_12v));
  //TRACE_INFO("12V: %.2f V\r\n", v_12v);
  //TRACE_INFO("BattV: %.2f V\r\n", v_battv);
  //TRACE_INFO("19V: %.2f V\r\n", v_19v);
  //TRACE_INFO("24V: %.2f V\r\n", v_24v);
  //TRACE_INFO("55V: %.2f V\r\n", v_55v);
}

// Andrew
void measure_batt(void) {
  // ADC Measure
  int raw_val = adc_read(&hadc1, ADC_CHANNEL_10);

  float pin_voltage = ((float)raw_val / 4095.0f) * 3.3f;
  float batt_voltage = pin_voltage * 16.67f;  // placeholder factor
  TRACE_INFO("%lf", batt_voltage);
  // BMS Measure (todo)

  TRACE_INFO("Batt: %.2f V\r\n", batt_voltage);
}

static float adc_to_current(uint16_t adc, float sensitivity) {
  float pin_v = (adc * VREF) / ADC_MAX;
  float centred_v = pin_v - VREF / 2.0f;
  return centred_v / sensitivity;
}

// Tanmay
void measure_a(void) {
  uint16_t adc_arm_motor = adc_read(&hadc1, ADC_CHANNEL_13);
  uint16_t adc_charger = adc_read(&hadc3, ADC_CHANNEL_9);
  uint16_t adc_batt = adc_read(&hadc3, ADC_CHANNEL_15);

  float i_arm_motor =
      adc_to_current(adc_arm_motor, 0.0133f);  // placeholder sensitivity
  float i_charger =
      adc_to_current(adc_charger, 0.0660f);          // placeholder sensitivity
  float i_batt = adc_to_current(adc_batt, 0.0110f);  // placeholder sensitivity

  TRACE_INFO("Currents - Arm+Motor: %.1fA, Charger: %.1fA, Batt: %.1fA\r\n",
               i_arm_motor, i_charger, i_batt);
}

// Turn off buses, arm, and motor
void shutoff_sequence(void) {
  // Turn off motor and arm
  motor_off();
  HAL_Delay(100);
  measure_a();
  measure_batt();
  arm_off();
  HAL_Delay(100);
  measure_a();
  measure_batt();

  // Turn off buses
  bus_55v_off();
  HAL_Delay(100);
  measure_v();
  bus_24v_off();
  HAL_Delay(100);
  measure_v();
  bus_19v_off();  // can remove if 19V not used, though it won't make a
                  // difference
  HAL_Delay(100);
  measure_v();
  bus_12v_off();
  HAL_Delay(100);
  measure_v();
  measure_a();
  measure_batt();  // if 5V bus powers sensor measure first
  bus_5v_off();
  HAL_Delay(100);
  measure_v();  // measure after to see if it shows value (depends if 5V powers
                // sensors)
  measure_a();
  measure_batt();
}

void power_sequence(void) {
  // 5V bus test
  bus_5v_on();  // works
  HAL_Delay(100);
  measure_v();
  bus_12v_on();  // works
  HAL_Delay(100);
  measure_v();
  // Skip 19V?
  bus_24v_on();  // works
  HAL_Delay(100);
  measure_v();
  bus_55v_on();  // works
  HAL_Delay(100);
  measure_v();

  // Turn on arm
  arm_on();  // doesn't work
  HAL_Delay(100);
  measure_a();
  measure_batt();

  // Turn on motor
  motor_on();
  HAL_Delay(100);
  measure_a();
  measure_batt();
}

void rsx_test(void) {
  // Ensure buses, arm, and motor are off
  shutoff_sequence();

  // Turn on all
  power_sequence();
  shutoff_sequence();
  power_sequence();
  shutoff_sequence();
  power_sequence();
  shutoff_sequence();
  power_sequence();
  shutoff_sequence();

  HAL_Delay(10000);
  osDelayTask(10000);

  // Turn off all
  shutoff_sequence();
}

void Estop_test(void) {
  // Ensure buses, arm, and motor are off
  shutoff_sequence();

  // Turn on all
  power_sequence();

  Estop_toggle();
  HAL_Delay(500);
  measure_v();
  measure_a();
  measure_batt();
  // Turn off all pins in case
  shutoff_sequence();
}


int parse_int(char_t *received_cmd, int *out)
{
    char *end;
    long val = strtol(received_cmd, &end, 10);

    if (end == received_cmd)  {
    	TRACE_INFO("parse_int end == received_cmd pointer");
    	return 1;          // no digits found
    }
    // allow trailing whitespace / CRLF
    while (*end == ' ' || *end == '\r' || *end == '\n' || *end == '\t') end++;
    if (*end != '\0')   {
    	TRACE_INFO("parse_int failed *end != '\\0' ");
    	return 1;        // extra junk in buffer
    }
    if (val < INT_MIN || val > INT_MAX) {
    	TRACE_INFO("parse_int failed val < INT_MIN || val > INT_MAX ");
    	return 1;              // overflow for int
    }

    *out = (int)val;
    //send the command to rsx_task
    //TODO: enable multiple commands at once
    xTaskNotify(rsx_task_handle, (1U << val) , eSetBits);
    return 0;                  // success
}

void rsxTask(void *param){
	uint32_t rsx_task_received;
	xTaskNotifyWait( 0, 0xFFFFFFFFUL,  &rsx_task_received, portMAX_DELAY);
	for (;;) {

        xTaskNotifyWait( 0, 0xFFFFFFFFUL,  &rsx_task_received, portMAX_DELAY);
        TRACE_INFO("rsx_task_received notify bits = 0x%lx\r\n", rsx_task_received);
        ///*
        if (rsx_task_received & ESTOP_CMD) 		Estop_toggle();
        if (rsx_task_received & MEASURE_V_CMD) 	measure_v();
        if (rsx_task_received & MEASURE_B_CMD){
            //xTaskNotify(spiSendTaskHandle, SPI_CMD_ADCV, eSetBits);
            //xTaskNotifyWait( 0, SPI_TX_DONE,  NULL, portMAX_DELAY);
        	measure_batt_bms(voltage_mv, 1);
            measure_batt();
        }
        if (rsx_task_received & MEASURE_A_CMD) 	measure_a();
        if (rsx_task_received & MOTOR_ON_CMD) 	motor_on();
        if (rsx_task_received & MOTOR_OFF_CMD) 	motor_off();
        if (rsx_task_received & ARM_ON_CMD) 	arm_on();
        if (rsx_task_received & ARM_OFF_CMD) 	arm_off();
        if (rsx_task_received & ON_5V_CMD) 		bus_5v_on();
        if (rsx_task_received & OFF_5V_CMD) 	bus_5v_off();
        if (rsx_task_received & ON_12V_CMD) 	bus_12v_on();
        if (rsx_task_received & OFF_12V_CMD) 	bus_12v_off();
        if (rsx_task_received & ON_24V_CMD) 	bus_24v_on();
        if (rsx_task_received & OFF_24V_CMD) 	bus_24v_off();
        if (rsx_task_received & ON_55V_CMD) 	bus_55v_on();
        if (rsx_task_received & OFF_55V_CMD) 	bus_55v_off();
//*/
//		//extern uint16_t voltage_mv;
//		measure_batt_bms(voltage_mv, 1);
//    //measure_batt();
//		if (voltage_mv[0]+voltage_mv[1]+voltage_mv[2]+voltage_mv[3] < 12000 && voltage_mv[0]+voltage_mv[1]+voltage_mv[2]+voltage_mv[3] != 0 ) {
//			//Estop_toggle();//12V UV
//			TRACE_INFO("UV! UV! UV!\r\n");
//			Estop_toggle();
//		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}
