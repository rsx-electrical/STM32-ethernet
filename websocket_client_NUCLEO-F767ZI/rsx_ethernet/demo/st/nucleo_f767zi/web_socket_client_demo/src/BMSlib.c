#include "BMSlib.h"
#include "string.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "PEC.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx.h"
#include "debug.h"

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

extern SPI_HandleTypeDef hspi1;

void measure_batt_v(uint16_t* mv, int print){
  adcv();
  //measure Voltage - adcv may take some time
  uint16_t cell_voltage_100uV[NUMCELLS]; //rdvab fills this arrays with ints with units of 100uV
  rdvab(cell_voltage_100uV, NUMCELLS);

  if(print){
    HAL_Delay(100);

    TRACE_INFO("measured voltages 1 to 6:");
    for(int i=0; i < NUMCELLS; i++){
        mv[i] = cell_voltage_100uV[i] / 10; //get in mV
        TRACE_INFO("%d", mv[i]);
    }
    TRACE_INFO("mV\n");
  }
}

void RSX_SPI_Init(void) {
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
}

void SPItransfer(const uint8_t* buffer, uint16_t size){ //send buffer
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_Transmit(&hspi1, (uint8_t*)buffer, size, 100);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void SPItransferReceive(const uint8_t* buffer, uint8_t* rx, uint16_t size){
  //send buffer, receive rx at same time. rx and buffer have are "size" bytes
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_TransmitReceive(&hspi1, (uint8_t*) buffer,rx, size, 100);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void SPItransferDMA(const uint8_t* buffer, uint16_t size){ //send buffer
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*) buffer, size);
}

void SPItransferReceiveDMA(const uint8_t* buffer, uint8_t* rx, uint16_t size){ //send buffer
  spiTransferComplete = 0;
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t*) buffer,rx, size);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET);  // Wait for SPI to finish shifting out last bits
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  // CS HIGH
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET);  // Wait for SPI to finish shifting out last bits
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  // CS HIGH
  spiTransferComplete = 1;
}

void print_buffer(uint8_t* buffer, int buffer_size){
  for (int i = 0; i < buffer_size; i++){
	  TRACE_INFO("%X ", buffer[i]);
  }
  TRACE_INFO("\n");
}

void adcv() {
  SPItransferDMA(ADCV, 4);
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

  // TRACE_INFO("sent 0x");
  // print_buffer(txbuffer, 12);
  // TRACE_INFO("\n");
  SPItransferDMA(txbuffer, 12); //send and receive data at the same time
  while(!spiTransferComplete){}; //poll until spi is done
  // TRACE_INFO("wrcfg done\n");
}


void rdvab(uint16_t* CV, int CVsize){ //get voltages, CV must has size of 6
  if (CVsize < 6){
    TRACE_INFO("CV array too small\n");
    return;
  }
 //4 uint8_ts sent, 8 uint8_ts received
  uint8_t rxbuffer_a[12], rxbuffer_b[12];


  SPItransferReceiveDMA(RDCVA,rxbuffer_a, 12); //send and receive data at the same time
  while (!spiTransferComplete){}
  uint8_t datareceived_a[6];
  memcpy(datareceived_a, &rxbuffer_a[4], 6);
  if(!checkPEC(rxbuffer_a[10], rxbuffer_a[11], datareceived_a, 6)){
    TRACE_INFO("PEC failed\n");
  }
  /*
  TRACE_INFO("Received 1: 0x");
  print_buffer(rxbuffer_a, 12);
  //*/

  SPItransferReceiveDMA(RDCVB,rxbuffer_b, 12);
  while (!spiTransferComplete){}
  uint8_t datareceived_b[6];
  memcpy(datareceived_b, &rxbuffer_b[4], 6);
  if(!checkPEC(rxbuffer_b[10], rxbuffer_b[11], datareceived_b, 6)){
    TRACE_INFO("PEC failed\n");
  }
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

  CV[0] += 7000; //Calibration
  CV[5] += 7000; //Calibration

}

uint8_t checkPEC(uint8_t pec0_received, uint8_t pec1_received, uint8_t* data, int data_len){
  uint16_t expected_pec = pec15((char*)data, data_len);
  uint8_t expected_pec1 = expected_pec & 0xFF;
  uint8_t expected_pec0 = (uint8_t)(expected_pec >> 8);
  if (pec0_received == expected_pec0 && pec1_received == expected_pec1) {
    return 1;
  }
  else {
     //TRACE_INFO("pec check failed\n");
    // TRACE_INFO("exp pec0 %X; exp pec1 %X; rec pec0 %X; rec pec1 %X\n",
	    //   expected_pec0, expected_pec1, pec0_received, pec1_received);
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
