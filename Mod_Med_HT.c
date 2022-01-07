/********************************************************************************/
/*				Revisión:				1.00									*/
/*				Tipo de comunicación:											*/
/*				Compilador:				MPLAB IDE 8 - HI-TECH 9.60				*/
/*				Fecha de creación:		01/03/2011								*/
/*				Fecha de modificación:	05/03/2011								*/
/*				Autor:					Deville									*/
/********************************************************************************/
/*						PROTOTIPO DE FUNCIONES									*/
#include "TCPIP Stack/TCPIP.h"
unsigned char ack,dat[8];
void Medicion_HT(unsigned char *cad)
{
	unsigned int cont=0;
	Start(); 
	Comando(0b10100000);
	Espera_ACK();
	cont=0;
	while(DATA && cont++<4000)
		Delay10us(2);
	Dos_Bytes();
	if(cont>3990)       // Falló la comunicacion con el sensor
    {
		dat[0]=0XFF;
		dat[1]=0XFF;
    }
	*cad++=dat[0];
	*cad++=dat[1];
	DelayMs(1);
	Start(); 
	Comando(0b11000000);
	Espera_ACK();
	cont=0;
	while(DATA && cont++<4000)
		Delay10us(4);
	Dos_Bytes();
	if(cont>3990)       // Falló la comunicacion con el sensor
    {
		dat[0]=0XFF;
		dat[1]=0XFF;
    }
	*cad++=dat[0];
	*cad++=dat[1];
	return;
}
void Start(void)
{
	C_DATA=0;    // Coloco DATA como salida.
	DATA=1;
	Delay10us(15);
	SCK=0;
	Delay10us(1);
	SCK=1;
	Delay10us(1);
	DATA=0; 
	Delay10us(1);
	SCK=0;
	Delay10us(1);
	SCK=1;
	Delay10us(1);
	DATA=1; 
	Delay10us(1);
	SCK=0;
	Delay10us(1);
	DATA=0; 
	Delay10us(1);
	return;
}
void Comando(unsigned char comando)
{
	volatile unsigned char k;
	for(k=0;k<8;k++)   //Comando.
	{
		SCK=0;
		Delay10us(1);
		DATA=comando&0b00000001;
		comando=comando>>1;
		Delay10us(1);
		SCK=1;
		Delay10us(1);
	}
	C_DATA=1;		// Coloco el pin como entrada.
	return;
}
void Espera_ACK(void)
{ 
	SCK=0;
	Delay10us(1);
	SCK=1;
	if(DATA)
		ack=0;
	else
		ack=1;
	Delay10us(1);
	SCK=0;
	return;

}
void Dos_Bytes(void)
{    
	volatile unsigned char k;
	for(k=0;k<=7;k++)    		    //RECIBO LA MEDICION MBS
    {
		Delay10us(1);
		SCK=1;
		dat[0]=(dat[0]<<1);			// Desplazo los bits un lugar
		dat[0]=DATA|dat[0];
		Delay10us(1);
		SCK=0;
    } 
	Envia_ACK(); //MSB
	for(k=8;k<=15;k++)    			 //RECIBO LA MEDICION LBS
	{
		Delay10us(1);
		SCK=1;
		dat[1]=(dat[1]<<1);			// Desplazo los bits un lugar
		dat[1]=DATA|dat[1];
		Delay10us(1);
		SCK=0;
    }
	Envia_No_ACK();
	return;
}
void Envia_ACK(void)
{ 
	C_DATA=0;    // Coloco DATA como salida.
	DATA=0;      // Envio ACK
	Delay10us(1);
	SCK=1;
	Delay10us(1);
	SCK=0; 
	C_DATA=1;    // Coloco DATA como entrada.
	return;
}
void Envia_No_ACK(void)
{ 
	C_DATA=0;    // Coloco DATA como salida.
	DATA=1;      // Envio ACK
	Delay10us(1);
	SCK=1;
	Delay10us(1);
	SCK=0; 
	C_DATA=1;    // Coloco DATA como entrada.
	return;
}
