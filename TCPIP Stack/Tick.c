/*********************************************************************
 *
 *                  Tick Manager for Timekeeping
 *
 *********************************************************************
 * FileName:        Tick.c
 * Dependencies:    Timer 0 (PIC18) or Timer 1 (PIC24F, PIC24H, 
 *					dsPIC30F, dsPIC33F, PIC32MX)
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32MX
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
********************************************************************/
#define __TICK_C

#include "TCPIP Stack/TCPIP.h"

static DWORD dwInternalTicks = 0;
static unsigned char vTickReading[6];

static void GetTickCopy(void);


/*********************************************************************
 * Function:        void TickInit(void)
 * PreCondition:    None
 * Input:           None
 * Output:          Tick manager is initialized.
 * Side Effects:    None
 * Overview:        Initializes Timer0 as a tick counter.
 * Note:            None
 ********************************************************************/
void TickInit(void)
{
	// Use Timer0 for 8 bit processors
	// Initialize the time
	TMR0H = 0;
	TMR0L = 0;
	// Set up the timer interrupt
	INTCON2bits.TMR0IP = 0;		// Low priority
	INTCONbits.TMR0IF = 0;
	INTCONbits.TMR0IE = 1;		// Enable interrupt
	// Timer0 on, 16-bit, internal timer, 1:256 prescalar
	T0CON = 0x87;
}
static void GetTickCopy(void)
{
	// Perform an Interrupt safe and synchronized read of the 48-bit 
	// tick value
	do {
		INTCONbits.TMR0IE = 1;	// Enable interrupt
		Nop();
		INTCONbits.TMR0IE = 0;	// Disable interrupt
		vTickReading[0] = TMR0L;
		vTickReading[1] = TMR0H;
		*((DWORD *) & vTickReading[2]) = dwInternalTicks;
	} while (INTCONbits.TMR0IF);
	INTCONbits.TMR0IE = 1;		// Enable interrupt
}
/*********************************************************************
 * Function:        DWORD TickGet(void)
 *					DWORD TickGetDiv256(void)
 *					DWORD TickGetDiv64K(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Current tick value is given.  Write timing 
 *					measurement code by calling this function and 
 *					comparing two successive results with TICK_SECOND, 
 *					TICK_SECOND*2, TICK_SECOND/1000, etc.
 *
 *					TickGet() returns the least significant 32 bits 
 *					of the internal tick counter.  It is useful for 
 *					measuring time differences up to a few hours.
 *
 *					TickGetDiv256() returns the same timer value, but 
 *					scaled by a factor of 256.  Useful for measuring 
 *					time diferences up to a few weeks.
 *
 *					TickGetDiv64K() returns the same timer value, but 
 *					scaled by a factor of 65536.  Useful for measuring 
 *					absolute time and time diferences up to a few 
 *					years.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
DWORD TickGet(void)
{
	GetTickCopy();
	return *((DWORD *) & vTickReading[0]);
}

DWORD TickGetDiv256(void)
{
	DWORD_VAL ret;
	GetTickCopy();
	ret.v[0] = vTickReading[1];	// Note: This copy must be done one 
	ret.v[1] = vTickReading[2];	// byte at a time to prevent misaligned 
	ret.v[2] = vTickReading[3];	// memory reads, which will reset the PIC.
	ret.v[3] = vTickReading[4];
	return ret.Val;
}

/*********************************************************************
 * Function:        void TickUpdate(void)
 * PreCondition:    None
 * Input:           None
 * Output:          None
 * Side Effects:    None
 * Overview:        Internal Tick and Seconds count are updated.
 * Note:            None
 ********************************************************************/
void TickUpdate(void)
{
	if (INTCONbits.TMR0IF)
	{
		// Increment internal high tick counter
		dwInternalTicks++;
		// Reset interrupt flag
		INTCONbits.TMR0IF = 0;
	}
}

