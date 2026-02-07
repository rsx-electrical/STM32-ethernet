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

// Ruth
void measure_v(void) {}

// Andrew
float measure_batt(void) {
  // ADC Measure
  ADC_ChannelConfTypeDef sConfig = {0};
  int raw_val = 0;

  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    TRACE_ERROR("ADC Config Error\r\n");
    return;
  }

  HAL_ADC_Start(&hadc1);

  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
    raw_val = HAL_ADC_GetValue(&hadc1);
  }

  HAL_ADC_Stop(&hadc1);

  float pin_voltage = ((float)raw_val / 4095.0f) * 3.3f;
  float batt_voltage =
      pin_voltage * 16.67f;  // DON'T KNOW FACTOR VOLTAGE IS DIVIDED BY
  printf("%lf", batt_voltage);
  // BMS Measure (todo)

  return batt_voltage;
}

// Tanmay
void measure_a(void) {}

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
