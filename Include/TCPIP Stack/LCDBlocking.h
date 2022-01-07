/*********************************************************************
 *
 *   LCD Access Routines header
 *
 *********************************************************************
 * FileName:        LCDBlocking.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __LCDBLOCKING_H
#define __LCDBLOCKING_H

// Do not include this source file if there is no LCD on the 
// target board
#ifdef USE_LCD


extern unsigned char LCDText[16 * 2 + 1];
void LCDInit(void);
void LCDUpdate(void);
void LCDErase(void);

#endif
#endif
