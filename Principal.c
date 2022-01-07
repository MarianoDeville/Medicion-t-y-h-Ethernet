/*********************************************************************
 * FileName:        Principal.c
 * Dependencies:    TCPIP.h
 * Processor:       PIC18F67J60
 * Complier:        HI-TECH PICC-18 9.50PL3 or higher
 ********************************************************************/
unsigned int desbordador;
#define THIS_IS_STACK_APPLICATION
#define BAUD_RATE       (9600)	// bps
#define SCK			RC3
#define DATA		RC4
#define C_DATA		TRISC4

#include "TCPIP Stack/TCPIP.h"
#include "I2C.c"
#include "Mod_Med_HT.c"

APP_CONFIG AppConfig;
unsigned char myDHCPBindCount = 0xFF;

__CONFIG(1, WDTEN & XINSTDIS & STVREN & DEBUGDIS & PROTECT);	// 
__CONFIG(2, HSPLL & WDTPS4K & FCMEN & IESOEN);					// 
__CONFIG(3, ETHLEDEN);					//

static void InitAppConfig(void);
static void InitializeBoard(void);
static void ProcessIO(void);
static void FormatNetBIOSName(unsigned char Name[16]);

void interrupt low_priority LowISR(void)
{
	TickUpdate();
	return;
}
void interrupt HighISR(void)
{
	return;
}

void main(void)
{
	InitializeBoard();			// Initialize any application specific hardware.
	TickInit();					// Following steps must be performed for all applications using the Microchip TCP/IP Stack.
	InitAppConfig();
	StackInit();				// Initialize core stack layers (MAC, ARP, TCP, UDP)
	CLRWDT();
	while(1)
	{
		StackTask();			// This task performs normal stack task including checking for incoming packet, type of packet and calling appropriate stack entity to process it.
		DiscoveryTask();		// Uso STACK_USE_ANNOUNCE
		NBNSTask();				// Lo uso para el nombre NetBios
		TCPServer(4321);		// Contesto los requerimientos de los clientes.
	}
}
/*********************************************************************
 * Function:        void InitializeBoard(void)
 * PreCondition:    None
 * Input:           None
 * Output:          None
 * Side Effects:    None
 * Overview:        Initialize board specific hardware.
 * Note:            None
 ********************************************************************/
static void InitializeBoard(void)
{
	TRISA = 0xFF;				// Configuro todos los pines como entradas
	TRISB = 0xFF;
	TRISC = 0xFF;
	TRISD = 0xFF;
	TRISE = 0xFF;
	TRISF = 0xFF;
	TRISG = 0xFF;
	OSCTUNE = 0x40;				// Enable 4x/5x PLL.
	RBPU = 0;					// Disable internal PORTB pull-ups
	// Enable Interrupts
	IPEN = 1;					// Enable interrupt priorities
	GIEH = 1;
	GIEL = 1;
	TRISC3=0;
	TRISC4=0;
	return;
}

/*********************************************************************
 * Function:        void InitAppConfig(void)
 * PreCondition:    MPFSInit() is already called.
 * Input:           None
 * Output:          Write/Read non-volatile config variables.
 * Side Effects:    None
 * Overview:        None
 * Note:            None
 ********************************************************************/
static const unsigned char SerializedMACAddress[6] =
	{ MY_DEFAULT_MAC_BYTE1, MY_DEFAULT_MAC_BYTE2, MY_DEFAULT_MAC_BYTE3,
	MY_DEFAULT_MAC_BYTE4, MY_DEFAULT_MAC_BYTE5, MY_DEFAULT_MAC_BYTE6
};

static void InitAppConfig(void)
{
	AppConfig.Flags.bIsDHCPEnabled = TRUE;
	AppConfig.Flags.bInConfigMode = TRUE;
	memcpypgm2ram((void *) &AppConfig.MyMACAddr,(const void *) SerializedMACAddress,sizeof(AppConfig.MyMACAddr));
	AppConfig.MyIPAddr.Val =
		MY_DEFAULT_IP_ADDR_BYTE1 | MY_DEFAULT_IP_ADDR_BYTE2 << 8ul |
		MY_DEFAULT_IP_ADDR_BYTE3 << 16ul | MY_DEFAULT_IP_ADDR_BYTE4 <<
		24ul;
	AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;
	AppConfig.MyMask.Val =
		MY_DEFAULT_MASK_BYTE1 | MY_DEFAULT_MASK_BYTE2 << 8ul |
		MY_DEFAULT_MASK_BYTE3 << 16ul | MY_DEFAULT_MASK_BYTE4 << 24ul;
	AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;
	AppConfig.MyGateway.Val =
		MY_DEFAULT_GATE_BYTE1 | MY_DEFAULT_GATE_BYTE2 << 8ul |
		MY_DEFAULT_GATE_BYTE3 << 16ul | MY_DEFAULT_GATE_BYTE4 << 24ul;
	AppConfig.PrimaryDNSServer.Val =
		MY_DEFAULT_PRIMARY_DNS_BYTE1 | MY_DEFAULT_PRIMARY_DNS_BYTE2 << 8ul
		| MY_DEFAULT_PRIMARY_DNS_BYTE3 << 16ul |
		MY_DEFAULT_PRIMARY_DNS_BYTE4 << 24ul;
	AppConfig.SecondaryDNSServer.Val =
		MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2 <<
		8ul | MY_DEFAULT_SECONDARY_DNS_BYTE3 << 16ul |
		MY_DEFAULT_SECONDARY_DNS_BYTE4 << 24ul;
	// Load the default NetBIOS Host Name
	memcpypgm2ram(AppConfig.NetBIOSName,(const void *) MY_DEFAULT_HOST_NAME, 16);
	FormatNetBIOSName(AppConfig.NetBIOSName);
	return;
}

// NOTE: Name[] must be at least 16 characters long.
// It should be exactly 16 characters, as defined by NetBIOS spec.
static void FormatNetBIOSName(unsigned char Name[])
{
	unsigned char i;
	Name[15] = '\0';
	strupr((char *) Name);
	i = 0;
	while(i<15u)
	{
		if(Name[i]=='\0')
		{
			while(i<15u)
				Name[i++]=' ';
			break;
		}
		i++;
	}
	return;
}
