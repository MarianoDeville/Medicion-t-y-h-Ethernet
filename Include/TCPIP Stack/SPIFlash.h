/*********************************************************************
 *
 * Data SPI Flash Access Routines
 *  -Will test with SST SST25VF016B
 *
 *********************************************************************
 * FileName:        SPIFlash.h
 * Dependencies:    Compiler.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
********************************************************************/
#ifndef __SPIFLASH_H
#define __SPIFLASH_H

void SPIFlashInit(void);
void SPIFlashGetArray(DWORD dwAddress, unsigned char *vData, WORD wLength);
void SPIFlashPut(DWORD dwAddress, unsigned char vData);
void SPIFlashPutArray(DWORD dwAddress, unsigned char *vData, WORD wLength);
void SPIFlashErase(DWORD dwAddress, DWORD dwLength);

#endif
