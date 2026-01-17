#include "rsx.h"

int main(void) {
    HAL_Init();            // Resets all peripherals and initializes Flash interface
    SystemClock_Config();  // Sets the speed of the processor
    MX_GPIO_Init();        // Powers up the pins you configured
    MX_ADC1_Init();        // Powers up the voltage/current sensors
    MX_SPI1_Init();        // Powers up the BMS communication

    while (1) {

    }
}