#include <stdlib.h>
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"
#include "rsx.h"

void Estop_toggle(void) {
    
}

void motor_on(void) {
    HAL_GPIO_WritePin(MOTOR_EN_PORT, MOTOR_EN_PIN, GPIO_PIN_SET);
}

void motor_off(void) {
    HAL_GPIO_WritePin(MOTOR_EN_PORT, MOTOR_EN_PIN, GPIO_PIN_RESET);
}