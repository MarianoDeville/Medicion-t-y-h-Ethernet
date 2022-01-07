/*********************************************************************
 *
 *  Microchip TCP/IP Stack Include File
 *
 *********************************************************************
 * FileName:        TCPIP.h
 * Dependencies:    
 * Processor:       PIC18F67J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __TCPIP_H
#define __TCPIP_H

#define VERSION 		"EVENTO"	// TCP/IP stack version

#include <string.h>
#include <stdlib.h>
#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "HardwareProfile.h"
#include "TCPIPConfig.h"
#include "TCPIP Stack/StackTsk.h"
#include "TCPIP Stack/Helpers.h"
#include "TCPIP Stack/Delay.h"
#include "TCPIP Stack/Tick.h"
#include "TCPIP Stack/MAC.h"
#include "TCPIP Stack/IP.h"
#include "TCPIP Stack/ARP.h"
#include "TCPIP Stack/UDP.h"
#include "TCPIP Stack/TCP.h"
#include "TCPIP Stack/ICMP.h"
#include "TCPIP Stack/Announce.h"
#include "TCPIP Stack/NBNS.h"
#include "TCPIP Stack/ServidorTCP.h"
#include "Mod_Med_HT.h"
#include "i2c.h"
#endif
