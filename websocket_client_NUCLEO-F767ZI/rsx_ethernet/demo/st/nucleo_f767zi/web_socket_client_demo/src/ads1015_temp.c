#include "ads1015_temp.h"
#include "os_port.h"   // osDelayTask, osCreateTask
#include "debug.h"     // TRACE_INFO, TRACE_ERROR

// -----------------------------------------------------------------------
// Globals readable from webSocketClientTest / buttonTask etc.
// -----------------------------------------------------------------------
int16_t  channel_temp_c[ADS1015_NUM_CHANNELS]    = {0};
uint16_t channel_voltage_mv[ADS1015_NUM_CHANNELS] = {0};

// -----------------------------------------------------------------------
// ADS1015 register addresses
// -----------------------------------------------------------------------
#define REG_CONVERSION  0x00
#define REG_CONFIG      0x01

// -----------------------------------------------------------------------
// Config register values for each single-ended channel
// Bit layout (16-bit, MSB first):
//   [15]    OS  = 1        (start single-shot conversion)
//   [14:12] MUX = 100/101/110/111 (AIN0..3 vs GND)
//   [11:9]  PGA = 010      (±2.048V, 1mV/LSB — covers MCP9700A 0-1.75V range)
//   [8]     MODE = 1       (single-shot)
//   [7:5]   DR  = 100      (1600 SPS — fast, low noise)
//   [4:0]   comparator disabled = 0b00011
//
// AIN0: MUX=100b  =>  config MSB = 1100 0010 = 0xC2,  LSB = 1000 0011 = 0x83
// AIN1: MUX=101b  =>  config MSB = 1101 0010 = 0xD2,  LSB = 0x83
// AIN2: MUX=110b  =>  config MSB = 1110 0010 = 0xE2,  LSB = 0x83
// AIN3: MUX=111b  =>  config MSB = 1111 0010 = 0xF2,  LSB = 0x83
// -----------------------------------------------------------------------
static const uint8_t config_msb[4] = { 0xC2, 0xD2, 0xE2, 0xF2 };
static const uint8_t config_lsb    = 0x83;

// -----------------------------------------------------------------------
// Read one channel from ADS1015
// Returns voltage in mV, or -1 on I2C error
// -----------------------------------------------------------------------
static int16_t ads1015_read_channel(I2C_HandleTypeDef *hi2c, uint8_t ch)
{
    uint8_t buf[3];
    int16_t raw;

    if (ch >= ADS1015_NUM_CHANNELS) return -1;

    // --- Write config register: trigger single-shot on this channel ---
    buf[0] = REG_CONFIG;
    buf[1] = config_msb[ch];
    buf[2] = config_lsb;
    if (HAL_I2C_Master_Transmit(hi2c, ADS1015_I2C_ADDR, buf, 3, 50) != HAL_OK)
    {
        TRACE_ERROR("ADS1015: I2C TX error ch%u\r\n", ch);
        return -1;
    }

    // --- Wait for conversion (1600 SPS = 0.625ms per sample, use 2ms) ---
    HAL_Delay(2);

    // --- Point register pointer to conversion register ---
    buf[0] = REG_CONVERSION;
    if (HAL_I2C_Master_Transmit(hi2c, ADS1015_I2C_ADDR, buf, 1, 50) != HAL_OK)
    {
        TRACE_ERROR("ADS1015: I2C ptr error ch%u\r\n", ch);
        return -1;
    }

    // --- Read 2 bytes ---
    if (HAL_I2C_Master_Receive(hi2c, ADS1015_I2C_ADDR, buf, 2, 50) != HAL_OK)
    {
        TRACE_ERROR("ADS1015: I2C RX error ch%u\r\n", ch);
        return -1;
    }

    // ADS1015 result is left-justified in 16 bits; actual 12-bit value is in [15:4]
    raw = (int16_t)((buf[0] << 8) | buf[1]);
    raw >>= 4;   // Shift to get 12-bit signed value

    // At PGA=±2.048V: 1 LSB = 1 mV
    return (raw > 0) ? raw : 0;
}

// -----------------------------------------------------------------------
// Convert ADS1015 mV reading to MCP9700A temperature
//   Formula: T(°C) = (Vout_mV - 500mV) / 10 mV/°C
// -----------------------------------------------------------------------
static int16_t mv_to_celsius(uint16_t mv)
{
    return (int16_t)((int32_t)(mv - MCP9700A_V0C_MV)) / MCP9700A_TC_MV_PER_C;
}

// -----------------------------------------------------------------------
// FreeRTOS task: reads all 4 channels every 500ms
// -----------------------------------------------------------------------
static void ads1015TempTask(void *param)
{
    extern I2C_HandleTypeDef hi2c1;  // Change to hi2c2/hi2c3 if needed

    TRACE_INFO("ADS1015: Temperature task started\r\n");

    while (1)
    {
        for (uint8_t ch = 0; ch < ADS1015_NUM_CHANNELS; ch++)
        {
            int16_t mv = ads1015_read_channel(&hi2c1, ch);

            if (mv >= 0)
            {
                channel_voltage_mv[ch] = (uint16_t)mv;
                channel_temp_c[ch]     = mv_to_celsius((uint16_t)mv);
            }
            else
            {
                // I2C error - keep last known value, log it
                TRACE_ERROR("ADS1015: Failed to read channel %u\r\n", ch);
            }
        }

        TRACE_INFO("ADS1015 Temps: CH0=%d°C  CH1=%d°C  CH2=%d°C  CH3=%d°C\r\n",
                   channel_temp_c[0], channel_temp_c[1],
                   channel_temp_c[2], channel_temp_c[3]);

        osDelayTask(500);
    }
}

// -----------------------------------------------------------------------
// Call this from main() after osInitKernel() to spin up the task
// -----------------------------------------------------------------------
void ads1015TempTaskCreate(void)
{
    OsTaskParameters taskParams = OS_TASK_DEFAULT_PARAMS;
    taskParams.stackSize = 300;
    taskParams.priority  = OS_TASK_PRIORITY_NORMAL;

    OsTaskId id = osCreateTask("TempSensor", ads1015TempTask, NULL, &taskParams);
    if (id == OS_INVALID_TASK_ID)
    {
        TRACE_ERROR("ADS1015: Failed to create temperature task!\r\n");
    }
}
