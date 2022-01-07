/*********************************************************************
 *
 *                  General Delay rouines
 *
 *********************************************************************
 * FileName:        Delay.c
 * Dependencies:    Compiler.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#define __DELAY_C

#include "Delay.h"

void DelayMs(unsigned char ms)
{
	unsigned char i;
	CLRWDT();
	while (ms--) {
		i = 4;
		while (i--)
			Delay10us(25);
	}
	return;
}

void DelayS(unsigned char s)
{
	unsigned char i;
	while (s--) {
		i = 4;
		while (i--)
			DelayMs(250);
	}
	return;
}
