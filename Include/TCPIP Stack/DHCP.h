/*********************************************************************
 *
 *                  DHCP Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        DHCP.h
 * Dependencies:    TCPIPStack.h
 *                  UDP.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __DHCP_H
#define __DHCP_H

#define DHCP_CLIENT_PORT                (68u)
#define DHCP_SERVER_PORT                (67u)

#define BOOT_REQUEST                    (1u)
#define BOOT_REPLY                      (2u)
#define BOOT_HW_TYPE                    (1u)
#define BOOT_LEN_OF_HW_TYPE             (6u)

#define DHCP_MESSAGE_TYPE               (53u)
#define DHCP_MESSAGE_TYPE_LEN           (1u)

#define DHCP_UNKNOWN_MESSAGE            (0u)

#define DHCP_DISCOVER_MESSAGE           (1u)
#define DHCP_OFFER_MESSAGE              (2u)
#define DHCP_REQUEST_MESSAGE            (3u)
#define DHCP_DECLINE_MESSAGE            (4u)
#define DHCP_ACK_MESSAGE                (5u)
#define DHCP_NAK_MESSAGE                (6u)
#define DHCP_RELEASE_MESSAGE            (7u)

#define DHCP_SERVER_IDENTIFIER          (54u)
#define DHCP_SERVER_IDENTIFIER_LEN      (4u)

#define DHCP_OPTION_ACK_MESSAGE			(53u)
#define DHCP_PARAM_REQUEST_LIST         (55u)
#define DHCP_PARAM_REQUEST_LIST_LEN     (4u)
#define DHCP_PARAM_REQUEST_IP_ADDRESS       (50u)
#define DHCP_PARAM_REQUEST_IP_ADDRESS_LEN   (4u)
#define DHCP_SUBNET_MASK                (1u)
#define DHCP_ROUTER                     (3u)
#define DHCP_DNS						(6u)
#define DHCP_HOST_NAME					(12u)
#define DHCP_IP_LEASE_TIME              (51u)
#define DHCP_END_OPTION                 (255u)


typedef struct __attribute__ ((aligned(2), packed)) {
	unsigned char MessageType;
	unsigned char HardwareType;
	unsigned char HardwareLen;
	unsigned char Hops;
	DWORD TransactionID;
	WORD SecondsElapsed;
	WORD BootpFlags;
	IP_ADDR ClientIP;
	IP_ADDR YourIP;
	IP_ADDR NextServerIP;
	IP_ADDR RelayAgentIP;
	MAC_ADDR ClientMAC;
} BOOTP_HEADER;

typedef enum _SM_DHCP {
	SM_DHCP_DISABLED = 0,
	SM_DHCP_GET_SOCKET,
	SM_DHCP_SEND_DISCOVERY,
	SM_DHCP_GET_OFFER,
	SM_DHCP_SEND_REQUEST,
	SM_DHCP_GET_REQUEST_ACK,
	SM_DHCP_BOUND,
	SM_DHCP_SEND_RENEW,
	SM_DHCP_GET_RENEW_ACK,
	SM_DHCP_SEND_RENEW2,
	SM_DHCP_GET_RENEW_ACK2,
	SM_DHCP_SEND_RENEW3,
	SM_DHCP_GET_RENEW_ACK3
} SM_DHCP;

typedef union _DHCP_CLIENT_FLAGS {
	struct {
		unsigned char bIsBound:1;
		unsigned char bOfferReceived:1;
		unsigned char bDHCPServerDetected:1;
	} bits;
	unsigned char Val;
} DHCP_CLIENT_FLAGS;

#if !defined(__DHCP_C)
extern DHCP_CLIENT_FLAGS DHCPFlags;
extern SM_DHCP smDHCPState;
extern unsigned char DHCPBindCount;
#endif

void DHCPReset(void);
void DHCPTask(void);
void DHCPServerTask(void);
void DHCPDisable(void);
void DHCPEnable(void);

/*********************************************************************
 * Macro:           BOOL DHCPIsBound(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE if DHCP is bound to given configuration
 *                  FALSE if DHCP has yet to be bound.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:
 ********************************************************************/
#define DHCPIsBound()       (DHCPFlags.bits.bIsBound)

#endif
