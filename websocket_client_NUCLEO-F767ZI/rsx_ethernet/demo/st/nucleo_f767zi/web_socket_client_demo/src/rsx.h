#ifndef RSX_H
#define RSX_H

#include <stdlib.h>
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"

// Define the pins based on the diagram provided
#define MOTOR_EN_PORT  GPIOD
#define MOTOR_EN_PIN   GPIO_PIN_11
#define BUS_55V_PORT   GPIOG
#define BUS_55V_PIN    GPIO_PIN_9
#define ESTOP_PORT     GPIOG
#define ESTOP_PIN      GPIO_PIN_1

// Function Prototypes for MCU Functions
void motor_on(void);
void motor_off(void);
void Estop_toggle(void);
void measure_v(void); 
void measure_batt(void); // Combines BMS and ADC readings
void measure_a(void);
void arm_on(void);
void arm_off(void);
void x_bus_on(void);
void x_bus_off(void);

#endif