#ifndef RSX_H
#define RSX_H

#include <stdlib.h>

#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"
#include "os_port.h"
#include "BMSlib.h"

// not really using Measurements rn
typedef struct {
  float v_5v, v_12v, v_19v, v_24v, v_55v;
  float v_batt_adc, v_batt_bms;
  float i_arm_motor, i_charger, i_batt;
} Measurements;

// Define the pins based on the diagram provided
#define MOTOR_EN_PORT GPIOD
#define MOTOR_EN_PIN GPIO_PIN_11

#define LED_GB_PORT GPIOG
#define LED_G_PIN GPIO_PIN_2
#define LED_B_PIN GPIO_PIN_3

#define LED_R_PORT GPIOF
#define LED_R_PIN GPIO_PIN_12

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

#define ADC_MAX 4095.0f
#define VREF 3.3f

// Divider Factors
#define DIV_12V 4.00
#define DIV_17V 17.0 / 3
#define DIV_19V 19.0 / 3
#define DIV_24V 8.00
#define DIV_55V 55.0 / 3

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
void shutoff_sequence(void);
int parse_int(char_t *received_cmd, int *out);
void rsxTask(void *param);

extern TaskHandle_t  rsx_task_handle;

#define ESTOP_CMD	(1U << 0)
#define MEASURE_V_CMD (1U << 1)
#define	MEASURE_B_CMD (1U << 2)
#define	MEASURE_A_CMD (1U << 3)
#define	MOTOR_ON_CMD (1U << 4)
#define	MOTOR_OFF_CMD (1U << 5)
#define	ARM_ON_CMD (1U << 6)
#define	ARM_OFF_CMD (1U << 7)
#define	ON_5V_CMD (1U << 8)
#define	ON_12V_CMD (1U << 9)
#define	ON_24V_CMD (1U << 10)
#define	ON_55V_CMD (1U << 11)
#define	OFF_5V_CMD (1U << 12)
#define	OFF_12V_CMD (1U << 13)
#define	OFF_24V_CMD (1U << 14)
#define	OFF_55V_CMD (1U << 15)


#endif
