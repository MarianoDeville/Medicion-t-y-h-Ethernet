/*********************************************************************
 *
 *	Hardware specific definitions
 *
 *********************************************************************
 * FileName:        HardwareProfile.h
 * Dependencies:    None
 * Processor:       PIC18F97J60
 ********************************************************************/
#ifndef __HARDWARE_PROFILE_H
#define __HARDWARE_PROFILE_H

#define PICDEMNET2

#define CLOCK_FREQ		(41666667ul)	// Hz

typedef struct {
	unsigned char BOR:1;
	unsigned char POR:1;
	unsigned char PD:1;
	unsigned char TO:1;
	unsigned char RI:1;
	unsigned char CM:1;
	unsigned char:1;
	unsigned char IPEN:1;
} RCONbits;
typedef struct {
	unsigned char BF:1;
	unsigned char UA:1;
	unsigned char R_W:1;
	unsigned char S:1;
	unsigned char P:1;
	unsigned char D_A:1;
	unsigned char CKE:1;
	unsigned char SMP:1;
} SSPSTATbits;
typedef struct {
	unsigned char SSPM0:1;
	unsigned char SSPM1:1;
	unsigned char SSPM2:1;
	unsigned char SSPM3:1;
	unsigned char CKP:1;
	unsigned char SSPEN:1;
	unsigned char SSPOV:1;
	unsigned char WCOL:1;
} SSPCON1bits;
typedef struct {
	unsigned char SEN:1;
	unsigned char RSEN:1;
	unsigned char PEN:1;
	unsigned char RCEN:1;
	unsigned char ACKEN:1;
	unsigned char ACKDT:1;
	unsigned char ACKSTAT:1;
	unsigned char GCEN:1;
} SSPCON2bits;
typedef struct {
	unsigned char RBIF:1;
	unsigned char INT0IF:1;
	unsigned char TMR0IF:1;
	unsigned char RBIE:1;
	unsigned char INT0IE:1;
	unsigned char TMR0IE:1;
	unsigned char GIEL:1;
	unsigned char GIEH:1;
} INTCONbits;
typedef struct {
	unsigned char RBIP:1;
	unsigned char INT3IP:1;
	unsigned char TMR0IP:1;
	unsigned char INTEDG3:1;
	unsigned char INTEDG2:1;
	unsigned char INTEDG1:1;
	unsigned char INTEDG0:1;
	unsigned char RBPU:1;
} INTCON2bits;
typedef struct {
	unsigned char ADON:1;
	unsigned char GO:1;
	unsigned char CHS0:1;
	unsigned char CHS1:1;
	unsigned char CHS2:1;
	unsigned char CHS3:1;
	unsigned char:1;
	unsigned char ADCAL:1;
} ADCON0bits;
typedef struct {
	unsigned char ADCS0:1;
	unsigned char ADCS1:1;
	unsigned char ADCS2:1;
	unsigned char ACQT0:1;
	unsigned char ACQT1:1;
	unsigned char ACQT2:1;
	unsigned char:1;
	unsigned char ADFM:1;
} ADCON2bits;
typedef struct {
	unsigned char TMR1IF:1;
	unsigned char TMR2IF:1;
	unsigned char CCP1IF:1;
	unsigned char SSPIF:1;
	unsigned char TXIF:1;
	unsigned char RCIF:1;
	unsigned char ADIF:1;
	unsigned char PSPIF:1;
} PIR1bits;
typedef struct {
	unsigned char CCP2IF:1;
	unsigned char TMR3IF:1;
	unsigned char HLVDIF:1;
	unsigned char BCLIF:1;
	unsigned char EEIF:1;
	unsigned char:1;
	unsigned char CMIF:1;
	unsigned char OSCFIF:1;
} PIR2bits;
typedef struct {
	unsigned char TMR1IE:1;
	unsigned char TMR2IE:1;
	unsigned char CCP1IE:1;
	unsigned char SSPIE:1;
	unsigned char TXIE:1;
	unsigned char RCIE:1;
	unsigned char ADIE:1;
	unsigned char PSPIE:1;
} PIE1bits;
typedef struct {
	unsigned char TMR1IP:1;
	unsigned char TMR2IP:1;
	unsigned char CCP1IP:1;
	unsigned char SSP1IP:1;
	unsigned char TXIP:1;
	unsigned char RCIP:1;
	unsigned char ADIP:1;
	unsigned char PSPIP:1;
} IPR1bits;
typedef struct {
	unsigned char T0PS0:1;
	unsigned char T0PS1:1;
	unsigned char T0PS2:1;
	unsigned char PSA:1;
	unsigned char T0SE:1;
	unsigned char T0CS:1;
	unsigned char T08BIT:1;
	unsigned char TMR0ON:1;
} T0CONbits;
typedef struct {
	unsigned char TX9D:1;
	unsigned char TRMT:1;
	unsigned char BRGH:1;
	unsigned char SENDB:1;
	unsigned char SYNC:1;
	unsigned char TXEN:1;
	unsigned char TX9:1;
	unsigned char CSRC:1;
} TXSTAbits;
typedef struct {
	unsigned char RX9D:1;
	unsigned char OERR:1;
	unsigned char FERR:1;
	unsigned char ADDEN:1;
	unsigned char CREN:1;
	unsigned char SREN:1;
	unsigned char RX9:1;
	unsigned char SPEN:1;
} RCSTAbits;
typedef struct {
	unsigned char LATA0:1;
	unsigned char LATA1:1;
	unsigned char LATA2:1;
	unsigned char LATA3:1;
	unsigned char LATA4:1;
	unsigned char LATA5:1;
	unsigned char REPU:1;
	unsigned char RDPU:1;
} LATAbits;
typedef struct {
	unsigned char TRISA0:1;
	unsigned char TRISA1:1;
	unsigned char TRISA2:1;
	unsigned char TRISA3:1;
	unsigned char TRISA4:1;
	unsigned char TRISA5:1;
	unsigned char:2;
} TRISAbits;
typedef struct {
	unsigned:5;
	unsigned ETHEN:1;
	unsigned PKTDEC:1;
	unsigned AUTOINC:1;
} ECON2bits;
typedef struct {
	unsigned PHYRDY:1;
	unsigned TXABRT:1;
	unsigned RXBUSY:1;
	unsigned:3;
	unsigned BUFER:1;
} ESTATbits;
typedef struct {
	unsigned:2;
	unsigned RXEN:1;
	unsigned TXRTS:1;
	unsigned CSUMEN:1;
	unsigned DMAST:1;
	unsigned RXRST:1;
	unsigned TXRST:1;
} ECON1bits;
typedef struct {
	unsigned RXERIF:1;
	unsigned TXERIF:1;
	unsigned:1;
	unsigned TXIF:1;
	unsigned LINKIF:1;
	unsigned DMAIF:1;
	unsigned PKTIF:1;
} EIRbits;
typedef struct {
	unsigned BUSY:1;
	unsigned SCAN:1;
	unsigned NVALID:1;
} MISTATbits;
typedef struct {
	unsigned ABDEN:1;
	unsigned WUE:1;
	unsigned:1;
	unsigned BRG16:1;
	unsigned TXCKP:1;
	unsigned RXDTP:1;
	unsigned RCIDL:1;
	unsigned ABDOVF:1;
} BAUDCONbits;

#define SPBRGH				SPBRGH1

#define RCONbits			(*((RCONbits*)&RCON))
#define SSP1STATbits		(*((SSPSTATbits*)&SSP1STAT))
#define INTCONbits			(*((INTCONbits*)&INTCON))
#define INTCON2bits			(*((INTCON2bits*)&INTCON2))
#define ADCON0bits			(*((ADCON0bits*)&ADCON0))
#define ADCON2bits			(*((ADCON2bits*)&ADCON2))
#define PIR1bits			(*((PIR1bits*)&PIR1))
#define PIR2bits			(*((PIR2bits*)&PIR2))
#define PIE1bits			(*((PIE1bits*)&PIE1))
#define IPR1bits			(*((IPR1bits*)&IPR1))
#define T0CONbits			(*((T0CONbits*)&T0CON))
#define TXSTAbits			(*((TXSTAbits*)&TXSTA))
#define RCSTAbits			(*((RCSTAbits*)&RCSTA))
#define SSPCON1bits			(*((SSPCON1bits*)&SSPCON1))
#define SSPCON2bits			(*((SSPCON2bits*)&SSPCON2))
#define LATAbits			(*((LATAbits*)&LATA))
#define TRISAbits			(*((TRISAbits*)&TRISA))
#define ECON1bits			(*((ECON1bits*)&ECON1))
#define ESTATbits			(*((ESTATbits*)&ESTAT))
#define ECON2bits			(*((ECON2bits*)&ECON2))
#define EIRbits				(*((EIRbits*)&EIR))
#define MISTATbits			(*((MISTATbits*)&MISTAT))
#define BAUDCONbits			(*((BAUDCONbits*)&BAUDCON1))

// 25LC256 I/O pins
#define EEPROM_CS_TRIS		(TRISD7)
#define EEPROM_CS_IO		(LATD7)
#define EEPROM_SCK_TRIS		(TRISC3)
#define EEPROM_SDI_TRIS		(TRISC4)
#define EEPROM_SDO_TRIS		(TRISC5)
#define EEPROM_SPI_IF		(SSP1IF)
#define EEPROM_SSPBUF		(SSP1BUF)
#define EEPROM_SPICON1		(SSP1CON1)
#define EEPROM_SPICON1bits	(SSP1CON1bits)
#define EEPROM_SPICON2		(SSP1CON2)
#define EEPROM_SPISTAT		(SSP1STAT)
#define EEPROM_SPISTATbits	(SSP1STATbits)

#define BusyUART()			BusyUSART()
#define CloseUART()			CloseUSART()
#define ConfigIntUART(a)	ConfigIntUSART(a)
#define DataRdyUART()		DataRdyUSART()
#define OpenUART(a,b,c)		OpenUSART(a,b,c)
#define ReadUART()			ReadUSART()
#define WriteUART(a)		WriteUSART(a)
#define getsUART(a,b,c)		getsUSART(b,a)
#define putsUART(a)			putsUSART(a)
#define getcUART()			ReadUSART()
#define putcUART(a)			WriteUSART(a)
#define putrsUART(a)		putrsUSART((far rom char*)a)

#endif
