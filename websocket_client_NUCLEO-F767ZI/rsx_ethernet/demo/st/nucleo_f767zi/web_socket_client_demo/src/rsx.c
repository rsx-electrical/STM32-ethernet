#include "rsx.h"

#include <stdlib.h>

#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"

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