/*********************************************************************
 *
 *	File Transfer Protocol (FTP) Client
 *  Module for Microchip TCP/IP Stack
 *	 -Provides ability to remotely upload new MPFS image (web pages) 
 *	  to external EEPROM memory
 *	 -Reference: RFC 959
 *
 *********************************************************************
 * FileName:        FTP.c
 * Dependencies:    TCP, Tick, MPFS, FTPVerify() callback
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
  ********************************************************************/
#define __FTP_C

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_FTP_SERVER)

#define FTP_COMMAND_PORT                (21u)
#define FTP_DATA_PORT                   (20u)
#define FTP_TIMEOUT                     (180ul * TICK_SECOND)
#define MAX_FTP_ARGS                    (7u)
#define MAX_FTP_CMD_STRING_LEN          (31u)

typedef enum _SM_FTP {
	SM_FTP_NOT_CONNECTED,
	SM_FTP_CONNECTED,
	SM_FTP_USER_NAME,
	SM_FTP_USER_PASS,
	SM_FTP_RESPOND
} SM_FTP;

typedef enum _SM_FTP_CMD {
	SM_FTP_CMD_IDLE,
	SM_FTP_CMD_WAIT,
	SM_FTP_CMD_RECEIVE,
	SM_FTP_CMD_WAIT_FOR_DISCONNECT
} SM_FTP_CMD;

typedef enum _FTP_COMMAND {
	FTP_CMD_USER,
	FTP_CMD_PASS,
	FTP_CMD_QUIT,
	FTP_CMD_STOR,
	FTP_CMD_PORT,
	FTP_CMD_ABORT,
	FTP_CMD_PWD,
	FTP_CMD_CWD,
	FTP_CMD_TYPE,
	FTP_CMD_UNKNOWN,
	FTP_CMD_NONE,
} FTP_COMMAND;

// Each entry in following table must match with that of FTP_COMMAND enum.
static const char *FTPCommandString[] = {
	"USER",						// FTP_CMD_USER
	"PASS",						// FTP_CMD_PASS
	"QUIT",						// FTP_CMD_QUIT
	"STOR",						// FTP_CMD_STOR
	"PORT",						// FTP_CMD_PORT
	"ABOR",						// FTP_CMD_ABORT
	"PWD ",						// FTP_CMD_PWD
	"CWD ",						// FTP_CMD_CWD
	"TYPE"						// FTP_CMD_TYPE
};

#define FTP_COMMAND_TABLE_SIZE  ( sizeof(FTPCommandString)/sizeof(FTPCommandString[0]) )

typedef enum _FTP_RESPONSE {
	FTP_RESP_BANNER,
	FTP_RESP_USER_OK,
	FTP_RESP_PASS_OK,
	FTP_RESP_QUIT_OK,
	FTP_RESP_STOR_OK,
	FTP_RESP_UNKNOWN,
	FTP_RESP_LOGIN,
	FTP_RESP_DATA_OPEN,
	FTP_RESP_DATA_READY,
	FTP_RESP_DATA_CLOSE,
	FTP_RESP_DATA_NO_SOCKET,
	FTP_RESP_PWD,
	FTP_RESP_OK,
	FTP_RESP_NONE				// This must always be the last
		// There is no corresponding string.
} FTP_RESPONSE;

// Each entry in following table must match with FTP_RESPONE enum
static const char *FTPResponseString[] = {
	"220 Ready\r\n",			// FTP_RESP_BANNER
	"331 Password required\r\n",	// FTP_RESP_USER_OK
	"230 Logged in\r\n",		// FTP_RESP_PASS_OK
	"221 Bye\r\n",				// FTP_RESP_QUIT_OK
	"500 \r\n",					// FTP_RESP_STOR_OK
	"502 Not implemented\r\n",	// FTP_RESP_UNKNOWN
	"530 Login required\r\n",	// FTP_RESP_LOGIN
	"150 Transferring data...\r\n",	// FTP_RESP_DATA_OPEN
	"125 Done\r\n",				// FTP_RESP_DATA_READY
	"226 Transfer Complete\r\n",	// FTP_RESP_DATA_CLOSE
	"425 Can't create data socket.\r\n",	// FTP_RESP_DATA_NO_SOCKET
	"257 \"/\" is current\r\n",	// FTP_RESP_PWD
	"200 Ok\r\n"				// FTP_RESP_OK
};

static union {
	struct {
		unsigned char bUserSupplied:1;
		unsigned char bLoggedIn:1;
	} Bits;
	unsigned char Val;
} FTPFlags;


static TCP_SOCKET FTPSocket;	// Main ftp command socket.
static TCP_SOCKET FTPDataSocket;	// ftp data socket.
static WORD_VAL FTPDataPort;	// ftp data port number as supplied by client

static SM_FTP smFTP;			// ftp server FSM state
static SM_FTP_CMD smFTPCommand;	// ftp command FSM state

static FTP_COMMAND FTPCommand;
static FTP_RESPONSE FTPResponse;

static unsigned char FTPUser[FTP_USER_NAME_LEN];
static unsigned char FTPString[MAX_FTP_CMD_STRING_LEN + 2];
static unsigned char FTPStringLen;
static unsigned char *FTP_argv[MAX_FTP_ARGS];	// Parameters for a ftp command
static unsigned char FTP_argc;	// Total number of params for a ftp command
static TICK lastActivity;		// Timeout keeper.

static MPFS FTPFileHandle;

// Private helper functions.
static void ParseFTPString(void);
static FTP_COMMAND ParseFTPCommand(unsigned char *cmd);
static void ParseFTPString(void);
static BOOL ExecuteFTPCommand(FTP_COMMAND cmd);
static BOOL PutFile(void);
static BOOL Quit(void);

#define FTP_PUT_ENABLED

/*********************************************************************
 * Function:        void FTPInit(void)
 *
 * PreCondition:    TCP module is already initialized.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Initializes internal variables of FTP
 *
 * Note:
 ********************************************************************/
void FTPInit(void)
{
	FTPSocket =
		TCPOpen(0, TCP_OPEN_SERVER, FTP_COMMAND_PORT,
				TCP_PURPOSE_FTP_COMMAND);
	FTPDataSocket = INVALID_SOCKET;
	smFTP = SM_FTP_NOT_CONNECTED;
	FTPStringLen = 0;
	FTPFlags.Val = 0;
	FTPDataPort.Val = FTP_DATA_PORT;
}

/*********************************************************************
 * Function:        void FTPServer(void)
 *
 * PreCondition:    FTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened FTP connections are served.
 *
 * Side Effects:    None
 *
 * Overview:
 *
 * Note:            This function acts as a task (similar to one in
 *                  RTOS).  This function performs its task in
 *                  co-operative manner.  Main application must call
 *                  this function repeatdly to ensure all open
 *                  or new connections are served on time.
 ********************************************************************/
BOOL FTPServer(void)
{
	unsigned char v;
	TICK currentTick;

	if (!TCPIsConnected(FTPSocket))
	{
		FTPStringLen = 0;
		FTPCommand = FTP_CMD_NONE;
		smFTP = SM_FTP_NOT_CONNECTED;
		FTPFlags.Val = 0;
		smFTPCommand = SM_FTP_CMD_IDLE;
		if (FTPDataSocket != INVALID_SOCKET)
		{
			TCPDisconnect(FTPDataSocket);
			FTPDataSocket = INVALID_SOCKET;
		}
		return TRUE;
	}
	if (TCPIsGetReady(FTPSocket))
	{
		lastActivity = TickGet();

		while (TCPGet(FTPSocket, &v))
		{
			FTPString[FTPStringLen++] = v;
			if (FTPStringLen == MAX_FTP_CMD_STRING_LEN)
				FTPStringLen = 0;
		}
		if (v == '\n') {
			FTPString[FTPStringLen] = '\0';
			FTPStringLen = 0;
			ParseFTPString();
			FTPCommand = ParseFTPCommand(FTP_argv[0]);
		}
	}
	else if (smFTP != SM_FTP_NOT_CONNECTED)
	{
		currentTick = TickGet();
		currentTick = TickGetDiff(currentTick, lastActivity);
		if (currentTick >= FTP_TIMEOUT)
		{
			lastActivity = TickGet();
			FTPCommand = FTP_CMD_QUIT;
			smFTP = SM_FTP_CONNECTED;
		}
	}
	switch (smFTP)
	{
	case SM_FTP_NOT_CONNECTED:
		FTPResponse = FTP_RESP_BANNER;
		lastActivity = TickGet();
		// No break - Continue...
	case SM_FTP_RESPOND:
	  SM_FTP_RESPOND_Label:
		// Make sure there is enough TCP TX FIFO space to put our response
		if (TCPIsPutReady(FTPSocket)<strlenpgm(FTPResponseString[FTPResponse]))
			return TRUE;
		TCPPutROMString(FTPSocket,(const unsigned char *)FTPResponseString[FTPResponse]);
		TCPFlush(FTPSocket);
		FTPResponse = FTP_RESP_NONE;
		smFTP = SM_FTP_CONNECTED;
		// No break - this will speed up little bit
	case SM_FTP_CONNECTED:
		if (FTPCommand != FTP_CMD_NONE)
		{
			if (ExecuteFTPCommand(FTPCommand))
			{
				if (FTPResponse != FTP_RESP_NONE)
					smFTP = SM_FTP_RESPOND;
				else if (FTPCommand == FTP_CMD_QUIT)
					smFTP = SM_FTP_NOT_CONNECTED;

				FTPCommand = FTP_CMD_NONE;
				smFTPCommand = SM_FTP_CMD_IDLE;
			}
			else if (FTPResponse != FTP_RESP_NONE)
			{
				smFTP = SM_FTP_RESPOND;
				goto SM_FTP_RESPOND_Label;
			}
		}
		break;
	}
	return TRUE;
}

static BOOL ExecuteFTPCommand(FTP_COMMAND cmd)
{
	switch (cmd)
	{
	case FTP_CMD_USER:
		FTPFlags.Bits.bUserSupplied = TRUE;
		FTPFlags.Bits.bLoggedIn = FALSE;
		FTPResponse = FTP_RESP_USER_OK;
		strncpy((char *) FTPUser, (char *) FTP_argv[1], sizeof(FTPUser));
		break;
	case FTP_CMD_PASS:
		if (!FTPFlags.Bits.bUserSupplied)
			FTPResponse = FTP_RESP_LOGIN;
		else
		{
			if (FTPVerify(FTPUser, FTP_argv[1]))
			{
				FTPFlags.Bits.bLoggedIn = TRUE;
				FTPResponse = FTP_RESP_PASS_OK;
			}
			else
				FTPResponse = FTP_RESP_LOGIN;
		}
		break;
	case FTP_CMD_QUIT:
		return Quit();
	case FTP_CMD_PORT:
		FTPDataPort.v[1] = (unsigned char) atoi((char *) FTP_argv[5]);
		FTPDataPort.v[0] = (unsigned char) atoi((char *) FTP_argv[6]);
		FTPResponse = FTP_RESP_OK;
		break;
	case FTP_CMD_STOR:
		return PutFile();
	case FTP_CMD_PWD:
		FTPResponse = FTP_RESP_PWD;
		break;
	case FTP_CMD_CWD:
	case FTP_CMD_TYPE:
		FTPResponse = FTP_RESP_OK;
		break;
	case FTP_CMD_ABORT:
		FTPResponse = FTP_RESP_OK;
		if (FTPDataSocket != INVALID_SOCKET)
		{
			TCPDisconnect(FTPDataSocket);
			FTPDataSocket = INVALID_SOCKET;
		}
		break;
	default:
		FTPResponse = FTP_RESP_UNKNOWN;
		break;
	}
	return TRUE;
}

static BOOL Quit(void)
{
	switch (smFTPCommand)
	{
	case SM_FTP_CMD_IDLE:
#if defined(FTP_PUT_ENABLED)
		if (smFTPCommand == SM_FTP_CMD_RECEIVE)
			MPFSClose();
#endif
		if (FTPDataSocket != INVALID_SOCKET)
		{
#if defined(FTP_PUT_ENABLED)
			MPFSClose();
#endif
			TCPDisconnect(FTPDataSocket);
			smFTPCommand = SM_FTP_CMD_WAIT;
		}
		else
			goto Quit_Done;
		break;
	case SM_FTP_CMD_WAIT:
		if (!TCPIsConnected(FTPDataSocket))
		{
		  Quit_Done:
			FTPResponse = FTP_RESP_QUIT_OK;
			smFTPCommand = SM_FTP_CMD_WAIT_FOR_DISCONNECT;
		}
		break;
	case SM_FTP_CMD_WAIT_FOR_DISCONNECT:
		if (TCPIsPutReady(FTPSocket))
		{
			if (TCPIsConnected(FTPSocket))
				TCPDisconnect(FTPSocket);
		}
		break;
	}
	return FALSE;
}

static BOOL PutFile(void)
{
	unsigned char v;
	switch (smFTPCommand)
	{
	case SM_FTP_CMD_IDLE:
		if (!FTPFlags.Bits.bLoggedIn)
		{
			FTPResponse = FTP_RESP_LOGIN;
			return TRUE;
		}
		else
		{
			FTPResponse=FTP_RESP_DATA_OPEN;
			FTPDataSocket=TCPOpen((PTR_BASE) & TCPGetRemoteInfo(FTPSocket)->remote,
						TCP_OPEN_NODE_INFO, FTPDataPort.Val,TCP_PURPOSE_FTP_DATA);
			// Make sure that a valid socket was available and returned
			// If not, return with an error
			if (FTPDataSocket == INVALID_SOCKET) {
				FTPResponse = FTP_RESP_DATA_NO_SOCKET;
				return TRUE;
			}
			smFTPCommand = SM_FTP_CMD_WAIT;
		}
		break;
	case SM_FTP_CMD_WAIT:
		if (TCPIsConnected(FTPDataSocket))
		{
#if defined(FTP_PUT_ENABLED)
			FTPFileHandle = MPFSFormat();
#endif
			smFTPCommand = SM_FTP_CMD_RECEIVE;
		}
		break;
	case SM_FTP_CMD_RECEIVE:
		if (TCPIsGetReady(FTPDataSocket))
		{
			// Reload timeout timer.
			lastActivity = TickGet();
			MPFSPutBegin(FTPFileHandle);
			while (TCPGet(FTPDataSocket, &v))
			{
#if defined(FTP_PUT_ENABLED)
				MPFSPut(v);
#endif
			}
			FTPFileHandle = MPFSPutEnd();
		}
		else if (!TCPIsConnected(FTPDataSocket))
		{
#if defined(FTP_PUT_ENABLED)
			MPFSClose();
#endif
			TCPDisconnect(FTPDataSocket);
			FTPDataSocket = INVALID_SOCKET;
			FTPResponse = FTP_RESP_DATA_CLOSE;
			return TRUE;
		}
	}
	return FALSE;
}

static FTP_COMMAND ParseFTPCommand(unsigned char *cmd)
{
	FTP_COMMAND i;
	for(i=0;i<(FTP_COMMAND)FTP_COMMAND_TABLE_SIZE;i++)
	{
		if(!memcmppgm2ram((void *) cmd,(const void *)FTPCommandString[i],3))
			return i;
	}
	return FTP_CMD_UNKNOWN;
}

static void ParseFTPString(void)
{
	unsigned char *p;
	unsigned char v;
	enum { SM_FTP_PARSE_PARAM, SM_FTP_PARSE_SPACE } smParseFTP;
	smParseFTP = SM_FTP_PARSE_PARAM;
	p= (unsigned char *) &FTPString[0];
	// Skip white blanks
	while (*p == ' ')
		p++;
	FTP_argv[0]=(unsigned char *) p;
	FTP_argc=1;
	while((v=*p))
	{
		switch (smParseFTP)
		{
		case SM_FTP_PARSE_PARAM:
			if (v == ' ' || v == ',')
			{
				*p = '\0';
				smParseFTP=SM_FTP_PARSE_SPACE;
			}
			else if (v =='\r'||v=='\n')
				*p = '\0';
			break;
		case SM_FTP_PARSE_SPACE:
			if (v != ' ')
			{
				FTP_argv[FTP_argc++] = (unsigned char *) p;
				smParseFTP = SM_FTP_PARSE_PARAM;
			}
			break;
		}
		p++;
	}
}

#endif							// #if defined(STACK_USE_FTP_SERVER)
