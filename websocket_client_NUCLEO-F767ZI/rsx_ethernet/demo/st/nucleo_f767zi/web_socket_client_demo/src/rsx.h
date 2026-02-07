#ifndef RSX_H
#define RSX_H

#include <stdlib.h>

#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"

typedef struct {
  float v_5v, v_12v, v_19v, v_24v, v_55v;
  float v_batt_adc, v_batt_bms;
  float i_motor, i_charger, i_batt;
} Measurements;

// Define the pins based on the diagram provided
#define MOTOR_EN_PORT GPIOD
#define MOTOR_EN_PIN GPIO_PIN_11

#define ARM_EN_PORT GPIOE
#define ARM_EN_PIN GPIO_PIN_6

#define BUS_5V_PORT GPIOF
#define BUS_5V_PIN GPIO_PIN_15
#define BUS_12V_PORT GPIOD
#define BUS_12V_PIN GPIO_PIN_13
#define BUS_19V_PORT GPIOF
#define BUS_19V_PIN GPIO_PIN_14
#define BUS_24V_PORT GPIOB
#define BUS_24V_PIN GPIO_PIN_6
#define BUS_55V_PORT GPIOB
#define BUS_55V_PIN GPIO_PIN_2

#define ESTOP_PORT GPIOG
#define ESTOP_PIN GPIO_PIN_1

#define MEASURE_5V_PORT GPIOB
#define MEASURE_5V_PIN GPIO_PIN_1
#define MEASURE_12V_PORT GPIOC
#define MEASURE_12V_PIN GPIO_PIN_2
#define MEASURE_19V_PORT GPIOA
#define MEASURE_19V_PIN GPIO_PIN_3
#define MEASURE_24V_PORT GPIOF
#define MEASURE_24V_PIN GPIO_PIN_4
#define MEASURE_55V_PORT GPIOF
#define MEASURE_55V_PIN GPIO_PIN_10

#define MEASURE_BATT_V_PORT GPIOC
#define MEASURE_BATT_V_PIN GPIO_PIN_0

#define MEASURE_MOTOR_A_PORT GPIOC
#define MEASURE_MOTOR_A_PIN GPIO_PIN_3
#define MEASURE_CHARGER_A_PORT GPIOF
#define MEASURE_CHARGER_A_PIN GPIO_PIN_3
#define MEASURE_BATT_A_PORT GPIOF
#define MEASURE_BATT_A_PIN GPIO_PIN_5

#define ADC_MAX     4095.0f
#define VREF        3.3f

// Divider Factors
#define DIV_12V  4.00
#define DIV_17V  17.0/3
#define DIV_19V  19.0/3
#define DIV_24V  8.00
#define DIV_55V  55.0/3

// Function Prototypes for MCU Functions
void motor_on(void);
void motor_off(void);
void Estop_toggle(void);
void measure_v(void);
void measure_batt(void);  // Combines BMS and ADC readings
void measure_a(void);
void arm_on(void);
void arm_off(void);
void bus_5v_on(void);
void bus_5v_off(void);
void bus_12v_on(void);
void bus_12v_off(void);
void bus_19v_on(void);
void bus_19v_off(void);
void bus_24v_on(void);
void bus_24v_off(void);
void bus_55v_on(void);
void bus_55v_off(void);
void Estop_test(void);
void rsx_test(void);

#endif
