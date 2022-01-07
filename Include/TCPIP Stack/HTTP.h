/*********************************************************************
 *
 *               HTTP definitions on Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        HTTP.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
********************************************************************/
#ifndef __HTTP_H
#define __HTTP_H

#define HTTP_PORT               (80u)

#define HTTP_START_OF_VAR       (0x0000u)
#define HTTP_END_OF_VAR         (0xFFFFu)

/*********************************************************************
 * Function:        void HTTPInit(void)
 *
 * PreCondition:    TCP must already be initialized.
 *
 * Input:           None
 *
 * Output:          HTTP FSM and connections are initialized
 *
 * Side Effects:    None
 *
 * Overview:        Set all HTTP connections to Listening state.
 *                  Initialize FSM for each connection.
 *
 * Note:            This function is called only one during lifetime
 *                  of the application.
 ********************************************************************/
void HTTPInit(void);

/*********************************************************************
 * Function:        void HTTPServer(void)
 *
 * PreCondition:    HTTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened HTTP connections are served.
 *
 * Side Effects:    None
 *
 * Overview:        Browse through each connections and let it
 *                  handle its connection.
 *                  If a connection is not finished, do not process
 *                  next connections.  This must be done, all
 *                  connections use some static variables that are
 *                  common.
 *
 * Note:            This function acts as a task (similar to one in
 *                  RTOS).  This function performs its task in
 *                  co-operative manner.  Main application must call
 *                  this function repeatdly to ensure all open
 *                  or new connections are served on time.
 ********************************************************************/
void HTTPServer(void);

#if defined(__HTTP_C)
	/*
	 * Main application must implement these callback functions
	 * to complete Http.c implementation.
	 */
extern WORD HTTPGetVar(unsigned char var, WORD ref, unsigned char *val);
extern void HTTPExecCmd(unsigned char **argv, unsigned char argc);
#endif

#endif
