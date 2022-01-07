/*********************************************************************
 *
 *	Generic TCP Server Example Application
 *  Module for Microchip TCP/IP Stack
 *   -Implements an example "ToUpper" TCP server on port 9760 and 
 *	  should be used as a basis for creating new TCP server 
 *    applications
 *	 -Reference: None.  Hopefully AN833 in the future.
 *
 *********************************************************************
 * FileName:        GenericTCPServer.c
 * Dependencies:    TCP
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32MX
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#define __GENERICTCPSERVER_C

#include "TCPIP Stack/TCPIP.h"

extern unsigned int desbordador;
/*********************************************************************
 * Function:        void GenericTCPServer(void)
 * PreCondition:    Stack is initialized()
 * Input:           None
 * Output:          None
 * Side Effects:    None
 * Overview:        None
 * Note:            None
 ********************************************************************/
void TCPServer(unsigned int server_port)
{
	unsigned char AppBuffer[32],cadena[8],i;
	static TCP_SOCKET MySocket;
	static enum _TCPServerState {
		SM_HOME = 0,
		SM_LISTENING,
	} TCPServerState = SM_HOME;
	switch (TCPServerState)
	{
	case SM_HOME:
		// Allocate a socket for this server to listen and accept connections on
		MySocket =TCPOpen(0,TCP_OPEN_SERVER,server_port,TCP_PURPOSE_DEFAULT);
		if(MySocket==INVALID_SOCKET)
			return;
		TCPServerState = SM_LISTENING;
		desbordador=0;
		break;

	case SM_LISTENING:
		CLRWDT();
		if(!TCPIsConnected(MySocket))							// Consulto si la conección está abierta.
			return;
		if(desbordador++>9000)
		{
			desbordador=0;
			TCPDisconnect(MySocket);							// Cierro la conexción TCP.
			TCPDiscard(MySocket);
			return;
		}
		i=TCPGetArray(MySocket, AppBuffer, sizeof(AppBuffer));	// Copio el mensaje recibido por tcp a una variable local.
		AppBuffer[i]=0;
		if(i>2)
		{
			if(!strcmp(AppBuffer,"Lecturas"))
			{
				Medicion_HT(cadena);							// Obtengo los valores del sensor.
				TCPPutArray(MySocket, cadena,4);				// Envio 4 bytes con la informacion.
			}
			TCPDisconnect(MySocket);							// Cierro la conexción TCP.
			TCPDiscard(MySocket);
		}
		break;
	}
	return;
}

	
