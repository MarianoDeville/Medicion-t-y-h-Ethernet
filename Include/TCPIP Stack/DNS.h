/*********************************************************************
 *
 *	DNS Client Module Header
 *
 *********************************************************************
 * FileName:        DNS.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __DNS_H
#define __DNS_H

// Type constants for DNSResolve(*HostName, Type)
#define DNS_TYPE_A				(1u)
#define DNS_TYPE_MX				(15u)

BOOL DNSBeginUsage(void);
void DNSResolve(unsigned char *HostName, unsigned char Type);
BOOL DNSIsResolved(IP_ADDR * HostIP);
BOOL DNSEndUsage(void);

void DNSResolveROM(const unsigned char *Hostname, unsigned char Type);

#endif
