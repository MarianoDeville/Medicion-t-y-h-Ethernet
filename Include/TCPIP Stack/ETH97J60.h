/*********************************************************************
 *
 *            Ethernet registers/bits for PIC18F97J60
 *
 *********************************************************************
 * FileName:        ETH97J60.h
 * Dependencies:    None
 * Processor:       PIC18F97J60 family
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/

#ifndef __ETH97J60_H
#define __ETH97J60_H
#include "GenericTypeDefs.h"

typedef union {
	unsigned char v[7];
	struct {
		unsigned short int ByteCount;
		unsigned CollisionCount:4;
		unsigned CRCError:1;
		unsigned LengthCheckError:1;
		unsigned LengthOutOfRange:1;
		unsigned Done:1;
		unsigned Multicast:1;
		unsigned Broadcast:1;
		unsigned PacketDefer:1;
		unsigned ExcessiveDefer:1;
		unsigned MaximumCollisions:1;
		unsigned LateCollision:1;
		unsigned Giant:1;
		unsigned Underrun:1;
		unsigned short int BytesTransmittedOnWire;
		unsigned ControlFrame:1;
		unsigned PAUSEControlFrame:1;
		unsigned BackpressureApplied:1;
		unsigned VLANTaggedFrame:1;
		unsigned Zeros:4;
	} bits;
} TXSTATUS;

typedef union {
	unsigned char v[4];
	struct {
		unsigned short int ByteCount;
		unsigned PreviouslyIgnored:1;
		unsigned RXDCPreviouslySeen:1;
		unsigned CarrierPreviouslySeen:1;
		unsigned CodeViolation:1;
		unsigned CRCError:1;
		unsigned LengthCheckError:1;
		unsigned LengthOutOfRange:1;
		unsigned ReceiveOk:1;
		unsigned Multicast:1;
		unsigned Broadcast:1;
		unsigned DribbleNibble:1;
		unsigned ControlFrame:1;
		unsigned PauseControlFrame:1;
		unsigned UnsupportedOpcode:1;
		unsigned VLANType:1;
		unsigned Zero:1;
	} bits;
} RXSTATUS;

/******************************************************************************
* PHY Register Locations
******************************************************************************/
#define PHCON1	0x00
#define PHSTAT1	0x01
#define PHCON2	0x10
#define PHSTAT2	0x11
#define PHIE	0x12
#define PHIR	0x13
#define PHLCON	0x14

typedef union {
	unsigned short int Val;
	WORD_VAL VAL;

	// PHCON1 bits ----------
	struct {
		unsigned:8;
		unsigned PDPXMD:1;
		unsigned:7;
	} PHCON1bits;

	// PHSTAT1 bits --------
	struct {
		unsigned:2;
		unsigned LLSTAT:1;
		unsigned:5;
		unsigned:8;
	} PHSTAT1bits;

	// PHCON2 bits ----------
	struct {
		unsigned:4;
		unsigned RXAPDIS:1;
		unsigned:3;
		unsigned HDLDIS:1;
		unsigned:5;
		unsigned FRCLNK:1;
		unsigned:1;
	} PHCON2bits;

	// PHSTAT2 bits --------
	struct {
		unsigned:8;
		unsigned:2;
		unsigned LSTAT:1;
		unsigned COLSTAT:1;
		unsigned RXSTAT:1;
		unsigned TXSTAT:1;
		unsigned:2;
	} PHSTAT2bits;

	// PHIE bits -----------
	struct {
		unsigned:1;
		unsigned PGEIE:1;
		unsigned:2;
		unsigned PLNKIE:1;
		unsigned:3;
		unsigned:8;
	} PHIEbits;

	// PHIR bits -----------
	struct {
		unsigned:2;
		unsigned PGIF:1;
		unsigned:1;
		unsigned PLNKIF:1;
		unsigned:3;
		unsigned:8;
	} PHIRbits;

	// PHLCON bits -------
	struct {
		unsigned:1;
		unsigned STRCH:1;
		unsigned LFRQ0:1;
		unsigned LFRQ1:1;
		unsigned LBCFG0:1;
		unsigned LBCFG1:1;
		unsigned LBCFG2:1;
		unsigned LBCFG3:1;
		unsigned LACFG0:1;
		unsigned LACFG1:1;
		unsigned LACFG2:1;
		unsigned LACFG3:1;
		unsigned:4;
	} PHLCONbits;
	struct {
		unsigned:1;
		unsigned STRCH:1;
		unsigned LFRQ:2;
		unsigned LBCFG:4;
		unsigned LACFG:4;
		unsigned:4;
	} PHLCONbits2;
} PHYREG;
/******************************************************************************
* Individual Register Bits
******************************************************************************/
// ETH/MAC/MII bits
// EIE bits ----------
#define	EIE_PKTIE		(1<<6)
#define	EIE_DMAIE		(1<<5)
#define	EIE_LINKIE		(1<<4)
#define	EIE_TXIE		(1<<3)
#define	EIE_TXERIE		(1<<1)
#define	EIE_RXERIE		(1)
// EIR bits ----------
#define	EIR_PKTIF		(1<<6)
#define	EIR_DMAIF		(1<<5)
#define	EIR_LINKIF		(1<<4)
#define	EIR_TXIF		(1<<3)
#define	EIR_TXERIF		(1<<1)
#define	EIR_RXERIF		(1)
// ESTAT bits ---------
#define	ESTAT_BUFER		(1<<6)
#define	ESTAT_RXBUSY	(1<<2)
#define	ESTAT_TXABRT	(1<<1)
#define	ESTAT_PHYRDY	(1)
// ECON2 bits --------
#define	ECON2_AUTOINC	(1<<7)
#define	ECON2_PKTDEC	(1<<6)
#define	ECON2_ETHEN		(1<<5)
// ECON1 bits --------
#define	ECON1_TXRST		(1<<7)
#define	ECON1_RXRST		(1<<6)
#define	ECON1_DMAST		(1<<5)
#define	ECON1_CSUMEN	(1<<4)
#define	ECON1_TXRTS		(1<<3)
#define	ECON1_RXEN		(1<<2)
// ERXFCON bits ------
#define	ERXFCON_UCEN	(1<<7)
#define	ERXFCON_ANDOR	(1<<6)
#define	ERXFCON_CRCEN	(1<<5)
#define	ERXFCON_PMEN	(1<<4)
#define	ERXFCON_MPEN	(1<<3)
#define	ERXFCON_HTEN	(1<<2)
#define	ERXFCON_MCEN	(1<<1)
#define	ERXFCON_BCEN	(1)
// MACON1 bits --------
#define	MACON1_TXPAUS	(1<<3)
#define	MACON1_RXPAUS	(1<<2)
#define	MACON1_PASSALL	(1<<1)
#define	MACON1_MARXEN	(1)
// MACON3 bits --------
#define	MACON3_PADCFG2	(1<<7)
#define	MACON3_PADCFG1	(1<<6)
#define	MACON3_PADCFG0	(1<<5)
#define	MACON3_TXCRCEN	(1<<4)
#define	MACON3_PHDREN	(1<<3)
#define	MACON3_HFRMEN	(1<<2)
#define	MACON3_FRMLNEN	(1<<1)
#define	MACON3_FULDPX	(1)
// MACON4 bits --------
#define	MACON4_DEFER	(1<<6)
// MICMD bits ---------
#define	MICMD_MIISCAN	(1<<1)
#define	MICMD_MIIRD		(1)
// MISTAT bits --------
#define	MISTAT_NVALID	(1<<2)
#define	MISTAT_SCAN		(1<<1)
#define	MISTAT_BUSY		(1)
// EFLOCON bits -----
#define	EFLOCON_FCEN1	(1<<1)
#define	EFLOCON_FCEN0	(1)
// PHY bits
// PHCON1 bits ----------
#define	PHCON1_PDPXMD	(1ul<<8)
// PHSTAT1 bits --------
#define	PHSTAT1_LLSTAT	(1ul<<2)
// PHCON2 bits ----------
#define	PHCON2_FRCLNK	(1ul<<14)
#define	PHCON2_HDLDIS	(1ul<<8)
#define PHCON2_RXAPDIS	(1ul<<4)
// PHSTAT2 bits --------
#define	PHSTAT2_TXSTAT	(1ul<<13)
#define	PHSTAT2_RXSTAT	(1ul<<12)
#define	PHSTAT2_COLSTAT	(1ul<<11)
#define	PHSTAT2_LSTAT	(1ul<<10)
// PHIE bits -----------
#define	PHIE_PLNKIE		(1ul<<4)
#define	PHIE_PGEIE		(1ul<<1)
// PHIR bits -----------
#define	PHIR_PLNKIF		(1ul<<4)
#define	PHIR_PGIF		(1ul<<2)
// PHLCON bits -------
#define	PHLCON_LACFG3	(1ul<<11)
#define	PHLCON_LACFG2	(1ul<<10)
#define	PHLCON_LACFG1	(1ul<<9)
#define	PHLCON_LACFG0	(1ul<<8)
#define	PHLCON_LBCFG3	(1ul<<7)
#define	PHLCON_LBCFG2	(1ul<<6)
#define	PHLCON_LBCFG1	(1ul<<5)
#define	PHLCON_LBCFG0	(1ul<<4)
#define	PHLCON_LFRQ1	(1ul<<3)
#define	PHLCON_LFRQ0	(1ul<<2)
#define	PHLCON_STRCH	(1ul<<1)

#endif
