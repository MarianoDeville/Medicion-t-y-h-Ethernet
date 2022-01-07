/*********************************************************************
 * FileName:        Compiler.h
 * Dependencies:    None
 * Processor:       PIC18F97F60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __COMPILER_H
#define __COMPILER_H

#define __18CXX					// lo define para toda la linea 18 de hitech.
#include <htc.h>
#include "HardwareProfile.h"
#define INSTR_FREQ			((CLOCK_FREQ+2ul)/4ul)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Base RAM pointer type for given architecture
#define PTR_BASE	WORD

// Definitions that apply to all compilers
#define memcmppgm2ram(a,b,c)	memcmp(a,b,c)
#define strcmppgm2ram(a,b)		strcmp(a,b)
#define memcpypgm2ram(a,b,c)	memcpy(a,b,c)
#define strcpypgm2ram(a,b)		strcpy(a,b)
#define strncpypgm2ram(a,b,c)	strncpy(a,b,c)
#define strstrrampgm(a,b)		strstr(a,b)
#define	strlenpgm(a)			strlen(a)
#define strchrpgm(a,b)			strchr(a,b)
#define strcatpgm2ram(a,b)		strcat(a,b)

#define	__attribute__(a)
#define ROM                 	const
#define rom
#define Nop()               	asm("NOP");
#define ClrWdt()				asm("CLRWDT");
#define Reset()					asm("RESET");

#endif
