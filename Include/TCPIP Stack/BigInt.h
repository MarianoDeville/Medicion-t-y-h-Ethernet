/*********************************************************************
 *
 *					Big Integer Class Headers
 *
 *********************************************************************
 * FileName:        BigInt.h
 * Dependencies:    none
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/

#ifndef __BIGINT_H
#define __BIGINT_H

#define BIGINT_DEBUG			0
#define BIGINT_DEBUG_COMPARE 	0
#define RSAEXP_DEBUG			0
#define BIGINT_DATA_SIZE		8ul		//bits
#define BIGINT_DATA_TYPE		BYTE
#define BIGINT_DATA_MAX			0xFFu
#define BIGINT_DATA_TYPE_2		WORD

typedef struct _BIGINT {
	BIGINT_DATA_TYPE *ptrLSB;		// Pointer to the least significant byte/word (lowest memory address)
	BIGINT_DATA_TYPE *ptrMSB;		// Pointer to the first non-zero most significant byte/word (higher memory address) if bMSBValid set
	BIGINT_DATA_TYPE *ptrMSBMax;	// Pointer to the maximum memory address that ptrMSB could ever be (highest memory address)
	BOOL bMSBValid;
} BIGINT;

typedef struct _BIGINT_ROM {
	const BIGINT_DATA_TYPE *ptrLSB;
	const BIGINT_DATA_TYPE *ptrMSB;
} BIGINT_ROM;

void BigInt(BIGINT * theInt, BIGINT_DATA_TYPE * data, WORD wWordLength);
void BigIntMod(BIGINT *, BIGINT *);
void BigIntMultiply(BIGINT *, BIGINT *, BIGINT *);

void BigIntAdd(BIGINT *, BIGINT *);
void BigIntSubtract(BIGINT *, BIGINT *);
void BigIntSubtractROM(BIGINT *, BIGINT_ROM *);
void BigIntCopy(BIGINT *, BIGINT *);
void BigIntSquare(BIGINT * a, BIGINT * res);
void BigIntZero(BIGINT * theInt);

int BigIntMagnitudeDifference(BIGINT * a, BIGINT * b);
int BigIntMagnitudeDifferenceROM(BIGINT * a, BIGINT_ROM * b);
signed char BigIntCompare(BIGINT *, BIGINT *);
unsigned short int BigIntMagnitude(BIGINT * n);

void BigIntSwapEndianness(BIGINT * a);

void BigIntPrint(const BIGINT * a);

void BigIntROM(BIGINT_ROM * theInt, const BIGINT_DATA_TYPE * data,WORD wWordLength);
void BigIntModROM(BIGINT *, BIGINT_ROM *);
void BigIntMultiplyROM(BIGINT *, BIGINT_ROM *, BIGINT *);
void BigIntAddROM(BIGINT *, BIGINT_ROM *);
void BigIntCopyROM(BIGINT *, BIGINT_ROM *);
signed char BigIntCompareROM(BIGINT *, BIGINT_ROM *);
unsigned short int BigIntMagnitudeROM(BIGINT_ROM * n);

extern const BIGINT_DATA_TYPE *_iBr, *_xBr;

void BigIntPrintROM(BIGINT_ROM *);

#endif
