/*********************************************************************
 *
 *               External serial data EEPROM Access Defs.
 *
 *********************************************************************
 * FileName:        XEEPROM.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __XEEPROM_H
#define __XEEPROM_H

typedef BOOL XEE_RESULT;
#define XEE_SUCCESS FALSE

void XEEInit(void);
XEE_RESULT XEEBeginWrite(DWORD address);
XEE_RESULT XEEWrite(unsigned char val);
XEE_RESULT XEEEndWrite(void);
XEE_RESULT XEEBeginRead(DWORD address);
unsigned char XEERead(void);
XEE_RESULT XEEReadArray(DWORD address, unsigned char *buffer,unsigned char length);
XEE_RESULT XEEEndRead(void);
BOOL XEEIsBusy(void);


#endif
