#include "BMSlib.h"
#include "string.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "PEC.h"

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
extern UART_HandleTypeDef huart3;

void measure_batt_v(uint16_t* mv, int print){
  adcv();
  //measure Voltage - adcv may take some time
  uint16_t cell_voltage_100uV[NUMCELLS]; //rdvab fills this arrays with ints with units of 100uV
  rdvab(cell_voltage_100uV, NUMCELLS);

  if(print){
    HAL_Delay(100);

    print_msg("measured voltages 1 to 6:");
    for(int i=0; i < NUMCELLS; i++){
        mv[i] = cell_voltage_100uV[i] / 10; //get in mV
        sprintf(msg, "%d ", mv[i]);
        print_msg(msg);
    }
    print_msg("mV\n");
  }
}

void SPItransfer(const uint8_t* buffer, uint16_t size){ //send buffer
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_Transmit(&hspi1, buffer, size, 100);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}


void SPItransferReceive(const uint8_t* buffer, uint8_t* rx, uint16_t size){
  //send buffer, receive rx at same time. rx and buffer have are "size" bytes
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_TransmitReceive(&hspi1, buffer,rx, size, 100);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void SPItransferDMA(const uint8_t* buffer, uint16_t size){ //send buffer
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_Transmit_DMA(&hspi1, buffer, size);
}

void SPItransferReceiveDMA(const uint8_t* buffer, uint8_t* rx, uint16_t size){ //send buffer
  spiTransferComplete = 0;
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // CS LOW
  HAL_SPI_TransmitReceive_DMA(&hspi1, buffer,rx, size);
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

void print_msg(char * msg) {
  HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), 100);
}

void print_buffer(uint8_t* buffer, int buffer_size){
  char msg[100];
  for (int i = 0; i < buffer_size; i++){
	  sprintf(msg, "%X ", buffer[i]);
	  print_msg(msg);
  }
  print_msg("\n");
}

void adcv() {
  SPItransferDMA(ADCV, 4);
}

void wrcfg(uint8_t* data, int data_size){
  if (data_size > 6){
    print_msg("WRCFG failed: data too big!\n");
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

  // print_msg("sent 0x");
  // print_buffer(txbuffer, 12);
  // print_msg("\n");
  SPItransferDMA(txbuffer, 12); //send and receive data at the same time
  while(!spiTransferComplete){}; //poll until spi is done
  // print_msg("wrcfg done\n");
}


void rdvab(uint16_t* CV, int CVsize){ //get voltages, CV must has size of 6
  if (CVsize < 6){
    print_msg("CV array too small\n");
    return;
  }
 //4 uint8_ts sent, 8 uint8_ts received
  uint8_t rxbuffer_a[12], rxbuffer_b[12];


  SPItransferReceiveDMA(RDCVA,rxbuffer_a, 12); //send and receive data at the same time
  while (!spiTransferComplete){}
  uint8_t datareceived_a[6];
  memcpy(datareceived_a, &rxbuffer_a[4], 6);
  if(!checkPEC(rxbuffer_a[10], rxbuffer_a[11], datareceived_a, 6)){
    print_msg("PEC failed\n");
  }
  /*
  print_msg("Received 1: 0x");
  print_buffer(rxbuffer_a, 12);
  //*/

  SPItransferReceiveDMA(RDCVB,rxbuffer_b, 12);
  while (!spiTransferComplete){}
  uint8_t datareceived_b[6];
  memcpy(datareceived_b, &rxbuffer_b[4], 6);
  if(!checkPEC(rxbuffer_b[10], rxbuffer_b[11], datareceived_b, 6)){
    print_msg("PEC failed\n");
  }
  /*
  print_msg("Received 2: 0x");
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
  char toprint[100];
  if (pec0_received == expected_pec0 && pec1_received == expected_pec1) {
    return 1;
  }
  else {
     //print_msg("pec check failed\n");
    // sprintf(toprint,"exp pec0 %X; exp pec1 %X; rec pec0 %X; rec pec1 %X\n",
    //   expected_pec0, expected_pec1, pec0_received, pec1_received);
    // print_msg(toprint);
    return 0;
  }
}

void dischargeCellX(uint8_t* data, int data_size, int cell_x){ //cell_x takes values 1 to 6, data is just any array of size 6
  if ((data_size > 6)||(cell_x > 6)||(cell_x < 1)){
    print_msg("discharge failed: bad arguments!");
    return;
  }

  for (int i = 0; i<data_size; i++){//CFGR0 & CFGR5
    data[i]=WRCFG_data_starter[i];
  }
  data[4] = 1<<(cell_x-1); //CFGR4
  
  wrcfg(data, data_size);
  // print_msg("discharge sent\n");
}
