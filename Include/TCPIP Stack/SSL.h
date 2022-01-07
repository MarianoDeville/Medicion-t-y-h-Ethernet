/*********************************************************************
 *
 *                  SSLv3 / TLS 1.0 Module Headers
 *
 *********************************************************************
 * FileName:        SSL.h
 * Dependencies:    None
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32MX
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 * Company:         Microchip Technology, Inc.
********************************************************************/

#ifndef __SSL_H
#define __SSL_H

/*********************************************************************
 * Server Configuration Settings
 ********************************************************************/
#define SSL_PORT               	(443u)	//listening port for SSL

#define SSL_VERSION				(0x0300)	//SSL version number
#define SSL_VERSION_HI			(0x03)	//SSL version number
#define SSL_VERSION_LO			(0x00)	//SSL version number

/*********************************************************************
 * Commands and Server Responses
 ********************************************************************/
	//SSL Protocol Codes
#define SSL_CHANGE_CIPHER_SPEC	20u
#define SSL_ALERT				21u
#define SSL_HANDSHAKE			22u
#define SSL_APPLICATION			23u
#define SSL_APPLICATION_DATA	0xFF

typedef enum _SSL_HANDSHAKE_TYPE {
	SSL_HELLO_REQUEST = 0u,
	SSL_CLIENT_HELLO = 1u,
	SSL_ANTIQUE_CLIENT_HELLO = 100u,
	SSL_SERVER_HELLO = 2u,
	SSL_CERTIFICATE = 11u,
	SSL_SERVER_HELLO_DONE = 14u,
	SSL_CLIENT_KEY_EXCHANGE = 16u,
	SSL_FINISHED = 20u
} SSL_HANDSHAKE_TYPE;

typedef enum _SSL_ALERT_LEVEL {
	SSL_ALERT_WARNING = 1u,
	SSL_ALERT_FATAL = 2u
} SSL_ALERT_LEVEL;

typedef enum _SSL_ALERT_DESCRIPTION {
	SSL_ALERT_CLOSE_NOTIFY = 0u,
	SSL_ALERT_UNEXPECTED_MESSAGE = 10u,
	SSL_ALERT_BAD_RECORD_MAC = 20u,
	SSL_ALERT_HANDSHAKE_FAILURE = 40u
} SSL_ALERT_DESCRIPTION;

/*********************************************************************
 * Memory Space Definitions
 ********************************************************************/
	//TODO: this ONLY works for a 512 bit key right now, but conserves
	//      hundreds of bytes of RAM
typedef union _SSL_BUFFER {
	struct {					//used as RSA Buffers in HS
		BIGINT_DATA_TYPE A[2 * RSA_KEY_WORDS];
		BIGINT_DATA_TYPE B[2 * RSA_KEY_WORDS];
	} rsa;
	struct {					//used as handshake MAC calcs
		HASH_SUM hash;
		unsigned char md5_hash[16];
		unsigned char sha_hash[20];
		unsigned char temp[256 - sizeof(HASH_SUM) - 16 - 20];
	}
	handshake;
	unsigned char full[256];	//used as ARCFOUR Sbox in app
} SSL_BUFFER;
typedef struct _SSL_SESSION {
	unsigned char sessionID[32];
	BIGINT_DATA_TYPE masterSecret[RSA_KEY_WORDS];
	unsigned char age;
} SSL_SESSION;
#define _SSL_TEMP_A 	curBuffer.rsa.A
#define _SSL_TEMP_B		curBuffer.rsa.B
#define ARCFOURsbox		curBuffer.full
#define SSL_INVALID_ID	(0xff)

/*********************************************************************
 * SSL State Machines
 ********************************************************************/

	//SSL State Machine
typedef enum _SM_SSL {
	SM_SSL_IDLE = 0u,
	SM_SSL_HANDSHAKE,
	SM_SSL_HANDSHAKE_DONE,
	SM_SSL_APPLICATION,
	SM_SSL_APPLICATION_DONE,
	SM_SSL_CLEANUP
} SM_SSL;
	//SSL Receive State Machine
typedef enum _SM_SSL_RX {
	SM_SSL_RX_IDLE = 0u,
	SM_SSL_RX_READ_DATA,
	SM_SSL_RX_SET_UP_DECRYPT,
	SM_SSL_RX_DECRYPT_PREMASTER_SECRET,
	SM_SSL_RX_CALC_MASTER_SECRET,
	SM_SSL_RX_GENERATE_KEYS
} SM_SSL_RX;
	//SSL Transmit State Machine
typedef enum _SM_SSL_TX {
	SM_SSL_TX_IDLE = 0u,
	SM_SSL_TX_SEND_MESSAGE,
	SM_SSL_TX_SEND_DATA,
	SM_SSL_TX_DISCONNECT,
	SM_SSL_TX_WAIT
} SM_SSL_TX;

/*********************************************************************
 * SSL Connection Struct Definition
 ********************************************************************/

typedef struct _SSL_CONN {
	SM_SSL_RX smRX;						//receiving state machine
	SM_SSL_TX smTX;						//sending state machine
	WORD rxBytesRem;					//bytes left to read in current record
	WORD rxHSBytesRem;					//bytes left in current Handshake submessage
	WORD txBytesRem;					//bytes left to write in current record
	SSL_HANDSHAKE_TYPE rxHSType;		//handshake message being received
	unsigned char rxProtocol;			//protocol for message being read
	unsigned char idSession;			//SSL_SESSION reference
	unsigned char idRXHash, idTXHash;	//SSH_HASH references for app mode
	unsigned char idSHA1, idMD5;		//SSH_HASH references for app mode
	unsigned char idRXSbox, idTXSbox;	//SSL_BUFFER references for S-Boxes
	SSL_ALERT_LEVEL reqAlertLevel;		//requested alert level
	SSL_ALERT_DESCRIPTION reqAlertDesc;	//requested alert type
	//flags relating to the client's current state
	union {
		struct {
			unsigned rxClientHello:1;
			unsigned rxClientKeyExchange:1;
			unsigned rxChangeCipherSpec:1;
			unsigned rxFinished:1;
			unsigned rxCloseNotify:1;
			unsigned okARCFOUR_128_MD5:1;
			unsigned okARCFOUR_40_MD5:1;
			unsigned reserved:1;
		} bits;
		unsigned char byte;
	} clientFlags;
	//flags relating to the server's current state
	union {
		struct {
			unsigned reqServerHello:1;
			unsigned txServerHello:1;
			unsigned reqServerCertDone:1;
			unsigned txServerCertDone:1;
			unsigned reqCCSFin:1;
			unsigned txCCSFin:1;
			unsigned reqAlert:1;
			unsigned txAlert:1;
		} bits;
		unsigned char byte;
	} serverFlags;
	//server area, switches usage for handshake vs application mode
	union {
		struct {
			unsigned char MACSecret[16];	//Server's MAC write secret
			DWORD sequence;					//Server's write sequence number
			ARCFOUR_CTX cryptCtx;			//Server's write encryption context
			unsigned char reserved[8];		//future expansion
		} app;
		unsigned char random[32];			//Server.random value
	} Server;
	//client area, switches usage for handshake vs application mode
	union {
		struct {
			unsigned char MACSecret[16];	//Client's MAC write secret
			DWORD sequence;					//Client's write sequence number
			ARCFOUR_CTX cryptCtx;			//Client's write encryption context
			unsigned char reserved[8];		//future expansion
		} app;
		unsigned char random[32];			//Client.random value
	} Client;
} SSL_CONN;
	//stored in RAM for speed
typedef struct _SSL_STUB {
	TCP_SOCKET SSLskt;						//socket being served
	TCP_SOCKET FWDskt;						//socket to loop app data through
	SM_SSL smSSL;							//SSL state machine
} SSL_STUB;
	//define how much SSL memory we need to reserve in Ethernet buffer
#define SSL_CONN_SIZE		((DWORD)sizeof(SSL_CONN))
#define SSL_CONN_SPACE		(SSL_CONN_SIZE*MAX_SSL_CONNECTIONS)
#define SSL_HASH_SIZE		((DWORD)sizeof(HASH_SUM))
#define SSL_HASH_SPACE		((DWORD)(SSL_HASH_SIZE*MAX_SSL_HASHES))
#define SSL_BUFFER_SIZE		((DWORD)sizeof(SSL_BUFFER))
#define SSL_BUFFER_SPACE	(SSL_BUFFER_SIZE*MAX_SSL_BUFFERS)
#define RESERVED_SSL_MEMORY ((DWORD)(SSL_CONN_SPACE + SSL_HASH_SPACE + SSL_BUFFER_SPACE))
void SSLInit(void);
void SSLServer(void);

#endif
