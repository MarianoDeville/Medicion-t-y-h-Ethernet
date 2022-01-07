/*********************************************************************
 * FileName:        TCPIPConfig.h
 * Dependencies:    Microchip TCP/IP Stack
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __TCPIPCONFIG_H
#define __TCPIPCONFIG_H

#include "TCPIP Stack/TCPIP.h"

/*
 * Modules to include in this project
 */
#define STACK_USE_GENERIC_TCP_SERVER_EXAMPLE	// ToUpper server example in GenericTCPServer.c
#define STACK_USE_ANNOUNCE						// Microchip Embedded Ethernet Device Discoverer server/client
#define STACK_USE_NBNS							// NetBIOS Name Service Server

#define MPFS_RESERVE_BLOCK              (8)
#define MAX_MPFS_HANDLES				(7ul)

/*
 * Following low level modules are automatically enabled/disabled based on high-level
 * module selections.
 * If you need them with your custom application, enable it here.
 */
#define STACK_USE_TCP

/*
 * Uncomment following line if this stack will be used in CLIENT
 * mode.  In CLIENT mode, some functions specific to client operation
 * are enabled.
 */
#define STACK_CLIENT_MODE


// Make sure that STACK_USE_TCP is defined if a service depends on 
// it
#if defined(STACK_USE_UART2TCP_BRIDGE) || \
	defined(STACK_USE_HTTP_SERVER) || \
	defined(STACK_USE_HTTP2_SERVER) || \
	defined(STACK_USE_FTP_SERVER) || \
	defined(STACK_USE_TELNET_SERVER) || \
	defined(STACK_USE_GENERIC_TCP_CLIENT_EXAMPLE) || \
	defined(STACK_USE_GENERIC_TCP_SERVER_EXAMPLE) || \
	defined(STACK_USE_SMTP_CLIENT) || \
	defined(STACK_USE_TCP_PERFORMANCE_TEST)
#if !defined(STACK_USE_TCP)
#define STACK_USE_TCP
#endif
#endif

// Make sure that STACK_USE_UDP is defined if a service depends 
// on it
#if defined(STACK_USE_DHCP_CLIENT) || \
	defined(STACK_USE_DNS) || \
	defined(STACK_USE_NBNS) || \
	defined(STACK_USE_SNMP_SERVER) || \
	defined(STACK_USE_TFTP_CLIENT) || \
	defined(STACK_USE_ANNOUNCE) || \
	defined(STACK_USE_UDP_PERFORMANCE_TEST) || \
	defined(STACK_USE_SNTP_CLIENT)
#if !defined(STACK_USE_UDP)
#define STACK_USE_UDP
#endif
#endif


//
// Default Address information - If not found in data EEPROM.
//
#define MY_DEFAULT_HOST_NAME			"MADING"

#define MY_DEFAULT_MAC_BYTE1            (0x00)
#define MY_DEFAULT_MAC_BYTE2            (0x04)
#define MY_DEFAULT_MAC_BYTE3            (0xA3)
#define MY_DEFAULT_MAC_BYTE4            (0x00)
#define MY_DEFAULT_MAC_BYTE5            (0x00)
#define MY_DEFAULT_MAC_BYTE6            (0x13)

#define MY_DEFAULT_IP_ADDR_BYTE1        (192ul)
#define MY_DEFAULT_IP_ADDR_BYTE2        (168ul)
#define MY_DEFAULT_IP_ADDR_BYTE3        (2ul)
#define MY_DEFAULT_IP_ADDR_BYTE4        (100ul)

#define MY_DEFAULT_MASK_BYTE1           (255ul)
#define MY_DEFAULT_MASK_BYTE2           (255ul)
#define MY_DEFAULT_MASK_BYTE3           (255ul)
#define MY_DEFAULT_MASK_BYTE4           (0ul)

#define MY_DEFAULT_GATE_BYTE1           MY_DEFAULT_IP_ADDR_BYTE1
#define MY_DEFAULT_GATE_BYTE2           MY_DEFAULT_IP_ADDR_BYTE2
#define MY_DEFAULT_GATE_BYTE3           MY_DEFAULT_IP_ADDR_BYTE3
#define MY_DEFAULT_GATE_BYTE4           (1ul)

#define MY_DEFAULT_PRIMARY_DNS_BYTE1	MY_DEFAULT_GATE_BYTE1
#define MY_DEFAULT_PRIMARY_DNS_BYTE2	MY_DEFAULT_GATE_BYTE2
#define MY_DEFAULT_PRIMARY_DNS_BYTE3	MY_DEFAULT_GATE_BYTE3
#define MY_DEFAULT_PRIMARY_DNS_BYTE4	(1ul)

#define MY_DEFAULT_SECONDARY_DNS_BYTE1	MY_DEFAULT_GATE_BYTE1
#define MY_DEFAULT_SECONDARY_DNS_BYTE2	MY_DEFAULT_GATE_BYTE2
#define MY_DEFAULT_SECONDARY_DNS_BYTE3	MY_DEFAULT_GATE_BYTE3
#define MY_DEFAULT_SECONDARY_DNS_BYTE4	(1ul)

//
// TCP and UDP protocol options
//

#if defined(STACK_USE_TCP)
	// Allocate how much total RAM (in bytes) you want to allocate 
	// for use by your TCP TCBs, RX FIFOs, and TX FIFOs.  
	// Sockets can be scattered across several different storage 
	// mediums if you are out of space on one medium.
#define TCP_ETH_RAM							32
#define TCP_ETH_RAM_BASE_ADDRESS			(BASE_TCB_ADDR)
#define TCP_ETH_RAM_SIZE					3600
#define TCP_PIC_RAM							1
#define TCP_PIC_RAM_BASE_ADDRESS			((PTR_BASE)&TCPBufferInPIC[0])
#define TCP_PIC_RAM_SIZE					32
#define TCP_SPI_RAM							2
#define TCP_SPI_RAM_BASE_ADDRESS			(0x0000u)
#define TCP_SPI_RAM_SIZE					0
	// These are the different types of TCP sockets 
	// that you want to use.  Each different type can have a 
	// different RX FIFO size, TX FIFO size, and even be stored in 
	// a different physical memory medium, optimizing storage
#define TCP_PURPOSE_DEFAULT					0
#define TCP_PURPOSE_GENERIC_TCP_CLIENT		1
#define TCP_PURPOSE_GENERIC_TCP_SERVER		2
#define TCP_PURPOSE_TELNET					3
#define TCP_PURPOSE_FTP_COMMAND				4
#define TCP_PURPOSE_FTP_DATA				5
#define TCP_PURPOSE_TCP_PERFORMANCE_TX		6
#define TCP_PURPOSE_TCP_PERFORMANCE_RX		7
#define TCP_PURPOSE_UART_2_TCP_BRIDGE		8
#define TCP_PURPOSE_MP3_CLIENT				9

#if defined(__TCP_C)
		// Define how many sockets are needed, what type they are,
		// where their TCB, TX FIFO, and RX FIFO should be stored, 
		// and how big the RX and TX FIFOs should be.  Making this 
		// initializer bigger or smaller defines how many total TCP
		// sockets are available.
		// Each socket requires up to 48 bytes of PIC RAM and 
		// 40+(TX FIFO size)+(RX FIFO size) bytes bytes of 
		// TCP_*_RAM each.
		// Note: The RX FIFO must be at least 1 byte in order to 
		// receive SYN and FIN messages required by TCP.  The TX 
		// FIFO can be zero if desired.
const struct {
	unsigned char vSocketPurpose;
	unsigned char vMemoryMedium;
	WORD wTXBufferSize;
	WORD wRXBufferSize;
} TCPSocketInitializer[] = {
	{
	TCP_PURPOSE_GENERIC_TCP_CLIENT, TCP_ETH_RAM, 125, 200}, {
	TCP_PURPOSE_GENERIC_TCP_SERVER, TCP_ETH_RAM, 20, 20}, {
	TCP_PURPOSE_TELNET, TCP_ETH_RAM, 150, 20},
	{
	TCP_PURPOSE_TCP_PERFORMANCE_TX, TCP_ETH_RAM, 256, 1},
	{
	TCP_PURPOSE_UART_2_TCP_BRIDGE, TCP_ETH_RAM, 256, 256},
	{
	TCP_PURPOSE_DEFAULT, TCP_ETH_RAM, 200, 200}, {
	TCP_PURPOSE_DEFAULT, TCP_ETH_RAM, 200, 200}, {
	TCP_PURPOSE_DEFAULT, TCP_ETH_RAM, 200, 200}, {
	TCP_PURPOSE_DEFAULT, TCP_ETH_RAM, 200, 200},
};
		// If PIC RAM is used to store TCP socket FIFOs and TCBs, 
		// let's allocate it so the linker dynamically chooses 
		// where to locate it and prevents other variables from 
		// overlapping with it
#if TCP_PIC_RAM_SIZE > 0
static unsigned char TCPBufferInPIC[TCP_PIC_RAM_SIZE]
	__attribute__ ((far));
#endif
#endif
#else
	// Don't allocate any RAM for TCP if TCP module isn't enabled
#define TCP_ETH_RAM_SIZE 0
#define TCP_PIC_RAM_SIZE 0
#define TCP_SPI_RAM_SIZE 0
#endif

// Maximum avaialble UDP Sockets
#define MAX_UDP_SOCKETS     (5ul)

// 
// HTTP2 Server options
//

// Maximum numbers of simultaneous HTTP connections allowed.
// Each connection consumes 2 bytes of RAM and a TCP socket
#define MAX_HTTP_CONNECTIONS	(1ul)


#endif
