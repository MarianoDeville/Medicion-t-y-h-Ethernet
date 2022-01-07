/*********************************************************************
 *
 *	UART <-> TCP Bridge Example
 *  Module for Microchip TCP/IP Stack
 *	 -Transmits all incoming TCP bytes on a socket out the UART 
 *    module, all incoming UART bytes out of the TCP socket.
 *	 -Reference: None (hopefully AN833 in the future)
 *
 *********************************************************************
 * FileName:        UART2TCPBridge.c
 * Dependencies:    TCP.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __UART2TCPBRIDGE_H
#define __UART2TCPBRIDGE_H

void UART2TCPBridgeInit(void);
void UART2TCPBridgeTask(void);
void UART2TCPBridgeISR(void);

#endif
