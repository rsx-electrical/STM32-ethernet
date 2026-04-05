# WebSocket Client for NUCLEO-F767ZI
This project implements a WebSocket client on the TM32 NUCLEO-F767ZIboard using the CycloneTCP library.

<br>

## Overview
Websocket is TCP-based, bidirectional communication protocol over Ethernet between the STM32 and a PC host.  
The implementation is based on example code from **Oryx Embedded**:

https://www.oryx-embedded.com/products/CycloneTCP

Once running, the STM32 connects to a WebSocket server and can both send and receive messages in real time.

<br>

## Features
- **Target MCU:** STM32F767ZI (NUCLEO-F767ZI board)  
- **Network Stack:** CycloneTCP v2.5.4  
- **RTOS:** FreeRTOS  
- **Interface:** 10/100 Mbps Ethernet via onboard LAN8742 PHY  
- **Demo Behavior:**
  - Initializes Ethernet and TCP/IP stack on startup.  
  - On USER button press, opens a WebSocket connection to the PC.  
  - Sends “Hello World!” to the server.  
  - Receives and prints messages from the server.  

<br>
