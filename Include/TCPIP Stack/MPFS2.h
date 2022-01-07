/*********************************************************************
 *
 *               Microchip File System
 *
 *********************************************************************
 * FileName:        MPFS.h
 * Dependencies:    StackTsk.H
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
********************************************************************/
#ifndef __MPFS2_H
#define __MPFS2_H

typedef DWORD MPFS_PTR;
#define MPFS_INVALID			(0xffffffffu)

#if defined(MPFS_USE_EEPROM)
#if defined(USE_EEPROM_25LC1024)
#define MPFS_WRITE_PAGE_SIZE		(256u)
#else
#define MPFS_WRITE_PAGE_SIZE		(64u)
#endif
#endif

#define MPFS_INVALID_HANDLE 		(0xffu)
typedef unsigned char MPFS_HANDLE;

// MPFS Flags
#define MPFS2_FLAG_ISZIPPED		((WORD)0x0001)
#define MPFS2_FLAG_HASINDEX		((WORD)0x0002)

// Stores each file handle's information
// Handles are free when addr = MPFS_INVALID
typedef struct _MPFS_STUB {
	MPFS_PTR addr;
	DWORD bytesRem;
	WORD fatID;
} MPFS_STUB;

// Indicates the method for MPFSSeek
typedef enum _MPFS_SEEK_MODE {
	MPFS_SEEK_START = 0u,
	MPFS_SEEK_END,
	MPFS_SEEK_FORWARD,
	MPFS_SEEK_REWIND
} MPFS_SEEK_MODE;

	//C30 routine to read program memory
#if defined(__C30__)
extern DWORD ReadProgramMemory(DWORD address);
#endif

void MPFSInit(void);

// MPFS Handle Management Functions
MPFS_HANDLE MPFSOpen(unsigned char *name);
#if defined(__18CXX)
MPFS_HANDLE MPFSOpenROM(const unsigned char *name);
#else
#define MPFSOpenROM(a)	MPFSOpen((unsigned char*) a);
#endif
void MPFSClose(MPFS_HANDLE hMPFS);
MPFS_HANDLE MPFSOpenID(WORD fatID);

// MPFS Metadata Accessors
DWORD MPFSGetTimestamp(MPFS_HANDLE hMPFS);
DWORD MPFSGetMicrotime(MPFS_HANDLE hMPFS);
WORD MPFSGetFlags(MPFS_HANDLE hMPFS);
DWORD MPFSGetSize(MPFS_HANDLE hMPFS);
DWORD MPFSGetBytesRem(MPFS_HANDLE hMPFS);
MPFS_PTR MPFSGetStartAddr(MPFS_HANDLE hMPFS);
MPFS_PTR MPFSGetEndAddr(MPFS_HANDLE hMPFS);
BOOL MPFSGetFilename(MPFS_HANDLE hMPFS, unsigned char *name, WORD len);
DWORD MPFSGetPosition(MPFS_HANDLE hMPFS);
#define MPFSTell(a)	MPFSGetPosition(a)
WORD MPFSGetID(MPFS_HANDLE hMPFS);

// MPFS Data Accessors
BOOL MPFSGet(MPFS_HANDLE hMPFS, unsigned char *c);
WORD MPFSGetArray(MPFS_HANDLE hMPFS, unsigned char *data, WORD len);
BOOL MPFSGetLong(MPFS_HANDLE hMPFS, DWORD * ul);
BOOL MPFSSeek(MPFS_HANDLE hMPFS, DWORD offset, MPFS_SEEK_MODE mode);

// MPFS Writing Functions
MPFS_HANDLE MPFSFormat(void);
void MPFSPutEnd(void);
WORD MPFSPutArray(MPFS_HANDLE hMPFS, unsigned char *data, WORD len);


#endif
