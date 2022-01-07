/*********************************************************************
 *
 *	TCP/IP Stack Manager
 *  Module for Microchip TCP/IP Stack
 *	 -Handles internal RX packet pre-processing prior to dispatching 
 *    to upper application layers.
 *	 -Reference: AN833
 *
 *********************************************************************
 * FileName:        StackTsk.c
 * Dependencies:    ARP, IP, Ethernet (ENC28J60.c or ETH97J60.c)
 * Processor:       PIC18F67J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
********************************************************************/
#define __STACKTSK_C

#include "TCPIP Stack/TCPIP.h"

// myDHCPBindCount defined in MainDemo.c;  Used to force an IP 
// address display update for IP Gleaning
extern unsigned char myDHCPBindCount;


// Stack FSM states.
typedef enum _SM_STACK {
	SM_STACK_IDLE,
	SM_STACK_MAC,
	SM_STACK_IP,
	SM_STACK_ARP,
	SM_STACK_TCP,
	SM_STACK_UDP
} SM_STACK;
static SM_STACK smStack;

NODE_INFO remoteNode;



/*********************************************************************
 * Function:        void StackInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Stack and its componets are initialized
 *
 * Side Effects:    None
 *
 * Note:            This function must be called before any of the
 *                  stack or its component routines are used.
 *
 ********************************************************************/
void StackInit(void)
{
	smStack = SM_STACK_IDLE;

#if defined(STACK_USE_IP_GLEANING) || defined(STACK_USE_DHCP_CLIENT)
	/*
	 * If DHCP or IP Gleaning is enabled,
	 * startup in Config Mode.
	 */
	AppConfig.Flags.bInConfigMode = TRUE;

#endif

	// Seed the rand() function
	srand(GenerateRandomDWORD());

	MACInit();

	ARPInit();

#if defined(STACK_USE_UDP)
	UDPInit();
#endif

#if defined(STACK_USE_TCP)
	TCPInit();
#endif

}


/*********************************************************************
 * Function:        void StackTask(void)
 *
 * PreCondition:    StackInit() is already called.
 *
 * Input:           None
 *
 * Output:          Stack FSM is executed.
 *
 * Side Effects:    None
 *
 * Note:            This FSM checks for new incoming packets,
 *                  and routes it to appropriate stack components.
 *                  It also performs timed operations.
 *
 *                  This function must be called periodically to
 *                  ensure timely responses.
 *
 ********************************************************************/
void StackTask(void)
{
	WORD dataCount;
	IP_ADDR tempLocalIP;
	unsigned char cFrameType;
	unsigned char cIPFrameType;


#if defined(STACK_USE_DHCP_CLIENT)
	// Normally, an application would not include  DHCP module
	// if it is not enabled. But in case some one wants to disable
	// DHCP module at run-time, remember to not clear our IP
	// address if link is removed.
	if (AppConfig.Flags.bIsDHCPEnabled) {
		if (!MACIsLinked()) {
			AppConfig.MyIPAddr.Val = AppConfig.DefaultIPAddr.Val;
			AppConfig.MyMask.Val = AppConfig.DefaultMask.Val;
			DHCPFlags.bits.bDHCPServerDetected = FALSE;
			AppConfig.Flags.bInConfigMode = TRUE;
			DHCPReset();
		}
		// DHCP must be called all the time even after IP configuration is
		// discovered.
		// DHCP has to account lease expiration time and renew the configuration
		// time.
		DHCPTask();

		if (DHCPIsBound())
			AppConfig.Flags.bInConfigMode = FALSE;
	}
#endif

#if defined(STACK_USE_TCP)
	// Perform all TCP time related tasks (retransmit, send acknowledge, close connection, etc)
	TCPTick();
#endif


	// Process as many incomming packets as we can
	while (1) {
		//if using the random module, generate entropy
#if defined(STACK_USE_RANDOM)
		RandomAdd(remoteNode.MACAddr.v[5]);
#endif

		// We are about to fetch a new packet, make sure that the 
		// UDP module knows that any old RX data it has laying 
		// around will now be gone.
#if defined(STACK_USE_UDP)
		UDPDiscard();
#endif

		// Fetch a packet (throws old one away, if not thrown away 
		// yet)
		if (!MACGetHeader(&remoteNode.MACAddr, &cFrameType))
			break;

		// Dispatch the packet to the appropriate handler
		switch (cFrameType) {
		case MAC_ARP:
			ARPProcess();
			break;

		case MAC_IP:
			if (!IPGetHeader
				(&tempLocalIP, &remoteNode, &cIPFrameType, &dataCount))
				break;

#if defined(STACK_USE_ICMP_SERVER) || defined(STACK_USE_ICMP_CLIENT)
			if (cIPFrameType == IP_PROT_ICMP) {
#if defined(STACK_USE_IP_GLEANING)
				if (AppConfig.Flags.bInConfigMode
					&& AppConfig.Flags.bIsDHCPEnabled) {
					// Accoriding to "IP Gleaning" procedure,
					// when we receive an ICMP packet with a valid
					// IP address while we are still in configuration
					// mode, accept that address as ours and conclude
					// configuration mode.
					if (tempLocalIP.Val != 0xffffffff) {
						AppConfig.Flags.bInConfigMode = FALSE;
						AppConfig.MyIPAddr = tempLocalIP;
						myDHCPBindCount--;
					}
				}
#endif

				// Process this ICMP packet if it the destination IP address matches our address or one of the broadcast IP addressees
				if ((tempLocalIP.Val == AppConfig.MyIPAddr.Val) ||
					(tempLocalIP.Val == 0xFFFFFFFF) ||
					(tempLocalIP.Val ==
					 ((AppConfig.MyIPAddr.Val & AppConfig.MyMask.
					   Val) | ~AppConfig.MyMask.Val))) {
					ICMPProcess(&remoteNode, dataCount);
				}

				break;
			}
#endif

#if defined(STACK_USE_TCP)
			if (cIPFrameType == IP_PROT_TCP) {
				TCPProcess(&remoteNode, &tempLocalIP, dataCount);
				break;
			}
#endif

#if defined(STACK_USE_UDP)
			if (cIPFrameType == IP_PROT_UDP) {
				// Stop processing packets if we came upon a UDP frame with application data in it
				if (UDPProcess(&remoteNode, &tempLocalIP, dataCount))
					return;
			}
#endif

			break;
		}
	}
}
