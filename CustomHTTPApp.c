/*********************************************************************
 * FileName:        CustomHTTPApp.c
 * Dependencies:    TCP/IP stack
 * Processor:       PIC18F97J60
 * Complier:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#define __CUSTOMHTTPAPP_C

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_HTTP2_SERVER)

#if defined(HTTP_USE_POST)
#if defined(USE_LCD)
static HTTP_IO_RESULT HTTPPostLCD(void);
#endif
#if defined(STACK_USE_MD5)
static HTTP_IO_RESULT HTTPPostMD5(void);
#endif
#if defined(STACK_USE_APP_RECONFIG)
static HTTP_IO_RESULT HTTPPostConfig(void);
#endif
#endif

extern HTTP_CONN curHTTP;
extern HTTP_STUB httpStubs[MAX_HTTP_CONNECTIONS];
extern unsigned char curHTTPID;

/*********************************************************************
 * Function:        unsigned char HTTPAuthenticate(unsigned char *user, unsigned char *pass, unsigned char *filename)
 *
 * PreCondition:    None
 *
 * Input:           *user: a pointer to the username string
 *					*pass: a pointer to the password string
 *					*filename: a pointer to the file name being requested
 *
 * Output:          0x00-0x79 if authentication fails
 *					0x80-0xff if authentication passes
 *
 * Side Effects:    None
 *
 * Overview:        The function is called at least once per 
 *					connection.  The first call has user and pass set
 *					to NULL, but the filename is set.  This call 
 *					determines if a password is later required, based
 *					only on filename.  Subsequent calls will have a 
 *					user and pass pointer, but the filename will be 
 *					NULL.  These calls occur every time that an
 *					Authorization: header is read and a user/pass
 *					is extracted.  The MSb of the return value 
 *					indicates whether or not authentication passes. 
 *					The remaining bits are application-defined, and 
 *					can be used by your application to manage groups, 
 *					multiple users with different permissions, etc.
 *
 * Note:            Return value is stored in curHTTP.isAuthorized
 ********************************************************************/
#if defined(HTTP_USE_AUTHENTICATION)
unsigned char
HTTPAuthenticate(unsigned char *user, unsigned char *pass,
				 unsigned char *filename)
{
	if (filename) {				// This is the first round...just
		// determine if auth is needed

		// If the filename begins with the folder "protect", then require
		// auth
		if (memcmppgm2ram(filename, (const void *) "protect", 7) == 0)
			return 0x00;		// Authentication will be needed later
#if defined(HTTP_MPFS_UPLOAD_REQUIRES_AUTH)
		if (memcmppgm2ram(filename, (const void *) "mpfsupload", 10) == 0)
			return 0x00;
#endif
		// You can match additional strings here to password protect other 
		// 
		// files.
		// You could switch this and exclude files from authentication.
		// You could also always return 0x00 to require auth for all
		// files.
		// You can return different values (0x00 to 0x79) to track
		// "realms" for below.
		return 0x80;			// No authentication required
	} else {					// This is a user/pass combination
		if (strcmppgm2ram((char *) user, (const char *) "admin") == 0
			&& strcmppgm2ram((char *) pass,
							 (const char *) "microchip") == 0)
			return 0x80;		// We accept this combination
		// You can add additional user/pass combos here.
		// If you return specific "realm" values above, you can base this 
		// decision on what specific file or folder is being accessed.
		// You could return different values (0x80 to 0xff) to indicate 
		// various users or groups, and base future processing decisions
		// in HTTPExecuteGet/Post or HTTPPrint callbacks on this value.
		return 0x00;			// Provided user/pass is invalid
	}
}
#endif

/*********************************************************************
 * Function:        HTTP_IO_RESULT HTTPExecuteGet(void)
 *
 * PreCondition:    curHTTP is loaded
 *
 * Input:           None
 *
 * Output:          HTTP_IO_DONE on success
 *					HTTP_IO_WAITING if waiting for asynchronous process
 *
 * Side Effects:    None
 *
 * Overview:        This function is called if data was read from the
 *					HTTP request from either the GET arguments, or
 *					any cookies sent.  curHTTP.data contains
 *					sequential pairs of strings representing the
 *					data received.  Any required authentication has 
 *					already been validated.
 *
 * Note:            In this simple example, HTTPGetROMArg is used to 
 *					search for values associated with argument names.
 *					At this point, the application may overwrite/modify
 *					curHTTP.data if additional storage associated with
 *					a connection is needed.  Cookies may be set; see
 *					HTTPExecutePostCookies for an example.  For 
 *					redirect functionality, set curHTTP.data to the 
 *					destination and change curHTTP.httpStatus to
 *					HTTP_REDIRECT.
 ********************************************************************/
HTTP_IO_RESULT HTTPExecuteGet(void)
{
	unsigned char *ptr;
	unsigned char filename[20];
	// Load the file name
	// Make sure unsigned char filename[] above is large enough for your
	// longest name
	MPFSGetFilename(curHTTP.file, filename, 20);
	// If its the forms.htm page
	if (!memcmppgm2ram(filename, "forms.htm", 9)) {
		// Seek out each of the four LED strings, and if it exists set the 
		// 
		// LED states
		ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "led4");
		if (ptr)
			LED4_IO = (*ptr == '1');
		ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "led3");
		if (ptr)
			LED3_IO = (*ptr == '1');
		ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "led2");
		if (ptr)
			LED2_IO = (*ptr == '1');
		ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "led1");
		if (ptr)
			LED1_IO = (*ptr == '1');
	}
	// If it's the LED updater file
	else if (!memcmppgm2ram(filename, "cookies.htm", 11)) {
		// This is very simple.  The names and values we want are already
		// in
		// the data array.  We just set the hasArgs value to indicate how
		// many
		// name/value pairs we want stored as cookies.
		// To add the second cookie, just increment this value.
		// remember to also add a dynamic variable callback to control the 
		// 
		// printout.
		curHTTP.hasArgs = 0x01;
	}
	// If it's the LED updater file
	else if (!memcmppgm2ram(filename, "leds.cgi", 8)) {
		// Determine which LED to toggle
		ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "led");
		// Toggle the specified LED
		switch (*ptr) {
		case '1':
			LED1_IO ^= 1;
			break;
		case '2':
			LED2_IO ^= 1;
			break;
		case '3':
			LED3_IO ^= 1;
			break;
		case '4':
			LED4_IO ^= 1;
			break;
		case '5':
			LED5_IO ^= 1;
			break;
		case '6':
			LED6_IO ^= 1;
			break;
		case '7':
			LED7_IO ^= 1;
			break;
		}
	}
	return HTTP_IO_DONE;
}

#if defined(HTTP_USE_POST)
/*********************************************************************
 * Function:        HTTP_IO_RESULT HTTPExecutePost(void)
 *
 * PreCondition:    curHTTP is loaded
 *
 * Input:           None
 *
 * Output:          HTTP_IO_DONE on success
 *					HTTP_IO_NEED_DATA if more data is requested
 *					HTTP_IO_WAITING if waiting for asynchronous process
 *
 * Side Effects:    None
 *
 * Overview:        This function is called if the request method was
 *					POST.  It is called after HTTPExecuteGet and 
 *					after any required authentication has been validated.
 *
 * Note:            In this example, this function calls additional
 *					helpers depending on which file was requested.
 ********************************************************************/
HTTP_IO_RESULT HTTPExecutePost(void)
{
	// Resolve which function to use and pass along
	unsigned char filename[20];
	// Load the file name
	// Make sure unsigned char filename[] above is large enough for your
	// longest name
	MPFSGetFilename(curHTTP.file, filename, 20);
#if defined(USE_LCD)
	if (!memcmppgm2ram(filename, "forms.htm", 9))
		return HTTPPostLCD();
#endif
#if defined(STACK_USE_MD5)
	if (!memcmppgm2ram(filename, "upload.htm", 10))
		return HTTPPostMD5();
#endif
#if defined(STACK_USE_APP_RECONFIG)
	if (!memcmppgm2ram(filename, "protect/config.htm", 18))
		return HTTPPostConfig();
#endif
	return HTTP_IO_DONE;
}

/*********************************************************************
 * Function:        HTTP_IO_RESULT HTTPPostLCD(void)
 *
 * PreCondition:    curHTTP is loaded
 *
 * Input:           None
 *
 * Output:          HTTP_IO_DONE on success
 *					HTTP_IO_NEED_DATA if more data is requested
 *					HTTP_IO_WAITING if waiting for asynchronous process
 *
 * Side Effects:    None
 *
 * Overview:        This function reads an input parameter "lcd" from
 *					the POSTed data, and writes that string to the
 *					board's LCD display.
 *
 * Note:            None
 ********************************************************************/
#if defined(USE_LCD)
static HTTP_IO_RESULT HTTPPostLCD(void)
{
	unsigned char *ptr;
	unsigned short int len;
	// Look for the lcd string
	len =
		TCPFindROMArray(sktHTTP, (const unsigned char *) "lcd=", 4, 0,
						FALSE);
	// If not found, then throw away almost all the data we have and ask
	// for more
	if (len == 0xffff) {
		curHTTP.byteCount -=
			TCPGetArray(sktHTTP, NULL, TCPIsGetReady(sktHTTP) - 4);
		return HTTP_IO_NEED_DATA;
	}
	// Throw away all data preceeding the lcd string
	curHTTP.byteCount -= TCPGetArray(sktHTTP, NULL, len);
	// Look for end of LCD string
	len = TCPFind(sktHTTP, '&', 0, FALSE);
	if (len == 0xffff)
		len = curHTTP.byteCount;
	// If not found, ask for more data
	if (curHTTP.byteCount > TCPIsGetReady(sktHTTP))
		return HTTP_IO_NEED_DATA;
	// Read entire LCD update string into buffer and parse it
	len = TCPGetArray(sktHTTP, curHTTP.data, len);
	curHTTP.byteCount -= len;
	curHTTP.data[len] = '\0';
	ptr = HTTPURLDecode(curHTTP.data);
	ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "lcd");
	// Copy up to 32 characters to the LCD
	if (strlen((char *) curHTTP.data) < 32u) {
		memset(LCDText, ' ', 32);
		strcpy((char *) LCDText, (char *) ptr);
	} else
		memcpy(LCDText, (void *) ptr, 32);
	LCDUpdate();
	strcpypgm2ram((char *) curHTTP.data, (const void *) "forms.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;
	return HTTP_IO_DONE;
}
#endif

/*********************************************************************
 * Function:        HTTP_IO_RESULT HTTPPostConfig(void)
 *
 * PreCondition:    curHTTP is loaded
 *
 * Input:           None
 *
 * Output:          HTTP_IO_DONE on success
 *					HTTP_IO_NEED_DATA if more data is requested
 *					HTTP_IO_WAITING if waiting for asynchronous process
 *
 * Side Effects:    None
 *
 * Overview:        This function reads an input parameter "lcd" from
 *					the POSTed data, and writes that string to the
 *					board's LCD display.
 *
 * Note:            None
 ********************************************************************/
#if defined(STACK_USE_APP_RECONFIG)
extern APP_CONFIG AppConfig;
#define HTTP_POST_CONFIG_MAX_LEN	(HTTP_MAX_DATA_LEN - sizeof(AppConfig) - 1)
static HTTP_IO_RESULT HTTPPostConfig(void)
{
	APP_CONFIG *app;
	unsigned char *ptr;
	unsigned short int len;
	// Set app config pointer to use data array
	app = (APP_CONFIG *) & curHTTP.data[HTTP_POST_CONFIG_MAX_LEN];
	// Use data[0] as a state machine.  0x01 is initialized, 0x02 is
	// error, else uninit
	if (curHTTP.data[0] != 0x01 && curHTTP.data[0] != 0x02) {
		// First run, so use current config as defaults
		memcpy((void *) app, (void *) &AppConfig, sizeof(AppConfig));
		app->Flags.bIsDHCPEnabled = 0;
		curHTTP.data[0] = 0x01;
	}
	// Loop over all parameters
	while (curHTTP.byteCount) {
		// Find end of next parameter string
		len = TCPFind(sktHTTP, '&', 0, FALSE);
		if (len == 0xffff && TCPIsGetReady(sktHTTP) == curHTTP.byteCount)
			len = TCPIsGetReady(sktHTTP);
		// If there's no end in sight, then ask for more data
		if (len == 0xffff)
			return HTTP_IO_NEED_DATA;
		// Read in as much data as we can
		if (len > HTTP_MAX_DATA_LEN - sizeof(AppConfig)) {	// If
			// there's 
			// too
			// much,
			// read as 
			// much as 
			// possible
			curHTTP.byteCount -=
				TCPGetArray(sktHTTP, curHTTP.data + 1,
							HTTP_POST_CONFIG_MAX_LEN);
			curHTTP.byteCount -=
				TCPGetArray(sktHTTP, NULL, len - HTTP_POST_CONFIG_MAX_LEN);
			curHTTP.data[HTTP_POST_CONFIG_MAX_LEN - 1] = '\0';
		} else {				// Otherwise, read as much as we wanted to
			curHTTP.byteCount -=
				TCPGetArray(sktHTTP, curHTTP.data + 1, len);
			curHTTP.data[len + 1] = '\0';
		}
		// Decode the string
		HTTPURLDecode(curHTTP.data + 1);
		// Compare the string to those we're looking for
		if (!memcmppgm2ram(curHTTP.data + 1, "ip\0", 3)) {
			if (StringToIPAddress(&curHTTP.data[3 + 1], &(app->MyIPAddr)))
				memcpy((void *) &(app->DefaultIPAddr),
					   (void *) &(app->MyIPAddr), sizeof(IP_ADDR));
			else
				curHTTP.data[0] = 0x02;
		} else if (!memcmppgm2ram(curHTTP.data + 1, "gw\0", 3)) {
			if (!StringToIPAddress
				(&curHTTP.data[3 + 1], &(app->MyGateway)))
				curHTTP.data[0] = 0x02;
		} else if (!memcmppgm2ram(curHTTP.data + 1, "subnet\0", 7)) {
			if (StringToIPAddress(&curHTTP.data[7 + 1], &(app->MyMask)))
				memcpy((void *) &(app->DefaultMask),
					   (void *) &(app->MyMask), sizeof(IP_ADDR));
			else
				curHTTP.data[0] = 0x02;
		} else if (!memcmppgm2ram(curHTTP.data + 1, "dns1\0", 5)) {
			if (!StringToIPAddress
				(&curHTTP.data[5 + 1], &(app->PrimaryDNSServer)))
				curHTTP.data[0] = 0x02;
		} else if (!memcmppgm2ram(curHTTP.data + 1, "dns2\0", 5)) {
			if (!StringToIPAddress
				(&curHTTP.data[5 + 1], &(app->SecondaryDNSServer)))
				curHTTP.data[0] = 0x02;
		} else if (!memcmppgm2ram(curHTTP.data + 1, "mac\0", 4)) {
			WORD_VAL w;
			unsigned char i, mac[12];
			ptr = &curHTTP.data[4 + 1];
			for (i = 0; i < 12; i++) {	// Read the MAC address
				// Skip non-hex bytes
				while (*ptr != 0x00 && !(*ptr >= '0' && *ptr < '9')
					   && !(*ptr >= 'A' && *ptr <= 'F') && !(*ptr >= 'a'
															 && *ptr <=
															 'f'))
					ptr++;
				// MAC string is over, so zeroize the rest
				if (*ptr == 0x00) {
					for (; i < 12; i++)
						mac[i] = '0';
					break;
				}
				// Save the MAC byte
				mac[i] = *ptr++;
			}
			// Read MAC Address, one byte at a time
			for (i = 0; i < 6; i++) {
				w.v[1] = mac[i * 2];
				w.v[0] = mac[i * 2 + 1];
				app->MyMACAddr.v[i] = hexatob(w);
			}
		} else if (!memcmppgm2ram(curHTTP.data + 1, "host\0", 5)) {
			memset(app->NetBIOSName, ' ', 15);
			app->NetBIOSName[15] = 0x00;
			memcpy((void *) app->NetBIOSName,
				   (void *) &curHTTP.data[5 + 1],
				   strlen((char *) &curHTTP.data[5 + 1]));
			strupr((char *) app->NetBIOSName);
		} else if (!memcmppgm2ram(curHTTP.data + 1, "dhcpenabled\0", 12)) {
			if (curHTTP.data[12 + 1] == '1')
				app->Flags.bIsDHCPEnabled = 1;
			else
				app->Flags.bIsDHCPEnabled = 0;
		}
		// Trash the separator character
		while (TCPFind(sktHTTP, '&', 0, FALSE) == 0
			   || TCPFind(sktHTTP, '\r', 0, FALSE) == 0
			   || TCPFind(sktHTTP, '\n', 0, FALSE) == 0) {
			curHTTP.byteCount -= TCPGet(sktHTTP, NULL);
		}
	}
	// Check if all settings were successful
	if (curHTTP.data[0] == 0x01) {	// Save the new AppConfig
		// If DCHP, then disallow editing of DefaultIP and DefaultMask
		if (app->Flags.bIsDHCPEnabled) {
			// If DHCP is enabled, then reset the default IP and mask
			app->DefaultIPAddr.v[0] = MY_DEFAULT_IP_ADDR_BYTE1;
			app->DefaultIPAddr.v[1] = MY_DEFAULT_IP_ADDR_BYTE2;
			app->DefaultIPAddr.v[2] = MY_DEFAULT_IP_ADDR_BYTE3;
			app->DefaultIPAddr.v[3] = MY_DEFAULT_IP_ADDR_BYTE4;
			app->DefaultMask.v[0] = MY_DEFAULT_MASK_BYTE1;
			app->DefaultMask.v[1] = MY_DEFAULT_MASK_BYTE2;
			app->DefaultMask.v[2] = MY_DEFAULT_MASK_BYTE3;
			app->DefaultMask.v[3] = MY_DEFAULT_MASK_BYTE4;
		}
		ptr = (unsigned char *) app;
#if defined(MPFS_USE_EEPROM)
		XEEBeginWrite(0x0000);
		XEEWrite(0x60);
		for (len = 0; len < sizeof(AppConfig); len++)
			XEEWrite(*ptr++);
		XEEEndWrite();
		while (XEEIsBusy());
#endif
		// Set the board to reboot to the new address
		strcpypgm2ram((char *) curHTTP.data,
					  (const void *) "/protect/reboot.htm?");
		memcpy((void *) (curHTTP.data + 20), (void *) app->NetBIOSName,
			   16);
		ptr = curHTTP.data;
		while (*ptr != ' ' && *ptr != '\0')
			ptr++;
		*ptr = '\0';
	} else {					// Error parsing IP, so don't save to
		// avoid errors
		strcpypgm2ram((char *) curHTTP.data,
					  (const void *) "/protect/config_error.htm");
	}
	curHTTP.httpStatus = HTTP_REDIRECT;
	return HTTP_IO_DONE;
}
#endif							// #if defined(STACK_USE_APP_RECONFIG)

/*********************************************************************
 * Function:        HTTP_IO_RESULT HTTPPostMD5(void)
 *
 * PreCondition:    curHTTP is loaded
 *
 * Input:           None
 *
 * Output:          HTTP_IO_DONE on success
 *					HTTP_IO_NEED_DATA if more data is requested
 *					HTTP_IO_WAITING if waiting for asynchronous process
 *
 * Side Effects:    None
 *
 * Overview:        This function accepts a POSTed file from the client
 *					and calculates its MD5 sum to be returned later.
 *
 * Note:            After the headers, the first line from the form
 *					will be the MIME separator.  Following that is 
 *					more headers about the file, which we discard. 
 *					After another CRLFCRLF, the file data begins, and 
 *					we read it 16 bytes at a time and add that to the 
 *					MD5 calculation.  The reading terminates when the
 *					separator string is encountered again on its own
 *					line.  Notice that the actual file data is trashed
 *					in this process, allowing us to accept files of 
 *					arbitrary size, not limited by RAM.  Also notice
 *					that the data buffer is used as an arbitrary 
 *					storage array for the result.  The %uploadedmd5%
 *					callback reads this data later to send back to 
 *					the client.
 ********************************************************************/
#if defined(STACK_USE_MD5)
static HTTP_IO_RESULT HTTPPostMD5(void)
{
	unsigned short int lenA, lenB;
	static HASH_SUM md5;		// Assume only one simultaneous MD5
	enum {
		SM_MD5_READ_SEPARATOR = 0u,	// Processed as the "default"
		// state
		SM_MD5_SKIP_TO_DATA,
		SM_MD5_READ_DATA
	} smMD5;
	// We don't care about curHTTP.data at this point,
	// so we'll use that for our buffer
	// curHTTP.data[0] is always at least overwritten with the leading '/'
	// from the filename, so we'll use that as a state machine variable.  
	// If it's value isn't 0x01 or 0x02, then we haven't passed the
	// separator.
	switch (curHTTP.data[0]) {
	case SM_MD5_SKIP_TO_DATA:
		// Look for the CRLFCRLF
		lenA =
			TCPFindROMArray(sktHTTP, (const unsigned char *) "\r\n\r\n", 4,
							0, FALSE);
		if (lenA != 0xffff) {	// Found it, so remove all data up to and
			// including
			lenA = TCPGetArray(sktHTTP, NULL, lenA + 4);
			curHTTP.byteCount -= lenA;
			curHTTP.data[0] = SM_MD5_READ_DATA;
		} else {				// Otherwise, remove as much as possible
			lenA = TCPGetArray(sktHTTP, NULL, TCPIsGetReady(sktHTTP) - 4);
			curHTTP.byteCount -= lenA;

			// Return the need more data flag
			return HTTP_IO_NEED_DATA;
		}

		// No break if we found the header terminator

	case SM_MD5_READ_DATA:

		// Find out how many bytes are available to be read
		lenA = TCPIsGetReady(sktHTTP);
		if (lenA > curHTTP.byteCount)
			lenA = curHTTP.byteCount;
		while (lenA > 0) {		// Add up to 64 bytes at a time to the sum
			lenB =
				TCPGetArray(sktHTTP, &(curHTTP.data[1]),
							(lenA < 64) ? lenA : 64);
			curHTTP.byteCount -= lenB;
			lenA -= lenB;
			MD5AddData(&md5, &curHTTP.data[1], lenB);
		}
		// If we've read all the data
		if (curHTTP.byteCount == 0) {	// Calculate and copy result to
			// curHTTP.data for printout
			curHTTP.data[0] = 0x05;
			MD5Calculate(&md5, &(curHTTP.data[1]));
			return HTTP_IO_DONE;
		}
		// Ask for more data
		return HTTP_IO_NEED_DATA;
		// Just started, so try to find the separator string
	case SM_MD5_READ_SEPARATOR:
	default:
		// Reset the MD5 calculation
		MD5Initialize(&md5);
		// See if a CRLF is in the buffer
		lenA =
			TCPFindROMArray(sktHTTP, (const unsigned char *) "\r\n", 2, 0,
							FALSE);
		if (lenA == 0xffff) {	// if not, ask for more data
			return HTTP_IO_NEED_DATA;
		}
		// If so, figure out where the last byte of data is
		// Data ends if CRLFseparator--CRLF, so 6+len bytes
		curHTTP.byteCount -= lenA + 6;
		// Read past the CRLF
		curHTTP.byteCount -= TCPGetArray(sktHTTP, NULL, lenA + 2);
		// Save the next state (skip to CRLFCRLF)
		curHTTP.data[0] = SM_MD5_SKIP_TO_DATA;
		// Ask for more data
		return HTTP_IO_NEED_DATA;
	}
}
#endif							// #if defined(STACK_USE_MD5)

#endif							// (use_post)

/*********************************************************************
 * Function:        void HTTPPrint_varname(TCP_SOCKET sktHTTP, 
 *							DWORD callbackPos, unsigned char *data)
 *
 * PreCondition:    None
 *
 * Input:           sktHTTP: the TCP socket to which to write
 *					callbackPos: 0 initially
 *						return value of last call for subsequent callbacks
 *					data: this connection's data buffer
 *
 * Output:          0 if output is complete
 *					application-defined otherwise
 *
 * Side Effects:    None
 *
 * Overview:        Outputs a variable to the HTTP client.
 *
 * Note:            Return zero to indicate that this callback function 
 *					has finished writing data to the TCP socket.  A 
 *					non-zero return value indicates that more data 
 *					remains to be written, and this callback should 
 *					be called again when more space is available in 
 *					the TCP TX FIFO.  This non-zero return value will 
 *					be the value of the parameter callbackPos for the 
 *					next call.
 ********************************************************************/

const unsigned char HTML_UP_ARROW[] = "up";
const unsigned char HTML_DOWN_ARROW[] = "dn";

void HTTPPrint_btn0(void)
{
	TCPPutROMArray(sktHTTP, (BUTTON0_IO ? HTML_UP_ARROW : HTML_DOWN_ARROW),
				   2);
	return;
}

void HTTPPrint_btn1(void)
{
	TCPPutROMArray(sktHTTP, (BUTTON1_IO ? HTML_UP_ARROW : HTML_DOWN_ARROW),
				   2);
	return;
}

void HTTPPrint_btn2(void)
{
	TCPPutROMArray(sktHTTP, (BUTTON2_IO ? HTML_UP_ARROW : HTML_DOWN_ARROW),
				   2);
	return;
}

void HTTPPrint_btn3(void)
{
	TCPPutROMArray(sktHTTP, (BUTTON3_IO ? HTML_UP_ARROW : HTML_DOWN_ARROW),
				   2);
	return;
}

void HTTPPrint_led1(void)
{
	TCPPut(sktHTTP, (LED1_IO ? '1' : '0'));
	return;
}

void HTTPPrint_led2(void)
{
	TCPPut(sktHTTP, (LED2_IO ? '1' : '0'));
	return;
}

void HTTPPrint_led3(void)
{
	TCPPut(sktHTTP, (LED3_IO ? '1' : '0'));
	return;
}

void HTTPPrint_led4(void)
{
	TCPPut(sktHTTP, (LED4_IO ? '1' : '0'));
	return;
}

void HTTPPrint_led5(void)
{
	TCPPut(sktHTTP, (LED5_IO ? '1' : '0'));
	return;
}

void HTTPPrint_led6(void)
{
	TCPPut(sktHTTP, (LED6_IO ? '1' : '0'));
	return;
}

void HTTPPrint_led7(void)
{
	TCPPut(sktHTTP, (LED7_IO ? '1' : '0'));
	return;
}

const unsigned char HTML_SELECTED[] = "selected";
void HTTPPrint_led4selected(void)
{
	if (LED4_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_led4notselected(void)
{
	if (!LED4_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_led3selected(void)
{
	if (LED3_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_led3notselected(void)
{
	if (!LED3_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_led2selected(void)
{
	if (LED2_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_led2notselected(void)
{
	if (!LED2_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_led1selected(void)
{
	if (LED1_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_led1notselected(void)
{
	if (!LED1_IO)
		TCPPutROMArray(sktHTTP, HTML_SELECTED, 8);
	return;
}

void HTTPPrint_pot0(void)
{
	char AN0String[8];
	unsigned short int ADval;
#if __C30__
	ADval = (unsigned short int) ADC1BUF0;
	// ADval *= (unsigned short int)10;
	// ADval /= (unsigned short int)102;
	uitoa(ADval, (unsigned char *) AN0String);
#else
	// Wait until A/D conversion is done
	ADCON0bits.GO = 1;
	while (ADCON0bits.GO);

	// Convert 10-bit value into ASCII string
	ADval = (unsigned short int) ADRES;
	// ADval *= (unsigned short int)10;
	// ADval /= (unsigned short int)102;
	uitoa(ADval, AN0String);
#endif
	TCPPutArray(sktHTTP, (void *) AN0String, strlen((char *) AN0String));
}

void HTTPPrint_version(void)
{
	TCPPutROMArray(sktHTTP, (const void *) VERSION,
				   strlenpgm((const char *) VERSION));
}

void HTTPPrint_hiddenLED0(void)
{
	TCPPut(sktHTTP, (LED0_IO ? '1' : '0'));
	return;
}

void HTTPPrint_builddate(void)
{
	TCPPutROMArray(sktHTTP, (const void *) __DATE__ " " __TIME__,
				   strlenpgm((const char *) __DATE__ " " __TIME__));
}

void HTTPPrint_lcdtext(void)
{
	unsigned short int len;
	// Determine how many bytes we can write
	len = TCPIsPutReady(sktHTTP);
#if defined(USE_LCD)
	// If just starting, set callbackPos
	if (curHTTP.callbackPos == 0)
		curHTTP.callbackPos = 32;
	// Write a byte at a time while we still can
	// It may take up to 12 bytes to write a character
	// (spaces and newlines are longer)
	while (len > 12 && curHTTP.callbackPos) {
		// After 16 bytes write a newline
		if (curHTTP.callbackPos == 16)
			len -=
				TCPPutROMArray(sktHTTP, (const unsigned char *) "<br />",
							   6);
		if (LCDText[32 - curHTTP.callbackPos] == ' '
			|| LCDText[32 - curHTTP.callbackPos] == '\0')
			len -=
				TCPPutROMArray(sktHTTP, (const unsigned char *) "&nbsp;",
							   6);
		else
			len -= TCPPut(sktHTTP, LCDText[32 - curHTTP.callbackPos]);
		curHTTP.callbackPos--;
	}
#else
	TCPPutROMArray(sktHTTP, (const unsigned char *) "No LCD Present", 14);
#endif
	return;
}

void HTTPPrint_hellomsg(void)
{
	unsigned char *ptr;
	ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "name");
	// We omit checking for space because this is the only data being
	// written
	if (ptr != NULL) {
		TCPPutROMArray(sktHTTP, (const unsigned char *) "Hello, ", 7);
		TCPPutArray(sktHTTP, ptr, strlen((char *) ptr));
	}
	return;
}

void HTTPPrint_cookiename(void)
{
	unsigned char *ptr;
	ptr = HTTPGetROMArg(curHTTP.data, (const unsigned char *) "name");
	if (ptr)
		TCPPutArray(sktHTTP, ptr, strlen((char *) ptr));
	else
		TCPPutROMArray(sktHTTP, (const unsigned char *) "not set", 7);
	return;
}

void HTTPPrint_uploadedmd5(void)
{
	unsigned char c, i;
	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;
	// Make sure there's enough output space
	if (TCPIsPutReady(sktHTTP) < 32 + 37 + 5)
		return;
	// Check for flag set in HTTPPostMD5
	if (curHTTP.data[0] != 0x05) {	// No file uploaded, so just
		// return
		TCPPutROMArray(sktHTTP,
					   (const unsigned char *) "<b>Upload a File</b>", 20);
		curHTTP.callbackPos = 0;
		return;
	}
	TCPPutROMArray(sktHTTP, (const unsigned char *)
				   "<b>Uploaded File's MD5 was:</b><br />", 37);
	// Write a byte of the md5 sum at a time
	for (i = 1; i <= 16; i++) {
		TCPPut(sktHTTP, btohexa_high(curHTTP.data[i]));
		TCPPut(sktHTTP, btohexa_low(curHTTP.data[i]));
		if ((i & 0x03) == 0)
			TCPPut(sktHTTP, ' ');
	}
	curHTTP.callbackPos = 0x00;
	return;
}

extern APP_CONFIG AppConfig;

void HTTPPrintIP(IP_ADDR ip)
{
	unsigned char digits[4];
	unsigned char i;
	for (i = 0; i < 4; i++) {
		if (i != 0)
			TCPPut(sktHTTP, '.');
		uitoa(ip.v[i], digits);
		TCPPutArray(sktHTTP, digits, strlen((char *) digits));
	}
}

void HTTPPrint_config_hostname(void)
{
	TCPPutArray(sktHTTP, AppConfig.NetBIOSName,
				strlen((char *) AppConfig.NetBIOSName));
	return;
}

void HTTPPrint_config_dhcpchecked(void)
{
	if (AppConfig.Flags.bIsDHCPEnabled)
		TCPPutROMArray(sktHTTP, (const unsigned char *) "checked", 7);
	return;
}

void HTTPPrint_config_ip(void)
{
	HTTPPrintIP(AppConfig.MyIPAddr);
	return;
}

void HTTPPrint_config_gw(void)
{
	HTTPPrintIP(AppConfig.MyGateway);
	return;
}

void HTTPPrint_config_subnet(void)
{
	HTTPPrintIP(AppConfig.MyMask);
	return;
}

void HTTPPrint_config_dns1(void)
{
	HTTPPrintIP(AppConfig.PrimaryDNSServer);
	return;
}

void HTTPPrint_config_dns2(void)
{
	HTTPPrintIP(AppConfig.SecondaryDNSServer);
	return;
}

void HTTPPrint_config_mac(void)
{
	unsigned char i;
	if (TCPIsPutReady(sktHTTP) < 18) {	// need 17 bytes to write a MAC
		curHTTP.callbackPos = 0x01;
		return;
	}
	// Write each byte
	for (i = 0; i < 6; i++) {
		if (i != 0)
			TCPPut(sktHTTP, ':');
		TCPPut(sktHTTP, btohexa_high(AppConfig.MyMACAddr.v[i]));
		TCPPut(sktHTTP, btohexa_low(AppConfig.MyMACAddr.v[i]));
	}
	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}

void HTTPPrint_reboot(void)
{
	// This is not so much a print function, but causes the board to
	// reboot
	// when the configuration is changed.  If called via an AJAX call,
	// this
	// will gracefully reset the board and bring it back online
	// immediately
	Reset();
}

void HTTPPrint_rebootaddr(void)
{								// This is the expected address of the
	// board upon rebooting
	TCPPutArray(sktHTTP, curHTTP.data, strlen((char *) curHTTP.data));
}

#endif
