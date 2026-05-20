#include "BMSlib.h"
#include "string.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "PEC.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx.h"
#include "debug.h"
#include "stm32f7xx_nucleo_144.h"
#include "error.h"



const uint8_t ADCVSC[4] =   {0x5,0x67,0x74,0x5A}; //cmd and pec of cell voltage and sc conversion pole
const uint8_t RDSTATB[12]=   {0x0,0x12,0x70,0x24,0,0,0,0,0,0,0,0};
const uint8_t RDCFG[12] = {0x0,0x2,0x2B,0xA,0,0,0,0,0,0,0,0};
const uint8_t WRCFG[12] = {0x0,0x1,0x3D,0x6E,0,0,0,0,0,0,0,0};
const uint8_t WRCFG_data_starter[6] = {0x7A,0,0,0,0,0x12}; //uint8_ts 1-3, 4 are set with discharge and uv/ov
const uint8_t RDCVA[12] ={0x0,0x4,0x7,0xC2,0,0,0,0,0,0,0,0};
const uint8_t RDCVB[12] = {0x0,0x6,0x9A,0x94,0,0,0,0,0,0,0,0};
const uint8_t ADCV[4] = {0x3, 0x60, 0xf4, 0x6c};
const uint8_t CLRCELL[4] = {0x7,0x11,0xc9,0xc0};
volatile uint8_t spiTransferComplete = 0;
volatile SPI_Command_t spiCmd = SPI_CMD_NONE;
 SPI_HandleTypeDef hspi3;
 TaskHandle_t  spiSendTaskHandle;
 TaskHandle_t  bmsTaskHandle;
 extern uint16_t batt_voltage_mv[NUMCELLS];

void measure_batt_bms(uint16_t* mv, int print){
  init_PEC15_Table();
  adcv();
  //measure Voltage - adcv may take some time
  uint16_t cell_voltage_100uV[NUMCELLS]; //rdvab fills this arrays with ints with units of 100uV
  HAL_Delay(5);
  rdvab(cell_voltage_100uV, NUMCELLS);

  if(print){
    HAL_Delay(100);

    for(int i=0; i < NUMCELLS; i++){
        mv[i] = cell_voltage_100uV[i] / 10; //get in mV
        	TRACE_INFO("C%0d=%d ", i, mv[i]);
        }
		TRACE_INFO("mV\r\n");

		TRACE_INFO("absolute voltages:");
		float tmp=0;
		for(int i=0; i < NUMCELLS; i++){
			TRACE_INFO("%2.3f ", tmp + (float)(cell_voltage_100uV[i]) / 10000);
			tmp = tmp + (float)(cell_voltage_100uV[i]) / 10000;
		}
		TRACE_INFO("V\r\n");
		TRACE_INFO("Total voltage: %3.3fV", tmp);

    }
}

void RSX_SPI_Init(void) {
	  hspi3.Instance = SPI3;
	  hspi3.Init.Mode = SPI_MODE_MASTER;
	  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	  hspi3.Init.CLKPolarity = SPI_POLARITY_HIGH;
	  hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
	  hspi3.Init.NSS = SPI_NSS_SOFT;
	  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
	  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	  hspi3.Init.CRCPolynomial = 10;

	  HAL_StatusTypeDef status3 = HAL_SPI_Init(&hspi3);
	  if (status3 != HAL_OK) TRACE_INFO("RSX: BMS HAL_SPI_Init failed\n");
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle){ //The HAL automatically calls HAL_SPI_MspInit(&hspi3) inside HAL_SPI_Init()
    GPIO_InitTypeDef GPIO_InitStruct = {0}, GPIO_InitStruct_CS = {0};

	if(spiHandle->Instance==SPI3)
		{
		/* Peripheral clock enable */
		__HAL_RCC_SPI3_CLK_ENABLE();

		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOC_CLK_ENABLE();
		/**SPI3 GPIO Configuration
		PB2     ------> SPI3_MOSI
		PC10     ------> SPI3_SCK
		PC11     ------> SPI3_MISO
		PC9      ------> SPI3_CS
		*/
		GPIO_InitStruct.Pin = SPI3_MOSI_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF7_SPI3;
		HAL_GPIO_Init(SPI3_MOSI_PORT, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = SPI3_MISO_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
		HAL_GPIO_Init(SPI3_MISO_PORT, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = SPI3_SCK_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
		HAL_GPIO_Init(SPI3_SCK_PORT, &GPIO_InitStruct);

        //config CS (PC9)
        GPIO_InitStruct_CS.Pin = SPI3_CS_PIN;
        GPIO_InitStruct_CS.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct_CS.Pull = GPIO_NOPULL;
        GPIO_InitStruct_CS.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(SPI3_CS_PORT, &GPIO_InitStruct_CS); //GPIO_C
        HAL_GPIO_WritePin(SPI3_CS_PORT, SPI3_CS_PIN, GPIO_PIN_SET);

	}

}

void rsxSpiSendTask(void *arg){
	uint32_t spi_task_received;
	init_PEC15_Table();
    for (;;) {
        // Wait until any SPI command is posted
        xTaskNotifyWait( 0, 0xFFFFFFFFUL,  &spi_task_received, portMAX_DELAY);
        TRACE_INFO("RSX: SPI send loop\n");
        if (spi_task_received & SPI_CMD_ADCV) {
        	measure_batt_bms(batt_voltage_mv, 1);
        }
        xTaskNotify(bmsTaskHandle, SPI_TX_DONE, eSetBits);
    }
}

void SPItransfer(const uint8_t* buffer, uint16_t size){ //send buffer
    HAL_GPIO_WritePin(SPI3_CS_PORT, SPI3_CS_PIN, GPIO_PIN_RESET);
	  HAL_SPI_Transmit(&hspi3, (uint8_t*)buffer, size, 100);
    HAL_GPIO_WritePin(SPI3_CS_PORT, SPI3_CS_PIN, GPIO_PIN_SET);
}

void SPItransferReceive(const uint8_t* buffer, uint8_t* rx, uint16_t size){
  //send buffer, receive rx at same time. rx and buffer have are "size" bytes
    HAL_GPIO_WritePin(SPI3_CS_PORT, SPI3_CS_PIN, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi3, (uint8_t*) buffer,rx, size, 100);
    HAL_GPIO_WritePin(SPI3_CS_PORT, SPI3_CS_PIN, GPIO_PIN_SET);
}


void print_buffer(uint8_t* buffer, int buffer_size){
  for (int i = 0; i < buffer_size; i++){
	  TRACE_INFO("%X ", buffer[i]);
  }
  TRACE_INFO("\r\n");
}

void adcv() {
	//TRACE_INFO("acdv\r\n");
  SPItransfer(ADCV, 4);
}

void wrcfg(uint8_t* data, int data_size){
  if (data_size > 6){
    TRACE_INFO("WRCFG failed: data too big!\n");
    return;
  }
  uint8_t txbuffer[12];
  for (int i = 0; i < 4; i++){//copy cmd & pec
    txbuffer[i] = WRCFG[i];
  }
  for (int i = 4; i <10; i++){//copy cmd & pec
    txbuffer[i] = data[i-4];
  }
  uint16_t pec = pec15((char*)data, data_size);
  uint8_t pec1 = pec & 0xFF;
  uint8_t pec0 = (uint8_t)(pec >> 8);
  txbuffer[10] = pec0; //copy pec
  txbuffer[11] = pec1; //copy pec

  SPItransfer(txbuffer, 12);//send and receive data at the same time
}


void rdvab(uint16_t* CV, int CVsize){ //get voltages, CV must has size of 6
 //4 uint8_ts sent, 8 uint8_ts received
  uint8_t rxbuffer_a[12], rxbuffer_b[12];
  SPItransferReceive(RDCVA,rxbuffer_a, 12);
  uint8_t datareceived_a[6];
  memcpy(datareceived_a, &rxbuffer_a[4], 6);
  if(!checkPEC(rxbuffer_a[10], rxbuffer_a[11], datareceived_a, 6)){
	  TRACE_INFO("PEC failed: ");
	  print_buffer(datareceived_a, sizeof(datareceived_a));
	  for (int i=0; i < sizeof(datareceived_a); i++){
		  datareceived_a[i]=0;
	  }
  }

  HAL_Delay(100);
/*
  TRACE_INFO("Received 1: 0x");
  print_buffer(rxbuffer_a, 12);
  */
  SPItransferReceive(RDCVB,rxbuffer_b, 12);
  uint8_t datareceived_b[6];
  memcpy(datareceived_b, &rxbuffer_b[4], 6);
  if(!checkPEC(rxbuffer_b[10], rxbuffer_b[11], datareceived_b, 6)){
	  TRACE_INFO("PEC failed: ");
	  print_buffer(datareceived_b, sizeof(datareceived_b));
	  for (int i=0; i < sizeof(datareceived_b); i++){
		  datareceived_b[i]=0;
	  }
  }
  HAL_Delay(100);
/*
  TRACE_INFO("Received 2: 0x");
  print_buffer(rxbuffer_b, 12);
  //*/
  CV[0] = (rxbuffer_a[5]<<8) | (rxbuffer_a[4] & 0xFF);
  CV[1] = (rxbuffer_a[7]<<8) | (rxbuffer_a[6] & 0xFF);
  CV[2] = (rxbuffer_a[9]<<8) | (rxbuffer_a[8] & 0xFF);
  CV[3] = (rxbuffer_b[5]<<8) | (rxbuffer_b[4] & 0xFF);
  CV[4] = (rxbuffer_b[7]<<8) | (rxbuffer_b[6] & 0xFF);
  CV[5] = (rxbuffer_b[9]<<8) | (rxbuffer_b[8] & 0xFF);


}

uint8_t checkPEC(uint8_t pec0_received, uint8_t pec1_received, uint8_t* data, int data_len){
  uint16_t expected_pec = pec15((char*)data, data_len);
  uint8_t expected_pec1 = expected_pec & 0xFF;
  uint8_t expected_pec0 = (uint8_t)(expected_pec >> 8);
  if (pec0_received == expected_pec0 && pec1_received == expected_pec1) {
    return 1;
  }
  else {
     TRACE_INFO("pec check failed\r\n");
     TRACE_INFO("exp pec0 %X; exp pec1 %X; rec pec0 %X; rec pec1 %X\r\n",
	       expected_pec0, expected_pec1, pec0_received, pec1_received);
    return 0;
  }
}

void dischargeCellX(uint8_t* data, int data_size, int cell_x){ //cell_x takes values 1 to 6, data is just any array of size 6
  if ((data_size > 6)||(cell_x > 6)||(cell_x < 1)){
    TRACE_INFO("discharge failed: bad arguments!");
    return;
  }

  for (int i = 0; i<data_size; i++){//CFGR0 & CFGR5
    data[i]=WRCFG_data_starter[i];
  }
  data[4] = 1<<(cell_x-1); //CFGR4
  
  wrcfg(data, data_size);
  // TRACE_INFO("discharge sent\n");
}


