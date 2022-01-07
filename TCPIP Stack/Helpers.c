/*********************************************************************
 *
 *           Helper Functions for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:		Helpers.C
 * Dependencies:	None
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32MX
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 * Company:			Microchip Technology, Inc.
 *
 ********************************************************************/
#define __HELPERS_C

#include "TCPIP Stack/TCPIP.h"

/******************************************************************************
 * Function:        DWORD GenerateRandomDWORD(void)
 *
 * PreCondition:    None
 * Input:           A/D converter, Timer0 (Timer1 on 16-bit PICs), 
 *					rand()/srand() C functions
 * Output:          32-bit random number.  The intention is for this value to 
 *					be completely random and unpredictable (i.e. 
 *					cryptographically secure and passing statistical 
 *					randomness analysis tests).  Whether or not this is true 
 *					on all (or any) devices/voltages/temperatures is untested.
 * Side Effects:    - A/D converter will be used.  (i.e. you must disable 
 *					  interrupts if you use the A/D converter in your ISR)
 *					- C rand() function will be reseeded with something random.
 *					- Timer0 (Timer1) will be used.  TMR0H:TMR0L (TMR1) will 
 *					  have a new value.
 * Overview:        Collects true randomness by comparing the A/D converter's 
 *					internal R/C oscillator clock with our main system clock.  
 *					By passing collected entropy to the C rand()/srand() 
 *					functions, the output is normalized to meet statistical 
 *					randomness tests.
 * Note:            This function times out after 1 second of attempting to 
 *					generate the random DWORD.  In such a case, the output 
 *					value may not be random.
 *					Typically, this function executes in around 500,000 
 *					instruction cycles or less.
 *****************************************************************************/
DWORD GenerateRandomDWORD(void)
{


	unsigned char vBitCount;
	WORD w, wTime, wLastValue;
	DWORD dwTotalTime;
	DWORD dwRandomResult;
/*
#if defined __18CXX
	{
		unsigned char ADCON0Save, ADCON2Save;
		unsigned char T0CONSave, TMR0HSave, TMR0LSave;
		unsigned int desbordador;
		// Save hardware SFRs
		ADCON0Save = ADCON0;
		ADCON2Save = ADCON2;
		T0CONSave = T0CON;
		TMR0LSave = TMR0L;
		TMR0HSave = TMR0H;
		// Set up Timer and A/D converter module
		ADCON0=0x01;			// Turn on the A/D module
		ADCON2=0x3F;			// 20 Tad acquisition, Frc A/D clock used for conversion
		T0CON=0x88;				// TMR0ON = 1, no prescalar
		vBitCount=0;
		dwTotalTime=0;
		while(1)
		{
			// Time the duration of an A/D acquisition and conversion
			TMR0H = 0x00;
			TMR0L = 0x00;
			ADCON0bits.GO = 1;
			ClrWdt();
			while (ADCON0bits.GO);
			*(((unsigned char *) &wTime) + 0) = TMR0L;
			*(((unsigned char *) &wTime) + 1) = TMR0H;
			w = rand();
			// Wait no longer than 1 second obtaining entropy
			dwTotalTime += wTime;
			if (dwTotalTime >= (DWORD) CLOCK_FREQ / 4ul) {
				dwRandomResult ^=
					rand() | (((DWORD) rand()) << 15ul) | (((DWORD) rand())
														   << 30ul);
				break;
			}
			// Keep sampling if minimal entropy was likely obtained this round
			if (wLastValue == wTime)
				continue;
			// Add this entropy into the pseudo random number generator by reseeding
			srand(w + (wLastValue - wTime));
			wLastValue = wTime;
			// Accumulate at least 32 bits of randomness over time
			dwRandomResult <<= 1;
			if (rand() >= 16384)
				dwRandomResult |= 0x1;
			// See if we've collected a fair amount of entropy and can quit early
			if (++vBitCount == 0u)
				break;
		}
		// Restore hardware SFRs
		ADCON0 = ADCON0Save;
		ADCON2 = ADCON2Save;
		TMR0H = TMR0HSave;
		TMR0L = TMR0LSave;
		T0CON = T0CONSave;
	}
#else
	{
		WORD AD1CON1Save, AD1CON2Save, AD1CON3Save;
		WORD T1CONSave, PR1Save;
		// Save hardware SFRs
		AD1CON1Save = AD1CON1;
		AD1CON2Save = AD1CON2;
		AD1CON3Save = AD1CON3;
		T1CONSave = T1CON;
		PR1Save = PR1;
		// Set up Timer and A/D converter module
		AD1CON1 = 0x80E4;		// Turn on the A/D module, auto-convert
		AD1CON2 = 0x003F;		// Interrupt after every 16th sample/convert
		AD1CON3 = 0x9F00;		// Frc A/D clock, 31 Tad acquisition
		T1CON = 0x8000;			// TON = 1, no prescalar
		PR1 = 0xFFFF;			// Don't clear timer early
		vBitCount = 0;
		dwTotalTime = 0;
		while (1) {
			ClrWdt();
#if defined(__C30__)
			while (!IFS0bits.AD1IF);
#else
			while (!IFS1bits.AD1IF);
#endif
			wTime = TMR1;
			TMR1 = 0x0000;

#if defined(__C30__)
			IFS0bits.AD1IF = 0;
#else
			IFS1bits.AD1IF = 0;
#endif
			w = rand();
			// Wait no longer than 1 second obtaining entropy
			dwTotalTime += wTime;
			if (dwTotalTime >= (DWORD) CLOCK_FREQ / 2ul) {
				dwRandomResult ^=
					rand() | (((DWORD) rand()) << 15) | (((DWORD) rand())
														 << 30);
				break;
			}
			// Keep sampling if minimal entropy was likely obtained this round
			if (wLastValue == wTime)
				continue;
			// Add this entropy into the pseudo random number generator by reseeding
			srand(w + (wLastValue - wTime));
			wLastValue = wTime;
			// Accumulate at least 32 bits of randomness over time
			dwRandomResult <<= 1;
			if (rand() >= 16384)
				dwRandomResult |= 0x1;
			// See if we've collected a fair amount of entropy and can quit early
			if (++vBitCount == 0u)
				break;
		}
		// Restore hardware SFRs
		AD1CON1 = AD1CON1Save;
		AD1CON2 = AD1CON2Save;
		AD1CON3 = AD1CON3Save;
		T1CON = T1CONSave;
		PR1 = PR1Save;
	}
#endif
*/
	return dwRandomResult;
}

#if defined(STACK_USE_BASE64_DECODE)
/*********************************************************************
 * Function:        WORD Base64Decode(unsigned char *cSourceData, WORD wSourceLen, unsigned char *cDestData, WORD wDestLen)
 * PreCondition:    None
 * Input:           *cSourceData: Pointer to a string of Base64 encoded data
 *					wSourceLen: Length of the Base64 data at cSourceData
 * 		            *cDestData: Pointer to write the decoded data
 *					wSourceLen: Maximum length that can be written to cDestData.  Decoded data is always at least 1/4 smaller than Base64 encoded data.
 * Output:          WORD: Number of decoded bytes written to cDestData
 * Side Effects:    None
 * Overview:        Performs a Base64 Decode on an ASCII string
 * Note:            This implementation is binary-safe, and will 
 *					ignore invalid characters (CR, LF, etc).  If 
 *					cSourceData is equal to cDestData, the data will 
 *					be converted in-place.  If cSourceData is not 
 *					equal to cDestData, but the regions overlap, the 
 *					behavior is undefined.
 ********************************************************************/
WORD Base64Decode(unsigned char *cSourceData, WORD wSourceLen,
				  unsigned char *cDestData, WORD wDestLen)
{
	unsigned char i;
	unsigned char vByteNumber;
	BOOL bPad;
	WORD wBytesOutput;
	vByteNumber = 0;
	wBytesOutput = 0;
	// Loop over all provided bytes
	while (wSourceLen--)
	{
		// Fetch a Base64 byte and decode it to the original 6 bits
		i = *cSourceData++;
		bPad = (i == '=');
		if (i >= 'A' && i <= 'Z')	// Regular data
			i -= 'A' - 0;
		else if (i >= 'a' && i <= 'z')
			i -= 'a' - 26;
		else if (i >= '0' && i <= '9')
			i -= '0' - 52;
		else if (i == '+' || i == '-')
			i = 62;
		else if (i == '/' || i == '_')
			i = 63;
		else					// Skip all padding (=) and non-Base64 characters
			continue;
		// Write the 6 bits to the correct destination location(s)
		if (vByteNumber == 0u) {
			if (bPad)			// Padding here would be illegal, treat it as a non-Base64 chacter and just skip over it
				continue;
			vByteNumber++;
			if (wBytesOutput >= wDestLen)
				break;
			wBytesOutput++;
			*cDestData = i << 2;
		} else if (vByteNumber == 1u) {
			vByteNumber++;
			*cDestData++ |= i >> 4;
			if (wBytesOutput >= wDestLen)
				break;
			if (bPad)
				continue;
			wBytesOutput++;
			*cDestData = i << 4;
		} else if (vByteNumber == 2u) {
			vByteNumber++;
			*cDestData++ |= i >> 2;
			if (wBytesOutput >= wDestLen)
				break;
			if (bPad)
				continue;
			wBytesOutput++;
			*cDestData = i << 6;
		} else if (vByteNumber == 3u) {
			vByteNumber = 0;
			*cDestData++ |= i;
		}
	}

	return wBytesOutput;
}
#endif							// #if defined(STACK_USE_BASE64_DECODE)


#if defined(STACK_USE_BASE64_ENCODE) || defined(STACK_USE_SMTP_CLIENT)
/*********************************************************************
 * Function:        WORD Base64Encode(unsigned char *cSourceData, WORD wSourceLen, unsigned char *cDestData, WORD wDestLen)
 * PreCondition:    None
 * Input:           *cSourceData: Pointer to a string of data to encode
 *					wSourceLen: Length of the data at cSourceData
 * 		            *cDestData: Pointer where to write the Base64 encoded data
 *					wSourceLen: Maximum length that can be written to cDestData.  Base64 encoded data is always at least 25% bigger than the original data.
 * Output:          WORD: Number of bytes written to cDestData.  This 
 *						  will always be a multiple of 4
 * Side Effects:    None
 * Overview:        Performs a Base64 encode on a binary block of data
 * Note:            If cSourceData overlaps with cDestData, the behavior is undefined.
 ********************************************************************/
WORD Base64Encode(unsigned char *cSourceData, WORD wSourceLen,
				  unsigned char *cDestData, WORD wDestLen)
{
	unsigned char i, j;
	unsigned char vOutput[4];
	WORD wOutputLen;
	wOutputLen = 0;
	while (wDestLen >= 4)
	{
		// Start out treating the output as all padding
		vOutput[0] = 0xFF;
		vOutput[1] = 0xFF;
		vOutput[2] = 0xFF;
		vOutput[3] = 0xFF;
		// Get 3 input octets and split them into 4 output hextets (6-bits each) 
		if (wSourceLen == 0)
			break;
		i = *cSourceData++;
		wSourceLen--;
		vOutput[0] = (i & 0xFC) >> 2;
		vOutput[1] = (i & 0x03) << 4;
		if (wSourceLen) {
			i = *cSourceData++;
			wSourceLen--;
			vOutput[1] |= (i & 0xF0) >> 4;
			vOutput[2] = (i & 0x0F) << 2;
			if (wSourceLen) {
				i = *cSourceData++;
				wSourceLen--;
				vOutput[2] |= (i & 0xC0) >> 6;
				vOutput[3] = i & 0x3F;
			}
		}
		// Convert hextets into Base 64 alphabet and store result
		for (i = 0; i < 4; i++) {
			j = vOutput[i];
			if (j <= 25)
				j += 'A' - 0;
			else if (j <= 51)
				j += 'a' - 26;
			else if (j <= 61)
				j += '0' - 52;
			else if (j == 62)
				j = '+';
			else if (j == 63)
				j = '/';
			else				// Padding
				j = '=';
			*cDestData++ = j;
		}
		// Update counters
		wDestLen -= 4;
		wOutputLen += 4;
	}

	return wOutputLen;
}
#endif							// #if defined(STACK_USE_BASE64_ENCODE) || defined(STACK_USE_SMTP)
/*********************************************************************
 * Function:		void uitoa(WORD Value, unsigned char *Buffer)
 * PreCondition:	None
 * Input:			Value: Unsigned 16-bit integer to be converted
 *					Buffer: Pointer to a location to write the string
 * Output:			*Buffer: Receives the resulting string
 * Side Effects:	None
 * Overview:		The function converts an unsigned integer (16 bits) 
 *					into a null terminated decimal string.
 * Note:			None
 ********************************************************************/
void uitoa(WORD Value, unsigned char *Buffer)
{
	volatile unsigned char i;
	WORD Digit;
	WORD Divisor;
	BOOL Printed = FALSE;
	if (Value) {
		for (i = 0, Divisor = 10000; i < 5; i++)
		{
			Digit = Value / Divisor;
			if (Digit || Printed)
			{
				*Buffer++ = '0' + Digit;
				Value -= Digit * Divisor;
				Printed = TRUE;
			}
			Divisor /= 10;
		}
	}
	else
		*Buffer++ = '0';
	*Buffer = '\0';
}
/*********************************************************************
 * Function:		void ultoa(DWORD Value, unsigned char *Buffer)
 * PreCondition:	None
 * Input:			Value: Unsigned 32-bit integer to be converted
 *					Buffer: Pointer to a location to write the string
 * Output:			*Buffer: Receives the resulting string
 * Side Effects:	None
 * Overview:		The function converts an unsigned integer (32 bits) 
 *					into a null terminated decimal string.
 * Note:			None
 ********************************************************************/
#if !defined(__18CXX) || defined(HI_TECH_C)
void ultoa(DWORD Value, unsigned char *Buffer)
{
	volatile unsigned char i;
	DWORD Digit;
	DWORD Divisor;
	BOOL Printed = FALSE;
	if (Value) {
		for (i = 0, Divisor = 1000000000; i < 10; i++)
		{
			Digit = Value / Divisor;
			if (Digit || Printed)
			{
				*Buffer++ = '0' + Digit;
				Value -= Digit * Divisor;
				Printed = TRUE;
			}
			Divisor /= 10;
		}
	}
	else
		*Buffer++ = '0';
	*Buffer = '\0';
}
#endif
/*********************************************************************
 * Function:        unsigned char btohexa_high(unsigned char b)
 * PreCondition:    None
 * Input:           One byte ranged 0x00-0xFF
 * Output:          An ascii byte (always uppercase) between '0'-'9' 
 *					or 'A'-'F' that corresponds to the upper 4 bits of
 *					the input byte.
 *					ex: b = 0xAE, btohexa_high() returns 'A'
 * Side Effects:    None
 * Overview:        None
 * Note:			None
 ********************************************************************/
unsigned char btohexa_high(unsigned char b)
{
	b >>= 4;
	return (b > 0x9u) ? b + 'A' - 10 : b + '0';
}
/*********************************************************************
 * Function:        unsigned char btohexa_low(unsigned char b)
 * PreCondition:    None
 * Input:           One byte ranged 0x00-0xFF
 * Output:          An ascii byte (always uppercase) between '0'-'9' 
 *					or 'A'-'F' that corresponds to the lower 4 bits of
 *					the input byte.
 *					ex: b = 0xAE, btohexa_low() returns 'E'
 * Side Effects:    None
 * Overview:        None
 * Note:			None
 ********************************************************************/
unsigned char btohexa_low(unsigned char b)
{
	b &= 0x0F;
	return (b > 9u) ? b + 'A' - 10 : b + '0';
}
/*********************************************************************
 * Function:        signed char stricmppgm2ram(unsigned char* a, const unsigned char* b)
 * PreCondition:    None
 * Input:           a: pointer to a string
 *					b: pointer to a string stored in program memory
 * Output:          0 if a = b
 *					1 if a > b
 *					-1 if a < b
 * Side Effects:    None
 * Overview:        Performs a case-insensitive comparison of two 
 *					strings.
 * Note:			None
 ********************************************************************/
signed char stricmppgm2ram(unsigned char *a, const unsigned char *b)
{
	unsigned char cA, cB;

	// Load first two characters
	cA = *a;
	cB = *b;
	// Loop until one string terminates
	while (cA != '\0' && cB != '\0')
	{
		// Shift case if necessary
		if (cA >= 'a' && cA <= 'z')
			cA -= 'a' - 'A';
		if (cB >= 'a' && cB <= 'z')
			cB -= 'a' - 'A';
		// Compare
		if (cA > cB)
			return 1;
		if (cA < cB)
			return -1;
		// Characters matched, so continue
		a++;
		b++;
		cA = *a;
		cB = *b;
	}
	// See if one string terminated first
	if (cA > cB)
		return 1;
	if (cA < cB)
		return -1;
	// Strings match
	return 0;
}
WORD swaps(WORD v)
{
	WORD_VAL t;
	unsigned char b;
	t.Val = v;
	b = t.v[1];
	t.v[1] = t.v[0];
	t.v[0] = b;
	return t.Val;
}
DWORD swapl(DWORD v)
{
	// Swap bytes 0 and 3
	((DWORD_VAL *) & v)->v[0] ^= ((DWORD_VAL *) & v)->v[3];
	((DWORD_VAL *) & v)->v[3] ^= ((DWORD_VAL *) & v)->v[0];
	((DWORD_VAL *) & v)->v[0] ^= ((DWORD_VAL *) & v)->v[3];
	// Swap bytes 1 and 2
	((DWORD_VAL *) & v)->v[1] ^= ((DWORD_VAL *) & v)->v[2];
	((DWORD_VAL *) & v)->v[2] ^= ((DWORD_VAL *) & v)->v[1];
	((DWORD_VAL *) & v)->v[1] ^= ((DWORD_VAL *) & v)->v[2];
	return v;
}
/*********************************************************************
 * Function:        WORD CalcIPChecksum(unsigned char* buffer, WORD count)
 * PreCondition:    None
 * Input:           buffer: pointer to an array of bytes to checkusm
 *							NOTE: pointer MUST be WORD aligned (even 
 *							memory address) on 16-bit and 32-bit PICs.  
 *							If it is not algined, a memory alignment 
 *							exception will occur.
 *					count: number of bytes to add to the checksum
 * Output:          16-bit one's complement of one's complement sum 
 *					of all words in the data (with zero padding if 
 *					an odd number of bytes are summed).  This checksum 
 *					is defined in RFC 793, among others.
 * Side Effects:    None
 * Overview:        Adds 16-bits at a time to the checksum (0x0000 
 *					starting value).  
 * Note:			Algorithm could be optimized to do 32-bit sums on 
 *					PIC32.
 ********************************************************************/
WORD CalcIPChecksum(unsigned char *buffer, WORD count)
{
	WORD i;
	WORD *val;
	DWORD_VAL sum={ 0x00000000ul };
	i=count>>1;
	val = (WORD *) buffer;
	// Calculate the sum of all words
	while(i--)
		sum.Val += (DWORD) * val++;
	// Add in the sum of the remaining byte, if present
	if (((WORD_VAL *) & count)->bits.b0)
		sum.Val += (DWORD) * (unsigned char *) val;
	// Do an end-around carry (one's complement arrithmatic)
	sum.Val = (DWORD) sum.w[0] + (DWORD) sum.w[1];
	// Do another end-around carry in case if the prior add 
	// caused a carry out
	sum.w[0] += sum.w[1];
	// Return the resulting checksum
	return ~sum.w[0];
}

/*********************************************************************
 * Function:        WORD CalcIPBufferChecksum(WORD len)
 * PreCondition:    TCPInit() is already called     AND
 *                  MAC buffer pointer set to starting of buffer
 * Input:           len     - Total number of bytes to calculate
 *                          checksum for.
 * Output:          16-bit checksum as defined by rfc 793.
 * Side Effects:    None
 * Overview:        This function performs checksum calculation in
 *                  MAC buffer itself.
 * Note:            None
 ********************************************************************/
#if defined(NON_MCHP_MAC)
WORD CalcIPBufferChecksum(WORD len)
{
	DWORD_VAL Checksum = { 0x00000000ul };
	WORD ChunkLen;
	unsigned char DataBuffer[20];	// Must be an even size
	WORD *DataPtr;
	desbordador=0;
	while(len)
	{
		if(desbordador++>65000)
		{
			desbordador=0;
			return 0;
		}
		// Obtain a chunk of data (less SPI overhead compared 
		// to requesting one byte at a time)
		ChunkLen = len > sizeof(DataBuffer) ? sizeof(DataBuffer) : len;
		MACGetArray(DataBuffer, ChunkLen);
		len -= ChunkLen;
		// Take care of a last odd numbered data byte
		if (((WORD_VAL *) & ChunkLen)->bits.b0)
		{
			DataBuffer[ChunkLen] = 0x00;
			ChunkLen++;
		}
		// Calculate the checksum over this chunk
		DataPtr = (WORD *) & DataBuffer[0];
		while (ChunkLen)
		{
			Checksum.Val += *DataPtr++;
			ChunkLen -= 2;
		}
	}
	// Do an end-around carry (one's complement arrithmatic)
	Checksum.Val = (DWORD) Checksum.w[0] + (DWORD) Checksum.w[1];
	// Do another end-around carry in case if the prior add 
	// caused a carry out
	Checksum.w[0] += Checksum.w[1];
	// Return the resulting checksum
	return ~Checksum.w[0];
}
#endif

/*********************************************************************
 * Function:		char *strupr(char *s)
 * PreCondition:	None
 * Input:		s: Pointer to a null terminated string to convert.
 * Output:		char* return: Pointer to the initial string
 *				*s: String is updated in to have all upper case 
 *					characters
 * Side Effects:	None
 * Overview:		The function sequentially converts all lower case 
 *				characters in the input s string to upper case 
 *				characters.  Non a-z characters are skipped.
 * Note:			None
 ********************************************************************/
#if !defined(__18CXX) || defined(HI_TECH_C)
char *strupr(char *s)
{
	char c;
	char *t;
	t = s;
	while ((c = *t))
	{
		if (c >= 'a' && c <= 'z')
			*t -= ('a' - 'A');
		t++;
	}
	return s;
}
#endif
/*********************************************************************
 * Function:		DWORD leftRotateDWORD(DWORD val, unsigned char bits)
 * PreCondition:	None
 * Input:			val - the value to rotate
 *					bits - the number of bits to rotate by
 * Output:			DWORD the rotated value
 * Side Effects:	None
 * Overview:		Efficiently performs a bit-wise left rotate of val
 * Note:			None
 ********************************************************************/
DWORD_VAL toRotate;				// Hi-Tech PICC18 cannot access local function variables from inline asm
DWORD leftRotateDWORD(DWORD val, unsigned char bits)
{
	unsigned char i, t;
	//DWORD_VAL toRotate;
	toRotate.Val = val;
	for (i = bits; i >= 8; i -= 8) {
		t = toRotate.v[3];
		toRotate.v[3] = toRotate.v[2];
		toRotate.v[2] = toRotate.v[1];
		toRotate.v[1] = toRotate.v[0];
		toRotate.v[0] = t;
	}
	for (; i != 0; i--)
	{
		asm("movlb (_toRotate)>>8");
		asm("bcf _STATUS,0,C");
		asm("btfsc (_toRotate)&0ffh+3,7,B");
		asm("bsf _STATUS,0,C");
		asm("rlcf (_toRotate)&0ffh+0,F,B");
		asm("rlcf (_toRotate)&0ffh+1,F,B");
		asm("rlcf (_toRotate)&0ffh+2,F,B");
		asm("rlcf (_toRotate)&0ffh+3,F,B");
	}
	return toRotate.Val;
}
