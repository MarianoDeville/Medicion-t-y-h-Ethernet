/*********************************************************************
 *
 * Data SPI RAM Access Routines
 *  -Tested with AMI Semiconductor N256S0830HDA
 *
 *********************************************************************
 * FileName:        SPIRAM.h
 * Dependencies:    Compiler.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __SPIRAM_H
#define __SPIRAM_H

void SPIRAMInit(void);
void SPIRAMGetArray(WORD wAddress, unsigned char *vData, WORD wLength);
void SPIRAMPutArray(WORD wAddress, unsigned char *vData, WORD wLength);

#define SPIRAMPutString(a,b)			SPIRAMPutArray(a, strlen((char*)b))

void SPIRAMPutROMArray(WORD wAddress, const unsigned char *vData,
					   WORD wLength);
#define SPIRAMPutROMString(a,b)		SPIRAMPutROMArray(a, strlenpgm((const char*)b))
#else
#define SPIRAMPutROMString(a,b)		SPIRAMPutArray(a, strlen((char*)b))
#define SPIRAMPutROMArray(a,b,c)	SPIRAMPutROMArray(a, b, c)

#endif
