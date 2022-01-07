/*********************************************************************
 *
 *	Announce Client and Server
 *  Module for Microchip TCP/IP Stack
 *	 -Provides device hostname and IP address discovery on a local 
 *    Ethernet subnet (same broadcast domain)
 *	 -Reference: None.  Hopefully AN833 in the future.
 *
 *********************************************************************
 * FileName:        Announce.c
 * Dependencies:    UDP
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#define __ANNOUNCE_C

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_ANNOUNCE)

#define ANNOUNCE_PORT	30303

extern NODE_INFO remoteNode;

/*********************************************************************
 * Function:        void AnnounceIP(void)
 *
 * PreCondition:    Stack is initialized()
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        AnnounceIP opens a UDP socket and transmits a 
 *					broadcast packet to port 30303.  If a computer is
 *					on the same subnet and a utility is looking for 
 *					packets on the UDP port, it will receive the 
 *					broadcast.  For this application, it is used to 
 *					announce the change of this board's IP address.
 *					The messages can be viewed with the MCHPDetect.exe
 *					program.
 *
 * Note:            A UDP socket must be available before this 
 *					function is called.  It is freed at the end of 
 *					the function.  MAX_UDP_SOCKETS may need to be 
 *					increased if other modules use UDP sockets.
 ********************************************************************/
void AnnounceIP(void)
{
	UDP_SOCKET MySocket;
	unsigned char i;
	MySocket = UDPOpen(2860, NULL, ANNOUNCE_PORT);			// Open a UDP socket for outbound broadcast transmission
	// Abort operation if no UDP sockets are available
	// If this ever happens, incrementing MAX_UDP_SOCKETS in 
	// StackTsk.h may help (at the expense of more global memory 
	// resources).
	if (MySocket == INVALID_UDP_SOCKET)
		return;
	// Make certain the socket can be written to
	while (!UDPIsPutReady(MySocket));
	// Begin sending our MAC address in human readable form.
	// The MAC address theoretically could be obtained from the 
	// packet header when the computer receives our UDP packet, 
	// however, in practice, the OS will abstract away the useful
	// information and it would be difficult to obtain.  It also 
	// would be lost if this broadcast packet were forwarded by a
	// router to a different portion of the network (note that 
	// broadcasts are normally not forwarded by routers).
	UDPPutArray((unsigned char *) AppConfig.NetBIOSName,sizeof(AppConfig.NetBIOSName) - 1);
	UDPPut('\r');
	UDPPut('\n');
	i = 0;
	while (1)											// Convert the MAC address bytes to hex (text) and then send it
	{
		UDPPut(btohexa_high(AppConfig.MyMACAddr.v[i]));
		UDPPut(btohexa_low(AppConfig.MyMACAddr.v[i]));
		if (++i == 6u)
			break;
		UDPPut('-');
	}
	// Send some other human readable information.
	UDPPutROMString((const unsigned char *)"\r\nDHCP/Power event occurred");
	UDPFlush();											// Send the packet
	UDPClose(MySocket);									// Close the socket so it can be used by other modules
}

void DiscoveryTask(void)
{
	static enum
	{
		DISCOVERY_HOME=0,
		DISCOVERY_LISTEN,
		DISCOVERY_REQUEST_RECEIVED,
		DISCOVERY_DISABLED
	} DiscoverySM = DISCOVERY_HOME;
	static UDP_SOCKET MySocket;
	unsigned char i;
	switch (DiscoverySM)
	{
		case DISCOVERY_HOME:
			// Open a UDP socket for inbound and outbound transmission
			// Since we expect to only receive broadcast packets and 
			// only send unicast packets directly to the node we last 
			// received from, the remote NodeInfo parameter can be anything
			MySocket = UDPOpen(ANNOUNCE_PORT, NULL, ANNOUNCE_PORT);
			if (MySocket == INVALID_UDP_SOCKET)
				return;
			else
				DiscoverySM++;
			break;
		case DISCOVERY_LISTEN:
			// Do nothing if no data is waiting
			if (!UDPIsGetReady(MySocket))
				return;
			// See if this is a discovery query or reply
			UDPGet(&i);
			UDPDiscard();
			if (i != 'D')
				return;
			DiscoverySM++;			// We received a discovery request, reply when we can
			// Change the destination to the unicast address of the last received packet
			memcpy((void *) &UDPSocketInfo[MySocket].remoteNode,(const void *) &remoteNode, sizeof(remoteNode));
			// No break needed.  If we get down here, we are now ready for the DISCOVERY_REQUEST_RECEIVED state
		case DISCOVERY_REQUEST_RECEIVED:
			if (!UDPIsPutReady(MySocket))
				return;
			// Begin sending our MAC address in human readable form.
			// The MAC address theoretically could be obtained from the 
			// packet header when the computer receives our UDP packet, 
			// however, in practice, the OS will abstract away the useful
			// information and it would be difficult to obtain.  It also 
			// would be lost if this broadcast packet were forwarded by a
			// router to a different portion of the network (note that 
			// broadcasts are normally not forwarded by routers).
			UDPPutArray((unsigned char *) AppConfig.NetBIOSName,sizeof(AppConfig.NetBIOSName)-1);
			UDPPut('\r');
			UDPPut('\n');
			i = 0;
			while (1)								// Convert the MAC address bytes to hex (text) and then send it
			{
				UDPPut(btohexa_high(AppConfig.MyMACAddr.v[i]));
				UDPPut(btohexa_low(AppConfig.MyMACAddr.v[i]));
				if (++i == 6u)
					break;
				UDPPut('-');
			}
			UDPPut('\r');
			UDPPut('\n');
			UDPFlush();								// Send the packet
			DiscoverySM = DISCOVERY_LISTEN;			// Listen for other discovery requests
			break;
		case DISCOVERY_DISABLED:
			break;
	}
}
#endif							//#if defined(STACK_USE_ANNOUNCE)
