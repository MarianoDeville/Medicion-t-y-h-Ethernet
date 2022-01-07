/*********************************************************************
 *
 *                  Helper Function Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        Helpers.h
 * Dependencies:    stacktsk.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __HELPERS_H
#define __HELPERS_H


#if !defined(__18CXX) || defined(HI_TECH_C)
char *strupr(char *s);
void ultoa(DWORD Value, unsigned char *Buffer);
#endif

#if defined(DEBUG)
#define DebugPrint(a)	{putrsUART(a);}
#else
#define DebugPrint(a)
#endif

DWORD GenerateRandomDWORD(void);
void uitoa(WORD Value, unsigned char *Buffer);
void UnencodeURL(unsigned char *URL);
WORD Base64Decode(unsigned char *cSourceData, WORD wSourceLen,
				  unsigned char *cDestData, WORD wDestLen);
WORD Base64Encode(unsigned char *cSourceData, WORD wSourceLen,
				  unsigned char *cDestData, WORD wDestLen);
BOOL StringToIPAddress(unsigned char *str, IP_ADDR * IPAddress);
unsigned char ReadStringUART(unsigned char *Dest, unsigned char BufferLen);
unsigned char hexatob(WORD_VAL AsciiChars);
unsigned char btohexa_high(unsigned char b);
unsigned char btohexa_low(unsigned char b);
signed char stricmppgm2ram(unsigned char *a, const unsigned char *b);

// const function variants for PIC18
#if defined(__18CXX)
BOOL ROMStringToIPAddress(const unsigned char *str, IP_ADDR * IPAddress);
#else
#define ROMStringToIPAddress(a,b)	StringToIPAddress((unsigned char*)a,b)
#endif


WORD swaps(WORD v);
DWORD swapl(DWORD v);

WORD CalcIPChecksum(unsigned char *buffer, WORD len);
WORD CalcIPBufferChecksum(WORD len);

#if defined(__18CXX)
DWORD leftRotateDWORD(DWORD val, unsigned char bits);
#else
#define leftRotateDWORD(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#endif


#endif
