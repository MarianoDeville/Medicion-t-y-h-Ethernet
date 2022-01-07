/********************************************************************************/
/*				Tipo de comunicación:	I2C										*/
/*				Compilador:				MPLAB IDE 8 - HI-TECH 9.60				*/
/*				Fecha de creación:		05/03/2011								*/
/*				Fecha de modificación:	05/03/2011								*/
/*				Autor:					Mariano Ariel Deville					*/
/********************************************************************************/
/*						PROTOTIPO DE FUNCIONES									*/
void Medicion_HT(unsigned char *cadena);
void Start(void);
void Envia_ACK(void);
void Espera_ACK(void);
void Comando(unsigned char dispositivo);
void Dos_Bytes(void);
void Envia_No_ACK(void);

