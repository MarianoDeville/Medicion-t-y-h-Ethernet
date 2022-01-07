/*********************************************************************
 *
 *	Microchip File System (MPFS) File Access API
 *  Module for Microchip TCP/IP Stack
 *	 -Provides single API for accessing web pages and other files 
 *    from internal program memory or an external serial EEPROM memory
 *	 -Reference: AN833
 *
 *********************************************************************
 * FileName:        MPFS.c
 * Dependencies:    SPIEEPROM
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#define __MPFS2_C

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_MPFS2)

//Supports long file names to 64 characters
#define MAX_FILE_NAME_LEN   (64u)

/*
 * MPFS Structure:
 *     [M][P][F][S]
 *     [unsigned char Ver Hi][unsigned char Ver Lo][DWORD Number of Files]
 *     [File Record 1][File Record 2]...[File Record N]
 *     [String 1][String 2]...[String N]
 *     [File Data 1][File Data 2]...[File Data N]
 *
 * File Record Structure (24 bytes):
 *     [WORD Name Hash][WORD Flags][DWORD String Ptr]
 *     [DWORD Data Ptr][DWORD Len]
 *     [DWORD Timestamp][DWORD Microtime]
 *
 * String Structure (1 to 64 bytes):
 *     ["path/to/file.ext"][0x00]
 *
 * File Data Structure (arbitrary length):
 *		[File Data]
 *
 * Note: Unlike previous versions, there are no delimiters or flags
 */

#if defined(STACK_USE_MPFS) && defined(STACK_USE_MPFS2)
#error Both MPFS and MPFS2 are included
#endif

// Track the MPFS File Handles
// MPFSStubs[0] is reserved for internal use (FAT access)
MPFS_STUB MPFSStubs[MAX_MPFS_HANDLES + 1];

// Allows the MPFS to be locked altogether
BOOL isMPFSLocked;

// Static Function Declarations
static void LoadFATRecord(MPFS_HANDLE hMPFS);

// Settings for EEPROM vs Flash
#if defined(MPFS_USE_EEPROM)

	// Start in EEPROM after the reserve block
#define MPFS_HEAD		MPFS_RESERVE_BLOCK

	// Tracks the last read address
MPFS_PTR lastRead;

#else

	// An address where MPFS data starts in program memory.
#if defined(__18CXX) || defined(__C32__)
extern const unsigned char MPFS_Start[];
#define MPFS_HEAD		((DWORD)(&MPFS_Start[0]))
#else
extern DWORD MPFS_Start;
#define MPFS_HEAD		MPFS_Start;
#endif

#endif

/*********************************************************************
 * Function:        BOOL MPFSInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Sets all MPFS handles to closed, and initializes
 *					EEPROM if necessary.
 *
 * Note:            This function is called only one during lifetime
 *                  of the application.
 ********************************************************************/
void MPFSInit(void)
{
	unsigned char i;
	for(i=1;i<=MAX_MPFS_HANDLES;i++)
	{
		MPFSStubs[i].addr=MPFS_INVALID;
	}
#if defined(MPFS_USE_EEPROM)
	XEEInit();							// Initialize the EEPROM access routines.
	lastRead = MPFS_INVALID;
#endif
	isMPFSLocked = FALSE;
}
/*********************************************************************
 * Function:        MPFS_HANDLE MPFSOpen(unsigned char* file)
 *
 * PreCondition:    None
 *
 * Input:           file: a NULL terminated file name.
 *
 * Output:          An MPFS_HANDLE to the opened file if found
 *                  MPFS_INVALID_HANDLE if file not found or no free handles
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
MPFS_HANDLE MPFSOpen(unsigned char *file)
{
	MPFS_HANDLE hMPFS;
	WORD nameHash, i;
	unsigned char *ptr, c;
	Nop();
	// Make sure MPFS is unlocked and we got a filename
	if (*file == '\0' || isMPFSLocked == TRUE)
		return MPFS_INVALID_HANDLE;
	// Calculate the name hash for faster searching
	for (nameHash = 0, ptr = file; *ptr != '\0'; ptr++)
		nameHash += *ptr;
	// Find a free file handle to use
	for (hMPFS = 1; hMPFS <= MAX_MPFS_HANDLES; hMPFS++)
		if (MPFSStubs[hMPFS].addr == MPFS_INVALID)
			break;
	if (hMPFS == MAX_MPFS_HANDLES)
		return MPFS_INVALID_HANDLE;
	MPFSStubs[0].addr=0;						// Initialize the FAT pointer
	MPFSStubs[0].bytesRem=8;					// Read in the number of records
	MPFSGetArray(0,NULL,6);
	MPFSGetArray(0,(unsigned char *)&i,2);
	MPFSStubs[0].bytesRem=i*(24);
	for (i = 0;									// Read in FAT records and compare the hash
		 MPFSGetArray(0, (unsigned char *) &(MPFSStubs[hMPFS].fatID),
					  2) == 2; i++) {
		if (MPFSStubs[hMPFS].fatID == nameHash)	// If the hash matches, compare the full filename
		{
			MPFSGetArray(0, NULL, 2);
			MPFSGetArray(0, (unsigned char *) &(MPFSStubs[hMPFS].addr), 4);
			MPFSStubs[hMPFS].bytesRem = 255;
			for (ptr = file; *ptr != '\0'; ptr++)	// Loop over the filename
			{
				MPFSGet(hMPFS, &c);
				if (*ptr != c)
					break;
			}
			MPFSGet(hMPFS, &c);
			if (c == '\0' && *ptr == '\0')		// Filename matches, so return true
			{
				MPFSGetArray(0, (unsigned char *) &(MPFSStubs[hMPFS].addr),4);
				MPFSGetArray(0,(unsigned char *) &(MPFSStubs[hMPFS].bytesRem), 4);
				MPFSStubs[hMPFS].fatID = i;
				return hMPFS;
			}
			else
			{			// No match, so skip to next
				MPFSGetArray(0, NULL, 16);
			}
		}
		else
		{				// No match, so skip to next
			MPFSGetArray(0, NULL, 22);
		}
	}
	MPFSStubs[hMPFS].addr = MPFS_INVALID;		// No file name matched, so return nothing
	return MPFS_INVALID_HANDLE;
}
MPFS_HANDLE MPFSOpenROM(const unsigned char *file)
{
	MPFS_HANDLE hMPFS;
	WORD nameHash, i;
	const unsigned char *ptr;
	unsigned char c;
	if(*file=='\0'||isMPFSLocked==TRUE)			// Make sure MPFS is unlocked and we got a filename
		return MPFS_INVALID_HANDLE;
	for(nameHash=0,ptr=file;*ptr!='\0';ptr++)	// Calculate the name hash for faster searching
		nameHash += *ptr;
	for(hMPFS=1;hMPFS<=MAX_MPFS_HANDLES;hMPFS++)	// Find a free file handle to use
		if (MPFSStubs[hMPFS].addr==MPFS_INVALID)
			break;
	if (hMPFS == MAX_MPFS_HANDLES)
		return MPFS_INVALID_HANDLE;
	MPFSStubs[0].addr=0;						// Initialize the FAT pointer
	MPFSStubs[0].bytesRem=8;					// Read in the number of records
	MPFSGetArray(0,NULL,6);
	MPFSGetArray(0,(unsigned char *) &i, 2);
	MPFSStubs[0].bytesRem=i*(24);
	for(i=0;MPFSGetArray(0,(unsigned char *)&(MPFSStubs[hMPFS].fatID),2)==2;i++)	// Read in FAT records and compare the hash
	{
		if(MPFSStubs[hMPFS].fatID==nameHash)	// If the hash matches, compare the full filename
		{
			MPFSGetArray(0, NULL, 2);
			MPFSGetArray(0, (unsigned char *) &(MPFSStubs[hMPFS].addr), 4);
			MPFSStubs[hMPFS].bytesRem = 255;
			for (ptr = file; *ptr != '\0'; ptr++)	// Loop over the filename
			{
				MPFSGet(hMPFS, &c);
				if (*ptr != c)
					break;
			}
			MPFSGet(hMPFS, &c);
			if (c == '\0' && *ptr == '\0')		// Filename matches, so return true
			{
				MPFSGetArray(0,(unsigned char *)&(MPFSStubs[hMPFS].addr),4);
				MPFSGetArray(0,(unsigned char *)&(MPFSStubs[hMPFS].bytesRem),4);
				MPFSStubs[hMPFS].fatID = i;
				return hMPFS;
			}
			else								// No match, so skip to next
			{
				MPFSGetArray(0, NULL, 16);
			}
		}
		else									// No match, so skip to next
		{
			MPFSGetArray(0, NULL, 22);
		}
	}
	MPFSStubs[hMPFS].addr = MPFS_INVALID;		// No file name matched, so return nothing
	return MPFS_INVALID_HANDLE;
}
/*********************************************************************
 * Function:        MPFS_HANDLE MPFSOpenID(WORD fatID)
 *
 * PreCondition:    None
 *
 * Input:           fatID: the FAT ID of a previously opened file
 *
 * Output:          An MPFS_HANDLE to the reopened file
 *                  MPFS_INVALID_HANDLE if file not found or no free handles
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            Use this function along with MPFSGetID() to 
 *					quickly re-open a file without tying up a 
 *					permanent MPFSStub
 ********************************************************************/
MPFS_HANDLE MPFSOpenID(WORD fatID)
{
	MPFS_HANDLE hMPFS;
	WORD i;
	if(isMPFSLocked==TRUE)							// Make sure MPFS is unlocked and we got a filename
		return MPFS_INVALID_HANDLE;
	for(hMPFS=1;hMPFS<=MAX_MPFS_HANDLES;hMPFS++)	// Find a free file handle to use
		if(MPFSStubs[hMPFS].addr==MPFS_INVALID)
			break;
	if(hMPFS==MAX_MPFS_HANDLES)
		return MPFS_INVALID_HANDLE;
	MPFSStubs[0].addr=0;							// Initialize the FAT pointer
	MPFSStubs[0].bytesRem=8;						// Read in the number of records
	MPFSGetArray(0,NULL,6);
	MPFSGetArray(0,(unsigned char *)&i,2);
	if(fatID>=i)									// Make sure ID isn't past the last record
		return MPFS_INVALID_HANDLE;
	MPFSStubs[hMPFS].fatID=fatID;					// Set up the file handle
	MPFSStubs[0].addr=8+24*fatID+8;
	MPFSStubs[0].bytesRem=8;
	MPFSGetArray(0,(unsigned char *)&MPFSStubs[hMPFS].addr,4);
	MPFSGetArray(0,(unsigned char *)&MPFSStubs[hMPFS].bytesRem,4);
	return hMPFS;
}
/*********************************************************************
 * Function:        void MPFSClose(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    None
 *
 * Input:           hMPFS file handle to be closed
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void MPFSClose(MPFS_HANDLE hMPFS)
{
	if (hMPFS != 0 && hMPFS <= MAX_MPFS_HANDLES)
		MPFSStubs[hMPFS].addr = MPFS_INVALID;
}
/*********************************************************************
 * Function:        BOOL MPFSGet(MPFS_HANDLE hMPFS, unsigned char* c)
 *
 * PreCondition:    hMPFS is a valid open handle
 *
 * Input:           hMPFS: the handle to read from
 *					c: the byte to read
 *
 * Output:          TRUE if a byte was read
 *					FALSE if EOF or hMPFS is invalid
 *
 * Side Effects:    None
 *
 * Overview:        Attempts to read a byte from current address
 *
 * Note:            None
 ********************************************************************/
BOOL MPFSGet(MPFS_HANDLE hMPFS, unsigned char *c)
{
	if (hMPFS > MAX_MPFS_HANDLES)				// Make sure we're reading a valid address
		return FALSE;
	if (MPFSStubs[hMPFS].addr==MPFS_INVALID||MPFSStubs[hMPFS].bytesRem==0)
		return FALSE;
	if (c == NULL)
	{
		MPFSStubs[hMPFS].addr++;
		MPFSStubs[hMPFS].bytesRem--;
		return TRUE;
	}
#if defined(MPFS_USE_EEPROM)					// Read function for EEPROM
	if (MPFSStubs[hMPFS].addr != lastRead + 1)	// For performance, cache the last read address
		XEEBeginRead(MPFSStubs[hMPFS].addr + MPFS_HEAD);
	*c = XEERead();
	lastRead = MPFSStubs[hMPFS].addr;
	MPFSStubs[hMPFS].addr++;
#else
	{
		DWORD dwHITECHWorkaround=MPFS_HEAD;
		*c=*((const unsigned char *) (MPFSStubs[hMPFS].addr +dwHITECHWorkaround));
		MPFSStubs[hMPFS].addr++;
	}
#endif
	MPFSStubs[hMPFS].bytesRem--;
	return TRUE;
}
/*********************************************************************
 * Function:        WORD MPFSGetArray(MPFS_HANDLE hMPFS, unsigned char* data, WORD len)
 *
 * PreCondition:    hMPFS is a valid open handle
 *
 * Input:           hMPFS: the handle to read from
 *					data: the array to read
 *					len: the number of bytes to read
 *
 * Output:          # of bytes read. if less than len, an EOF occurred
 *
 * Side Effects:    None
 *
 * Overview:        Reads an array of bytes from the current address
 *
 * Note:            None
 ********************************************************************/
WORD MPFSGetArray(MPFS_HANDLE hMPFS, unsigned char *data, WORD len)
{
	if(hMPFS > MAX_MPFS_HANDLES)			// Make sure we're reading a valid address
		return 0;
	if(len>MPFSStubs[hMPFS].bytesRem)		// Determine how many we can actually read
		len=MPFSStubs[hMPFS].bytesRem;
	if(MPFSStubs[hMPFS].addr==MPFS_INVALID||len==0)	// Make sure we're reading a valid address
		return 0;
	if(data == NULL)
	{
		MPFSStubs[hMPFS].addr += len;
		MPFSStubs[hMPFS].bytesRem -= len;
		return len;
	}
#if defined(MPFS_USE_EEPROM)	// Read the data
	XEEReadArray(MPFSStubs[hMPFS].addr + MPFS_HEAD, data, len);
	MPFSStubs[hMPFS].addr += len;
	MPFSStubs[hMPFS].bytesRem -= len;
	lastRead = MPFS_INVALID;
#else
	{
		DWORD dwHITECHWorkaround = MPFS_HEAD;
		memcpypgm2ram(data,(const void *) (MPFSStubs[hMPFS].addr +dwHITECHWorkaround), len);
		MPFSStubs[hMPFS].addr += len;
		MPFSStubs[hMPFS].bytesRem -= len;
	}
#endif
	return len;
}
/*********************************************************************
 * Function:        MPFS MPFSFormat(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          A valid MPFS handle that can be used for MPFSPut
 *
 * Side Effects:    None
 *
 * Overview:        Prepares MPFS image to get re-written
 *                  Declares MPFS as in use.
 *
 * Note:            MPFS will be unaccessible until MPFSClose is
 *                  called.
 ********************************************************************/
MPFS_HANDLE MPFSFormat(void)
{
#if defined(MPFS_USE_EEPROM)
	unsigned char i;
	for (i=0;i<MAX_MPFS_HANDLES;i++)				// Close all files
		MPFSStubs[i].addr = MPFS_INVALID;
	isMPFSLocked=TRUE;								// Lock the image
	MPFSStubs[0].addr = 0;							// Set FAT ptr for writing
	MPFSStubs[0].fatID = 0xffff;
	MPFSStubs[0].bytesRem=MPFS_WRITE_PAGE_SIZE-(((unsigned char)MPFSStubs[0].addr+MPFS_HEAD)&(MPFS_WRITE_PAGE_SIZE-1));
	if(XEEBeginWrite(MPFSStubs[0].addr + MPFS_HEAD) == XEE_SUCCESS)	// Set up EEPROM for writing
		return 0x00;
#endif
	return MPFS_INVALID_HANDLE;
}
/*********************************************************************
 * Function:        WORD MPFSPutArray(MPFS_HANDLE hMPFS, unsigned char *data, WORD len)
 *
 * PreCondition:    MPFSFormat() must have been called
 *
 * Input:           hMPFS: the MPFS handle for writing
 *					data: the data array to write
 *					len: how many bytes to write
 *
 * Output:          number of bytes successfully written
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            Actual write may not get started until internal
 *                  write page is full.  To ensure that previously
 *                  data gets written, caller must call MPFSPutEnd()
 *                  after last call to MPFSPutArray().
 ********************************************************************/
WORD MPFSPutArray(MPFS_HANDLE hMPFS, unsigned char *data, WORD len)
{
#if defined(MPFS_USE_EEPROM)
	WORD count;
	for(count=0;count<len;count++)
		{
		XEEWrite(data[count]);
		MPFSStubs[hMPFS].addr++;
		MPFSStubs[hMPFS].bytesRem--;
		if (MPFSStubs[hMPFS].bytesRem == 0)
		{
			MPFSPutEnd();
			isMPFSLocked=TRUE;
			XEEBeginWrite(MPFSStubs[hMPFS].addr + MPFS_HEAD);
			MPFSStubs[hMPFS].bytesRem = MPFS_WRITE_PAGE_SIZE;
		}
	}
	return count;
#else
	return 0;
#endif
}
/*********************************************************************
 * Function:        MPFS MPFSPutEnd(void)
 *
 * PreCondition:    XEEBeginWrite() is already called.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            Actual write may not get started until internal
 *                  write page is full.  To ensure that previously
 *                  data gets written, caller must call MPFSPutEnd()
 *                  after last call to MPFSPut().
 ********************************************************************/
void MPFSPutEnd(void)
{
#if defined(MPFS_USE_EEPROM)
	isMPFSLocked = FALSE;
	XEEEndWrite();
	while (XEEIsBusy());
#endif
}
/*********************************************************************
 * Function:        BOOL MPFSSeek(MPFS_HANDLE hMPFS, DWORD offset,
 *									MPFS_SEEK_MODE mode)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file handle to use
 *					offset: how far to seek
 *					mode: one of the MPFS_SEEK_MODE constants
 *
 * Output:          TRUE if the seek succeeded
 *					FALSE if the handle or new location was invalid
 *
 * Side Effects:    None.
 *
 * Overview:        Moves the current read pointer to a new location.
 *					Set mode to MPFS_SEEK_START to seek offset bytes
 *					from the beginning.  _FORWARD and _REWIND seek 
 *					from the current position.  _END seeks offset 
 *					bytes before the end of the file.
 *
 * Note:            None.
 ********************************************************************/
BOOL MPFSSeek(MPFS_HANDLE hMPFS, DWORD offset, MPFS_SEEK_MODE mode)
{
	DWORD temp;
	if (hMPFS>MAX_MPFS_HANDLES)			// Make sure a valid file is open
		return FALSE;
	if (MPFSStubs[hMPFS].addr==MPFS_INVALID)
		return FALSE;
	switch (mode)
	{
	case MPFS_SEEK_START:				// Seek offset bytes from start
		temp=MPFSGetSize(hMPFS);
		if(offset>temp)
			return FALSE;
		MPFSStubs[hMPFS].addr=MPFSGetStartAddr(hMPFS)+offset;
		MPFSStubs[hMPFS].bytesRem=temp-offset;
		return TRUE;
	case MPFS_SEEK_FORWARD:				// Seek forwards offset bytes
		if(offset>MPFSStubs[hMPFS].bytesRem)
			return FALSE;
		MPFSStubs[hMPFS].addr+=offset;
		MPFSStubs[hMPFS].bytesRem-=offset;
		return TRUE;
	case MPFS_SEEK_REWIND:				// Seek backwards offset bytes
		temp=MPFSGetStartAddr(hMPFS);
		if(MPFSStubs[hMPFS].addr-offset<temp)
			return FALSE;
		MPFSStubs[hMPFS].addr-=offset;
		MPFSStubs[hMPFS].bytesRem+=offset;
		return TRUE;
	case MPFS_SEEK_END:					// Seek so that offset bytes remain in file
		temp=MPFSGetSize(hMPFS);
		if (offset>temp)
			return FALSE;
		MPFSStubs[hMPFS].addr=MPFSGetEndAddr(hMPFS)-offset;
		MPFSStubs[hMPFS].bytesRem=offset;
		return TRUE;
	default:
		return FALSE;
	}
}
/*********************************************************************
 * Function:        BOOL MPFSGetLong(MPFS_HANDLE hMPFS, DWORD *ul)
 *
 * PreCondition:    MPFSOpen() and MPFSBeginGet()
 *
 * Input:           hMPFS: the handle from which to read
 *					ul: pointer to an DWORD to read
 *
 * Output:          TRUE on success
 *					FALSE on EOF
 * 
 * Side Effects:    None
 *
 * Overview:        Reads an DWORD value from an MPFS file
 *
 * Note:            None
 ********************************************************************/
BOOL MPFSGetLong(MPFS_HANDLE hMPFS, DWORD * ul)
{
	return (MPFSGetArray(hMPFS,(unsigned char *)ul,4)==4);
}
/*********************************************************************
 * Function:        void LoadFATRecord(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file whose FAT record to locate
 *
 * Output:          MPFSStubs[0] is ready to read the record
 * 
 * Side Effects:    None
 *
 * Overview:        Reads a Timestamp from the MPFS Fat
 *
 * Note:            None
 ********************************************************************/
static void LoadFATRecord(MPFS_HANDLE hMPFS)
{
	// Locate the FAT pointer to front of record
	MPFSStubs[0].bytesRem=24;
	MPFSStubs[0].addr=8+MPFSStubs[hMPFS].fatID*24;
}
/*********************************************************************
 * Function:        DWORD MPFSGetTimestamp(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to read the timestamp from
 *
 * Output:          the timestamp
 * 
 * Side Effects:    None
 *
 * Overview:        Reads a Timestamp from the MPFS Fat
 *
 * Note:            None
 ********************************************************************/
DWORD MPFSGetTimestamp(MPFS_HANDLE hMPFS)
{
	DWORD val;
	if(hMPFS>MAX_MPFS_HANDLES)					// Make sure a valid file is open
		return 0x00000000;
	if(MPFSStubs[hMPFS].addr==MPFS_INVALID)
		return 0x00000000;
	LoadFATRecord(hMPFS);						// Move to the point for reading
	MPFSStubs[0].addr+=16;
	MPFSGetArray(0,(unsigned char *)&val,4);	// Read the value and return
	return val;
}
/*********************************************************************
 * Function:        DWORD MPFSGetMicrotime(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to read the microtime from
 *
 * Output:          the microtime
 * 
 * Side Effects:    None
 *
 * Overview:        Reads a Microtime from the MPFS Fat
 *
 * Note:            None
 ********************************************************************/
DWORD MPFSGetMicrotime(MPFS_HANDLE hMPFS)
{
	DWORD val;
	if(hMPFS>MAX_MPFS_HANDLES)					// Make sure a valid file is open
		return 0x00000000;
	if(MPFSStubs[hMPFS].addr==MPFS_INVALID)
		return 0x00000000;
	LoadFATRecord(hMPFS);						// Move to the point for reading
	MPFSStubs[0].addr += 20;
	MPFSGetArray(0,(unsigned char *)&val,4);	// Read the value and return
	return val;
}
/*********************************************************************
 * Function:        WORD MPFSGetFlags(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to read the microtime from
 *
 * Output:          the microtime
 * 
 * Side Effects:    None
 *
 * Overview:        Reads a Microtime from the MPFS Fat
 *
 * Note:            None
 ********************************************************************/
WORD MPFSGetFlags(MPFS_HANDLE hMPFS)
{
	WORD val;
	if(hMPFS>MAX_MPFS_HANDLES)					// Make sure a valid file is open
		return 0x0000;
	if(MPFSStubs[hMPFS].addr==MPFS_INVALID)
		return 0x0000;
	LoadFATRecord(hMPFS);						//move to the point for reading
	MPFSStubs[0].addr+=2;
	MPFSGetArray(0,(unsigned char *)&val,2);	//read the value and return
	return val;
}
/*********************************************************************
 * Function:        DWORD MPFSGetSize(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to find the size of
 *
 * Output:          the file size
 * 
 * Side Effects:    None
 *
 * Overview:        Reads a Length parameter from the MPFS Fat
 *
 * Note:            None
 ********************************************************************/
DWORD MPFSGetSize(MPFS_HANDLE hMPFS)
{
	DWORD val;
	if(hMPFS>MAX_MPFS_HANDLES)					// Make sure a valid file is open
		return 0x00000000;
	if(MPFSStubs[hMPFS].addr==MPFS_INVALID)
		return 0x00000000;
	LoadFATRecord(hMPFS);						// Move to the point for reading
	MPFSStubs[0].addr+=12;
	MPFSGetArray(0,(unsigned char *)&val,4);	// Read the value and return
	return val;
}
/*********************************************************************
 * Function:        DWORD MPFSGetBytesRem(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file handle
 *
 * Output:          number of bytes remaining to be read
 * 
 * Side Effects:    None
 *
 * Overview:        Determines how many bytes remain in the file
 *
 * Note:            None
 ********************************************************************/
DWORD MPFSGetBytesRem(MPFS_HANDLE hMPFS)
{
	if (hMPFS > MAX_MPFS_HANDLES)	// Make sure a valid file is open
		return 0x00000000;
	if (MPFSStubs[hMPFS].addr == MPFS_INVALID)
		return 0x00000000;
	return MPFSStubs[hMPFS].bytesRem;
}
/*********************************************************************
 * Function:        MPFS_PTR MPFSGetStartAddr(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to locate the start address of
 *
 * Output:          the start address
 * 
 * Side Effects:    None
 *
 * Overview:        Reads a starting file address from the MPFS Fat
 *
 * Note:            None
 ********************************************************************/
MPFS_PTR MPFSGetStartAddr(MPFS_HANDLE hMPFS)
{
	MPFS_PTR val;
	// Make sure a valid file is open
	if (hMPFS > MAX_MPFS_HANDLES)
		return 0;
	if (MPFSStubs[hMPFS].addr == MPFS_INVALID)
		return MPFS_INVALID;
	// Move to the point for reading
	LoadFATRecord(hMPFS);
	MPFSStubs[0].addr += 8;
	// Read the value and return
	MPFSGetArray(0, (unsigned char *) &val, 4);
	return val;
}
/*********************************************************************
 * Function:        MPFS_PTR MPFSGetEndAddr(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to locate the start address of
 *
 * Output:          the start address
 * 
 * Side Effects:    None
 *
 * Overview:        Reads an ending file address from the MPFS Fat
 *
 * Note:            None
 ********************************************************************/
MPFS_PTR MPFSGetEndAddr(MPFS_HANDLE hMPFS)
{
	MPFS_PTR start, len;

	// Make sure a valid file is open
	if (hMPFS > MAX_MPFS_HANDLES)
		return MPFS_INVALID;
	if (MPFSStubs[hMPFS].addr == MPFS_INVALID)
		return MPFS_INVALID;

	// Move to the point for reading
	LoadFATRecord(hMPFS);
	MPFSStubs[0].addr += 8;

	// Read the value and return
	MPFSGetArray(0, (unsigned char *) &start, 4);
	MPFSGetArray(0, (unsigned char *) &len, 4);
	return start + len;
}

/*********************************************************************
 * Function:        void MPFSGetFilename(MPFS_HANDLE hMPFS, unsigned char *name, WORD len)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to locate the start address of
 *					name: buffer to place the filename in
 *					len: size of buffer
 *
 * Output:          name: the filename of the current file
 * 
 * Side Effects:    None
 *
 * Overview:        Finds the filename of the currently opened file
 *
 * Note:            None
 ********************************************************************/
BOOL MPFSGetFilename(MPFS_HANDLE hMPFS, unsigned char *name, WORD len)
{
	DWORD addr;

	// Make sure a valid file is open
	if (hMPFS > MAX_MPFS_HANDLES)
		return FALSE;
	if (MPFSStubs[hMPFS].addr == MPFS_INVALID)
		return FALSE;

	// Move to the point for reading
	LoadFATRecord(hMPFS);
	MPFSStubs[0].addr += 4;
	MPFSGetArray(0, (unsigned char *) &(addr), 4);
	MPFSStubs[0].addr = addr;

	MPFSStubs[0].bytesRem = 255;

	// Read the value and return
	MPFSGet(0, name);
	len--;
	for (; len > 0 && *name != '\0'; len--) {
		name++;
		MPFSGet(0, name);
	}

	return TRUE;
}

/*********************************************************************
 * Function:        DWORD MPFSGetPosition(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to locate the position in
 *
 * Output:          the current position of the file pointer
 * 
 * Side Effects:    None
 *
 * Overview:        Determines the position of the file pointer
 *
 * Note:            Calling MPFSSeek(hMPFS, pos, MPFS_SEEK_START)
 *					will return the pointer to this position at a
 *					later time. (Where pos is the value returned by
 *					this function.)
 ********************************************************************/
DWORD MPFSGetPosition(MPFS_HANDLE hMPFS)
{
	return MPFSStubs[hMPFS].addr - MPFSGetStartAddr(hMPFS);
}

/*********************************************************************
 * Function:        DWORD MPFSGetID(MPFS_HANDLE hMPFS)
 *
 * PreCondition:    hMPFS is a valid open file handle
 *
 * Input:           hMPFS: the file to find an ID for
 *
 * Output:          the FAT ID of the current file
 * 
 * Side Effects:    None
 *
 * Overview:        Determines the FAT ID of the current file.
 *
 * Note:            The return value of this function can be passed
 *					to MPFSOpenID() to quickly reopen this file.
 ********************************************************************/
WORD MPFSGetID(MPFS_HANDLE hMPFS)
{
	return MPFSStubs[hMPFS].fatID;
}


#endif							//#if defined(STACK_USE_MPFS)
