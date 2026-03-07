/**
 * @file main.c
 * @brief Main routine
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.4
 **/

// Dependencies
#include <stdlib.h>

#include "core/net.h"
#include "debug.h"
#include "dhcp/dhcp_client.h"
#include "drivers/mac/stm32f7xx_eth_driver.h"
#include "drivers/phy/lan8742_driver.h"
#include "hardware/stm32f7xx/stm32f7xx_crypto.h"
#include "ipv6/slaac.h"
#include "rng/hmac_drbg.h"
#include "rng/trng.h"
#include "rsx.h"
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_nucleo_144.h"
#include "web_socket/web_socket.h"
#include "BMSlib.h"
#include "ads1015_temp.h"

// Ethernet interface configuration
#define APP_IF_NAME "eth0"
#define APP_HOST_NAME "test"
#define APP_MAC_ADDR "02-00-00-00-00-01"

#define APP_USE_DHCP_CLIENT DISABLED
#define APP_IPV4_HOST_ADDR "192.168.0.20"
#define APP_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IPV4_DEFAULT_GATEWAY "192.168.0.254"
#define APP_IPV4_PRIMARY_DNS "8.8.8.8"
#define APP_IPV4_SECONDARY_DNS "8.8.8.4"

#define APP_USE_SLAAC DISABLED
#define APP_IPV6_LINK_LOCAL_ADDR "fe80::767"
#define APP_IPV6_PREFIX "2001:db8::"
#define APP_IPV6_PREFIX_LENGTH 64
#define APP_IPV6_GLOBAL_ADDR "2001:db8::767"
#define APP_IPV6_ROUTER "fe80::1"
#define APP_IPV6_PRIMARY_DNS "2001:4860:4860::8888"
#define APP_IPV6_SECONDARY_DNS "2001:4860:4860::8844"

// Application configuration
#define APP_WS_SERVER_NAME "192.168.0.10"
#define APP_WS_SERVER_PORT 8080
#define APP_WS_SERVER_URI "/"

#define APP_SET_CIPHER_SUITES DISABLED
#define APP_SET_SERVER_NAME DISABLED
#define APP_SET_TRUSTED_CA_LIST DISABLED

// Global variables
OsEvent appEvent;
bool_t buttonEventFlag;

DhcpClientSettings dhcpClientSettings;
DhcpClientContext dhcpClientContext;
SlaacSettings slaacSettings;
SlaacContext slaacContext;
HmacDrbgContext hmacDrbgContext;
uint8_t seed[48];
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc3;
TaskHandle_t  rsx_task_handle;
uint16_t voltage_mv[NUMCELLS];

/**
 * @brief System clock configuration
 **/



void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // Enable Power Control clock
  __HAL_RCC_PWR_CLK_ENABLE();

  // Enable HSE Oscillator and activate PLL with HSE as source
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  // Enable overdrive mode
  HAL_PWREx_EnableOverDrive();

  // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
  // clocks dividers
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
}

/**
 * @brief MPU configuration
 **/

void MPU_Config(void) {
  MPU_Region_InitTypeDef MPU_InitStruct;

  // Disable MPU
  HAL_MPU_Disable();

  // SRAM2 (no cache)
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x2007C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.SubRegionDisable = 0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  // Enable MPU
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
 * @brief Random data generation callback function
 * @param[out] data Buffer where to store the random data
 * @param[in] length Number of bytes that are required
 * @return Error code
 **/

error_t webSocketClientRngCallback(uint8_t *data, size_t length) {
  error_t error;

  // Generate some random data
  error = hmacDrbgGenerate(&hmacDrbgContext, data, length);
  // Return status code
  return error;
}

/**
 * @brief TLS initialization callback
 * @param[in] webSocket Handle referencing a WebSocket
 * @param[in] tlsContext Pointer to the TLS context
 * @return Error code
 **/

error_t webSocketClientTlsInitCallback(WebSocket *webSocket,
                                       TlsContext *tlsContext) {
  error_t error;

  // Debug message
  TRACE_INFO("WebSocket: TLS initialization callback\r\n");

  // Set the PRNG algorithm to be used
  error = tlsSetPrng(tlsContext, HMAC_DRBG_PRNG_ALGO, &hmacDrbgContext);
  // Any error to report?
  if (error) return error;

#if (APP_SET_CIPHER_SUITES == ENABLED)
  // Preferred cipher suite list
  error = tlsSetCipherSuites(tlsContext, cipherSuites, arraysize(cipherSuites));
  // Any error to report?
  if (error) return error;
#endif

#if (APP_SET_SERVER_NAME == ENABLED)
  // Set the fully qualified domain name of the server
  error = tlsSetServerName(tlsContext, APP_WS_SERVER_NAME);
  // Any error to report?
  if (error) return error;
#endif

#if (APP_SET_TRUSTED_CA_LIST == ENABLED)
  // Import the list of trusted CA certificates
  error = tlsSetTrustedCaList(tlsContext, trustedCaList, strlen(trustedCaList));
  // Any error to report?
  if (error) return error;
#endif

  // Successful processing
  return NO_ERROR;
}

/**
 * @brief WebSocket client test routine
 * @return Error code
 **/

error_t webSocketClientTest(void) {
  error_t error;
  size_t length;
  systime_t timestamp;
  systime_t currentTime;
  WebSocket *webSocket;
  WebSocketFrameType type;
  IpAddr serverIpAddr;
  SocketEventDesc eventDesc[1];
  char_t buffer[256];
  int cmd_received;

  // Debug message
  TRACE_INFO("\r\n\r\nWebSocket: Resolving server name...\r\n");

  // Resolve server name
  error = getHostByName(NULL, APP_WS_SERVER_NAME, &serverIpAddr, 0);

  // Any error to report?
  if (error) {
    // Debug message
    TRACE_INFO("WebSocket: Failed to resolve server name!\r\n");
    // Exit immediately
    return error;
  }

  // Open a new WebSocket
  webSocket = webSocketOpen();
  // Failed to open WebSocket?
  if (webSocket == NULL) return ERROR_OPEN_FAILED;

  // Start of exception handling block
  do {
    // Set timeout value for blocking operations
    error = webSocketSetTimeout(webSocket, 30000);
    // Any error to report?
    if (error) break;

    // Set the hostname of the remote server
    error = webSocketSetHost(webSocket, APP_WS_SERVER_NAME);
    // Any error to report?
    if (error) break;

#if (APP_WS_SERVER_PORT == 443)
    // Register TLS initialization callback
    error = webSocketRegisterTlsInitCallback(webSocket,
                                             webSocketClientTlsInitCallback);
    // Any error to report?
    if (error) return error;
#endif

    // Debug message
    TRACE_INFO("\r\nWebSocket: Connecting to server...\r\n");

    // Establish the WebSocket connection
    error = webSocketConnect(webSocket, &serverIpAddr, APP_WS_SERVER_PORT,
                             APP_WS_SERVER_URI);
    // Any error to report?
    if (error) break;

    // Debug message
    TRACE_INFO("WebSocket: Connected to server\r\n");

    // Format message
    length = sprintf(buffer, "Hello World!");

    // Debug message
    TRACE_INFO("WebSocket: Sending message (%" PRIuSIZE " bytes)...\r\n",
               length);
    TRACE_INFO("  %s\r\n", buffer);
 //coco
    /*
    for (;;){
    	//coco
    	length = sprintf(buffer, "%4d,%4d,%4d,%4d", voltage_mv[0], voltage_mv[1], voltage_mv[2], voltage_mv[3] );
    	TRACE_INFO("sending  %s\r\n", buffer);
    	if (voltage_mv[0]+voltage_mv[1]+voltage_mv[2]+voltage_mv[3] < 12000 && voltage_mv[0]+voltage_mv[1]+voltage_mv[2]+voltage_mv[3] != 0 ) {
    		//Estop_toggle();//12V UV
    		TRACE_INFO("UV! UV! UV!\r\n");
    		Estop_toggle();
    	}



    // Send data to the WebSocket server
    	error = webSocketSend(webSocket, buffer, length, WS_FRAME_TYPE_TEXT, NULL);

    // Any error to report?
    	if (error) {
    		TRACE_INFO("oops broke sender ethernet");
    		break;
    	}
    	vTaskDelay(pdMS_TO_TICKS(500));

	}
	*/
    // Save current time
    timestamp = osGetSystemTime();

    // Process events
    while (1) {
      // Set the events the application is interested in
      eventDesc[0].socket = webSocket->socket;
      eventDesc[0].eventMask = SOCKET_EVENT_RX_READY;

      // Check whether application data are pending in the receive buffer
      if (webSocketIsRxReady(webSocket)) {
        // No need to poll the underlying socket for incoming traffic...
        eventDesc[0].eventFlags = SOCKET_EVENT_RX_READY;
        error = NO_ERROR;
      } else {
        // Wait for incoming traffic from the remote host. A non-infinite
        // timeout is provided to manage the idle timeout (60s). refer to the
        // end of the loop
        error = socketPoll(eventDesc, arraysize(eventDesc), &appEvent, 1000);
      }

      // Check status code
      if (error == NO_ERROR || error == ERROR_WAIT_CANCELED) {
        if (eventDesc[0].eventFlags & SOCKET_EVENT_RX_READY) {
          // Receive data from the remote WebSocket server
          error = webSocketReceive(webSocket, buffer, sizeof(buffer) - 1, &type,
                                   &length);
          // Any error to report?
          if (error) break;

          // Check the type of received data
          if (type == WS_FRAME_TYPE_TEXT || type == WS_FRAME_TYPE_BINARY) {
            // Properly terminate the string with a NULL character
            buffer[length] = '\0';

            // Debug message
            TRACE_INFO("WebSocket: Message received (%" PRIuSIZE
                       " bytes)...\r\n",
                       length);
            TRACE_INFO("received: \"%s\"\r\n", buffer);
            //parse buffer for command and set events
            if (parse_int(buffer, &cmd_received)) {
            	TRACE_INFO("failed parsing cmd");
            }

          } else if (type == WS_FRAME_TYPE_PING) {
            // Debug message
            TRACE_INFO("WebSocket: Ping message received (%" PRIuSIZE
                       " bytes)...\r\n",
                       length);
            // Debug message
            TRACE_INFO("WebSocket: Sending pong message (%" PRIuSIZE
                       " bytes)...\r\n",
                       length);

            // Send a Pong frame in response
            error = webSocketSend(webSocket, buffer, length, WS_FRAME_TYPE_PONG,
                                  NULL);
            // Any error to report?
            if (error) break;
          }

          // Save current time
          timestamp = osGetSystemTime();
        }
      }

       // Check user  state
       if (buttonEventFlag) {
         // Clear flag
         buttonEventFlag = FALSE;

         // Format event message
         length = sprintf(buffer, "User button pressed!");
		length = sprintf(buffer, "%4d,%4d,%4d,%4d", voltage_mv[0], voltage_mv[1], voltage_mv[2], voltage_mv[3] );
		TRACE_INFO("sending  %s\r\n", buffer);



         // Debug message
         TRACE_INFO("WebSocket: Sending message (%" PRIuSIZE " bytes)...\r\n",
                    length);
         TRACE_INFO("  %s\r\n", buffer);

         // Send a message to the WebSocket server
         error =
             webSocketSend(webSocket, buffer, length, WS_FRAME_TYPE_TEXT,
             NULL);
         // Any error to report?
         if (error) break;

         // Save current time
         timestamp = osGetSystemTime();
       }

      // Get current time
      currentTime = osGetSystemTime();

      // Manage idle timeout (if applicable)
      if (timeCompare(currentTime, timestamp + 60000) >= 0) {
        // Close WebSocket connection
        error = NO_ERROR;
        break;
      }
    }

    // Properly close the WebSocket connection
    webSocketShutdown(webSocket);

    // End of exception handling block
  } while (0);

  // Release the WebSocket
  webSocketClose(webSocket);

  // Return error code
  return error;
}

/**
 * @brief User task
 * @param[in] param Unused parameter
 **/

void userTask(void *param) {
  // Endless loop
  while (1) {
    // Wait for the user button to be pressed
    osWaitForEvent(&appEvent, INFINITE_DELAY);

    // Clear flag
    buttonEventFlag = FALSE;

    // WebSocket client test routine
    webSocketClientTest();
  }
}

/**
 * @brief Button task
 **/

void buttonTask(void *param) {
  bool_t state = FALSE;
  bool_t prevState = FALSE;

  // Endless loop
  while (1) {
    // Retrieve user button state
    state = BSP_PB_GetState(BUTTON_KEY);

    // User button pressed?
    if (state && !prevState) {
      // Notify the WebSocket task of the event
      buttonEventFlag = TRUE;
      osSetEvent(&appEvent);
    }

    // Save current state
    prevState = state;

    // Loop delay
    osDelayTask(100);
  }
}

/**
 * @brief LED task
 * @param[in] param Unused parameter
 **/

void ledTask(void *param) {
  // Endless loop
  while (1) {
    BSP_LED_On(LED1);
    osDelayTask(100);
    BSP_LED_Off(LED1);
    osDelayTask(900);
  }
}

/**
 * @brief RSX task
 **/
/*
//COco made a testing version of this function in rsx.c
void rsxTask(void *param) {
  rsx_test();
  osDelayTask(10000);
  Estop_test();
  osDeleteTask(NULL);
}
//*/

#define STATE_IDLE 0
#define ENABLE_5 1
#define ENABLE_12 2
#define ENABLE_19 3
#define ENABLE_24 4
#define ENABLE_55 5
#define ENABLE_ARM 6
#define ENABLE_MOTOR 7

void rsxButtonTask(void *param) {
  int button_value = 0;
  while (1) {
    //if (buttonEventFlag) {
      // Clear flag
      //buttonEventFlag = FALSE;
      // Check user button state
      //switch (button_value) {
//        case STATE_IDLE:
//          bus_5v_on();
//          BSP_LED_On(LED1);
//          TRACE_INFO("RSX: 5V Enabled\r\n");
//          button_value = ENABLE_5;
//          break;
//
//        case ENABLE_5:
//          bus_12v_on();
//          BSP_LED_Off(LED1);
//
//          TRACE_INFO("RSX: 12V Enabled\r\n");
//          button_value = ENABLE_12;
//          break;
//
//        case ENABLE_12:
//          bus_19v_on();
//          BSP_LED_On(LED1);
//          TRACE_INFO("RSX: 19V Enabled\r\n");
//          button_value = ENABLE_19;
//          break;
//
//        case ENABLE_19:
//          bus_24v_on();
//          BSP_LED_Off(LED1);
//
//          TRACE_INFO("RSX: 24V Enabled\r\n");
//          button_value = ENABLE_24;
//          break;
//
//        case ENABLE_24:
//          bus_55v_on();
//          BSP_LED_On(LED1);
//
//          TRACE_INFO("RSX: 55V Enabled\r\n");
//          button_value = ENABLE_55;
//          break;
//
//        case ENABLE_55:
//          arm_on();
//          BSP_LED_Off(LED1);
//          TRACE_INFO("RSX: Arm Enabled\r\n");
//          button_value = ENABLE_ARM;
//          break;
//
//        case ENABLE_ARM:
//          motor_on();
//          BSP_LED_On(LED1);
//          TRACE_INFO("RSX: Motor Enabled\r\n");
//          button_value = ENABLE_MOTOR;
//          break;
//
//        case ENABLE_MOTOR:
//          shutoff_sequence();
//          BSP_LED_Off(LED1);
//          TRACE_INFO("RSX: All Systems Off\r\n");
//          button_value = STATE_IDLE;
//          break;

        //default:

//          measure_v();
//          button_value = STATE_IDLE;
          //break;
      //}
    //}
  }
}

/**
 * @brief Main entry point
 * @return Unused value
 * @todo Call rsxTask in main()
 **/

int_t main(void) {
  error_t error;
  OsTaskId taskId;
  OsTaskParameters taskParams;
  NetInterface *interface;
  MacAddr macAddr;
#if (APP_USE_DHCP_CLIENT == DISABLED)
  Ipv4Addr ipv4Addr;
#endif
#if (APP_USE_SLAAC == DISABLED)
  Ipv6Addr ipv6Addr;
#endif

  // MPU configuration
  MPU_Config();
  // HAL library initialization
  HAL_Init();
  // Configure the system clock
  SystemClock_Config();

  // Enable I-cache and D-cache
  SCB_EnableICache();
  SCB_EnableDCache();

  // Initialize kernel
  osInitKernel();
  // Configure debug UART
  debugInit(115200);
  RSX_GPIO_Init();
  RSX_SPI_Init();

  // Start-up message
  TRACE_INFO("\r\n");
  TRACE_INFO("****************************************\r\n");
  TRACE_INFO("*** CycloneTCP WebSocket Client Demo ***\r\n");
  TRACE_INFO("****************************************\r\n");
  TRACE_INFO("Copyright: 2010-2025 Oryx Embedded SARL\r\n");
  TRACE_INFO("Compiled: %s %s\r\n", __DATE__, __TIME__);
  TRACE_INFO("Target: STM32F767\r\n");
  TRACE_INFO("\r\n");

  // LED configuration
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);

  // Clear LEDs
  BSP_LED_Off(LED1);
  BSP_LED_Off(LED2);
  BSP_LED_Off(LED3);

  // Initialize user button
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);

  // Initialize hardware cryptographic accelerator
  error = stm32f7xxCryptoInit();
  // Any error to report?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to initialize hardware crypto accelerator!\r\n");
  }

  // Generate a random seed
  error = trngGetRandomData(seed, sizeof(seed));
  // Any error to report?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to generate random data!\r\n");
  }

  // PRNG initialization
  error = hmacDrbgInit(&hmacDrbgContext, SHA256_HASH_ALGO);
  // Any error to report?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to initialize PRNG!\r\n");
  }

  // Properly seed the PRNG
  error = hmacDrbgSeed(&hmacDrbgContext, seed, sizeof(seed));
  // Any error to report?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to seed PRNG!\r\n");
  }

  // TCP/IP stack initialization
  error = netInit();
  // Any error to report?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to initialize TCP/IP stack!\r\n");
  }

  // Configure the first Ethernet interface
  interface = &netInterface[0];

  // Set interface name
  netSetInterfaceName(interface, APP_IF_NAME);
  // Set host name
  netSetHostname(interface, APP_HOST_NAME);
  // Set host MAC address
  macStringToAddr(APP_MAC_ADDR, &macAddr);
  netSetMacAddr(interface, &macAddr);
  // Select the relevant network adapter
  netSetDriver(interface, &stm32f7xxEthDriver);
  netSetPhyDriver(interface, &lan8742PhyDriver);

  // Initialize network interface
  error = netConfigInterface(interface);
  // Any error to report?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to configure interface %s!\r\n", interface->name);
  }

#if (IPV4_SUPPORT == ENABLED)
#if (APP_USE_DHCP_CLIENT == ENABLED)
  // Get default settings
  dhcpClientGetDefaultSettings(&dhcpClientSettings);
  // Set the network interface to be configured by DHCP
  dhcpClientSettings.interface = interface;
  // Disable rapid commit option
  dhcpClientSettings.rapidCommit = FALSE;

  // DHCP client initialization
  error = dhcpClientInit(&dhcpClientContext, &dhcpClientSettings);
  // Failed to initialize DHCP client?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to initialize DHCP client!\r\n");
  }

  // Start DHCP client
  error = dhcpClientStart(&dhcpClientContext);
  // Failed to start DHCP client?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to start DHCP client!\r\n");
  }
#else
  // Set IPv4 host address
  ipv4StringToAddr(APP_IPV4_HOST_ADDR, &ipv4Addr);
  ipv4SetHostAddr(interface, ipv4Addr);

  // Set subnet mask
  ipv4StringToAddr(APP_IPV4_SUBNET_MASK, &ipv4Addr);
  ipv4SetSubnetMask(interface, ipv4Addr);

  // Set default gateway
  ipv4StringToAddr(APP_IPV4_DEFAULT_GATEWAY, &ipv4Addr);
  ipv4SetDefaultGateway(interface, ipv4Addr);

  // Set primary and secondary DNS servers
  ipv4StringToAddr(APP_IPV4_PRIMARY_DNS, &ipv4Addr);
  ipv4SetDnsServer(interface, 0, ipv4Addr);
  ipv4StringToAddr(APP_IPV4_SECONDARY_DNS, &ipv4Addr);
  ipv4SetDnsServer(interface, 1, ipv4Addr);
#endif
#endif

#if (IPV6_SUPPORT == ENABLED)
#if (APP_USE_SLAAC == ENABLED)
  // Get default settings
  slaacGetDefaultSettings(&slaacSettings);
  // Set the network interface to be configured
  slaacSettings.interface = interface;

  // SLAAC initialization
  error = slaacInit(&slaacContext, &slaacSettings);
  // Failed to initialize SLAAC?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to initialize SLAAC!\r\n");
  }

  // Start IPv6 address autoconfiguration process
  error = slaacStart(&slaacContext);
  // Failed to start SLAAC process?
  if (error) {
    // Debug message
    TRACE_ERROR("Failed to start SLAAC!\r\n");
  }
#else
  // Set link-local address
  ipv6StringToAddr(APP_IPV6_LINK_LOCAL_ADDR, &ipv6Addr);
  ipv6SetLinkLocalAddr(interface, &ipv6Addr);

  // Set IPv6 prefix
  ipv6StringToAddr(APP_IPV6_PREFIX, &ipv6Addr);
  ipv6SetPrefix(interface, 0, &ipv6Addr, APP_IPV6_PREFIX_LENGTH);

  // Set global address
  ipv6StringToAddr(APP_IPV6_GLOBAL_ADDR, &ipv6Addr);
  ipv6SetGlobalAddr(interface, 0, &ipv6Addr);

  // Set default router
  ipv6StringToAddr(APP_IPV6_ROUTER, &ipv6Addr);
  ipv6SetDefaultRouter(interface, 0, &ipv6Addr);

  // Set primary and secondary DNS servers
  ipv6StringToAddr(APP_IPV6_PRIMARY_DNS, &ipv6Addr);
  ipv6SetDnsServer(interface, 0, &ipv6Addr);
  ipv6StringToAddr(APP_IPV6_SECONDARY_DNS, &ipv6Addr);
  ipv6SetDnsServer(interface, 1, &ipv6Addr);
#endif
#endif

  // Register RNG callback
  webSocketRegisterRandCallback(webSocketClientRngCallback);

  // Create an event object
  if (!osCreateEvent(&appEvent)) {
    // Debug message
    TRACE_ERROR("Failed to create event!\r\n");
  }

  // Set task parameters
  taskParams = OS_TASK_DEFAULT_PARAMS;
  taskParams.stackSize = 500;
  taskParams.priority = OS_TASK_PRIORITY_NORMAL;

  // Create user task
  taskId = osCreateTask("User", userTask, NULL, &taskParams);
  // Failed to create the task?
  if (taskId == OS_INVALID_TASK_ID) {
    // Debug message
    TRACE_ERROR("Failed to create task!\r\n");
  }

  // Set task parameters
  taskParams = OS_TASK_DEFAULT_PARAMS;
  taskParams.stackSize = 500;
  taskParams.priority = OS_TASK_PRIORITY_NORMAL;

  // Create a task to manage button events
  taskId = osCreateTask("Button", buttonTask, NULL, &taskParams);
  // Failed to create the task?
  if (taskId == OS_INVALID_TASK_ID) {
    // Debug message
    TRACE_ERROR("Failed to create task!\r\n");
  }

  // Set task parameters
  taskParams = OS_TASK_DEFAULT_PARAMS;
  taskParams.stackSize = 200;
  taskParams.priority = OS_TASK_PRIORITY_NORMAL;

  // Create a task to blink the LED
  taskId = osCreateTask("LED", ledTask, NULL, &taskParams);
  // Failed to create the task?
  if (taskId == OS_INVALID_TASK_ID) {
    // Debug message
    TRACE_ERROR("Failed to create task!\r\n");
  }

  rsx_task_handle = osCreateTask("rsx", rsxTask, NULL, &taskParams);
  if (rsx_task_handle == NULL) {
    // Debug message
    TRACE_ERROR("Failed to create task!\r\n");
  }

  taskId = osCreateTask("button", rsxButtonTask, NULL, &taskParams);
  if (taskId == OS_INVALID_TASK_ID) {
    // Debug message
    TRACE_ERROR("Failed to create task!\r\n");
  }


  spiSendTaskHandle = osCreateTask("spi_send", rsxSpiSendTask, NULL, &taskParams);
  if (spiSendTaskHandle == NULL) {
    // Debug message
    TRACE_ERROR("Failed to create task!\r\n");
  }

  // Create temperature sensor task
   ads1015TempTaskCreate();   // <--- ADD THIS LINE

   // Start the execution of tasks
   osStartKernel();

  // This function should never return
  return 0;
}
