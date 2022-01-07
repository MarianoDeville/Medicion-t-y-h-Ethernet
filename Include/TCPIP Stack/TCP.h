/*********************************************************************
 *
 *                  TCP Module Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        TCP.h
 * Dependencies:    StackTsk.h
 * Processor:       PIC18F97J60
 * Compiler:        Microchip Technology, Inc.
 ********************************************************************/
#ifndef __TCP_H
#define __TCP_H

typedef unsigned char TCP_SOCKET;

#define INVALID_SOCKET      (0xFE)
#define UNKNOWN_SOCKET      (0xFF)
#define LOOPBACK_IP			(DWORD)0x7f000001	//127.0.0.1
// TCP States as defined by RFC 793
typedef enum _TCP_STATE {
	TCP_LOOPBACK = 0,			// Special state for loopback sockets
	TCP_LOOPBACK_CLOSED,		// Special state for loopback sockets
	TCP_GET_DNS_MODULE,			// Special state for TCP client mode sockets
	TCP_DNS_RESOLVE,			// Special state for TCP client mode sockets
	TCP_GATEWAY_SEND_ARP,		// Special state for TCP client mode sockets
	TCP_GATEWAY_GET_ARP,		// Special state for TCP client mode sockets
	TCP_LISTEN,					// Normal TCP states specified in RFC
	TCP_SYN_SENT,
	TCP_SYN_RECEIVED,
	TCP_ESTABLISHED,
	TCP_FIN_WAIT_1,
	TCP_FIN_WAIT_2,
	TCP_CLOSING,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_CLOSED
} TCP_STATE;
// TCP Control Block (TCB) Information
// Stubs are stored in local PIC RAM
// Current size is 29 bytes (PIC18, 30 bytes (PIC24/dsPIC), or 48 (PIC32)
typedef struct _TCB_STUB {
	PTR_BASE bufferTxStart;		// TCB is located sizeof(TCB) bytes before this address
	PTR_BASE bufferRxStart;
	PTR_BASE bufferEnd;
	PTR_BASE txHead;
	PTR_BASE txTail;
	PTR_BASE rxHead;
	PTR_BASE rxTail;
	DWORD eventTime;			// Packet retransmissions, state changes
	WORD eventTime2;			// Window updates, automatic transmission
	union {
		WORD delayedACKTime;	// Delayed Acknowledgement timer
		WORD closeWaitTime;		// TCP_CLOSE_WAIT timeout timer
	} OverlappedTimers;
	TCP_STATE smState;
	struct {
		unsigned char bServer:1;
		unsigned char bTimerEnabled:1;
		unsigned char bTimer2Enabled:1;
		unsigned char bDelayedACKTimerEnabled:1;
		unsigned char bOneSegmentReceived:1;
		unsigned char bHalfFullFlush:1;
		unsigned char bTXASAP:1;
		unsigned char bTXFIN:1;
		unsigned char bSocketReset:1;
		unsigned char filler:7;
	} Flags;
	WORD_VAL remoteHash;		// Hash consists of remoteIP, remotePort, localPort for connected sockets.  It is a localPort number only for listening server sockets.
	unsigned char vMemoryMedium;
} TCB_STUB;

// The rest of the TCB is stored in Ethernet buffer RAM
// Current size is 37 (PIC18), 38 (PIC24/dsPIC), or 40 bytes (PIC32)
typedef struct _TCB {
	DWORD retryInterval;
	DWORD MySEQ;
	DWORD RemoteSEQ;
	PTR_BASE txUnackedTail;
	WORD_VAL remotePort;
	WORD_VAL localPort;
	WORD remoteWindow;
	WORD wFutureDataSize;
	union {
		NODE_INFO niRemoteMACIP;	// 6 bytes for MAC and IP address
		DWORD dwRemoteHost;		// RAM or const pointer to a hostname string (ex: "www.microchip.com")
	} remote;
	SHORT sHoleSize;
	struct {
		unsigned char bFINSent:1;
		unsigned char bSYNSent:1;
		unsigned char bRemoteHostIsROM:1;
		unsigned char filler:5;
	} flags;
	unsigned char retryCount;
	unsigned char vSocketPurpose;
} TCB;
typedef struct _SOCKET_INFO {
	NODE_INFO remote;
	WORD_VAL remotePort;
} SOCKET_INFO;
void TCPInit(void);
SOCKET_INFO *TCPGetRemoteInfo(TCP_SOCKET hTCP);
#define TCPListen(port)					TCPOpen(0, TCP_OPEN_SERVER, port, TCP_PURPOSE_DEFAULT)
#define TCPConnect(remote,port)			TCPOpen((DWORD)remote, TCP_OPEN_NODE_INFO, port, TCP_PURPOSE_DEFAULT)
BOOL TCPWasReset(TCP_SOCKET hTCP);
BOOL TCPIsConnected(TCP_SOCKET hTCP);
void TCPDisconnect(TCP_SOCKET hTCP);
WORD TCPIsPutReady(TCP_SOCKET hTCP);
BOOL TCPPut(TCP_SOCKET hTCP, unsigned char byte);
WORD TCPPutArray(TCP_SOCKET hTCP, unsigned char *Data, WORD Len);
unsigned char *TCPPutString(TCP_SOCKET hTCP, unsigned char *Data);
WORD TCPIsGetReady(TCP_SOCKET hTCP);
WORD TCPGetRxFIFOFree(TCP_SOCKET hTCP);
BOOL TCPGet(TCP_SOCKET hTCP, unsigned char *byte);
WORD TCPGetArray(TCP_SOCKET hTCP, unsigned char *buffer, WORD count);
WORD TCPFindEx(TCP_SOCKET hTCP, unsigned char cFind, WORD wStart,WORD wSearchLen, BOOL bTextCompare);
#define TCPFind(a,b,c,d)				TCPFindEx(a,b,c,0,d)
WORD TCPFindArrayEx(TCP_SOCKET hTCP, unsigned char *cFindArray, WORD wLen,WORD wStart, WORD wSearchLen, BOOL bTextCompare);
#define TCPFindArray(a,b,c,d,e)			TCPFindArrayEx(a,b,c,d,0,e)
void TCPDiscard(TCP_SOCKET hTCP);
BOOL TCPProcess(NODE_INFO * remote, IP_ADDR * localIP, WORD len);
void TCPTick(void);
void TCPFlush(TCP_SOCKET hTCP);
#define TCP_OPEN_SERVER					0
#define TCP_OPEN_RAM_HOST				1
#define TCP_OPEN_ROM_HOST				2
#define TCP_OPEN_IP_ADDRESS				3
#define TCP_OPEN_NODE_INFO				4
TCP_SOCKET TCPOpen(DWORD dwRemoteHost,unsigned char vRemoteHostType,WORD wPort,unsigned char vSocketPurpose);
WORD TCPFindROMArrayEx(TCP_SOCKET hTCP,const unsigned char *cFindArray,WORD wLen,WORD wStart,WORD wSearchLen,BOOL bTextCompare);
#define TCPFindROMArray(a,b,c,d,e)		TCPFindROMArrayEx(a,b,c,d,0,e)
WORD TCPPutROMArray(TCP_SOCKET hTCP, const unsigned char *Data, WORD Len);
const unsigned char *TCPPutROMString(TCP_SOCKET hTCP,const unsigned char *Data);
WORD TCPGetTxFIFOFull(TCP_SOCKET hTCP);
#define TCPGetRxFIFOFull(a)				TCPIsGetReady(a)
#define TCPGetTxFIFOFree(a) 			TCPIsPutReady(a)
#define TCP_ADJUST_GIVE_REST_TO_RX		0x01u
#define TCP_ADJUST_GIVE_REST_TO_TX		0x02u
#define TCP_ADJUST_PRESERVE_RX			0x04u
#define TCP_ADJUST_PRESERVE_TX			0x08u
BOOL TCPAdjustFIFOSize(TCP_SOCKET hTCP, WORD wMinRXSize, WORD wMinTXSize,unsigned char vFlags);
TCP_SOCKET TCPOpenLoopback(WORD destPort);
BOOL TCPCloseLoopback(TCP_SOCKET hTCP);
BOOL TCPIsLoopback(TCP_SOCKET hTCP);
WORD TCPInject(TCP_SOCKET hTCP, unsigned char *buffer, WORD len);
WORD TCPSteal(TCP_SOCKET hTCP, unsigned char *buffer, WORD len);

#endif
