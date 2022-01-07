/*********************************************************************
 *
 *                  MAC Module Defs for Microchip Stack
 *
 *********************************************************************
 * FileName:        MAC.h
 * Dependencies:    StackTsk.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __MAC_H
#define __MAC_H

#include "TCPIP Stack/ETH97J60.h"

#define MAC_TX_BUFFER_SIZE			(1500ul)

// A generic structure representing the Ethernet header starting all Ethernet 
// frames
typedef struct __attribute__ ((aligned(2), packed)) {
	MAC_ADDR DestMACAddr;
	MAC_ADDR SourceMACAddr;
	WORD_VAL Type;
} ETHER_HEADER;

#define MAC_IP      	(0x00u)
#define MAC_ARP     	(0x06u)
#define MAC_UNKNOWN 	(0xFFu)

/*
 * Microchip Ethernet controller specific MAC items
 */
#if !defined(STACK_USE_HTTP2_SERVER)
#define RESERVED_HTTP_MEMORY 0ul
#endif

#if !defined(STACK_USE_SSL_SERVER)
#define RESERVED_SSL_MEMORY 0ul
#endif

// MAC RAM definitions
#define RAMSIZE	8192ul
#define TXSTART (RAMSIZE - (1ul+1514ul+7ul) - TCP_ETH_RAM_SIZE - RESERVED_HTTP_MEMORY - RESERVED_SSL_MEMORY)
#define RXSTART	(0ul)			// Should be an even memory address; must be 0 for errata
#define	RXSTOP	((TXSTART-2ul) | 0x0001ul)	// Odd for errata workaround
#define RXSIZE	(RXSTOP-RXSTART+1ul)

#define BASE_TX_ADDR	(TXSTART + 1ul)
#define BASE_TCB_ADDR	(BASE_TX_ADDR + (1514ul+7ul))
#define BASE_HTTPB_ADDR (BASE_TCB_ADDR + TCP_ETH_RAM_SIZE)
#define BASE_SSLB_ADDR	(BASE_HTTPB_ADDR + RESERVED_HTTP_MEMORY)

#if (RXSIZE < 1400) || (RXSIZE > RAMSIZE)
#error Warning, Ethernet RX buffer is tiny.  Reduce TCP socket count, the size of each TCP socket, or move sockets to a different RAM
#endif

WORD MACCalcRxChecksum(WORD offset, WORD len);
WORD CalcIPBufferChecksum(WORD len);

void MACPowerDown(void);
void MACPowerUp(void);
void WritePHYReg(unsigned char Register, WORD Data);
PHYREG ReadPHYReg(unsigned char Register);
void SetRXHashTableEntry(MAC_ADDR DestMACAddr);

// ENC28J60 specific
void SetCLKOUT(unsigned char NewConfig);
unsigned char GetCLKOUT(void);

/******************************************************************************
 * Macro:        	void SetLEDConfig(WORD NewConfig)
 *
 * PreCondition:    SPI bus must be initialized (done in MACInit()).
 *
 * Input:           NewConfig - xxx0: Pulse stretching disabled
 *								xxx2: Pulse stretch to 40ms (default)
 *								xxx6: Pulse stretch to 73ms
 *								xxxA: Pulse stretch to 139ms
 *								
 *								xx1x: LEDB - TX
 *								xx2x: LEDB - RX (default)
 *								xx3x: LEDB - collisions
 *								xx4x: LEDB - link
 *								xx5x: LEDB - duplex
 *								xx7x: LEDB - TX and RX
 *								xx8x: LEDB - on
 *								xx9x: LEDB - off
 *								xxAx: LEDB - blink fast
 *								xxBx: LEDB - blink slow
 *								xxCx: LEDB - link and RX
 *								xxDx: LEDB - link and TX and RX
 *								xxEx: LEDB - duplex and collisions
 *
 *								x1xx: LEDA - TX
 *								x2xx: LEDA - RX
 *								x3xx: LEDA - collisions
 *								x4xx: LEDA - link (default)
 *								x5xx: LEDA - duplex
 *								x7xx: LEDA - TX and RX
 *								x8xx: LEDA - on
 *								x9xx: LEDA - off
 *								xAxx: LEDA - blink fast
 *								xBxx: LEDA - blink slow
 *								xCxx: LEDA - link and RX
 *								xDxx: LEDA - link and TX and RX
 *								xExx: LEDA - duplex and collisions
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Writes the value of NewConfig into the PHLCON PHY register.  
 *					The LED pins will beginning outputting the new 
 *					configuration immediately.
 *
 * Note:            
 *****************************************************************************/
#define SetLEDConfig(NewConfig)		WritePHYReg(PHLCON, NewConfig)


/******************************************************************************
 * Macro:        	WORD GetLEDConfig(void)
 *
 * PreCondition:    SPI bus must be initialized (done in MACInit()).
 *
 * Input:           None
 *
 * Output:          WORD -	xxx0: Pulse stretching disabled
 *							xxx2: Pulse stretch to 40ms (default)
 *							xxx6: Pulse stretch to 73ms
 *							xxxA: Pulse stretch to 139ms
 *								
 *							xx1x: LEDB - TX
 *							xx2x: LEDB - RX (default)
 *							xx3x: LEDB - collisions
 *							xx4x: LEDB - link
 *							xx5x: LEDB - duplex
 *							xx7x: LEDB - TX and RX
 *							xx8x: LEDB - on
 *							xx9x: LEDB - off
 *							xxAx: LEDB - blink fast
 *							xxBx: LEDB - blink slow
 *							xxCx: LEDB - link and RX
 *							xxDx: LEDB - link and TX and RX
 *							xxEx: LEDB - duplex and collisions
 *
 * 							x1xx: LEDA - TX
 *							x2xx: LEDA - RX
 *							x3xx: LEDA - collisions
 *							x4xx: LEDA - link (default)
 *							x5xx: LEDA - duplex
 *							x7xx: LEDA - TX and RX
 *							x8xx: LEDA - on
 *							x9xx: LEDA - off
 *							xAxx: LEDA - blink fast
 *							xBxx: LEDA - blink slow
 *							xCxx: LEDA - link and RX
 *							xDxx: LEDA - link and TX and RX
 *							xExx: LEDA - duplex and collisions
 *
 * Side Effects:    None
 *
 * Overview:        Returns the current value of the PHLCON register.
 *
 * Note:            None
 *****************************************************************************/
#define GetLEDConfig()		ReadPHYReg(PHLCON).Val

void MACInit(void);
BOOL MACIsLinked(void);
BOOL MACGetHeader(MAC_ADDR * remote, unsigned char *type);
void MACSetReadPtrInRx(WORD offset);
WORD MACSetWritePtr(WORD address);
WORD MACSetReadPtr(WORD address);
unsigned char MACGet(void);
WORD MACGetArray(unsigned char *val, WORD len);
void MACDiscardRx(void);
WORD MACGetFreeRxSize(void);
void MACMemCopyAsync(WORD destAddr, WORD sourceAddr, WORD len);
BOOL MACIsMemCopyDone(void);
void MACPutHeader(MAC_ADDR * remote, unsigned char type, WORD dataLen);
BOOL MACIsTxReady(void);
void MACPut(unsigned char val);
void MACPutArray(unsigned char *val, WORD len);
void MACFlush(void);
void MACPutROMArray(const unsigned char *val, WORD len);

#endif
