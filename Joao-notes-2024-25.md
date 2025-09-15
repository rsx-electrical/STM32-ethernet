[Original Google Doc](https://docs.google.com/document/d/11fnf1_0UNkNoBxbQVy7jODhYvrkaA3BpCkaGHNBnnio/edit?usp=sharing)

--------
## Ethernet Debugging & Configuration Guide for STM32F7 with lwIP and FreeRTOS
This guide is intended for engineers working on an STM32F7 project using FreeRTOS and the lwIP stack. In our current implementation, the static IP is set up correctly (e.g. 10.0.0.200/255.255.255.0 with gateway 10.0.0.1), but the MAC address is not configured properly—the board’s MAC appears as all zeros in the ARP table. This document explains the relevant components, what each file does, and provides specific next steps and debugging measures.

## 1. Overview of the Project Architecture
Hardware and Ethernet PHY:
 The board uses an STM32F7 microcontroller with an integrated Ethernet MAC and an external LAN8742A PHY. The PHY converts digital signals (from the MCU’s MAC) into the electrical signals for the Ethernet cable and negotiates the physical link (speed, duplex, etc.).


lwIP TCP/IP Stack:
 lwIP is a lightweight TCP/IP stack that handles IP, ARP, ICMP (e.g., ping), UDP, TCP, DHCP (if enabled), and more. It provides the core network protocols that let your board communicate over Ethernet.


FreeRTOS Integration:
 FreeRTOS is used to run multiple tasks. In this project, there is an auto-generated task (often called “EthIf”) that handles Ethernet input, so that incoming frames (including ping requests) are processed automatically.


Static IP Configuration:
 The static IP, netmask, and gateway are set in the auto-generated lwip.c file. In our case, the intended configuration is:


IP: 10.0.0.200
Netmask: 255.255.255.0
Gateway: 10.0.0.1
Issue at Hand:
 Although the static IP appears to be set correctly in lwip.c, when the board is running, the MAC address printed in the ARP table appears as all zeros (00-00-00-00-00-00). Without a valid MAC address, the board will not respond properly to ARP requests or ping tests.



## 2. What Is a MAC Address and Why Is It Important?
Definition:
 A MAC (Media Access Control) address is a unique 48-bit identifier assigned to a network interface (usually written as 6 groups of two hexadecimal digits, e.g. 00:80:E1:12:34:56).


Role in Ethernet:


Layer 2 Addressing: The MAC address is used for local network communication. Ethernet frames include both source and destination MAC addresses.
ARP (Address Resolution Protocol): Devices use ARP to resolve an IP address to a MAC address. If a device’s MAC is all zeros, other network nodes cannot correctly deliver Ethernet frames to it.
Uniqueness: A valid MAC address ensures that the network switch or router can correctly identify and forward traffic.
In Our Project:
 In the **ethernetif.c** file, the MAC address is set during the initialization of the Ethernet handle (heth). Currently, it is defined as:

```
uint8_t MACAddr[6];
MACAddr[0] = 0x00;
MACAddr[1] = 0x80;
MACAddr[2] = 0xE1;
MACAddr[3] = 0x00;
MACAddr[4] = 0x00;
MACAddr[5] = 0x00;
heth.Init.MACAddr = &MACAddr[0];
```
 However, if this variable is not retained properly (for example, if it is a local variable that goes out of scope) or if the hardware isn’t configured to use it, the MAC might be seen as all zeros.



## 3. Overview of Key Files
**main.c**  
Purpose:  
 Initializes the MCU hardware, clocks, GPIO, UART, USB, and then calls MX_LWIP_Init(). It creates the default task that eventually prints the IP address.
Relevant Code:  
 In StartDefaultTask(), the code retrieves the IP address from the global gnetif and prints it over UART.
 Note: The IP printout shows that outside of the default task the static IP (10.0.0.200) is correct, but inside the default task it later prints as if all addresses (IP, mask, gateway) are 10.0.0.1—indicating another part of the code might be overriding these values.
**lwip.c**  
Purpose:  
 This file initializes the lwIP stack. It sets the static IP, netmask, and gateway by using predefined arrays:  
```
IP_ADDRESS[0] = 10;  IP_ADDRESS[1] = 0;  IP_ADDRESS[2] = 0;  IP_ADDRESS[3] = 200;
NETMASK_ADDRESS[0] = 255; NETMASK_ADDRESS[1] = 255; NETMASK_ADDRESS[2] = 255; NETMASK_ADDRESS[3] = 0;
GATEWAY_ADDRESS[0] = 10; GATEWAY_ADDRESS[1] = 0; GATEWAY_ADDRESS[2] = 0; GATEWAY_ADDRESS[3] = 1;
```
 It then calls **netif_add()** to create and configure the global network interface (gnetif).
**ethernetif.c**
Purpose:  
 Implements the low-level driver interface between the Ethernet peripheral (MAC/PHY) and lwIP.
Key Parts:  
The initialization function **low_level_init()** configures the Ethernet handle (heth), assigns the MAC address, and initializes the PHY (LAN8742A) using functions like LAN8742_Init().
It also creates a dedicated thread (often named “EthIf”) that calls ethernetif_input() to process incoming packets.
MAC Address Configuration:  
 The file sets the MAC address using a local variable MACAddr[] before calling HAL_ETH_Init(). If MACAddr is local to low_level_init(), it may be overwritten or lost. This is a possible source of error.

## 4. Possible Next Steps & Debugging Measures
### A. Verify and Retain the MAC Address  
Change MACAddr Declaration:  
 Instead of declaring MACAddr as a local variable in low_level_init(), declare it as a static or global variable in ethernetif.c.
 Example:  

```
static uint8_t MACAddr[6] = {0x00, 0x80, 0xE1, 0x12, 0x34, 0x56};
heth.Init.MACAddr = MACAddr;
```
 This ensures that the MAC address persists for the lifetime of the application.


Print the MAC Address:  
 Right after initializing the Ethernet peripheral (after calling HAL_ETH_Init(&heth)), add a debug print to output the MAC address via UART:

```
char macStr[32];
snprintf(macStr, sizeof(macStr), "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         heth.Init.MACAddr[0], heth.Init.MACAddr[1],
         heth.Init.MACAddr[2], heth.Init.MACAddr[3],
         heth.Init.MACAddr[4], heth.Init.MACAddr[5]);
HAL_UART_Transmit(&huart3, (uint8_t*)macStr, strlen(macStr), HAL_MAX_DELAY);
```
 This helps confirm that the MAC is correctly stored and used.  


### B. Ensure Static IP Configuration Is Not Overridden
Double-Check lwip.c Settings:
 Verify that the macros (IP_ADDRESS[], NETMASK_ADDRESS[], GATEWAY_ADDRESS[]) are set correctly in lwip.c.
Search for Other Overrides:
 Use the IDE’s “Find in Files” feature to search for calls to netif_set_addr( or similar functions that might be reconfiguring gnetif.
### C. Confirm PHY and Link Status
PHY Initialization:
 In **ethernetif.c**, check the return value from LAN8742_Init(&LAN8742). If it fails, the link will never come up.
Link Thread Debugging:
 In the ethernet_link_thread() function, insert debug prints to log changes in link status (e.g., “Link UP” or “Link DOWN”).
Verify RMII/MII and Clock Settings:
 Confirm your .ioc settings and hardware connections (clock to PHY, correct RMII configuration) are correct.
### D. Check the FreeRTOS Task Setup
Since a dedicated input thread is already spawned in lwip.c (the EthIf thread), ensure no additional polling loops (like manual calls to ethernetif_input()) conflict with it.
### E. Test and Verify with a Ping
Once you have fixed the MAC issue and verified that the static IP is correct, use your PC’s ARP table and ping test.
In your ARP table, the MAC for 10.0.0.200 should show the valid MAC (not all zeros).

## 5. Final Summary
Current State:  
 The lwIP initialization in lwip.c correctly sets the static IP to 10.0.0.200, netmask to 255.255.255.0, and gateway to 10.0.0.1.
 However, the MAC address is not properly retained or set; it appears as 00-00-00-00-00-00 in the network, causing ARP issues and preventing ping responses.


What Each File Does:  


**main.c**: Initializes peripherals and the RTOS scheduler; creates a default task that calls MX_LWIP_Init() and prints network configuration.  
**lwip.c**: Sets up the TCP/IP stack with static IP configuration and spawns necessary lwIP threads (input and link threads).  
**ethernetif.c**: Implements the low-level driver that bridges the Ethernet hardware (MAC/PHY) with lwIP. It is responsible for configuring the Ethernet peripheral (including MAC address) and managing incoming/outgoing packets.  
Next Steps & Debugging:  


Retain the MAC Address: Declare the MAC address as a static or global variable in ethernetif.c rather than a local one.  
Print the MAC: Add UART debug prints in ethernetif.c after **HAL_ETH_Init()** to verify the MAC address.  
Review Overriding Code: Search for any calls to **netif_set_addr()** or DHCP that may override your static configuration.  
Check PHY and Link Status: Use debug prints in **ethernet_link_thread()** to monitor PHY initialization and link changes.  
Test Connectivity: Once the MAC and IP settings are confirmed, check your router’s ARP table and try pinging the board from your PC.  
Following this guide should help you understand how the Ethernet stack is configured, what each module does, and provide a clear path to resolving the MAC address issue so that your board responds properly to network traffic.
  


