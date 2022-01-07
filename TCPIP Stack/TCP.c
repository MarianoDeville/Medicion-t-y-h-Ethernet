/*********************************************************************
 *
 *	Transmission Control Protocol (TCP) Communications Layer
 *  Module for Microchip TCP/IP Stack
 *	 -Provides reliable, handshaked transport of application stream 
 *    oriented data with flow control
 *	 -Reference: RFC 793
 *
 *********************************************************************
 * FileName:        TCP.c
 * Dependencies:    IP, Tick, Ethernet (ENC28J60.c or ETH97J60.c), 
 *					ARP (optional), DNS (optional)
 * Processor:       PIC18F67J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 * Company:         Microchip Technology, Inc.
 *
 ********************************************************************/
#define __TCP_C

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_TCP)

#define LOCAL_PORT_START_NUMBER (3000u)
#define LOCAL_PORT_END_NUMBER   (5000u)
extern unsigned int desbordador;
// A lot of pointer dereference code can be removed if you 
// locally copy TCBStubs to an absolute memory location.
// If you define TCP_OPTIMIZE_FOR_SIZE, local caching will 
// occur and will substantially decrease the entire TCP const 
// footprint (up to 35%).  If you leave TCP_OPTIMIZE_FOR_SIZE 
// undefined, the local caching will be disabled.  On PIC18 
// products, this will improve TCP performance/throughput by 
// approximately 15%.
#define TCP_OPTIMIZE_FOR_SIZE

// TCP Maximum Segment Size (TX and RX)
#define TCP_MAX_SEG_SIZE			(1024)

// TCP Timeout and retransmit numbers
#define TCP_START_TIMEOUT_VAL   	((TICK)TICK_SECOND*1)
#define TCP_DELAYED_ACK_TIMEOUT		((TICK)TICK_SECOND/10)
#define TCP_FIN_WAIT_2_TIMEOUT		((TICK)TICK_SECOND*5)
#define TCP_KEEP_ALIVE_TIMEOUT		((TICK)TICK_SECOND*5)	// 10
#define TCP_CLOSE_WAIT_TIMEOUT		((TICK)TICK_SECOND/5)
#define TCP_MAX_RETRIES			    (5u)
#define TCP_MAX_SYN_RETRIES			(2u)	// Smaller than all other retries to reduce SYN flood DoS duration

#define TCP_AUTO_TRANSMIT_TIMEOUT_VAL	(TICK_SECOND/25ull)

// TCP Flags defined in RFC
#define FIN     (0x01)
#define SYN     (0x02)
#define RST     (0x04)
#define PSH     (0x08)
#define ACK     (0x10)
#define URG     (0x20)

// TCP Header
typedef struct _TCP_HEADER {
	WORD SourcePort;
	WORD DestPort;
	DWORD SeqNumber;
	DWORD AckNumber;
	struct
	{
		unsigned char Reserved3:4;
		unsigned char Val:4;
	} DataOffset;

	union {
		struct {
			unsigned char flagFIN:1;
			unsigned char flagSYN:1;
			unsigned char flagRST:1;
			unsigned char flagPSH:1;
			unsigned char flagACK:1;
			unsigned char flagURG:1;
			unsigned char Reserved2:2;
		} bits;
		unsigned char byte;
	} Flags;

	WORD Window;
	WORD Checksum;
	WORD UrgentPointer;
} TCP_HEADER;

// TCP Options as defined by RFC
#define TCP_OPTIONS_END_OF_LIST     (0x00u)
#define TCP_OPTIONS_NO_OP           (0x01u)
#define TCP_OPTIONS_MAX_SEG_SIZE    (0x02u)
typedef struct _TCP_OPTIONS {
	unsigned char Kind;
	unsigned char Length;
	WORD_VAL MaxSegSize;
} TCP_OPTIONS;


// Local temp port numbers.
#ifdef STACK_CLIENT_MODE
static WORD NextPort __attribute__ ((persistent));
#endif

#define TCP_SOCKET_COUNT	(sizeof(TCPSocketInitializer)/sizeof(TCPSocketInitializer[0]))

static TCB_STUB TCBStubs[TCP_SOCKET_COUNT] = { '\0' };
static TCB MyTCB;
static TCP_SOCKET hCurrentTCP = INVALID_SOCKET;

static void TCPRAMCopy(void *wDest, unsigned char vDestType, void *wSource,
					   unsigned char vSourceType, WORD wLength);

#define SENDTCP_RESET_TIMERS	0x01
#define SENDTCP_KEEP_ALIVE		0x02
static void SendTCP(unsigned char vTCPFlags, unsigned char vSendFlags);
static void HandleTCPSeg(TCP_HEADER * h, WORD len);
static BOOL FindMatchingSocket(TCP_HEADER * h, NODE_INFO * remote);
static void SwapTCPHeader(TCP_HEADER * header);
static void CloseSocket(void);
static void SyncTCB(void);


#if defined(TCP_OPTIMIZE_FOR_SIZE)
static TCB_STUB MyTCBStub;

	// Flushes MyTCBStub cache and loads up the specified TCB_STUB
	// Does nothing on cache hit
static void SyncTCBStub(TCP_SOCKET hTCP)
{
	if (hCurrentTCP == hTCP)
		return;

	if (hCurrentTCP != INVALID_SOCKET) {
		// Save the current TCB stub
		memcpy((void *) &TCBStubs[hCurrentTCP], (void *) &MyTCBStub,
			   sizeof(MyTCBStub));
	}

	hCurrentTCP = hTCP;

	if (hTCP == INVALID_SOCKET)
		return;

	// Load up the new TCB stub
	memcpy((void *) &MyTCBStub, (void *) &TCBStubs[hTCP],
		   sizeof(MyTCBStub));
}
#else
#define SyncTCBStub(a)	hCurrentTCP = a
#define MyTCBStub		TCBStubs[hCurrentTCP]
#endif



// Flushes MyTCB cache and loads up the specified TCB
// Does nothing on cache hit
static void SyncTCB(void)
{
	static TCP_SOCKET hLastTCB = INVALID_SOCKET;

	if (hLastTCB == hCurrentTCP)
		return;

	if (hLastTCB != INVALID_SOCKET) {
		// Save the current TCB
		TCPRAMCopy((void *) (TCBStubs[hLastTCB].bufferTxStart -
							 sizeof(MyTCB)),
				   TCBStubs[hLastTCB].vMemoryMedium, &MyTCB, TCP_PIC_RAM,
				   sizeof(MyTCB));
	}
	// Load up the new TCB
	hLastTCB = hCurrentTCP;
	TCPRAMCopy(&MyTCB, TCP_PIC_RAM,
			   (void *) (TCBStubs[hCurrentTCP].bufferTxStart -
						 sizeof(MyTCB)), TCBStubs[hLastTCB].vMemoryMedium,
			   sizeof(MyTCB));
}


/*********************************************************************
* Function:        void TCPInit(void)
*
* PreCondition:    None
*
* Input:           None
*
* Output:          TCP is initialized.
*
* Side Effects:    None
*
* Overview:        Initialize all socket states
*
* Note:            This function is called only once during lifetime
*                  of the application.
********************************************************************/
void TCPInit(void)
{
	volatile unsigned char i;
	unsigned char vSocketsAllocated;
	WORD wTXSize, wRXSize;
	PTR_BASE ptrBaseAddress;
	unsigned char vMedium;
#if TCP_ETH_RAM_SIZE > 0
	WORD wCurrentETHAddress = TCP_ETH_RAM_BASE_ADDRESS;
#endif
#if TCP_PIC_RAM_SIZE > 0
	PTR_BASE ptrCurrentPICAddress = TCP_PIC_RAM_BASE_ADDRESS;
#endif
#if TCP_SPI_RAM_SIZE > 0
	WORD wCurrentSPIAddress = TCP_SPI_RAM_BASE_ADDRESS;
#endif

	// Allocate all socket FIFO addresses
	vSocketsAllocated = 0;
	for (i=0;i<TCP_SOCKET_COUNT;i++)
	{
		// Generate all needed sockets of each type (TCP_PURPOSE_*)
		SyncTCBStub(i);
		SyncTCB();

		vMedium = TCPSocketInitializer[i].vMemoryMedium;
		wTXSize = TCPSocketInitializer[i].wTXBufferSize;
		wRXSize = TCPSocketInitializer[i].wRXBufferSize;

		switch (vMedium) {
#if TCP_ETH_RAM_SIZE > 0
		case TCP_ETH_RAM:
			ptrBaseAddress = wCurrentETHAddress;
			wCurrentETHAddress += sizeof(TCB) + wTXSize + 1 + wRXSize + 1;
			// Do a sanity check to ensure that we aren't going to use memory that hasn't been allocated to us.
			// If your code locks up right here, it means you've incorrectly allocated your TCP socket buffers in TCPIPConfig.h.  See the TCP memory allocation section.  More RAM needs to be allocated to the base memory mediums, or the individual sockets TX and RX FIFOS and socket quantiy needs to be shrunken.
			while (wCurrentETHAddress >
				   TCP_ETH_RAM_BASE_ADDRESS + TCP_ETH_RAM_SIZE);
			break;
#endif

#if TCP_PIC_RAM_SIZE > 0
		case TCP_PIC_RAM:
			ptrBaseAddress = ptrCurrentPICAddress;
			ptrCurrentPICAddress +=
				sizeof(TCB) + wTXSize + 1 + wRXSize + 1;
			// Do a sanity check to ensure that we aren't going to use memory that hasn't been allocated to us.
			// If your code locks up right here, it means you've incorrectly allocated your TCP socket buffers in TCPIPConfig.h.  See the TCP memory allocation section.  More RAM needs to be allocated to the base memory mediums, or the individual sockets TX and RX FIFOS and socket quantiy needs to be shrunken.
			while (ptrCurrentPICAddress >
				   TCP_PIC_RAM_BASE_ADDRESS + TCP_PIC_RAM_SIZE);
			break;
#endif

#if TCP_SPI_RAM_SIZE > 0
		case TCP_SPI_RAM:
			ptrBaseAddress = wCurrentSPIAddress;
			wCurrentSPIAddress += sizeof(TCB) + wTXSize + 1 + wRXSize + 1;
			// Do a sanity check to ensure that we aren't going to use memory that hasn't been allocated to us.
			// If your code locks up right here, it means you've incorrectly allocated your TCP socket buffers in TCPIPConfig.h.  See the TCP memory allocation section.  More RAM needs to be allocated to the base memory mediums, or the individual sockets TX and RX FIFOS and socket quantiy needs to be shrunken.
			while (wCurrentSPIAddress >
				   TCP_SPI_RAM_BASE_ADDRESS + TCP_SPI_RAM_SIZE);
			break;
#endif

		default:
			while(1);			// Undefined allocation medium.  Go fix your TCPIPConfig.h TCP memory allocations.
		}

		MyTCB.vSocketPurpose = TCPSocketInitializer[i].vSocketPurpose;

		MyTCBStub.vMemoryMedium = vMedium;
		MyTCBStub.bufferTxStart = ptrBaseAddress + sizeof(TCB);
		MyTCBStub.bufferRxStart = MyTCBStub.bufferTxStart + wTXSize + 1;
		MyTCBStub.bufferEnd = MyTCBStub.bufferRxStart + wRXSize;
		MyTCBStub.smState = TCP_CLOSED;
		MyTCBStub.Flags.bServer = FALSE;
		CloseSocket();
	}
}

/*********************************************************************
* Function:		TCP_SOCKET TCPOpen(DWORD dwRemoteHost, unsigned char vRemoteHostType, WORD wPort, unsigned char vSocketPurpose)
*
* PreCondition:	TCPInit() is already called.
*
* Input:		dwRemoteHost: Pointer to a string (note that you must type cast it to a DWORD)
*					Client sockets - Give a null terminated 
*						string of the remote hostname to connect 
*						to (ex: "www.microchip.com" or 
*						"192.168.1.123"), the destination IP address 
*						(ex: 0xC0A8017B), or a pointer to a NODE_INFO 
*						structure with the remote IP address and 
*						remote node or gateway MAC address specified.  
*						Note that if a string is given, it must be 
*						statically allocated in memory and cannot be 
*						modified or deallocated until TCPIsConnected() 
*						returns TRUE.
*					Server sockets - Ignored
*				bRemoteHostType:
*					TCP_OPEN_SEVER - indicates that we are making a server socket and the dwRemoteHost parameter is ignored
*					TCP_OPEN_RAM_HOST - indicates to create a client socket and the dwRemoteHost parameter is a RAM pointer
*					TCP_OPEN_ROM_HOST - indicates to create a client socket and the dwRemoteHost parameter is a const pointer
*					TCP_OPEN_IP_ADDRESS - indicates to create a client socket and the dwRemoteHost parameter is a literal IP address
*					TCP_OPEN_NODE_INFO - indicates to create a client socket and the dwRemoteHost parameter is a pointer to a NODE_INFO structure containing the exact remote IP address and MAC address to use
*				wPort:
*					Client sockets - remote TCP port to connect 
*						to (ex: 25 typically for SMTP).
*					Server sockets - local TCP port to listen on
*						(ex: 80 typically for HTTP)
*				vSocketPurpose: 
*					Any of TCP_PURPOSE_* constants defining what TX 
*					and RX FIFO sizes to use, where to store the 
*					socket, and other paramters.  Use 
*					TCP_PURPOSE_DEFAULT to accept default socket 
*					parameters.
*
* Output:		On success: TCP_SOCKET handle is returned.  Save and 
*							use this handle for all other TCP APIs.  
*				On failure: INVALID_SOCKET is returned.
*
* Side Effects:	None
*
* Overview:		Obtains a free TCP socket from the stack (of the 
*				specified type) and returns.  For client mode sockets, 
*				this internally	starts a DNS query of the remoteHost 
*				hostname (if needed), ARPs the gateway host (if 
*				needed), and then sends a TCP connection request 
*				(SYN) to the remote node on the specified port.
*
* Note:			If TCP_OPEN_RAM_HOST or TCP_OPEN_ROM_HOST are used 
*				for the destination type, you must also enable the 
*				DNS client module.
*				Edit the TCPIPConfig.h file to edit socket purpose
*				parameters.
********************************************************************/
TCP_SOCKET TCPOpen(DWORD dwRemoteHost, unsigned char vRemoteHostType,
				   WORD wPort, unsigned char vSocketPurpose)
{
	volatile TCP_SOCKET hTCP;

	// Make sure the DNS client module is enabled if this is a client 
	// mode socket who's IP address needs to be resolved first
#if !defined(STACK_USE_DNS)
	if ((vRemoteHostType == TCP_OPEN_RAM_HOST)
		|| (vRemoteHostType == TCP_OPEN_ROM_HOST))
		return INVALID_SOCKET;
#endif

	// Find an available socket that matches the specified socket type
	for(hTCP=0;hTCP<TCP_SOCKET_COUNT;hTCP++)
	{
		SyncTCBStub(hTCP);

		// Sockets that are in use will be in a non-closed state
		if (MyTCBStub.smState != TCP_CLOSED)
			continue;

		SyncTCB();

		// See if this socket matches the desired type
		if (MyTCB.vSocketPurpose != vSocketPurpose)
			continue;

		// See if this is a server socket
		if (vRemoteHostType == TCP_OPEN_SERVER) {
			MyTCB.localPort.Val = wPort;
			MyTCBStub.Flags.bServer = TRUE;
			MyTCBStub.smState = TCP_LISTEN;
			MyTCBStub.remoteHash.Val = wPort;
		}
		// Handle all the client mode socket types
		else {
			// Each new socket that is opened by this node, gets the 
			// next sequential local port number.
			if (NextPort < LOCAL_PORT_START_NUMBER
				|| NextPort > LOCAL_PORT_END_NUMBER)
				NextPort = LOCAL_PORT_START_NUMBER;

			// Set the non-zero TCB fields
			MyTCB.localPort.Val = NextPort++;
			MyTCB.remotePort.Val = wPort;

			// Flag to start the DNS, ARP, SYN processes
			MyTCBStub.eventTime = TickGet();
			MyTCBStub.Flags.bTimerEnabled = 1;

			switch (vRemoteHostType) {
			case TCP_OPEN_RAM_HOST:
			case TCP_OPEN_ROM_HOST:
				MyTCB.remote.dwRemoteHost = dwRemoteHost;
				MyTCB.flags.bRemoteHostIsROM =
					(vRemoteHostType == TCP_OPEN_ROM_HOST);
				MyTCBStub.smState = TCP_GET_DNS_MODULE;
				break;

			case TCP_OPEN_IP_ADDRESS:
				// dwRemoteHost is a literal IP address.  This 
				// doesn't need DNS and can skip directly to the 
				// Gateway ARPing step.
				MyTCBStub.remoteHash.Val =
					(((DWORD_VAL *) & dwRemoteHost)->w[1] +
					 ((DWORD_VAL *) & dwRemoteHost)->w[0] +
					 wPort) ^ MyTCB.localPort.Val;
				MyTCB.remote.niRemoteMACIP.IPAddr.Val = dwRemoteHost;
				MyTCB.retryCount = 0;
				MyTCB.retryInterval = (TICK_SECOND / 4) / 256;
				MyTCBStub.smState = TCP_GATEWAY_SEND_ARP;
				break;

			case TCP_OPEN_NODE_INFO:
				MyTCBStub.remoteHash.Val =
					(((NODE_INFO *) (PTR_BASE) dwRemoteHost)->IPAddr.w[1] +
					 ((NODE_INFO *) (PTR_BASE) dwRemoteHost)->IPAddr.w[0] +
					 wPort) ^ MyTCB.localPort.Val;
				memcpy((void *) &MyTCB.remote,
					   (void *) (PTR_BASE) dwRemoteHost,
					   sizeof(NODE_INFO));
				MyTCBStub.smState = TCP_SYN_SENT;
				SendTCP(SYN, SENDTCP_RESET_TIMERS);
				break;
			}
		}


		return hTCP;
	}

	// If there is no socket available, return error.
	return INVALID_SOCKET;
}

/*********************************************************************
* Function:        BOOL TCPIsConnected(TCP_SOCKET hTCP)
*
* PreCondition:    TCPInit() is already called.
*
* Input:           hTCP    - Socket to be checked for connection.
*
* Output:          TRUE    if given socket is connected
*                  FALSE   if given socket is not connected.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            A socket is said to be connected only if it is in 
*				   the TCP_ESTABLISHED state
********************************************************************/
BOOL TCPIsConnected(TCP_SOCKET hTCP)
{
	SyncTCBStub(hTCP);
	return (MyTCBStub.smState == TCP_ESTABLISHED)
		|| (MyTCBStub.smState == TCP_LOOPBACK);
}
/*********************************************************************
* Function:        void TCPDisconnect(TCP_SOCKET hTCP)
* PreCondition:    TCPInit() is already called
* Input:           hTCP - Socket to be disconnected.
* Output:          A disconnect request is sent for given socket.  
*				   This function does nothing if the socket isn't 
*				   currently connected.
* Side Effects:    None
* Overview:        None
* Note:            None
********************************************************************/
void TCPDisconnect(TCP_SOCKET hTCP)
{
	SyncTCBStub(hTCP);
	// Delete all data in the RX FIFO
	// In this stack's API, the application TCP handle is 
	// immediately invalid after calling this function, so there 
	// is no longer any way to receive data from the TCP RX FIFO, 
	// even though the data is still there.  Leaving the data there 
	// could interfere with the remote node sending us a FIN if our
	// RX window is zero
	MyTCBStub.rxTail = MyTCBStub.rxHead;
	switch (MyTCBStub.smState) {
#if defined(STACK_CLIENT_MODE) && defined(STACK_USE_DNS)
	case TCP_DNS_RESOLVE:
		DNSEndUsage();			// Release the DNS module, since the user is aborting
		CloseSocket();
		break;
#endif
	case TCP_SYN_SENT:
		CloseSocket();
		break;

	case TCP_SYN_RECEIVED:
	case TCP_ESTABLISHED:
		// Send the FIN
		SendTCP(FIN | ACK, SENDTCP_RESET_TIMERS);
		MyTCBStub.smState = TCP_FIN_WAIT_1;
		break;

	case TCP_CLOSE_WAIT:
		// Send the FIN
		SendTCP(FIN | ACK, SENDTCP_RESET_TIMERS);
		MyTCBStub.smState = TCP_LAST_ACK;
		break;

	case TCP_LOOPBACK:
		// Indicate to loopback owner that the loop is done
		// Loopback owner must clean up later
		MyTCBStub.smState = TCP_LOOPBACK_CLOSED;

		// These states are all already closed or don't need explicit disconnecting -- they will disconnect by themselves after a while
		//case TCP_CLOSED:
		//case TCP_LISTEN:
		//case TCP_CLOSING:
		//case TCP_TIME_WAIT:
		//case TCP_LOOPBACK_CLOSED
		//  return;
		// This state will close itself after some delay, however, 
		// this is handled so that the user can call TCPDisconnect() 
		// twice to immediately close a socket (using an RST) without 
		// having to get an ACK back from the remote node.  This is 
		// great for instance when the application determines that 
		// the remote node has been physically disconnected and 
		// already knows that no ACK will be returned.  Alternatively, 
		// if the application needs to immediately reuse the socket 
		// regardless of what the other node's state is in (half open).
	case TCP_FIN_WAIT_1:
	case TCP_FIN_WAIT_2:
	case TCP_LAST_ACK:
	default:
		SendTCP(RST | ACK, 0);
		CloseSocket();
		break;
	}
}

/*********************************************************************
* Function:        void TCPFlush(TCP_SOCKET hTCP)
*
* PreCondition:    TCPInit() is already called.
*
* Input:           s       - Socket whose data is to be transmitted.
*
* Output:          None
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
void TCPFlush(TCP_SOCKET hTCP)
{
	SyncTCBStub(hTCP);
	SyncTCB();

	if (MyTCBStub.txHead != MyTCB.txUnackedTail) {
		// Send the TCP segment with all unacked bytes
		SendTCP(PSH | ACK, SENDTCP_RESET_TIMERS);
	}
}
/*********************************************************************
* Function:        WORD TCPIsPutReady(TCP_SOCKET hTCP)
*
* PreCondition:    TCPInit() is already called.
*
* Input:           hTCP: handle of socket to test
*
* Output:          Number of bytes that can be immediately placed 
*				   in the transmit buffer.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
WORD TCPIsPutReady(TCP_SOCKET hTCP)
{
	unsigned char i;

	SyncTCBStub(hTCP);

	i = MyTCBStub.smState;

	// Unconnected sockets shouldn't be transmitting anything.
	if (!((i == TCP_ESTABLISHED) || (i == TCP_CLOSE_WAIT) ||
		  (i == TCP_LOOPBACK_CLOSED) || (i == TCP_LOOPBACK)))
		return 0;

	// Calculate the free space in this socket's TX FIFO
	if (MyTCBStub.txHead >= MyTCBStub.txTail)
		return (MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart - 1) -
			(MyTCBStub.txHead - MyTCBStub.txTail);
	else
		return MyTCBStub.txTail - MyTCBStub.txHead - 1;
}

/*********************************************************************
* Function:        WORD TCPPutArray(TCP_SOCKET hTCP, unsigned char *data, WORD len)
*
* PreCondition:    None
*
* Input:           hTCP    - Socket handle to use
*                  data    - Pointer to data to put
*				   len     - Count of bytes to put
*
* Output:          Count of bytes actually placed in the TX buffer
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
WORD TCPPutArray(TCP_SOCKET hTCP, unsigned char *data, WORD len)
{
	WORD wActualLen;
	WORD wFreeTXSpace;
	WORD wRightLen = 0;

	SyncTCBStub(hTCP);

	wFreeTXSpace = TCPIsPutReady(hTCP);
	if (wFreeTXSpace == 0u) {
		TCPFlush(hTCP);
		return 0;
	}

	wActualLen = wFreeTXSpace;
	if (wFreeTXSpace > len)
		wActualLen = len;

	// Send all current bytes if we are crossing half full
	// This is required to improve performance with the delayed 
	// acknowledgement algorithm
	if ((!MyTCBStub.Flags.bHalfFullFlush)
		&& (wFreeTXSpace <=
			((MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart) >> 1))) {
		TCPFlush(hTCP);
		MyTCBStub.Flags.bHalfFullFlush = TRUE;
	}
	// See if we need a two part put
	if (MyTCBStub.txHead + wActualLen >= MyTCBStub.bufferRxStart) {
		wRightLen = MyTCBStub.bufferRxStart - MyTCBStub.txHead;
		TCPRAMCopy((void *) MyTCBStub.txHead, MyTCBStub.vMemoryMedium,
				   data, TCP_PIC_RAM, wRightLen);
		data += wRightLen;
		wActualLen -= wRightLen;
		MyTCBStub.txHead = MyTCBStub.bufferTxStart;
	}

	TCPRAMCopy((void *) MyTCBStub.txHead, MyTCBStub.vMemoryMedium, data,
			   TCP_PIC_RAM, wActualLen);
	MyTCBStub.txHead += wActualLen;

	// Send these bytes right now if we are out of TX buffer space
	if (wFreeTXSpace <= len) {
		TCPFlush(hTCP);
	}
	// If not already enabled, start a timer so this data will 
	// eventually get sent even if the application doens't call
	// TCPFlush()
	else if (!MyTCBStub.Flags.bTimer2Enabled) {
		MyTCBStub.Flags.bTimer2Enabled = TRUE;
		MyTCBStub.eventTime2 =
			(WORD) TickGetDiv256() +
			TCP_AUTO_TRANSMIT_TIMEOUT_VAL / 256ull;
	}

	return wActualLen + wRightLen;
}

/*********************************************************************
* Function:        void TCPDiscard(TCP_SOCKET hTCP)
*
* PreCondition:    TCPInit() is already called.
*
* Input:           hTCP       - socket handle
*
* Output:          None
*
* Side Effects:    None
*
* Overview:        Removes all pending data from the socket's RX 
*				   FIFO.
*
* Note:            None
********************************************************************/
void TCPDiscard(TCP_SOCKET hTCP)
{
	if (TCPIsGetReady(hTCP)) {
		SyncTCBStub(hTCP);

		// Delete all data in the RX buffer
		MyTCBStub.rxTail = MyTCBStub.rxHead;

		// Send a Window update message to the remote node
		SendTCP(ACK, SENDTCP_RESET_TIMERS);
	}
}


/*********************************************************************
* Function:        WORD TCPIsGetReady(TCP_SOCKET hTCP)
*
* PreCondition:    TCPInit() is already called.
*
* Input:           hTCP       - socket to test
*
* Output:          Number of bytes that are available in socket 'hTCP' 
*				   for immediate retrieval
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
WORD TCPIsGetReady(TCP_SOCKET hTCP)
{
	SyncTCBStub(hTCP);

	// If the loopback is closed, reject all input
	if (MyTCBStub.smState == TCP_LOOPBACK_CLOSED)
		return 0;

	if (MyTCBStub.rxHead >= MyTCBStub.rxTail)
		return MyTCBStub.rxHead - MyTCBStub.rxTail;
	else
		return (MyTCBStub.bufferEnd - MyTCBStub.rxTail + 1) +
			(MyTCBStub.rxHead - MyTCBStub.bufferRxStart);
}

/*********************************************************************
* Function:        WORD TCPGetArray(TCP_SOCKET hTCP, unsigned char *buffer,
*                                      WORD len)
*
* PreCondition:    TCPInit() is already called
*
* Input:           hTCP    - socket handle
*                  buffer  - Buffer to hold received data.
*                  len     - Buffer length
*
* Output:          Number of bytes loaded into buffer.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
WORD TCPGetArray(TCP_SOCKET hTCP, unsigned char *buffer, WORD len)
{
	WORD ActualLen;
	WORD RightLen = 0;

	SyncTCBStub(hTCP);

	ActualLen = TCPIsGetReady(hTCP);
	if (len > ActualLen)
		len = ActualLen;

	// See if we need a two part get
	if (MyTCBStub.rxTail + len > MyTCBStub.bufferEnd) {
		RightLen = MyTCBStub.bufferEnd - MyTCBStub.rxTail + 1;
		if (buffer) {
			TCPRAMCopy(buffer, TCP_PIC_RAM, (void *) MyTCBStub.rxTail,
					   MyTCBStub.vMemoryMedium, RightLen);
			buffer += RightLen;
		}
		len -= RightLen;
		MyTCBStub.rxTail = MyTCBStub.bufferRxStart;
	}

	if (buffer)
		TCPRAMCopy(buffer, TCP_PIC_RAM, (void *) MyTCBStub.rxTail,
				   MyTCBStub.vMemoryMedium, len);
	MyTCBStub.rxTail += len;

	// Send a window update if we've run low on data
	if (ActualLen - len <= len) {
		MyTCBStub.Flags.bTXASAP = 1;
	} else if (!MyTCBStub.Flags.bTimer2Enabled)
		// If not already enabled, start a timer so a window 
		// update will get sent to the remote node at some point
	{
		MyTCBStub.Flags.bTimer2Enabled = TRUE;
		MyTCBStub.eventTime2 =
			(WORD) TickGetDiv256() +
			TCP_AUTO_TRANSMIT_TIMEOUT_VAL / 256ull;
	}

	return len + RightLen;
}

#if defined(__18CXX)
WORD TCPFindROMArrayEx(TCP_SOCKET hTCP, const unsigned char *cFindArray,
					   WORD wLen, WORD wStart, WORD wSearchLen,
					   BOOL bTextCompare)
{
	PTR_BASE ptrRead;
	WORD wDataLen;
	WORD wBytesUntilWrap;
	PTR_BASE ptrLocation;
	WORD wLenStart;
	const unsigned char *cFindArrayStart;
	unsigned char i, j, k;
	BOOL isFinding;
	unsigned char buffer[32];
	if (wLen == 0u)
		return 0u;
	SyncTCBStub(hTCP);
	// Find out how many bytes are in the RX FIFO and return 
	// immediately if we won't possibly find a match
	wDataLen = TCPIsGetReady(hTCP) - wStart;
	if (wDataLen < wLen)
		return 0xFFFFu;
	if (wSearchLen && wDataLen > wSearchLen)
		wDataLen = wSearchLen;
	ptrLocation = MyTCBStub.rxTail + wStart;
	if (ptrLocation > MyTCBStub.bufferEnd)
		ptrLocation -= MyTCBStub.bufferEnd - MyTCBStub.bufferRxStart + 1;
	ptrRead = ptrLocation;
	wBytesUntilWrap = MyTCBStub.bufferEnd - ptrLocation + 1;
	ptrLocation = wStart;
	wLenStart = wLen;
	cFindArrayStart = cFindArray;
	j = *cFindArray++;
	isFinding = FALSE;
	if (bTextCompare)
	{
		if (j>='a'&&j<='z')
			j-=32;
	}
	desbordador=0;
	// Search for the array
	while(1)
	{
		if(desbordador++>65000)				// Me aseguro de que salga en algún momento.
		{
			desbordador=0;
			SendTCP(RST|ACK,0);
			CloseSocket();
			return 0xFFFFu;
		}
		// Figure out how big of a chunk to read
		k=sizeof(buffer);
		if(k>wBytesUntilWrap)
			k=wBytesUntilWrap;
		if((WORD)k>wDataLen)
			k=wDataLen;
		// Read a chunk of data into the buffer
		TCPRAMCopy(buffer,TCP_PIC_RAM,(void *) ptrRead,MyTCBStub.vMemoryMedium,(WORD)k);
		ptrRead+=k;
		wBytesUntilWrap-=k;
		if(wBytesUntilWrap==0)
		{
			ptrRead=MyTCBStub.bufferRxStart;
			wBytesUntilWrap=0xFFFFu;
		}
		// Convert everything to uppercase
		if (bTextCompare)
		{
			for(i=0;i<k;i++)
			{
				if(buffer[i]>='a'&&buffer[i]<='z')
					buffer[i]-=32;
				if(j==buffer[i])
				{
					if(--wLen==0)
						return ptrLocation-wLenStart+i+1;
					j=*cFindArray++;
					isFinding=TRUE;
					if(j>='a'&&j<='z')
						j-=32;
				}
				else
				{
					wLen = wLenStart;
					if (isFinding)
					{
						cFindArray = cFindArrayStart;
						j=*cFindArray++;
						if(j>='a'&&j<='z')
							j-=32;
						isFinding = FALSE;
					}
				}
			}
		}
		else					// Compare as is
		{
			for(i = 0; i < k; i++)
			{
				if(j==buffer[i])
				{
					if (--wLen == 0)
						return ptrLocation - wLenStart + i + 1;
					j = *cFindArray++;
					isFinding = TRUE;
				}
				else
				{
					wLen = wLenStart;
					if (isFinding)
					{
						cFindArray = cFindArrayStart;
						j=*cFindArray++;
						isFinding = FALSE;
					}
				}
			}
		}
		// Check to see if it is impossible to find a match
		wDataLen-=k;
		if(wDataLen<wLen)
			return 0xFFFFu;
		ptrLocation+=k;
	}
	return 0xFFFFu;
}
#endif

/*********************************************************************
* Function:        void TCPTick(void)
*
* PreCondition:    TCPInit() is already called.
*
* Input:           None
*
* Output:          Each socket FSM is executed for any timeout
*                  situation.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
void TCPTick(void)
{
	volatile TCP_SOCKET hTCP;
	BOOL bRetransmit;
	BOOL bCloseSocket;
	unsigned char vFlags;
	IP_ADDR ipTemp;

	// Periodically all "not closed" sockets must perform timed operations
	for (hTCP = 0; hTCP < TCP_SOCKET_COUNT; hTCP++)
	{
		SyncTCBStub(hTCP);
		vFlags = 0x00;
		bRetransmit = FALSE;
		bCloseSocket = FALSE;
		// Transmit ASAP data if the medium is available
		if (MyTCBStub.Flags.bTXASAP) {
			if (MACIsTxReady())
				vFlags = ACK;
		}
		// Perform any needed window updates and data transmissions
		if (MyTCBStub.Flags.bTimer2Enabled) {
			// See if the timeout has occured, and we need to send a new window update and pending data
			if ((SHORT) (MyTCBStub.eventTime2 - (WORD) TickGetDiv256()) <=
				(SHORT) 0)
				vFlags = ACK;
		}
		// Process Delayed ACKnowledgement timer
		if (MyTCBStub.Flags.bDelayedACKTimerEnabled) {
			// See if the timeout has occured and delayed ACK needs to be sent
			if ((SHORT)
				(MyTCBStub.OverlappedTimers.delayedACKTime -
				 (WORD) TickGetDiv256()) <= (SHORT) 0)
				vFlags = ACK;
		}
		// Process TCP_CLOSE_WAIT timer
		if (MyTCBStub.smState == TCP_CLOSE_WAIT)
		{
			// Automatically close the socket on our end if the application 
			// fails to call TCPDisconnect() is a reasonable amount of time.
			if ((SHORT)
				(MyTCBStub.OverlappedTimers.closeWaitTime -
				 (WORD) TickGetDiv256()) <= (SHORT) 0) {
				vFlags = FIN | ACK;
				MyTCBStub.smState = TCP_LAST_ACK;
			}
		}
		if (vFlags)
			SendTCP(vFlags, SENDTCP_RESET_TIMERS);
		// The TCP_CLOSED, TCP_LISTEN, and sometimes the TCP_ESTABLISHED 
		// state don't need any timeout events, so see if the timer is enabled
		if (!MyTCBStub.Flags.bTimerEnabled) {
#if defined(TCP_KEEP_ALIVE_TIMEOUT)
			// Only the established state has any use for keep-alives
			if (MyTCBStub.smState == TCP_ESTABLISHED) {
				// If timeout has not occured, do not do anything.
				if ((LONG) (TickGet() - MyTCBStub.eventTime) < (LONG) 0)
					continue;
				SyncTCB();
				SendTCP(ACK, SENDTCP_KEEP_ALIVE);
				MyTCBStub.eventTime = TickGet() + TCP_KEEP_ALIVE_TIMEOUT;
			}
#endif
			continue;
		}
		// If timeout has not occured, do not do anything.
		if ((LONG) (TickGet() - MyTCBStub.eventTime) < (LONG) 0)
			continue;
		// Load up extended TCB information
		SyncTCB();
		// A timeout has occured.  Respond to this timeout condition
		// depending on what state this socket is in.
		switch (MyTCBStub.smState) {
#if defined(STACK_CLIENT_MODE)
#if defined(STACK_USE_DNS)
		case TCP_GET_DNS_MODULE:
			if (DNSBeginUsage()) {
				MyTCBStub.smState = TCP_DNS_RESOLVE;
#if defined(__C30__)
				DNSResolve((unsigned char *) (WORD) MyTCB.remote.
						   dwRemoteHost, DNS_TYPE_A);
#else
				if (MyTCB.flags.bRemoteHostIsROM)
					DNSResolveROM((const unsigned char *) MyTCB.remote.
								  dwRemoteHost, DNS_TYPE_A);
				else
					DNSResolve((unsigned char *) MyTCB.remote.dwRemoteHost,
							   DNS_TYPE_A);
#endif
			}
			break;
		case TCP_DNS_RESOLVE:
			if (DNSIsResolved(&ipTemp)) {
				if (DNSEndUsage()) {
					MyTCBStub.smState = TCP_GATEWAY_SEND_ARP;
					MyTCBStub.remoteHash.Val =
						(ipTemp.w[1] + ipTemp.w[0] +
						 MyTCB.remotePort.Val) ^ MyTCB.localPort.Val;
					MyTCB.remote.niRemoteMACIP.IPAddr.Val = ipTemp.Val;
					MyTCB.retryCount = 0;
					MyTCB.retryInterval = (TICK_SECOND / 4) / 256;
				} else {
					MyTCBStub.eventTime = TickGet() + 10 * TICK_SECOND;
					MyTCBStub.smState = TCP_GET_DNS_MODULE;
				}
			}
			break;
#endif							// #if defined(STACK_USE_DNS)

		case TCP_GATEWAY_SEND_ARP:
			// Obtain the MAC address associated with the server's IP address (either direct MAC address on same subnet, or the MAC address of the Gateway machine)
			MyTCBStub.eventTime2 = TickGetDiv256();
			ARPResolve(&MyTCB.remote.niRemoteMACIP.IPAddr);
			MyTCBStub.smState = TCP_GATEWAY_GET_ARP;
			break;
		case TCP_GATEWAY_GET_ARP:
			// Wait for the MAC address to finish being obtained
			if (!ARPIsResolved
				(&MyTCB.remote.niRemoteMACIP.IPAddr,
				 &MyTCB.remote.niRemoteMACIP.MACAddr)) {
				// Time out if too much time is spent in this state
				// Note that this will continuously send out ARP 
				// requests for an infinite time if the Gateway 
				// never responds
				if (TickGetDiv256() - MyTCBStub.eventTime2 >
					MyTCB.retryInterval) {
					// Exponentially increase timeout until we reach 6 attempts then stay constant
					if (MyTCB.retryCount < 6) {
						MyTCB.retryCount++;
						MyTCB.retryInterval <<= 1;
					}
					// Retransmit ARP request
					MyTCBStub.smState = TCP_GATEWAY_SEND_ARP;
				}
				break;
			}
			// Send out SYN connection request to remote node
			// This automatically disables the Timer from 
			// continuously firing for this socket
			vFlags = SYN;
			bRetransmit = FALSE;
			MyTCBStub.smState = TCP_SYN_SENT;
			break;
#endif							// #if defined(STACK_CLIENT_MODE)

		case TCP_SYN_SENT:
			// Keep sending SYN until we hear from remote node.
			// This may be for infinite time, in that case
			// caller must detect it and do something.
			vFlags = SYN;
			bRetransmit = TRUE;
			break;

		case TCP_SYN_RECEIVED:
			// We must receive ACK before timeout expires.
			// If not, resend SYN+ACK.
			// Abort, if maximum attempts counts are reached.
			if (MyTCB.retryCount < TCP_MAX_SYN_RETRIES) {
				vFlags = SYN | ACK;
				bRetransmit = TRUE;
			} else {
				if (MyTCBStub.Flags.bServer) {
					vFlags = RST | ACK;
					bCloseSocket = TRUE;
				} else {
					vFlags = SYN;
				}
			}
			break;

		case TCP_ESTABLISHED:
		case TCP_CLOSE_WAIT:
			// Retransmit any unacknowledged data
			if (MyTCB.retryCount < TCP_MAX_RETRIES) {
				vFlags = PSH | ACK;
				bRetransmit = TRUE;
			} else {
				// No response back for too long, close connection
				// This could happen, for instance, if the communication 
				// medium was lost
				MyTCBStub.smState = TCP_FIN_WAIT_1;
				vFlags = FIN | ACK;
			}
			break;

		case TCP_FIN_WAIT_1:
			if (MyTCB.retryCount < TCP_MAX_RETRIES) {
				// Send another FIN
				vFlags = FIN | ACK;
				bRetransmit = TRUE;
			} else {
				// Close on our own, we can't seem to communicate 
				// with the remote node anymore
				vFlags = RST | ACK;
				bCloseSocket = TRUE;
			}
			break;

		case TCP_FIN_WAIT_2:
			// Close on our own, we can't seem to communicate 
			// with the remote node anymore
			vFlags = RST | ACK;
			bCloseSocket = TRUE;
			break;

		case TCP_CLOSING:
			if (MyTCB.retryCount < TCP_MAX_RETRIES) {
				// Send another ACK+FIN (the FIN is retransmitted 
				// automatically since it hasn't been acknowledged by 
				// the remote node yet)
				vFlags = ACK;
				bRetransmit = TRUE;
			} else {
				// Close on our own, we can't seem to communicate 
				// with the remote node anymore
				vFlags = RST | ACK;
				bCloseSocket = TRUE;
			}
			break;

		case TCP_LAST_ACK:
			// Send some more FINs or close anyway
			if (MyTCB.retryCount < TCP_MAX_RETRIES) {
				vFlags = FIN | ACK;
				bRetransmit = TRUE;
			} else {
				vFlags = RST | ACK;
				bCloseSocket = TRUE;
			}
			break;
		}

		if (vFlags)
		{
			if (bRetransmit)
			{
				// Set the appropriate retry time
				MyTCB.retryCount++;
				MyTCB.retryInterval <<= 1;
				// Transmit all unacknowledged data over again
				// Roll back unacknowledged TX tail pointer to cause retransmit to occur
				MyTCB.MySEQ -=
					(LONG) (SHORT) (MyTCB.txUnackedTail -
									MyTCBStub.txTail);
				if (MyTCB.txUnackedTail < MyTCBStub.txTail)
					MyTCB.MySEQ -=
						(LONG) (SHORT) (MyTCBStub.bufferRxStart -
										MyTCBStub.bufferTxStart);
				MyTCB.txUnackedTail = MyTCBStub.txTail;
				SendTCP(vFlags, 0);
			}
			else
				SendTCP(vFlags, SENDTCP_RESET_TIMERS);
		}
		if (bCloseSocket)
			CloseSocket();
	}
}
/*********************************************************************
* Function:        BOOL TCPProcess(NODE_INFO* remote,
*                                  IP_ADDR *localIP,
*                                  WORD len)
* PreCondition:    TCPInit() is already called     AND
*                  TCP segment is ready in MAC buffer
* Input:           remote      - Remote node info
*                  len         - Total length of TCP semgent.
* Output:          TRUE if this function has completed its task
*                  FALSE otherwise
* Side Effects:    None
* Overview:        None
* Note:            None
********************************************************************/
BOOL TCPProcess(NODE_INFO * remote, IP_ADDR * localIP, WORD len)
{
	TCP_HEADER TCPHeader;
	PSEUDO_HEADER pseudoHeader;
	WORD_VAL checksum1;
	WORD_VAL checksum2;
	unsigned char optionsSize;

	// Calculate IP pseudoheader checksum.
	pseudoHeader.SourceAddress = remote->IPAddr;
	pseudoHeader.DestAddress = *localIP;
	pseudoHeader.Zero = 0x0;
	pseudoHeader.Protocol = IP_PROT_TCP;
	pseudoHeader.Length = len;

	SwapPseudoHeader(pseudoHeader);

	checksum1.Val = ~CalcIPChecksum((unsigned char *) &pseudoHeader,
									sizeof(pseudoHeader));

	// Now calculate TCP packet checksum in NIC RAM - should match
	// pesudo header checksum
	checksum2.Val = CalcIPBufferChecksum(len);

	// Compare checksums.
	if (checksum1.Val != checksum2.Val) {
		MACDiscardRx();
		return TRUE;
	}
#if defined(DEBUG_GENERATE_RX_LOSS)
	// Throw RX packets away randomly
	if (rand() > DEBUG_GENERATE_RX_LOSS) {
		MACDiscardRx();
		return TRUE;
	}
#endif

	// Retrieve TCP header.
	IPSetRxBuffer(0);
	MACGetArray((unsigned char *) &TCPHeader, sizeof(TCPHeader));
	SwapTCPHeader(&TCPHeader);


	// Skip over options to retrieve data bytes
	optionsSize = (unsigned char) ((TCPHeader.DataOffset.Val << 2) -
								   sizeof(TCPHeader));
	len = len - optionsSize - sizeof(TCPHeader);

	// Prevent loopback injection attacks.  Discard packets 
	// arriving externally and addressed from the loopback IP
	if (remote->IPAddr.Val == LOOPBACK_IP) {
		MACDiscardRx();
		return TRUE;
	}
	// Find matching socket.
	if (FindMatchingSocket(&TCPHeader, remote)) {
		HandleTCPSeg(&TCPHeader, len);
	}
//  else
//  {
//      // \TODO: RFC 793 specifies that if the socket is closed 
//      // and a segment arrives, we should send back a RST if 
//      // the RST bit in the incoming packet is not set.  The 
//      // code to respond with the RST packet and believable SEQ 
//      // and ACK numbers needs to be implemented.
//      //if(!TCPHeader.Flags.bits.flagRST)
//      //  SendTCP(RST, SENDTCP_RESET_TIMERS);
//  }

	// Finished with this packet, discard it and free the Ethernet RAM for new packets
	MACDiscardRx();

	return TRUE;
}


/*********************************************************************
* Function:        static void SendTCP(unsigned char vTCPFlags, unsigned char vSendFlags)
*
* PreCondition:    TCPInit() is already called
*
* Input:		   vTCPFlags: Additional TCP flags to include
*				   vSendFlags: Any combination of SENDTCP_* constants 
*						that will modify the transmit behavior or contents
*							SENDTCP_RESET_TIMERS: Indicates if this packet is a retransmission (no reset) or a new packet (reset required)
*							SENDTCP_KEEP_ALIVE: Instead of transmitting normal data, a garbage octet is transmitted according to RFC 1122 section 4.2.3.6
*						
*
* Output:          A TCP segment is assembled and transmitted
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
static void SendTCP(unsigned char vTCPFlags, unsigned char vSendFlags)
{
	WORD_VAL wVal;
	TCP_HEADER header;
	TCP_OPTIONS options;
	PSEUDO_HEADER pseudoHeader;
	WORD len;
	WORD wEffectiveWindow;

	// Do not send anything on loopbacked ports
	if (MyTCBStub.smState == TCP_LOOPBACK
		|| MyTCBStub.smState == TCP_LOOPBACK_CLOSED)
		return;

	SyncTCB();

	// FINs must be handled specially
	if (vTCPFlags & FIN) {
		MyTCBStub.Flags.bTXFIN = 1;
		vTCPFlags &= ~FIN;
	}
	// Status will now be synched, disable automatic future 
	// status transmissions
	MyTCBStub.Flags.bTimer2Enabled = 0;
	MyTCBStub.Flags.bDelayedACKTimerEnabled = 0;
	MyTCBStub.Flags.bOneSegmentReceived = 0;
	MyTCBStub.Flags.bTXASAP = 0;
	MyTCBStub.Flags.bHalfFullFlush = 0;

	//  Make sure that we can write to the MAC transmit area
	while(!IPIsTxReady());

	// Put all socket application data in the TX space
	if (vTCPFlags & (SYN | RST)) {
		// Don't put any data in SYN and RST messages
		len = 0;
	} else {
		// Begin copying any application data over to the TX space
		if (MyTCBStub.txHead == MyTCB.txUnackedTail) {
			// All caught up on data TX, no real data for this packet
			len = 0;

			// If we are to transmit a FIN, make sure we can put one in this packet
			if (MyTCBStub.Flags.bTXFIN) {
				if (MyTCB.remoteWindow)
					vTCPFlags |= FIN;
			}
		} else if (MyTCBStub.txHead > MyTCB.txUnackedTail) {
			len = MyTCBStub.txHead - MyTCB.txUnackedTail;
			wEffectiveWindow = MyTCB.remoteWindow;
			if (MyTCB.txUnackedTail >= MyTCBStub.txTail)
				wEffectiveWindow -= MyTCB.txUnackedTail - MyTCBStub.txTail;
			else
				wEffectiveWindow -=
					(MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart) -
					(MyTCBStub.txTail - MyTCB.txUnackedTail);

			if (len > wEffectiveWindow)
				len = wEffectiveWindow;

			if (len > TCP_MAX_SEG_SIZE) {
				len = TCP_MAX_SEG_SIZE;
				MyTCBStub.Flags.bTXASAP = 1;
			}
			// If we are to transmit a FIN, make sure we can put one in this packet
			if (MyTCBStub.Flags.bTXFIN) {
				if ((len != wEffectiveWindow) && (len != TCP_MAX_SEG_SIZE))
					vTCPFlags |= FIN;
			}
			// Copy application data into the raw TX buffer
			TCPRAMCopy((void *) (BASE_TX_ADDR + sizeof(ETHER_HEADER) +
								 sizeof(IP_HEADER) + sizeof(TCP_HEADER)),
					   TCP_ETH_RAM, (void *) MyTCB.txUnackedTail,
					   MyTCBStub.vMemoryMedium, len);
			MyTCB.txUnackedTail += len;
		} else {
			pseudoHeader.Length =
				MyTCBStub.bufferRxStart - MyTCB.txUnackedTail;
			len =
				pseudoHeader.Length + MyTCBStub.txHead -
				MyTCBStub.bufferTxStart;

			wEffectiveWindow = MyTCB.remoteWindow;
			if (MyTCB.txUnackedTail >= MyTCBStub.txTail)
				wEffectiveWindow -= MyTCB.txUnackedTail - MyTCBStub.txTail;
			else
				wEffectiveWindow -=
					(MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart) -
					(MyTCBStub.txTail - MyTCB.txUnackedTail);

			if (len > wEffectiveWindow)
				len = wEffectiveWindow;

			if (len > TCP_MAX_SEG_SIZE) {
				len = TCP_MAX_SEG_SIZE;
				MyTCBStub.Flags.bTXASAP = 1;
			}
			// If we are to transmit a FIN, make sure we can put one in this packet
			if (MyTCBStub.Flags.bTXFIN) {
				if ((len != wEffectiveWindow) && (len != TCP_MAX_SEG_SIZE))
					vTCPFlags |= FIN;
			}

			if (pseudoHeader.Length > wEffectiveWindow)
				pseudoHeader.Length = wEffectiveWindow;

			if (pseudoHeader.Length > TCP_MAX_SEG_SIZE)
				pseudoHeader.Length = TCP_MAX_SEG_SIZE;

			// Copy application data into the raw TX buffer
			TCPRAMCopy((void *) (BASE_TX_ADDR + sizeof(ETHER_HEADER) +
								 sizeof(IP_HEADER) + sizeof(TCP_HEADER)),
					   TCP_ETH_RAM, (void *) MyTCB.txUnackedTail,
					   MyTCBStub.vMemoryMedium, pseudoHeader.Length);
			pseudoHeader.Length = len - pseudoHeader.Length;

			// Copy any left over chunks of application data over
			if (pseudoHeader.Length) {
				TCPRAMCopy((void *) (PTR_BASE) (BASE_TX_ADDR +
												sizeof(ETHER_HEADER) +
												sizeof(IP_HEADER) +
												sizeof(TCP_HEADER) +
												(MyTCBStub.bufferRxStart -
												 MyTCB.txUnackedTail)),
						   TCP_ETH_RAM, (void *) MyTCBStub.bufferTxStart,
						   MyTCBStub.vMemoryMedium, pseudoHeader.Length);
			}

			MyTCB.txUnackedTail += len;
			if (MyTCB.txUnackedTail >= MyTCBStub.bufferRxStart)
				MyTCB.txUnackedTail -=
					MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart;
		}
	}

	// Ensure that all packets with data of some kind are 
	// retransmitted by TCPTick() until acknowledged
	// Pure ACK packets with no data are not ACKed back in TCP
	if (len || (vTCPFlags & (SYN | FIN))) {
		if (vSendFlags & SENDTCP_RESET_TIMERS) {
			MyTCB.retryCount = 0;
			MyTCB.retryInterval = TCP_START_TIMEOUT_VAL;
		}

		MyTCBStub.eventTime = TickGet() + MyTCB.retryInterval;
		MyTCBStub.Flags.bTimerEnabled = 1;
	} else if (vSendFlags & SENDTCP_KEEP_ALIVE) {
		// Generate a dummy byte
		MyTCB.MySEQ -= 1;
		len = 1;
	} else if (MyTCBStub.Flags.bTimerEnabled) {
		// If we have data to transmit, but the remote RX window is zero, 
		// so we aren't transmitting any right now then make sure to not 
		// extend the retry counter or timer.  This will stall our TX 
		// with a periodic ACK sent to the remote node.
		if (!(vSendFlags & SENDTCP_RESET_TIMERS)) {
			// Roll back retry counters since we can't send anything
			MyTCB.retryCount--;
			MyTCB.retryInterval >>= 1;
		}

		MyTCBStub.eventTime = TickGet() + MyTCB.retryInterval;
	}


	header.SourcePort = MyTCB.localPort.Val;
	header.DestPort = MyTCB.remotePort.Val;
	header.SeqNumber = MyTCB.MySEQ;
	header.AckNumber = MyTCB.RemoteSEQ;
	header.Flags.bits.Reserved2 = 0;
	header.DataOffset.Reserved3 = 0;
	header.Flags.byte = vTCPFlags;
	header.UrgentPointer = 0;

	// Update our send sequence number and ensure retransmissions 
	// of SYNs and FINs use the right sequence number
	MyTCB.MySEQ += (DWORD) len;
	if (vTCPFlags & SYN) {
		if (MyTCB.flags.bSYNSent)
			header.SeqNumber--;
		else {
			MyTCB.MySEQ++;
			MyTCB.flags.bSYNSent = 1;
		}
	}
	if (vTCPFlags & FIN) {
		if (MyTCB.flags.bFINSent)
			header.SeqNumber--;
		else {
			MyTCB.MySEQ++;
			MyTCB.flags.bFINSent = 1;
		}
	}
	// Calculate the amount of free space in the RX buffer area of this socket
	if (MyTCBStub.rxHead >= MyTCBStub.rxTail)
		header.Window =
			(MyTCBStub.bufferEnd - MyTCBStub.bufferRxStart) -
			(MyTCBStub.rxHead - MyTCBStub.rxTail);
	else
		header.Window = MyTCBStub.rxTail - MyTCBStub.rxHead - 1;

	// Calculate the amount of free space in the MAC RX buffer area and adjust window if needed
	wVal.Val = MACGetFreeRxSize() - 64;
	if ((SHORT) wVal.Val < (SHORT) 0)
		wVal.Val = 0;
	// Force the remote node to throttle back if we are running low on general RX buffer space
	if (header.Window > wVal.Val)
		header.Window = wVal.Val;

	SwapTCPHeader(&header);


	len += sizeof(header);
	header.DataOffset.Val = sizeof(header) >> 2;

	// Insert the MSS (Maximum Segment Size) TCP option if this is SYN packet
	if (vTCPFlags & SYN) {
		len += sizeof(options);
		options.Kind = TCP_OPTIONS_MAX_SEG_SIZE;
		options.Length = 0x04;

		// Load MSS and swap to big endian
		options.MaxSegSize.Val =
			(((TCP_MAX_SEG_SIZE -
			   4) & 0x00FF) << 8) | (((TCP_MAX_SEG_SIZE -
									   4) & 0xFF00) >> 8);

		header.DataOffset.Val += sizeof(options) >> 2;
	}
	// Calculate IP pseudoheader checksum.
	pseudoHeader.SourceAddress = AppConfig.MyIPAddr;
	pseudoHeader.DestAddress = MyTCB.remote.niRemoteMACIP.IPAddr;
	pseudoHeader.Zero = 0x0;
	pseudoHeader.Protocol = IP_PROT_TCP;
	pseudoHeader.Length = len;
	SwapPseudoHeader(pseudoHeader);
	header.Checksum =
		~CalcIPChecksum((unsigned char *) &pseudoHeader,
						sizeof(pseudoHeader));

	// Write IP header
	MACSetWritePtr(BASE_TX_ADDR + sizeof(ETHER_HEADER));
	IPPutHeader(&MyTCB.remote.niRemoteMACIP, IP_PROT_TCP, len);
	MACPutArray((unsigned char *) &header, sizeof(header));
	if (vTCPFlags & SYN)
		MACPutArray((unsigned char *) &options, sizeof(options));

	// Update the TCP checksum
	MACSetReadPtr(BASE_TX_ADDR + sizeof(ETHER_HEADER) + sizeof(IP_HEADER));
	wVal.Val = CalcIPBufferChecksum(len);
#if defined(DEBUG_GENERATE_TX_LOSS)
	// Damage TCP checksums on TX packets randomly
	if (rand() > DEBUG_GENERATE_TX_LOSS) {
		wVal.Val++;
	}
#endif
	MACSetWritePtr(BASE_TX_ADDR + sizeof(ETHER_HEADER) +
				   sizeof(IP_HEADER) + 16);
	MACPutArray((unsigned char *) &wVal, sizeof(WORD));

	// Physically start the packet transmission over the network
	MACFlush();
}



/*********************************************************************
* Function:        static BOOL FindMatchingSocket(TCP_HEADER *h,
*                                      NODE_INFO* remote)
*
* PreCondition:    TCPInit() is already called
*
* Input:           h           - TCP Header to be matched against.
*                  remote      - Node who sent this header.
*
* Output:          A socket that matches with given header and remote
*                  node is searched.
*                  If such socket is found, its index is saved in hCurrentTCP and the function returns TRUE
*                  else INVALID_SOCKET is placed in hCurrentTCP and FALSE is returned
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            This function also loads MyTCBStub and MyTCB upon 
*				   exit if a match was found.
********************************************************************/
static BOOL FindMatchingSocket(TCP_HEADER * h, NODE_INFO * remote)
{
	volatile TCP_SOCKET hTCP;
	TCP_SOCKET partialMatch;
	WORD hash;

	partialMatch = INVALID_SOCKET;
	hash =
		(remote->IPAddr.w[1] + remote->IPAddr.w[0] +
		 h->SourcePort) ^ h->DestPort;

	for (hTCP = 0; hTCP < TCP_SOCKET_COUNT; hTCP++) {
		SyncTCBStub(hTCP);

		if (MyTCBStub.smState == TCP_CLOSED) {
			continue;
		} else if (MyTCBStub.smState == TCP_LISTEN) {
			if (MyTCBStub.remoteHash.Val == h->DestPort)
				partialMatch = hTCP;
			continue;
		} else if (MyTCBStub.remoteHash.Val != hash) {
			continue;
		}

		SyncTCB();
		if (h->DestPort == MyTCB.localPort.Val &&
			h->SourcePort == MyTCB.remotePort.Val &&
			remote->IPAddr.Val == MyTCB.remote.niRemoteMACIP.IPAddr.Val) {
			return TRUE;
		}
	}

	// We are not listening on this port, nor is a socket using it
	if (partialMatch == INVALID_SOCKET)
		return FALSE;

	// Set up the extended TCB with the info needed to establish a 
	// connection and return this socket to the caller
	SyncTCBStub(partialMatch);
	SyncTCB();

	MyTCBStub.remoteHash.Val = hash;

	memcpy((void *) &MyTCB.remote, (void *) remote, sizeof(NODE_INFO));
	MyTCB.remotePort.Val = h->SourcePort;
	MyTCB.localPort.Val = h->DestPort;
	((DWORD_VAL *) (&MyTCB.MySEQ))->w[0] = rand();
	((DWORD_VAL *) (&MyTCB.MySEQ))->w[1] = rand();
	MyTCB.txUnackedTail = MyTCBStub.bufferTxStart;

	return TRUE;
}



/*********************************************************************
* Function:        static void SwapTCPHeader(TCP_HEADER* header)
*
* PreCondition:    None
*
* Input:           header      - TCP Header to be swapped.
*
* Output:          Given header is swapped.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
static void SwapTCPHeader(TCP_HEADER * header)
{
	header->SourcePort = swaps(header->SourcePort);
	header->DestPort = swaps(header->DestPort);
	header->SeqNumber = swapl(header->SeqNumber);
	header->AckNumber = swapl(header->AckNumber);
	header->Window = swaps(header->Window);
	header->Checksum = swaps(header->Checksum);
	header->UrgentPointer = swaps(header->UrgentPointer);
}



/*********************************************************************
* Function:        static void CloseSocket(void)
*
* PreCondition:    SyncTCBStub has been called with the correct TCP 
*				   socket handle to close
*
* Input:           None
*
* Output:          Given socket information is reset and any
*                  buffered bytes held by this socket are discarded.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
static void CloseSocket(void)
{
	SyncTCB();

	MyTCBStub.remoteHash.Val = MyTCB.localPort.Val;
	MyTCBStub.txHead = MyTCBStub.bufferTxStart;
	MyTCBStub.txTail = MyTCBStub.bufferTxStart;
	MyTCBStub.rxHead = MyTCBStub.bufferRxStart;
	MyTCBStub.rxTail = MyTCBStub.bufferRxStart;
	MyTCBStub.smState = MyTCBStub.Flags.bServer ? TCP_LISTEN : TCP_CLOSED;
	MyTCBStub.Flags.bTimerEnabled = 0;
	MyTCBStub.Flags.bTimer2Enabled = 0;
	MyTCBStub.Flags.bDelayedACKTimerEnabled = 0;
	MyTCBStub.Flags.bOneSegmentReceived = 0;
	MyTCBStub.Flags.bHalfFullFlush = 0;
	MyTCBStub.Flags.bTXASAP = 0;
	MyTCBStub.Flags.bTXFIN = 0;
	MyTCBStub.Flags.bSocketReset = 1;

	MyTCB.flags.bFINSent = 0;
	MyTCB.flags.bSYNSent = 0;
	MyTCB.txUnackedTail = MyTCBStub.bufferTxStart;
	((DWORD_VAL *) (&MyTCB.MySEQ))->w[0] = rand();
	((DWORD_VAL *) (&MyTCB.MySEQ))->w[1] = rand();
	MyTCB.sHoleSize = -1;

}



/*********************************************************************
* Function:        static void HandleTCPSeg(TCP_HEADER *h, WORD len)
*
* PreCondition:    TCPInit() is already called     AND
*                  TCPProcess() is the caller.
*				   SyncTCBStub() was called with the correct socket
*
* Input:           h           - TCP Header
*                  len         - Total buffer length.
*
* Output:          TCP FSM is executed on given socket with
*                  given TCP segment.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
static void HandleTCPSeg(TCP_HEADER * h, WORD len)
{
	DWORD dwTemp;
	WORD wTemp;
	LONG lMissingBytes;
	WORD wMissingBytes;
	WORD wFreeSpace;
	unsigned char localHeaderFlags;
	DWORD localAckNumber;
	DWORD localSeqNumber;
	WORD wSegmentLength;
	BOOL bSegmentAcceptable;

	// Cache a few variables in local RAM.  
	// PIC18s take a fair amount of code and execution time to 
	// dereference pointers frequently.
	localHeaderFlags = h->Flags.byte;
	localAckNumber = h->AckNumber;
	localSeqNumber = h->SeqNumber;

	// We received a packet, reset the keep alive timer
#if defined(TCP_KEEP_ALIVE_TIMEOUT)
	if (!MyTCBStub.Flags.bTimerEnabled)
		MyTCBStub.eventTime = TickGet() + TCP_KEEP_ALIVE_TIMEOUT;
#endif

	// Handle TCP_LISTEN and TCP_SYN_SENT states
	// Both of these states will return, so code following this 
	// state machine need not check explicitly for these two 
	// states.
	switch (MyTCBStub.smState) {
	case TCP_LISTEN:
		// First: check RST flag
		if (localHeaderFlags & RST) {
			CloseSocket();		// Unbind remote IP address/port info
			return;
		}
		// Second: check ACK flag, which would be invalid
		if (localHeaderFlags & ACK) {
			// Use a believable sequence number and reset the remote node
			MyTCB.MySEQ = localAckNumber;
			SendTCP(RST, 0);
			CloseSocket();		// Unbind remote IP address/port info
			return;
		}
		// Third: check for SYN flag, which is what we're looking for
		if (localHeaderFlags & SYN) {
			// We now have a sequence number for the remote node
			MyTCB.RemoteSEQ = localSeqNumber + 1;

			// Set Initial Send Sequence (ISS) number
			// Nothing to do on this step... ISS already set in CloseSocket()

			// Respond with SYN + ACK
			SendTCP(SYN | ACK, SENDTCP_RESET_TIMERS);
			MyTCBStub.smState = TCP_SYN_RECEIVED;
		} else {
			CloseSocket();		// Unbind remote IP address/port info
		}

		// Fourth: check for other text and control
		// Nothing to do since we don't support this
		return;

	case TCP_SYN_SENT:
		// Second: check the RST bit
		// This is out of order because this stack has no API for 
		// notifying the application that the connection seems to 
		// be failing.  Instead, the application must time out and 
		// the stack will just keep trying in the mean time.
		if (localHeaderFlags & RST)
			return;

		// First: check ACK bit
		if (localHeaderFlags & ACK) {
			if (localAckNumber != MyTCB.MySEQ) {
				// Send a RST packet with SEQ = SEG.ACK, but retain our SEQ 
				// number for arivial of any other SYN+ACK packets
				localSeqNumber = MyTCB.MySEQ;	// Save our original SEQ number
				MyTCB.MySEQ = localAckNumber;	// Set SEQ = SEG.ACK
				SendTCP(RST, SENDTCP_RESET_TIMERS);	// Send the RST
				MyTCB.MySEQ = localSeqNumber;	// Restore original SEQ number
				return;
			}
		}
		// Third: check the security and precedence
		// No such feature in this stack.  We want to accept all connections.

		// Fourth: check the SYN bit
		if (localHeaderFlags & SYN) {
			// We now have an initial sequence number and window size
			MyTCB.RemoteSEQ = localSeqNumber + 1;
			MyTCB.remoteWindow = h->Window;

			if (localHeaderFlags & ACK) {
				SendTCP(ACK, SENDTCP_RESET_TIMERS);
				MyTCBStub.smState = TCP_ESTABLISHED;
				MyTCBStub.Flags.bTimerEnabled = 0;
			} else {
				SendTCP(SYN | ACK, SENDTCP_RESET_TIMERS);
				MyTCBStub.smState = TCP_SYN_RECEIVED;
			}
		}
		// Fifth: drop the segment if neither SYN or RST is set
		return;
	}

	//
	// First: check the sequence number
	//
	wSegmentLength = len;
	if (localHeaderFlags & FIN)
		wSegmentLength++;
	if (localHeaderFlags & SYN)
		wSegmentLength++;

	// Calculate the RX FIFO space
	if (MyTCBStub.rxHead >= MyTCBStub.rxTail)
		wFreeSpace =
			(MyTCBStub.bufferEnd - MyTCBStub.bufferRxStart) -
			(MyTCBStub.rxHead - MyTCBStub.rxTail);
	else
		wFreeSpace = MyTCBStub.rxTail - MyTCBStub.rxHead - 1;

	// Calculate the number of bytes ahead of our head pointer this segment skips
	lMissingBytes = localSeqNumber - MyTCB.RemoteSEQ;
	wMissingBytes = (WORD) lMissingBytes;

	// Run TCP acceptability tests to verify that this packet has a valid sequence number
	bSegmentAcceptable = FALSE;
	if (wSegmentLength) {
		// Check to see if we have free space, and if so, if any of the data falls within the freespace
		if (wFreeSpace) {
			// RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
			if ((lMissingBytes >= (LONG) 0)
				&& (wFreeSpace > lMissingBytes))
				bSegmentAcceptable = TRUE;
			else {
				// RCV.NXT =< SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND
				if ((lMissingBytes + (LONG) wSegmentLength > (LONG) 0)
					&& (lMissingBytes <=
						(LONG) (SHORT) (wFreeSpace - wSegmentLength)))
					bSegmentAcceptable = TRUE;
			}

			if ((lMissingBytes < (LONG) wFreeSpace)
				&& ((SHORT) wMissingBytes + (SHORT) wSegmentLength >
					(SHORT) 0))
				bSegmentAcceptable = TRUE;
		}
		// Segments with data are not acceptable if we have no free space
	} else {
		// Zero length packets are acceptable if they fall within our free space window
		// SEG.SEQ = RCV.NXT
		if (lMissingBytes == 0) {
			bSegmentAcceptable = TRUE;
		} else {
			// RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
			if ((lMissingBytes >= (LONG) 0)
				&& (wFreeSpace > lMissingBytes))
				bSegmentAcceptable = TRUE;
		}
	}

	if (!bSegmentAcceptable) {
		// Unacceptable segment, drop it and respond appropriately
		if (!(localHeaderFlags & RST))
			SendTCP(ACK, SENDTCP_RESET_TIMERS);
		return;
	}

	//
	// Second: check the RST bit
	//
	//
	// Fourth: check the SYN bit
	//
	// Note, that since the third step is not implemented, we can 
	// combine this second and fourth step into a single operation.
	if (localHeaderFlags & (RST | SYN)) {
		CloseSocket();
		return;
	}
	//
	// Third: check the security and precedence
	//
	// Feature not supported.  Let's process this segment.

	//
	// Fifth: check the ACK bit
	//
	if (!(localHeaderFlags & ACK))
		return;

	switch (MyTCBStub.smState) {
	case TCP_SYN_RECEIVED:
		if (localAckNumber != MyTCB.MySEQ) {
			// Send a RST packet with SEQ = SEG.ACK, but retain our SEQ 
			// number for arivial of any other correct packets
			localSeqNumber = MyTCB.MySEQ;	// Save our original SEQ number
			MyTCB.MySEQ = localAckNumber;	// Set SEQ = SEG.ACK
			SendTCP(RST, SENDTCP_RESET_TIMERS);	// Send the RST
			MyTCB.MySEQ = localSeqNumber;	// Restore original SEQ number
			return;
		}
		MyTCBStub.smState = TCP_ESTABLISHED;
		// No break

	case TCP_ESTABLISHED:
	case TCP_FIN_WAIT_1:
	case TCP_FIN_WAIT_2:
	case TCP_CLOSE_WAIT:
	case TCP_CLOSING:
		// Drop the packet if it ACKs something we haven't sent
		if ((LONG) (MyTCB.MySEQ - localAckNumber) < (LONG) 0) {
			SendTCP(ACK, SENDTCP_RESET_TIMERS);
			return;
		}
		// Throw away all ACKnowledged TX data:
		// Calculate what the last acknowledged sequence number was (ignoring any FINs we sent)
		dwTemp =
			MyTCB.MySEQ - (LONG) (SHORT) (MyTCB.txUnackedTail -
										  MyTCBStub.txTail);
		if (MyTCB.txUnackedTail < MyTCBStub.txTail)
			dwTemp -= MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart;

		// Calcluate how many bytes were ACKed with this packet
		dwTemp = localAckNumber - dwTemp;
		if (((LONG) (dwTemp) > (LONG) 0)
			&& (dwTemp <=
				MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart)) {
			MyTCBStub.Flags.bHalfFullFlush = FALSE;

			// Bytes ACKed, free up the TX FIFO space
			MyTCBStub.txTail += dwTemp;
			if (MyTCBStub.txTail >= MyTCBStub.bufferRxStart) {
				MyTCBStub.txTail -=
					MyTCBStub.bufferRxStart - MyTCBStub.bufferTxStart;
			}
		}
		// No need to keep our retransmit timer going if we have nothing that needs ACKing anymore
		if (MyTCBStub.txTail == MyTCBStub.txHead) {
			// Make sure there isn't a "FIN byte in our TX FIFO"
			if (MyTCBStub.Flags.bTXFIN == 0) {
				MyTCBStub.Flags.bTimerEnabled = 0;
			} else {
				// "Throw away" FIN byte from our TX FIFO if it has been ACKed
				if (MyTCB.MySEQ == localAckNumber) {
					MyTCBStub.Flags.bTimerEnabled = 0;
					MyTCBStub.Flags.bTXFIN = 0;
					MyTCB.flags.bFINSent = 0;
				}
			}
		}
		// Update the local stored copy of the RemoteWindow
		// If previously we had a zero window, and now we don't 
		// immediately send whatever was pending
		if ((MyTCB.remoteWindow == 0u) && h->Window)
			MyTCBStub.Flags.bTXASAP = 1;
		MyTCB.remoteWindow = h->Window;

		// A couple of states must do all of the TCP_ESTABLISHED stuff, but also a little more
		if (MyTCBStub.smState == TCP_FIN_WAIT_1) {
			// Check to see if our FIN has been ACKnowledged
			if (MyTCB.MySEQ == localAckNumber) {
				// Reset our timer for forced closure if the remote node 
				// doesn't send us a FIN in a timely manner.
				MyTCBStub.eventTime = TickGet() + TCP_FIN_WAIT_2_TIMEOUT;
				MyTCBStub.Flags.bTimerEnabled = 1;
				MyTCBStub.smState = TCP_FIN_WAIT_2;
			}
		} else if (MyTCBStub.smState == TCP_FIN_WAIT_2) {
			// RFC noncompliance:
			// The remote node should not keep sending us data 
			// indefinitely after we send a FIN to it.  
			// However, some bad stacks may still keep sending 
			// us data indefinitely after ACKing our FIN.  To 
			// prevent this from locking up our socket, let's 
			// send a RST right now and close forcefully on 
			// our side.
			if (!(localHeaderFlags & FIN)) {
				MyTCB.MySEQ = localAckNumber;	// Set SEQ = SEG.ACK
				SendTCP(RST | ACK, 0);
				CloseSocket();
				return;
			}
		} else if (MyTCBStub.smState == TCP_CLOSING) {
			// Check to see if our FIN has been ACKnowledged
			if (MyTCB.MySEQ == localAckNumber) {
				// RFC not recommended: We should be going to 
				// the TCP_TIME_WAIT state right here and 
				// starting a 2MSL timer, but since we have so 
				// few precious sockets, we can't afford to 
				// leave a socket waiting around doing nothing 
				// for a long time.  If the remote node does 
				// not recieve this ACK, it'll have to figure 
				// out on it's own that the connection is now 
				// closed.
				CloseSocket();
			}

			return;
		}

		break;

	case TCP_LAST_ACK:
		// Check to see if our FIN has been ACKnowledged
		if (MyTCB.MySEQ == localAckNumber)
			CloseSocket();
		return;

//      case TCP_TIME_WAIT:
//          // Nothing is supposed to arrive here.  If it does, reset the quiet timer.
//          SendTCP(ACK, SENDTCP_RESET_TIMERS);
//          return;
	}

	//
	// Sixth: Check the URG bit
	//
	// Urgent packets are not supported in this stack, so we
	// will throw them away instead
	if (localHeaderFlags & URG)
		return;

	//
	// Seventh: Process the segment text
	//
	// Throw data away if in a state that doesn't accept data
	if (MyTCBStub.smState == TCP_CLOSE_WAIT)
		return;
	if (MyTCBStub.smState == TCP_CLOSING)
		return;
	if (MyTCBStub.smState == TCP_LAST_ACK)
		return;
//  if(MyTCBStub.smState == TCP_TIME_WAIT)
//      return;

	// Copy any valid segment data into our RX FIFO, if any
	if (len) {
		// See if there are bytes we must skip
		if ((SHORT) wMissingBytes <= 0) {
			// Position packet read pointer to start of useful data area.
			IPSetRxBuffer((h->DataOffset.Val << 2) - wMissingBytes);
			len += wMissingBytes;

			// Truncate packets that would overflow our TCP RX FIFO
			// and request a retransmit by sending a duplicate ACK
			if (len > wFreeSpace)
				len = wFreeSpace;

			MyTCB.RemoteSEQ += (DWORD) len;

			// Copy the application data from the packet into the socket RX FIFO
			// See if we need a two part copy (spans bufferEnd->bufferRxStart)
			if (MyTCBStub.rxHead + len > MyTCBStub.bufferEnd) {
				wTemp = MyTCBStub.bufferEnd - MyTCBStub.rxHead + 1;
				TCPRAMCopy((void *) MyTCBStub.rxHead,
						   MyTCBStub.vMemoryMedium, (void *) -1,
						   TCP_ETH_RAM, wTemp);
				TCPRAMCopy((void *) MyTCBStub.bufferRxStart,
						   MyTCBStub.vMemoryMedium, (void *) -1,
						   TCP_ETH_RAM, len - wTemp);
				MyTCBStub.rxHead = MyTCBStub.bufferRxStart + (len - wTemp);
			} else {
				TCPRAMCopy((void *) MyTCBStub.rxHead,
						   MyTCBStub.vMemoryMedium, (void *) -1,
						   TCP_ETH_RAM, len);
				MyTCBStub.rxHead += len;
			}

			// See if we have a hole and other data waiting already in the RX FIFO
			if (MyTCB.sHoleSize != -1) {
				MyTCB.sHoleSize -= len;
				wTemp = MyTCB.wFutureDataSize + MyTCB.sHoleSize;

				// See if we just closed up a hole, and if so, advance head pointer
				if ((SHORT) wTemp < (SHORT) 0) {
					MyTCB.sHoleSize = -1;
				} else if (MyTCB.sHoleSize <= 0) {
					MyTCB.RemoteSEQ += wTemp;
					MyTCBStub.rxHead += wTemp;
					if (MyTCBStub.rxHead > MyTCBStub.bufferEnd)
						MyTCBStub.rxHead -=
							MyTCBStub.bufferEnd - MyTCBStub.bufferRxStart +
							1;
					MyTCB.sHoleSize = -1;
				}
			}
		}						// This packet is out of order or we lost a packet, see if we can generate a hole to accomodate it
		else if ((SHORT) wMissingBytes > 0) {
			// Truncate packets that would overflow our TCP RX FIFO
			if (len + wMissingBytes > wFreeSpace)
				len = wFreeSpace - wMissingBytes;

			// Position packet read pointer to start of useful data area.
			IPSetRxBuffer(h->DataOffset.Val << 2);

			// See if we need a two part copy (spans bufferEnd->bufferRxStart)
			if (MyTCBStub.rxHead + wMissingBytes + len >
				MyTCBStub.bufferEnd) {
				// Calculate number of data bytes to copy before wraparound
				wTemp =
					MyTCBStub.bufferEnd - MyTCBStub.rxHead + 1 -
					wMissingBytes;
				if ((SHORT) wTemp >= 0) {
					TCPRAMCopy((void *) (MyTCBStub.rxHead + wMissingBytes),
							   MyTCBStub.vMemoryMedium, (void *) -1,
							   TCP_ETH_RAM, wTemp);
					TCPRAMCopy((void *) MyTCBStub.bufferRxStart,
							   MyTCBStub.vMemoryMedium, (void *) -1,
							   TCP_ETH_RAM, len - wTemp);
				} else {
					TCPRAMCopy((void *) (MyTCBStub.rxHead + wMissingBytes -
										 (MyTCBStub.bufferEnd -
										  MyTCBStub.bufferRxStart + 1)),
							   MyTCBStub.vMemoryMedium, (void *) -1,
							   TCP_ETH_RAM, len);
				}
			} else {
				TCPRAMCopy((void *) (MyTCBStub.rxHead + wMissingBytes),
						   MyTCBStub.vMemoryMedium, (void *) -1,
						   TCP_ETH_RAM, len);
			}

			// Record the hole is here
			if (MyTCB.sHoleSize == -1) {
				MyTCB.sHoleSize = wMissingBytes;
				MyTCB.wFutureDataSize = len;
			} else {
				// We already have a hole, see if we can shrink the hole 
				// or extend the future data size
				if (wMissingBytes < (WORD) MyTCB.sHoleSize) {
					if ((wMissingBytes + len >
						 (WORD) MyTCB.sHoleSize + MyTCB.wFutureDataSize)
						|| (wMissingBytes + len < (WORD) MyTCB.sHoleSize))
						MyTCB.wFutureDataSize = len;
					else
						MyTCB.wFutureDataSize =
							(WORD) MyTCB.sHoleSize +
							MyTCB.wFutureDataSize - wMissingBytes;
					MyTCB.sHoleSize = wMissingBytes;
				} else if (wMissingBytes + len >
						   (WORD) MyTCB.sHoleSize +
						   MyTCB.wFutureDataSize) {
					// Make sure that there isn't a second hole between 
					// our future data and this TCP segment's future data
					if (wMissingBytes <=
						(WORD) MyTCB.sHoleSize + MyTCB.wFutureDataSize)
						MyTCB.wFutureDataSize +=
							wMissingBytes + len - (WORD) MyTCB.sHoleSize -
							MyTCB.wFutureDataSize;
				}

			}
		}
	}
	// Send back an ACK of the data (+SYN | FIN) we just received, 
	// if any.  To minimize bandwidth waste, we are implementing 
	// the delayed acknowledgement algorithm here, only sending 
	// back an immediate ACK if this is the second segment received.  
	// Otherwise, a 200ms timer will cause the ACK to be transmitted.
	if (wSegmentLength) {
		// For non-established sockets, let's delete all data in 
		// the RX buffer immediately after receiving it.  This is 
		// not really how TCP was intended to operate since a 
		// socket cannot receive any response after it sends a FIN,
		// but our TCP application API doesn't readily accomodate
		// receiving data after calling TCPDisconnect(), which 
		// invalidates the application TCP handle.  By deleting all 
		// data, we'll ensure that the RX window is nonzero and 
		// the remote node will be able to send us a FIN response, 
		// which needs an RX window of at least 1.
		if (MyTCBStub.smState != TCP_ESTABLISHED)
			MyTCBStub.rxTail = MyTCBStub.rxHead;

		if (MyTCBStub.Flags.bOneSegmentReceived) {
			SendTCP(ACK, SENDTCP_RESET_TIMERS);
			SyncTCB();
			// bOneSegmentReceived is cleared in SendTCP(), so no need here
		} else {
			MyTCBStub.Flags.bOneSegmentReceived = TRUE;

			// Do not send an ACK immediately back.  Instead, we will 
			// perform delayed acknowledgements.  To do this, we will 
			// just start a timer
			if (!MyTCBStub.Flags.bDelayedACKTimerEnabled) {
				MyTCBStub.Flags.bDelayedACKTimerEnabled = 1;
				MyTCBStub.OverlappedTimers.delayedACKTime =
					(WORD) TickGetDiv256() +
					(WORD) ((TCP_DELAYED_ACK_TIMEOUT) >> 8);
			}
		}
	}
	// See if all data is acknowledged.  If so, there is no need 
	// to keep the retransmission timer running right now.
	if (MyTCBStub.txHead == MyTCBStub.txTail) {
		MyTCBStub.Flags.bTimerEnabled = 0;
	}

	//
	// Eighth: check the FIN bit
	//
	if (localHeaderFlags & FIN) {
		// Note: Since we don't have a good means of storing "FIN bytes" 
		// in our TCP RX FIFO, we must ensure that FINs are processed 
		// in-order.
		if (MyTCB.RemoteSEQ + 1 == localSeqNumber + (DWORD) wSegmentLength) {
			// FINs are treated as one byte of data for ACK sequencing
			MyTCB.RemoteSEQ++;

			switch (MyTCBStub.smState) {
			case TCP_SYN_RECEIVED:
				// RFC in exact: Our API has no need for the user 
				// to explicitly close a socket that never really 
				// got opened fully in the first place, so just 
				// transmit a FIN automatically and jump to 
				// TCP_LAST_ACK
				MyTCBStub.smState = TCP_LAST_ACK;
				SendTCP(FIN | ACK, SENDTCP_RESET_TIMERS);
				return;

			case TCP_ESTABLISHED:
				// Go to TCP_CLOSE_WAIT state
				MyTCBStub.smState = TCP_CLOSE_WAIT;

				// For legacy applications that don't call 
				// TCPDisconnect() as needed and expect the TCP/IP 
				// Stack to automatically close sockets when the 
				// remote node sends a FIN, let's start a timer so 
				// that we will eventually close the socket automatically
				MyTCBStub.OverlappedTimers.closeWaitTime =
					(WORD) TickGetDiv256() +
					(WORD) ((TCP_CLOSE_WAIT_TIMEOUT) >> 8);
				break;

			case TCP_FIN_WAIT_1:
				if (MyTCB.MySEQ == localAckNumber) {
					// RFC not recommended: We should be going to 
					// the TCP_TIME_WAIT state right here and 
					// starting a 2MSL timer, but since we have so 
					// few precious sockets, we can't afford to 
					// leave a socket waiting around doing nothing 
					// for a long time.  If the remote node does 
					// not recieve this ACK, it'll have to figure 
					// out on it's own that the connection is now 
					// closed.
					SendTCP(ACK, 0);
					CloseSocket();
					return;
				} else {
					MyTCBStub.smState = TCP_CLOSING;
				}
				break;

			case TCP_FIN_WAIT_2:
				// RFC not recommended: We should be going to 
				// the TCP_TIME_WAIT state right here and 
				// starting a 2MSL timer, but since we have so 
				// few precious sockets, we can't afford to 
				// leave a socket waiting around doing nothing 
				// for a long time.  If the remote node does 
				// not recieve this ACK, it'll have to figure 
				// out on it's own that the connection is now 
				// closed.
				SendTCP(ACK, 0);
				CloseSocket();
				return;
			}

			// Acknowledge receipt of FIN
			SendTCP(ACK, SENDTCP_RESET_TIMERS);
		}
	}
}

/*********************************************************************
 * Function:        static void TCPRAMCopy( WORD wDest, 
 *											unsigned char wDestType, 
 *											WORD wSource, 
 *											unsigned char wSourceType, 
 *											WORD wLength)
 *
 * PreCondition:    None
 *
 * Input:           wDest: Address to write to
 *					wDestType: TCP_PIC_RAM - wDest pointer is to PIC RAM
 *							   TCP_ETH_RAM - wDest pointer is to Ethernet RAM
 *							   TCP_SPI_RAM - wDest pointer is to external SPI based RAM
 *					wSource: Address to read from
 *					wSourceType: TCP_PIC_RAM - wSource pointer is to PIC RAM
 *							     TCP_ETH_RAM - wSource pointer is to Ethernet RAM
 *							     TCP_SPI_RAM - wSource pointer is to external SPI based RAM
 *					wLength: Number of bytes to copy
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Copies data from one location to any other 
 *					location.  
 *
 * Note:            Copying to a destination region that overlaps 
 *					with the source address is supported only if 
 *					the destination start address is at a lower 
 *					memory address (closer to 0x0000) than the 
 *					source pointer.
********************************************************************/
static void TCPRAMCopy(void *ptrDest, unsigned char vDestType,
					   void *ptrSource, unsigned char vSourceType,
					   WORD wLength)
{
#if defined(SPIRAM_CS_TRIS)
	unsigned char vBuffer[16];
	WORD w;
#endif

	switch (vSourceType) {
	case TCP_PIC_RAM:
		switch (vDestType) {
		case TCP_PIC_RAM:
			memcpy(ptrDest, ptrSource, wLength);
			break;

		case TCP_ETH_RAM:
			if (!((WORD_VAL *) & ptrDest)->bits.b15)
				MACSetWritePtr((WORD) (PTR_BASE) ptrDest);
			MACPutArray((unsigned char *) ptrSource, wLength);
			break;

#if defined(SPIRAM_CS_TRIS)
		case TCP_SPI_RAM:
			SPIRAMPutArray((PTR_BASE) ptrDest, (unsigned char *) ptrSource,
						   wLength);
			break;
#endif
		}
		break;

	case TCP_ETH_RAM:
		switch (vDestType) {
		case TCP_PIC_RAM:
			if (!((WORD_VAL *) & ptrSource)->bits.b15)
				MACSetReadPtr((PTR_BASE) ptrSource);
			MACGetArray((unsigned char *) ptrDest, wLength);
			break;

		case TCP_ETH_RAM:
			MACMemCopyAsync((PTR_BASE) ptrDest, (PTR_BASE) ptrSource,
							wLength);
			while(!MACIsMemCopyDone());
			break;

#if defined(SPIRAM_CS_TRIS)
		case TCP_SPI_RAM:
			if (!((WORD_VAL *) & ptrSource)->bits.b15)
				MACSetReadPtr((PTR_BASE) ptrSource);
			w = sizeof(vBuffer);
			while(wLength)
			{
				if(w>wLength)
					w=wLength;
				// Read and write a chunk   
				MACGetArray(vBuffer, w);
				SPIRAMPutArray((PTR_BASE) ptrDest, vBuffer, w);
				ptrDest+=w;
				wLength-=w;
			}
			break;
#endif
		}
		break;

#if defined(SPIRAM_CS_TRIS)
	case TCP_SPI_RAM:
		switch (vDestType) {
		case TCP_PIC_RAM:
			SPIRAMGetArray((PTR_BASE) ptrSource, (unsigned char *) ptrDest,
						   wLength);
			break;

		case TCP_ETH_RAM:
			if (!((WORD_VAL *) & ptrDest)->bits.b15)
				MACSetWritePtr((PTR_BASE) ptrDest);
			w = sizeof(vBuffer);
			while(wLength)
			{
				if(w>wLength)
					w=wLength;
				// Read and write a chunk   
				SPIRAMGetArray((PTR_BASE) ptrSource, vBuffer, w);
				ptrSource+=w;
				MACPutArray(vBuffer,w);
				wLength-=w;
			}
			break;

		case TCP_SPI_RAM:
			// Copy all of the data over in chunks
			w = sizeof(vBuffer);
			while(wLength)
			{
				if (w > wLength)
					w = wLength;

				SPIRAMGetArray((PTR_BASE) ptrSource, vBuffer, w);
				SPIRAMPutArray((PTR_BASE) ptrDest, vBuffer, w);
				ptrSource += w;
				ptrDest += w;
				wLength -= w;
			}
			break;
		}
		break;
#endif
	}
}

#endif							//#if defined(STACK_USE_TCP)
