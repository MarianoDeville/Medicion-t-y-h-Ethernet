/*********************************************************************
 *
 *					ARCFOUR Cryptography Headers
 *
 *********************************************************************
 * FileName:        ARCFOUR.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/

#ifndef __ARCFOUR_H
#define __ARCFOUR_H

typedef struct _ARCFOUR_CTX {
	unsigned char i;
	unsigned char j;
	unsigned char *Sbox;
} ARCFOUR_CTX;

void ARCFOURInitialize(unsigned char *key, unsigned char len,
					   ARCFOUR_CTX * ctx);
void ARCFOURCrypt(unsigned char *data, unsigned char len,
				  ARCFOUR_CTX * ctx);

#endif
