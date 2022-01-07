/*********************************************************************
 *
 *                  General Delay rouines
 *
 *********************************************************************
 * FileName:        Delay.h
 * Dependencies:    Compiler.h
 * Processor:       PIC18F67J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __DELAY_H
#define __DELAY_H

#include "Compiler.h"

#if !defined(INSTR_FREQ)
#error INSTR_FREQ must be defined.
#endif

#define Delay10us(x)			\
{								\
	unsigned long _dcnt;		\
	_dcnt=x*((unsigned long)(0.00001/(1.0/INSTR_FREQ)/6));	\
	while(_dcnt--);				\
}
void DelayMs(unsigned char ms);
void DelayS(unsigned char s);

#endif
