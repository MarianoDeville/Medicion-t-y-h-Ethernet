/*********************************************************************
 *
 *  Internet Control Message Protocol (ICMP) Server
 *  Module for Microchip TCP/IP Stack
 *   -Provides "ping" diagnostics
 *	 -Reference: RFC 792
 *
 *********************************************************************
 * FileName:        ICMP.c
 * Dependencies:    IP, ARP
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32MX
 * Compiler:		HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#define __ICMP_C

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_ICMP_SERVER) || defined(STACK_USE_ICMP_CLIENT)

#if defined(STACK_USE_ICMP_CLIENT)

#define ICMP_TIMEOUT	(4ul*TICK_SECOND)

typedef struct _ICMP_HEADER {
	unsigned char vType;
	unsigned char vCode;
	WORD_VAL wvChecksum;
	WORD_VAL wvIdentifier;
	WORD_VAL wvSequenceNumber;
} ICMP_HEADER;

static ICMP_HEADER ICMPHeader;
static TICK ICMPTimer;
static struct {
	unsigned char bICMPInUse:1;
	unsigned char bReplyValid:1;
} ICMPFlags = {
0x00};

static NODE_INFO ICMPRemote;
static enum {
	SM_IDLE = 0,
	SM_ARP_RESOLVE,
	SM_GET_ECHO
} ICMPState;
#endif							//#if defined(STACK_USE_ICMP_CLIENT)

/*********************************************************************
 * Function:        void ICMPProcess(void)
 *
 * PreCondition:    MAC buffer contains ICMP type packet.
 *
 * Input:           *remote: Pointer to a NODE_INFO structure of the 
 *					ping requester
 *					len: Count of how many bytes the ping header and 
 *					payload are in this IP packet
 *
 * Output:          Generates an echo reply, if requested
 *					Validates and sets ICMPFlags.bReplyValid if a 
 *					correct ping response to one of ours is received.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void ICMPProcess(NODE_INFO * remote, WORD len)
{
	DWORD_VAL dwVal;
	MACGetArray((unsigned char *) &dwVal, sizeof(dwVal));	// Obtain the ICMP header Type, Code, and Checksum fields
	if (dwVal.w[0] == 0x0008)								// See if this is an ICMP echo (ping) request
	{
		// Validate the checksum using the Microchip MAC's DMA module
		// The checksum data includes the precomputed checksum in the 
		// header, so a valid packet will always have a checksum of 
		// 0x0000 if the packet is not disturbed.
		if (MACCalcRxChecksum(0 + sizeof(IP_HEADER), len))
			return;
		// Calculate new Type, Code, and Checksum values
		dwVal.v[0]=0x00;									// Type: 0 (ICMP echo/ping reply)
		dwVal.v[2]+=8;										// Subtract 0x0800 from the checksum
		if(dwVal.v[2]<8)
		{
			dwVal.v[3]++;
			if(dwVal.v[3]==0)
				dwVal.v[2]++;
		}
		MACSetWritePtr(BASE_TX_ADDR + sizeof(ETHER_HEADER));	// Position the write pointer for the next IPPutHeader operation
		// Wait for TX hardware to become available (finish transmitting 
		// any previous packet)
		while (!IPIsTxReady());
		IPPutHeader(remote, IP_PROT_ICMP, len);				// Create IP header in TX memory
		MACPutArray((unsigned char *) &dwVal, sizeof(dwVal));	// Copy ICMP response into the TX memory
		MACMemCopyAsync(-1, -1, len - 4);
		while (!MACIsMemCopyDone());
		MACFlush();											// Transmit the echo reply packet
	}
#if defined(STACK_USE_ICMP_CLIENT)
	else if (dwVal.w[0] == 0x0000)							// See if this an ICMP Echo reply to our request
	{
		MACGetArray((unsigned char *) &dwVal, sizeof(dwVal));		// Get the sequence number and identifier fields
		if (dwVal.w[0] != 0xEFBE)							// See if the identifier matches the one we sent
			return;
		if (dwVal.w[1] != ICMPHeader.wvSequenceNumber.Val)
			return;
		IPSetRxBuffer(0);									// Validate the ICMP checksum field
		if (CalcIPBufferChecksum(sizeof(ICMP_HEADER) + 2))	// Two bytes of payload were sent in the echo request
			return;
		ICMPFlags.bReplyValid = 1;							// Flag that we received the response and stop the timer ticking
		ICMPTimer = TickGet() - ICMPTimer;
	}
#endif
}
/*********************************************************************
 * Function:        void ICMPSendPing(DWORD dwRemoteIP)
 *
 * PreCondition:    ICMPBeginUsage() returned TRUE
 *
 * Input:           dwRemoteIP: IP Address to ping.  Must be stored 
 *								big endian.  Ex: 192.168.0.1 should be
 *								passed as 0xC0A80001.
 *
 * Output:          Begins the process of transmitting an ICMP echo 
 *					request.  This normally involves an ARP 
 *					resolution procedure first.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
#if defined(STACK_USE_ICMP_CLIENT)
void ICMPSendPing(DWORD dwRemoteIP)
{
	// Figure out the MAC address to send to
	ICMPRemote.IPAddr.Val = dwRemoteIP;
	ARPResolve(&ICMPRemote.IPAddr);
	// Set up the ping packet
	ICMPHeader.vType = 0x08;							// 0x08: Echo (ping) request
	ICMPHeader.vCode = 0x00;
	ICMPHeader.wvChecksum.Val = 0x0000;
	ICMPHeader.wvIdentifier.Val = 0xEFBE;
	ICMPHeader.wvSequenceNumber.Val++;
	ICMPHeader.wvChecksum.Val=CalcIPChecksum((unsigned char *)&ICMPHeader,sizeof(ICMPHeader));
	ICMPTimer = TickGet();								// Kick off the ICMPGetReply() state machine
	ICMPFlags.bReplyValid = 0;
	ICMPState = SM_ARP_RESOLVE;
}
/*********************************************************************
 * Function:        LONG ICMPGetReply(void)
 *
 * PreCondition:    ICMPBeginUsage() returned TRUE and ICMPSendPing() 
 *					was called
 *
 * Input:           None
 *
 * Output:          -2: No response received yet
 *					-1: Operation timed out (longer than ICMP_TIMEOUT) 
 *						has elapsed.
 *					>=0: Number of TICKs that elapsed between 
 *						 initial ICMP transmission and reception of 
 *						 a valid echo.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
LONG ICMPGetReply(void)
{
	switch (ICMPState)
	{
	case SM_IDLE:
		return -1;
	case SM_ARP_RESOLVE:
		if (ARPIsResolved(&ICMPRemote.IPAddr, &ICMPRemote.MACAddr))	// See if the ARP reponse was successfully received
		{
			MACSetWritePtr(BASE_TX_ADDR + sizeof(ETHER_HEADER));	// Position the write pointer for the next IPPutHeader operation
			// Wait for TX hardware to become available (finish transmitting 
			// any previous packet)
			while (!IPIsTxReady());
			IPPutHeader(&ICMPRemote, IP_PROT_ICMP,sizeof(ICMP_HEADER) + 2);	// Create IP header in TX memory
			MACPutArray((unsigned char *) &ICMPHeader, sizeof(ICMPHeader));
			MACPut(0x00);											// Send two dummy bytes as ping payload 
			MACPut(0x00);											// (needed for compatibility with some buggy NAT routers)
			MACFlush();
			ICMPState = SM_GET_ECHO;								// MAC Address resolved and echo sent, advance state
			return -2;
		}
		if (TickGet() - ICMPTimer > ICMP_TIMEOUT)					// See if the ARP/echo request timed out
		{
			ICMPState = SM_IDLE;
			return -1;
		}
		return -2;													// No ARP response back yet
	case SM_GET_ECHO:
		if (ICMPFlags.bReplyValid)									// See if the echo was successfully received
		{
			return (LONG) ICMPTimer;
		}
		if (TickGet() - ICMPTimer > ICMP_TIMEOUT)					// See if the ARP/echo request timed out
		{
			ICMPState = SM_IDLE;
			return -1;
		}
		return -2;													// No echo response back yet
	}
	return -2; // LO AGREGUE YO PORQUE ME DABA WARNING. /////////////////////
}
/*********************************************************************
 * Function:        BOOL ICMPBeginUsage(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE: You have successfully gained ownership of 
 *						  the ICMP client module and can now use the 
 *						  ICMPSendPing() and ICMPGetReply() functions.
 *					FALSE: Some other application is using the ICMP 
 *						   client module.  Calling ICMPSendPing() 
 *						   will corrupt the other application's ping 
 *						   result.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BOOL ICMPBeginUsage(void)
{
	if (ICMPFlags.bICMPInUse)
		return FALSE;
	ICMPFlags.bICMPInUse = TRUE;
	return TRUE;
}
/*********************************************************************
 * Function:        void ICMPEndUsage(void)
 *
 * PreCondition:    ICMPBeginUsage() was called by you and it 
 *					returned TRUE.
 *
 * Input:           None
 *
 * Output:          Your ownership of the ICMP module is released.  
 *					You can no longer use ICMPSendPing().
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void ICMPEndUsage(void)
{
	ICMPFlags.bICMPInUse = FALSE;
}
#endif							//#if defined(STACK_USE_ICMP_CLIENT)

#endif							//#if defined(STACK_USE_ICMP_SERVER) || defined(STACK_USE_ICMP_CLIENT)
