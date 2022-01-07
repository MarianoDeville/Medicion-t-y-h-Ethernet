/*********************************************************************
 *
 *               HTTP Headers for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        HTTP2.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
********************************************************************/

#ifndef __HTTP2_H
#define __HTTP2_H

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_HTTP2_SERVER)

/*********************************************************************
 * Server Configuration Settings
 ********************************************************************/
#define HTTP_PORT               (80u)	// Listening port for HTTP server
#define HTTP_MAX_DATA_LEN		(100u)	// Max bytes to store get and cookie args
#define HTTP_MIN_CALLBACK_FREE	(16u)	// Min bytes free in TX FIFO before callbacks execute
#define HTTP_CACHE_LEN			("600")	// Max lifetime (sec) of static responses as string
#define HTTP_TIMEOUT			(45u)	// Max time (sec) to await more data before

	// Authentication requires Base64 decoding
#if defined(HTTP_USE_AUTHENTICATION)
#ifndef STACK_USE_BASE64_DECODE
#define STACK_USE_BASE64_DECODE
#endif
#endif

/*********************************************************************
 * Commands and Server Responses
 ********************************************************************/
	// Supported Commands and Server Response Codes
typedef enum _HTTP_STATUS {
	HTTP_GET = 0u,				// GET command is being processed
	HTTP_POST,					// POST command is being processed
	HTTP_UNAUTHORIZED,			// 401 Unauthorized will be returned
	HTTP_NOT_FOUND,				// 404 Not Found will be returned
	HTTP_OVERFLOW,				// 414 Request-URI Too Long will be returned
	HTTP_INTERNAL_SERVER_ERROR,	// 500 Internal Server Error will be returned
	HTTP_NOT_IMPLEMENTED,		// 501 Not Implemented (not a GET or POST command)
#if defined(HTTP_MPFS_UPLOAD)
	HTTP_MPFS_FORM,				// Show the MPFS Upload form
	HTTP_MPFS_UP,				// An MPFS Upload is being processed
	HTTP_MPFS_OK,				// An MPFS Upload was successful
	HTTP_MPFS_ERROR,			// An MPFS Upload was not a valid image
#endif
	HTTP_REDIRECT,				// 302 Redirect will be returned
	HTTP_SSL_REQUIRED			// 403 Forbidden is returned, indicating SSL is required
} HTTP_STATUS;

/*********************************************************************
 * HTTP State Definitions
 ********************************************************************/

	// Basic HTTP Connection State Machine
typedef enum _SM_HTTP2 {
	SM_HTTP_IDLE = 0u,			// Socket is idle
	SM_HTTP_PARSE_REQUEST,		// Parses the first line for a file name and GET args
	SM_HTTP_PARSE_HEADERS,		// Reads and parses headers one at a time
	SM_HTTP_AUTHENTICATE,		// Validates the current authorization state
	SM_HTTP_PROCESS_GET,		// Invokes user callback for GET args or cookies
	SM_HTTP_PROCESS_POST,		// Invokes user callback for POSTed data
	SM_HTTP_PROCESS_REQUEST,	// Begins the process of returning data
	SM_HTTP_SERVE_HEADERS,		// Sends any required headers for the response
	SM_HTTP_SERVE_COOKIES,		// Adds any cookies to the response
	SM_HTTP_SERVE_BODY,			// Serves the actual content
	SM_HTTP_SEND_FROM_CALLBACK,	// Invokes a dynamic variable callback
	SM_HTTP_DISCONNECT,			// Disconnects the server and closes all files
	SM_HTTP_WAIT				// Unused state
} SM_HTTP2;

	// Result States for Read and Write Operations
typedef enum _HTTP_IO_RESULT {
	HTTP_IO_DONE = 0u,			// Finished with procedure
	HTTP_IO_NEED_DATA,			// More data needed to continue, call again later
	HTTP_IO_WAITING,			// Waiting for asynchronous process to complete, call again later
} HTTP_IO_RESULT;

	// File Type Definitions
typedef enum _HTTP_FILE_TYPE {
	HTTP_TXT = 0u,				// File is a text document
	HTTP_HTM,					// File is HTML (extension .htm)
	HTTP_HTML,					// File is HTML (extension .html)
	HTTP_CGI,					// File is HTML (extension .cgi)
	HTTP_XML,					// File is XML (extension .xml)
	HTTP_CSS,					// File is stylesheet (extension .css)
	HTTP_GIF,					// File is GIF image (extension .gif)
	HTTP_PNG,					// File is PNG image (extension .png)
	HTTP_JPG,					// File is JPG image (extension .jpg)
	HTTP_JAVA,					// File is java (extension .java)
	HTTP_WAV,					// File is audio (extension .wav)
	HTTP_UNKNOWN				// File type is unknown
} HTTP_FILE_TYPE;

	// HTTP Connection Struct
	// Stores partial state data for each connection
	// Meant for storage in fast access RAM
typedef struct _HTTP_STUB {
	SM_HTTP2 sm;				// Current connection state
	TCP_SOCKET socket;			// Socket being served
} HTTP_STUB;

#define sktHTTP		httpStubs[curHTTPID].socket	// Access the current socket
#define smHTTP		httpStubs[curHTTPID].sm	// Access the current state machine

	// Stores extended state data for each connection
typedef struct _HTTP_CONN {
	DWORD byteCount;			// How many bytes have been read so far
	DWORD nextCallback;			// Byte index of the next callback
	DWORD callbackID;			// Callback ID to execute, also used as watchdog timer
	DWORD callbackPos;			// Callback position indicator
	unsigned char *ptrData;		// Points to first free byte in data
	unsigned char *ptrRead;		// Points to current read location
	MPFS_HANDLE file;			// File pointer for the file being served
	MPFS_HANDLE offsets;		// File pointer for any offset info being used
	unsigned char hasArgs;		// True if there were get or cookie arguments   
	unsigned char isAuthorized;	// 0x00-0x79 on fail, 0x80-0xff on pass
	HTTP_STATUS httpStatus;		// Request method/status
	HTTP_FILE_TYPE fileType;	// File type to return with Content-Type
	unsigned char data[HTTP_MAX_DATA_LEN];	// General purpose data buffer
} HTTP_CONN;

#define RESERVED_HTTP_MEMORY ( (DWORD)MAX_HTTP_CONNECTIONS * (DWORD)sizeof(HTTP_CONN))

void HTTPInit(void);
void HTTPServer(void);

	/*
	 * Main application must implement these callback functions
	 * to complete HTTP2 implementation.
	 * These functions may set cookies by setting curHTTP.hasArgs to
	 *   indicate the number of cookies, and curHTTP.data with null terminated
	 *   name and value pairs.  (Both name and value are null terminated.)
	 * These functions may store info in curHTTP.data, but will be overwriting
	 * any GET or cookie arguments.
	 */
HTTP_IO_RESULT HTTPExecuteGet(void);
#ifdef HTTP_USE_POST
HTTP_IO_RESULT HTTPExecutePost(void);
#endif

#ifdef HTTP_USE_AUTHENTICATION
extern unsigned char HTTPAuthenticate(unsigned char *user,unsigned char *pass,unsigned char *filename);
#endif

unsigned char *HTTPURLDecode(unsigned char *data);
unsigned char *HTTPGetArg(unsigned char *data, unsigned char *arg);
void HTTPIncFile(const unsigned char *file);

		// const function variants for PIC18
unsigned char *HTTPGetROMArg(unsigned char *data, const unsigned char *arg);

#endif

#endif
