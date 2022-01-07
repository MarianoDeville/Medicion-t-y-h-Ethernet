/*********************************************************************
 *
 *                  ARP Module Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        ARP.h
 * Dependencies:    Stacktsk.h
 *                  MAC.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#ifndef __ARP_H
#define __ARP_H

/*********************************************************************
 * Function:        void ARPInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          ARP Cache is initialized.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
#ifdef STACK_CLIENT_MODE
void ARPInit(void);
#else
#define ARPInit()
#endif

/*********************************************************************
 * Function:        BOOL ARPProcess(void)
 *
 * PreCondition:    ARP packet is ready in MAC buffer.
 *
 * Input:           None
 *
 * Output:          ARP FSM is executed.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BOOL ARPProcess(void);

/*********************************************************************
 * Function:        void ARPResolve(IP_ADDR* IPAddr)
 *
 * PreCondition:    None
 *
 * Input:           IPAddr  - IP Address to be resolved.
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        An ARP request is sent.
 *
 * Note:
 ********************************************************************/
void ARPResolve(IP_ADDR * IPAddr);

/*********************************************************************
 * Function:        BOOL ARPIsResolved(IP_ADDR* IPAddr,
 *                                      MAC_ADDR *MACAddr)
 *
 * PreCondition:    None
 *
 * Input:           IPAddr      - IPAddress to be resolved.
 *                  MACAddr     - Buffer to hold corresponding
 *                                MAC Address.
 *
 * Output:          TRUE if given IP Address has been resolved.
 *                  FALSE otherwise.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            This function is available only when
 *                  STACK_CLIENT_MODE is defined.
 ********************************************************************/
BOOL ARPIsResolved(IP_ADDR * IPAddr, MAC_ADDR * MACAddr);

#endif
