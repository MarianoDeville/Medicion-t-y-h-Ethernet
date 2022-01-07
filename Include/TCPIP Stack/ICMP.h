/*********************************************************************
 *
 *                  ICMP Module Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        ICMP.h
 * Dependencies:    StackTsk.h
 *                  IP.h
 *                  MAC.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __ICMP_H
#define __ICMP_H

void ICMPProcess(NODE_INFO * remote, WORD len);

BOOL ICMPBeginUsage(void);
void ICMPSendPing(DWORD dwRemoteIP);
LONG ICMPGetReply(void);
void ICMPEndUsage(void);

#endif
