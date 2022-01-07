/*********************************************************************
 *
 *                  UDP Module Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        UDP.h
 * Dependencies:    StackTsk.h
 *                  MAC.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __UDP_H
#define __UDP_H


typedef WORD UDP_PORT;
typedef unsigned char UDP_SOCKET;

typedef struct _UDP_SOCKET_INFO {
	NODE_INFO remoteNode;
	UDP_PORT remotePort;
	UDP_PORT localPort;
} UDP_SOCKET_INFO;

#define INVALID_UDP_SOCKET      (0xffu)
#define INVALID_UDP_PORT        (0ul)

/*
 * All module utilizing UDP module will get extern definition of
 * activeUDPSocket.  While UDP module itself will define activeUDPSocket.
 */
#if !defined(__UDP_C)
extern UDP_SOCKET activeUDPSocket;
extern UDP_SOCKET_INFO UDPSocketInfo[MAX_UDP_SOCKETS];
extern WORD UDPTxCount;
extern WORD UDPRxCount;
#endif

typedef struct _UDP_HEADER {
	UDP_PORT SourcePort;
	UDP_PORT DestinationPort;
	WORD Length;
	WORD Checksum;
} UDP_HEADER;

void UDPInit(void);

UDP_SOCKET UDPOpen(UDP_PORT localPort, NODE_INFO * remoteNode,UDP_PORT remotePort);
void UDPClose(UDP_SOCKET s);
BOOL UDPProcess(NODE_INFO * remoteNode, IP_ADDR * localIP, WORD len);

void UDPSetTxBuffer(WORD wOffset);
WORD UDPIsPutReady(UDP_SOCKET s);
BOOL UDPPut(unsigned char v);
WORD UDPPutArray(unsigned char *cData, WORD wDataLen);
unsigned char *UDPPutString(unsigned char *strData);
void UDPFlush(void);

// const function variants for PIC18
#if defined(__18CXX)
WORD UDPPutROMArray(const unsigned char *cData, WORD wDataLen);
const unsigned char *UDPPutROMString(const unsigned char *strData);
#else
#define UDPPutROMArray(a,b)	UDPPutArray((unsigned char*)a,b)
#define UDPPutROMString(a)	UDPPutString((unsigned char*)a)
#endif

WORD UDPIsGetReady(UDP_SOCKET s);
BOOL UDPGet(unsigned char *v);
WORD UDPGetArray(unsigned char *cData, WORD wDataLen);
void UDPDiscard(void);


/*********************************************************************
 * Macro:           UDPSetRxBuffer(a)
 *
 * PreCondition:    None
 *
 * Input:           a       - Offset
 *
 * Output:          Next Read/Write access to receive buffer is
 *                  set to offset 'b'
 *
 * Side Effects:    None
 *
 * Note:            None
 *
 ********************************************************************/
#define UDPSetRxBuffer(a) IPSetRxBuffer(a+sizeof(UDP_HEADER))


#endif
