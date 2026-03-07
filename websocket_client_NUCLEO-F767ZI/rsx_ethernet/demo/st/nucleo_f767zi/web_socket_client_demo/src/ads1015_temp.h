//talk to ADS1015 via I²C

//read AIN0

//convert to volts

//convert to °C

#ifndef ADS1015_TEMP_H
#define ADS1015_TEMP_H

#include "stm32f7xx_hal.h"
#include <stdint.h>

// ADS1015: ADDR pin -> GND = 0x48 (7-bit), shifted left for HAL
#define ADS1015_I2C_ADDR        (0x48 << 1)

// Number of MCP9700A sensors connected (AIN0..AIN3)
#define ADS1015_NUM_CHANNELS    4

// MCP9700A transfer function constants (from datasheet)
// VOUT = 500mV + 10mV/C  =>  T = (VOUT_mV - 500) / 10
#define MCP9700A_V0C_MV         500     // Output voltage at 0°C in mV
#define MCP9700A_TC_MV_PER_C    10      // Temperature coefficient mV/°C

// Global results — read these from anywhere (webSocket task etc.)
extern int16_t  channel_temp_c[ADS1015_NUM_CHANNELS];   // Temperature in °C
extern uint16_t channel_voltage_mv[ADS1015_NUM_CHANNELS]; // Raw voltage in mV

// Call once to create the FreeRTOS polling task
void ads1015TempTaskCreate(void);

#endif // ADS1015_TEMP_H
