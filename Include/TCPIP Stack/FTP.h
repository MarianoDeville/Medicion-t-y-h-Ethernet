/*********************************************************************
 *
 *                  FTP Server Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        ftp.h
 * Dependencies:    StackTsk.h
 *                  tcp.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __FTP_H
#define __FTP_H

#define FTP_USER_NAME_LEN       (10)

/*********************************************************************
 * Function:        BOOL FTPVerify(char *login, char *password)
 *
 * PreCondition:    None
 *
 * Input:           login       - Telnet User login name
 *                  password    - Telnet User password
 *
 * Output:          TRUE if login and password verfies
 *                  FALSE if login and password does not match.
 *
 * Side Effects:    None
 *
 * Overview:        Compare given login and password with internal
 *                  values and return TRUE or FALSE.
 *
 * Note:            This is a callback from Telnet Server to
 *                  user application.  User application must
 *                  implement this function in his/her source file
 *                  return correct response based on internal
 *                  login and password information.
 ********************************************************************/
#ifdef __FTP_C
extern BOOL FTPVerify(unsigned char *login, unsigned char *password);
#endif

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
 * Overview:        Initializes internal variables of Telnet
 *
 * Note:
 ********************************************************************/
void FTPInit(void);

/*********************************************************************
 * Function:        void FTPServer(void)
 *
 * PreCondition:    FTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened Telnet connections are served.
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
BOOL FTPServer(void);

#endif
