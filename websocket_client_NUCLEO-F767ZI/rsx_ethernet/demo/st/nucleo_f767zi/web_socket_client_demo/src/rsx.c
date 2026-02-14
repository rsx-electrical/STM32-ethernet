#include "rsx.h"

#include <stdlib.h>

#include "debug.h"

extern ADC_HandleTypeDef hadc1;

void Estop_toggle(void) {
  HAL_GPIO_WritePin(ESTOP_PORT, ESTOP_PIN, GPIO_PIN_SET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(ESTOP_PORT, ESTOP_PIN, GPIO_PIN_RESET);
}

void arm_on(void) { HAL_GPIO_WritePin(ARM_EN_PORT, ARM_EN_PIN, GPIO_PIN_SET); }

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
	HAL_GPIO_WritePin(LED_SIGNAL_PORT, LED_G_PIN, GPIO_PIN_SET);
}

void LED_G_off() {
	HAL_GPIO_WritePin(LED_SIGNAL_PORT, LED_G_PIN, GPIO_PIN_RESET);
}

void LED_B_on() {
	HAL_GPIO_WritePin(LED_SIGNAL_PORT, LED_B_PIN, GPIO_PIN_SET);
}

void LED_B_off() {
	HAL_GPIO_WritePin(LED_SIGNAL_PORT, LED_B_PIN, GPIO_PIN_RESET);
}

void LED_R_on() {
	HAL_GPIO_WritePin(LED_SIGNAL_PORT, LED_R_PIN, GPIO_PIN_SET);
}

void LED_R_off() {
	HAL_GPIO_WritePin(LED_SIGNAL_PORT, LED_R_PIN, GPIO_PIN_RESET);
}

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;

// Ruth
static uint16_t adc_read(ADC_HandleTypeDef *hadc, uint32_t channel) {
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  //sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5; //TODO: this was erroring out, coco commented this out

  HAL_ADC_ConfigChannel(hadc,
                        &sConfig);  // Use the passed-in ADC (hadc1 or hadc3)

  HAL_ADC_Start(hadc);
  HAL_ADC_PollForConversion(hadc, 10);
  uint16_t value = HAL_ADC_GetValue(hadc);
  HAL_ADC_Stop(hadc);

  return value;
}

static float adc_to_voltage(uint16_t adc) { return (adc * VREF) / ADC_MAX; }

void measure_v(void) {
  uint16_t adc_12v = adc_read(&hadc1, ADC_CHANNEL_12);
  uint16_t adc_17v = adc_read(&hadc1, ADC_CHANNEL_10);
  uint16_t adc_19v = adc_read(&hadc1, ADC_CHANNEL_3);
  uint16_t adc_24v = adc_read(&hadc1, ADC_CHANNEL_14);
  uint16_t adc_55v = adc_read(&hadc1, ADC_CHANNEL_8);

  float v_12v = adc_to_voltage(adc_12v) * DIV_12V;
  float v_17v = adc_to_voltage(adc_17v) * DIV_17V;
  float v_19v = adc_to_voltage(adc_19v) * DIV_19V;
  float v_24v = adc_to_voltage(adc_24v) * DIV_24V;
  float v_55v = adc_to_voltage(adc_55v) * DIV_55V;

  TRACE_INFO("12V: %.2f V\r\n", v_12v);
  TRACE_INFO("17V: %.2f V\r\n", v_17v);
  TRACE_INFO("19V: %.2f V\r\n", v_19v);
  TRACE_INFO("24V: %.2f V\r\n", v_24v);
  TRACE_INFO("55V: %.2f V\r\n", v_55v);
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
