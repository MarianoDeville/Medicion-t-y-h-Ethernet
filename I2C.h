/********************************************************************************/
/*			UTILIZACION DEL MODULO I2C DEL PIC18F4620							*/
/*				Revisión:				1.00									*/
/*				PIC:					PIC18F4620								*/
/*				Compilador:				MPLAB IDE 8.53 - HI-TECH PICC18 9.50	*/
/*				Fecha de creación:		29/06/2010								*/
/*				Fecha de modificación:	09/09/2010								*/
/*				Autor:					Mariano Ariel Deville					*/
/********************************************************************************/
/*						PROTOTIPO DE FUNCIONES									*/
/********************************************************************************/
void I2C_Setup(unsigned int velo);
void I2C_Wait_Idle(void);
void I2C_Start(void);
void I2C_RepStart(void);
void I2C_Stop(void);
unsigned char I2C_Read(unsigned char ack);
unsigned char I2C_Write(unsigned char i2cWriteData);
