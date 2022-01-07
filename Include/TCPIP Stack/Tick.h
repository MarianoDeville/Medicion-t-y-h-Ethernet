/*********************************************************************
 *
 *                  Tick Manager for PIC18
 *
 *********************************************************************
 * FileName:        Tick.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __TICK_H
#define __TICK_H

#include "TCPIP Stack/TCPIP.h"

typedef DWORD TICK;

// This value is used by TCP to implement timeout actions.
// For this definition, the Timer must be initialized to use 
// a 1:256 prescalar in Tick.c.  If using a 32kHz watch crystal 
// as the time base, modify the Tick.c file to use no prescalar.
#if defined(__C32__)
#define TICKS_PER_SECOND		((PERIPHERAL_FREQ+128ull)/256ull)	// Internal core clock drives timer
#else
#define TICKS_PER_SECOND		((INSTR_FREQ+128ull)/256ull)	// Internal core clock drives timer
#endif
//#define TICKS_PER_SECOND      (32768ul)                                   // 32kHz crystal drives timer with no scalar


#define TICK_SECOND				((QWORD)TICKS_PER_SECOND)
#define TICK_MINUTE				((QWORD)TICKS_PER_SECOND)*60ull)
#define TICK_HOUR				((QWORD)TICKS_PER_SECOND*3600ull)

#define TickGetDiff(a, b)       ((a)-(b))

void TickInit(void);
DWORD TickGet(void);
DWORD TickGetDiv256(void);
DWORD TickGetDiv64K(void);
DWORD TickConvertToMilliseconds(DWORD dwTickValue);
void TickUpdate(void);

#endif
