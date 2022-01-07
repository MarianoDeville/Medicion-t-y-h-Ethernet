/*********************************************************************
 *
 *					Hash Function Library Headers
 *
 *********************************************************************
 * FileName:        Hashes.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/

#ifndef __HASHES_H
#define __HASHES_H

typedef enum _HASH_TYPE {
	HASH_MD5 = 0u,
	HASH_SHA1
} HASH_TYPE;

typedef struct _HASH_SUM {
	DWORD h0;
	DWORD h1;
	DWORD h2;
	DWORD h3;
	DWORD h4;
	DWORD bytesSoFar;
	unsigned char partialBlock[64];
	HASH_TYPE hashType;
} HASH_SUM;

#if defined(STACK_USE_SHA1)
void SHA1Initialize(HASH_SUM * theSum);
void SHA1AddData(HASH_SUM * theSum, unsigned char *data, WORD len);
void SHA1Calculate(HASH_SUM * theSum, unsigned char *result);
	// const function variants for PIC18
#if defined(__18CXX)
void SHA1AddROMData(HASH_SUM * theSum, const unsigned char *data,
					WORD len);
#else
#define SHA1AddROMData(a,b,c)	SHA1AddData(a,(unsigned char*)b,c)
#endif
#endif

#if defined(STACK_USE_MD5)
void MD5Initialize(HASH_SUM * theSum);
void MD5AddData(HASH_SUM * theSum, unsigned char *data, WORD len);
void MD5Calculate(HASH_SUM * theSum, unsigned char *result);
	// const function variants for PIC18
#if defined(__18CXX)
void MD5AddROMData(HASH_SUM * theSum, const unsigned char *data, WORD len);
#else
#define MD5AddROMData(a,b,c)	MD5AddData(a,(unsigned char*)b,c)
#endif
#endif

void HashAddData(HASH_SUM * theSum, unsigned char *data, WORD len);
// const function variants for PIC18
#if defined(__18CXX)
void HashAddROMData(HASH_SUM * theSum, const unsigned char *data,
					WORD len);
#else
#define HashAddROMData(a,b,c)	HashAddData(a,(unsigned char*)b,c)
#endif

#endif
