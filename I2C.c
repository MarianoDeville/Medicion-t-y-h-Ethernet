/********************************************************************************/
/*			UTILIZACION DEL MODULO I2C DEL PIC18F4620							*/
/*				Revisión:				1.00									*/
/*				PIC:					PIC18F67J60								*/
/*				Compilador:				MPLAB IDE 8.53 - HI-TECH PICC18 9.50	*/
/*				Fecha de creación:		01/03/2011								*/
/*				Fecha de modificación:	05/03/2011								*/
/*				Autor:					Mariano Ariel Deville					*/
/********************************************************************************/
#include "i2c.h"
/********************************************************************************/
/*				CONFIGURACION E INICIALIZACION DEL MODULO						*/
/********************************************************************************/
void I2C_Setup(unsigned int velo)
{
	CLRWDT();
	TRISC3=1;			// Set SCL and SDA pins as inputs.
	TRISC4=1;
	SMP=1;
	SSP1CON1=0x38;		// Set I2C master mode.
//	SSP1CON2=0b00001111;
	SSP1CON2=0x00;
//	SSP1ADD=(PIC_CLK/(velo*4000))-1;	// clock=Osc/(4*(sspad+1)).
	SSP1ADD=255;			// clock=Osc/(4*(sspad+1)).
	GCEN=0;
	CKE=1;				// Use I2C levels worked also with '0'.
	SMP=1;				// Disable slew rate control  worked also with '0'.
	PSPIF=0;			// Clear SSPIF interrupt flag.
	BCL1IF=0;			// Clear bus collision flag.
}
/********************************************************************************/
/*						*/
/********************************************************************************/
void I2C_Wait_Idle(void)
{
	while((SSP1CON2&0x1F)|RW)
		continue;
}
/********************************************************************************/
/*						COMIENZO LA TRANSMISION									*/
/********************************************************************************/
void I2C_Start(void)
{
	I2C_Wait_Idle();
	SEN=1;
}
/********************************************************************************/
/*						REINICIALIZO LA TRANSMISION								*/
/********************************************************************************/
void I2C_RepStart(void)
{
	I2C_Wait_Idle();
	RSEN=1;
}
/********************************************************************************/
/*						TERMINO LA TRANSMISION									*/
/********************************************************************************/
void I2C_Stop(void)
{
	I2C_Wait_Idle();
	PEN=1;
}
/********************************************************************************/
/*						LEO UN CARACTER 										*/
/********************************************************************************/
unsigned char I2C_Read(unsigned char ack)
{
	unsigned char i2cReadData;
	I2C_Wait_Idle();
	RCEN=1;
	I2C_Wait_Idle();
	i2cReadData=SSP1BUF;
	I2C_Wait_Idle();
	ACKEN=1;               // send acknowledge sequence
	ACKDT=!ack;
	return(i2cReadData);
}
/********************************************************************************/
/*						ESCRIBO UN CARACTER										*/
/********************************************************************************/
unsigned char I2C_Write(unsigned char i2cWriteData)
{
	I2C_Wait_Idle();
	SSP1BUF=i2cWriteData;
	return(!ACKSTAT);		// function returns '1' if transmission is acknowledged.
}
