/*********************************************************************
 *
 *	Big Integer Library
 *  Library for Microchip TCP/IP Stack
 *	 - Provides support for integers larger than 32 bits
 *
 *********************************************************************
 * FileName:        BigInt.c
 * Dependencies:    none
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/
#define __BIGINT_C

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_BIGINT)

#define mLSB(a) (a->ptrLSB)
#define mMSB(a) (BigIntMSB(a))
#define mLSBR(a) (a->ptrLSB)
#define mMSBR(a) (a->ptrMSB)

/*********************************************************************
 * Function:        void BigInt(BIGINT* theInt, unsigned char *data, int size)
 *
 * PreCondition:    None
 *
 * Input:           *theInt: the integer to create
 *					*data: a pointer to the data
 *					size: the number of bytes in the integer
 *
 * Output:          The BIGINT is ready to use
 *
 * Side Effects:    None
 *
 * Overview:        Call BigInt() to correctly set up the pointers.
 *
 * Note:            None
 ********************************************************************/
void BigInt(BIGINT * theInt, BIGINT_DATA_TYPE * data, int size)
{
	theInt->data = data;
	theInt->size = size;
	theInt->ptrLSB = data + size - 1;
	theInt->MSBvalid = 0;
}

void BigIntROM(BIGINT_ROM * theInt, const BIGINT_DATA_TYPE * data, int size)
{
	theInt->data = data;
	theInt->size = size;
	theInt->ptrLSB = data + size - 1;
	theInt->ptrMSB = data;					//find the MSB, which can never change
	while (*theInt->ptrMSB == 0)
		theInt->ptrMSB++;
}

/*********************************************************************
 * Function:        void BigIntZero(BIGINT* theInt)
 *
 * PreCondition:    None
 *
 * Input:           *theInt: the integer to clear
 *
 * Output:          theInt = 0
 *
 * Side Effects:    None
 *
 * Overview:        Call BigIntZero() zero all data bytes in the BigInt
 *
 * Note:            None
 ********************************************************************/
void BigIntZero(BIGINT * theInt)
{
	_iA = mLSB(theInt);
	_xA = theInt->data;
	_zeroBI();
	theInt->ptrMSB = theInt->ptrLSB;				//set up the new MSB pointer
	theInt->MSBvalid = 1;
}

/*********************************************************************
 * Function:        void BigIntMod(BIGINT *n, const BIGINT* m)
 *
 * PreCondition:    None
 *
 * Input:           *n: a pointer to the number
 *					*m: a pointer to the modulus
 *					
 * Output:          *n contains the modded number
 *
 * Side Effects:    None
 *
 * Overview:        Call RSABigMod() to calculate the modulus of two
 *					really big numbers.
 *
 * Note:            Supports at least 2048 bits
 ********************************************************************/
void BigIntMod(BIGINT * n, BIGINT * m)
{
	//declarations
	BIGINT_DATA_TYPE *iN, MSBm, offset;
	BIGINT_DATA_TYPE_2 qHatInt, topTwoWords;
	BIGINT_DATA_TYPE oldMSB;
	WORD wDebug;
	MSBm = *mMSB(m);
	iN = mMSB(n);
	while ((wDebug = BigIntMagnitudeDifference(n, m)) > 0)
	{
		//find qHat = MSBn:MSBn-1/MSBm
		((BIGINT_DATA_TYPE *) & topTwoWords)[1] = *iN;
		((BIGINT_DATA_TYPE *) & topTwoWords)[0] = *(iN + 1);
		qHatInt = topTwoWords / MSBm;
		if (qHatInt > BIGINT_DATA_MAX)
			qHatInt = BIGINT_DATA_MAX;
#if BIGINT_DEBUG
		putrsUART("\r\n\r\n    n = ");
		BigIntPrint(n);
		putrsUART("\r\n    m = ");
		BigIntPrint(m);
		putrsUART("\r\n    qHat (");
		putulhexUART(qHatInt);
		putrsUART(") = topTwo(");
		putulhexUART(topTwoWords);
		putrsUART(") / (");
		putulhexUART(MSBm);
		putrsUART(") ");
#endif
		//Once qHat is determined, we multiply M by qHat, shift it up
		//as many bytes as possible, and subtract the result.
		//An alternative to this is to multiply and subtract in the same step
		//rather than multiply into an accumulator and subtract the
		//accumulated value.  The _mas function does this, and saves 
		//memory and clock cycles in doing so.
		//first, shift the LSB pointer (which has the effect of shifting
		//bytes left without the overhead of moving data in memory)
		offset = BigIntMagnitudeDifference(n, m) - 1;
		n->ptrLSB -= offset;
		oldMSB = *mMSB(n);				//save the old MSB and set up the pointers
		_xA = mMSB(m);
		_iA = mLSB(m);
		_iR = mLSB(n);
		_B = (BIGINT_DATA_TYPE) qHatInt;
		//Do the multiply and subtract
		//Occassionally this results in underflow...this is solved below.
		_masBI();
		//qHat may have been 1 or 2 greater than possible.  If so,
		//the new MSB will be greater than the old one, so we *add*
		//M back to N in the shifted position until overflow occurs
		//and this case corrects itself.
		while (oldMSB < *mMSB(n))
		{
			_xA = mMSB(n);
			_xB = mMSB(m);
			_iA = mLSB(n);
			_iB = mLSB(m);
			_addBI();
		}
		n->ptrLSB += offset;				//now reset the LSB pointer and invalidate the MSB
		//move our MSB pointer while possible
		while (*iN == 0u)
		{
			n->ptrMSB++;
			iN++;
		}
	}
	//clean up last few multiples
	while (BigIntCompare(n, m) >= 0)
	{
		BigIntSubtract(n, m);
	}

}

void BigIntModROM(BIGINT * n, BIGINT_ROM * m)
{
	//declarations
	BIGINT_DATA_TYPE *iN, MSBm, *LSBn;
	BIGINT_DATA_TYPE_2 qHatInt, topTwoWords;
	BIGINT_DATA_TYPE oldMSB;
	MSBm = *mMSBR(m);
	iN = mMSB(n);
	//set up assembly pointers in m
	_xBr = mMSBR(m);
	_iBr = mLSBR(m);
	while (BigIntMagnitudeDifferenceROM(n, m) > 0)
	{
		//find qHat = MSBn:MSBn-1/MSBm
		((BIGINT_DATA_TYPE *) & topTwoWords)[1] = *iN;
		((BIGINT_DATA_TYPE *) & topTwoWords)[0] = *(iN + 1);
		qHatInt = topTwoWords / MSBm;
		if (qHatInt > BIGINT_DATA_MAX)
			qHatInt = BIGINT_DATA_MAX;
#if BIGINT_DEBUG
		putrsUART("\r\n\r\n    n = ");
		BigIntPrint(n);
		putrsUART("\r\n    m = ");
		BigIntPrintROM(m);
		putrsUART("\r\n    qHat (");
		putulhexUART(qHatInt);
		putrsUART(") = topTwo(");
		putulhexUART(topTwoWords);
		putrsUART(") / (");
		putulhexUART(MSBm);
		putrsUART(") ");
#endif
		//Once qHat is determined, we multiply M by qHat, shift it up
		//as many bytes as possible, and subtract the result.
		//An alternative to this is to multiply and subtract in the same step
		//rather than multiply into an accumulator and subtract the
		//accumulated value.  The _mas function does this, and saves 
		//memory and clock cycles in doing so.
		//first, shift the LSB pointer (which has the effect of shifting
		//bytes left without the overhead of moving data in memory)
		LSBn = mLSB(n) - (BigIntMagnitudeDifferenceROM(n, m) - 1);
		//n->ptrLSB -= offset;
		//save the old MSB and set up the pointers
		oldMSB = *mMSB(n);
		_iR = LSBn;
		_B = (BIGINT_DATA_TYPE) qHatInt;
		//Do the multiply and subtract
		//Occassionally this results in underflow...this is solved below.
		_masBIROM();
		//qHat may have been 1 or 2 greater than possible.  If so,
		//the new MSB will be greater than the old one, so we *add*
		//M back to N in the shifted position until overflow occurs
		//and this case corrects itself.
		while (oldMSB < *mMSB(n)) {
			_xA = mMSB(n);
			_iA = LSBn;
			_addBIROM();
		}
		//now reset the LSB pointer
		//n->ptrLSB += offset;
		//move our MSB pointer while possible
		while (*iN == 0x00) {
			n->ptrMSB++;
			iN++;
		}
	}
	//clean up last few multiples
	while (BigIntCompareROM(n, m) >= 0) {
		_iA = mLSB(n);
		_xA = n->data;
		_subBIROM();
		//invalidate MSB pointer
		n->MSBvalid = 0;
	}
}

/*********************************************************************
 * Function:        int BigIntCompare(const BIGINT *a, const BIGINT *b)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to the first number
 *					*b: a pointer to the second number
 *
 * Output:          0 if *a == *b
 *					x if a > b
 *					-x if a < b
 *					Where x represents the magnitude of difference in zero-bytes
 *
 * Side Effects:    None
 *
 * Overview:        Determines if a > b, a < b, or a == b and by what degree
 *
 * Note:            Supports at least 2048 bits.
 *					The magnitude of difference is returned as the number of bytes before 
 *					value appears in the second number while the first has a value.  This 
 *					is useful in the mod operation, because we easily can shift the mod by
 *					[magnitude] bytes before subtracting and thus greatly reduce the 
 *					iterations required.
 ********************************************************************/
int BigIntCompare(BIGINT * a, BIGINT * b)
{
	int temp;
	temp = BigIntMagnitude(a);
	temp -= BigIntMagnitude(b);
#if BIGINT_DEBUG_COMPARE
	putrsUART("\r\n    Compared Magnitudes |a|:");
	putulhexUART(BigIntMagnitude(a));
	putrsUART(" |b|:");
	putulhexUART(BigIntMagnitude(b));
	putrsUART(" diff:");
	putulhexUART(temp);
#endif
	if (temp > 0) {
#if BIGINT_DEBUG_COMPARE
		putrsUART(" a > b");
#endif
		return 1;
	} else if (temp < 0) {
#if BIGINT_DEBUG_COMPARE
		putrsUART(" a < b");
#endif
		return -1;
	} else {
#if BIGINT_DEBUG_COMPARE
		putrsUART(" Checking further bytes...");
#endif
		return BigIntCompare_helper(a, b, BigIntMSBi(a), BigIntMSBi(b));
	}
}

int BigIntCompareROM(BIGINT * a, BIGINT_ROM * b)
{
	int temp;
	temp = BigIntMagnitude(a);
	temp -= BigIntMagnitudeROM(b);
#if BIGINT_DEBUG_COMPARE
	putrsUART("\r\n    Compared Magnitudes |a|:");
	putulhexUART(BigIntMagnitude(a));
	putrsUART(" |b|:");
	putulhexUART(BigIntMagnitude(b));
	putrsUART(" diff:");
	putulhexUART(temp);
#endif
	if (temp > 0)
	{
#if BIGINT_DEBUG_COMPARE
		putrsUART(" a > b");
#endif
		return 1;
	}
	else if (temp < 0)
	{
#if BIGINT_DEBUG_COMPARE
		putrsUART(" a < b");
#endif
		return -1;
	}
	else
	{
#if BIGINT_DEBUG_COMPARE
		putrsUART(" Checking further bytes...");
#endif
		return BigIntCompare_helperROM(a, b, BigIntMSBi(a),(mMSBR(b) - b->data));
	}
}

/*********************************************************************
 * Function:        int BigIntCompare_helper(BIGINT *a, BIGINT *b,  int iA, int iB)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to the first number
 *					*b: a pointer to the second number
 *					iA: byte to start at in a
 *					iB: byte to start at in b
 *
 * Output:          0 if *a == *b
 *					1 if a > b
 *					-1 if a < b
 *
 * Side Effects:    None
 *
 * Overview:        Determines if a > b, a < b, or a == b starting from an arbitrary position
 *
 * Note:            Internal to library.  Supports at least 2048 bits.
 *					This performs the actual comparison, with support for
 *					a byte offset shift.  This aids in speeding up the
 *					BigIntMod() operation by knocking out the biggest orders
 *					of magnitude in one operation.
 ********************************************************************/
int BigIntCompare_helper(BIGINT * a, BIGINT * b, int iA, int iB)
{
#if BIGINT_DEBUG_COMPARE
	putrsUART("\r\n        Comparing...");
#endif
	//compare byte by byte as long as they're equal (and there's bytes left
	for (; iA < a->size && iB < b->size; iB++, iA++)
	{
#if BIGINT_DEBUG_COMPARE
		putrsUART(" ");
		puthexUART(a->data[iA]);
		putrsUART("?");
		puthexUART(b->data[iB]);
#endif
		if (a->data[iA] > b->data[iB])				//a > b
		{
#if BIGINT_DEBUG_COMPARE
			putrsUART(" a > b");
#endif
			return 1;
		}
		else if (a->data[iA] < b->data[iB])			//a < b
		{
#if BIGINT_DEBUG_COMPARE
			putrsUART(" a < b");
#endif
			return -1;
		}
	}
#if BIGINT_DEBUG_COMPARE
	putrsUART(" a = b");
#endif
	return 0;
}

int BigIntCompare_helperROM(BIGINT * a, BIGINT_ROM * b, int iA, int iB)
{
#if BIGINT_DEBUG_COMPARE
	putrsUART("\r\n        Comparing...");
#endif
	//compare byte by byte as long as they're equal (and there's bytes left
	for (; iA < a->size && iB < b->size; iB++, iA++) {
#if BIGINT_DEBUG_COMPARE
		putrsUART(" ");
		puthexUART(a->data[iA]);
		putrsUART("?");
		puthexUART(b->data[iB]);
#endif
		if (a->data[iA] > b->data[iB]) {	//a > b
#if BIGINT_DEBUG_COMPARE
			putrsUART(" a > b");
#endif
			return 1;
		} else if (a->data[iA] < b->data[iB]) {	//a < b
#if BIGINT_DEBUG_COMPARE
			putrsUART(" a < b");
#endif
			return -1;
		}
	}
#if BIGINT_DEBUG_COMPARE
	putrsUART(" a = b");
#endif
	return 0;
}

/*********************************************************************
 * Function:        int BigIntMagnitudeDifference(const BIGINT *n)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to the first number
 *					*b: a pointer to the second number
 *					
 * Output:          Returns the magnitude of difference in zero-bytes
 *
 * Side Effects:    None
 *
 * Overview:        Helps to quickly determine a byte shift for operations
 *
 * Note:            Supports at least 2048 bits
 ********************************************************************/
int BigIntMagnitudeDifference(BIGINT * a, BIGINT * b)
{
	return BigIntMagnitude(a) - BigIntMagnitude(b);
}

int BigIntMagnitudeDifferenceROM(BIGINT * a, BIGINT_ROM * b)
{
	return BigIntMagnitude(a) - BigIntMagnitudeROM(b);
}

/*********************************************************************
 * Function:        int BigIntMagnitude(const BIGINT *n)
 *
 * PreCondition:    None
 *
 * Input:           *n: a pointer to the number
 *					
 * Output:          Returns the number of significant bytes in the data
 *
 * Side Effects:    None
 *
 * Overview:        Helps to quickly determine the magnitude of the number
 *
 * Note:            Supports at least 2048 bits
 ********************************************************************/
unsigned int BigIntMagnitude(BIGINT * n)
{
	return (int) n->size - (mMSB(n) - n->data);
}

unsigned int BigIntMagnitudeROM(BIGINT_ROM * n)
{
	return (int) n->size - (mMSBR(n) - n->data);
}

/*********************************************************************
 * Function:        int BigIntMSBi(const BIGINT *n)
 *
 * PreCondition:    None
 *
 * Input:           *n: a pointer to the number
 *					
 * Output:          Returns the index of the most significant byte
 *
 * Side Effects:    None
 *
 * Overview:        Finds the  index of the most significant byte
 *
 * Note:            Supports at least 2048 bits
 ********************************************************************/
int BigIntMSBi(BIGINT * n)
{
	return mMSB(n) - n->data;
}

/*********************************************************************
 * Function:        BIGINT_DATA_TYPE* BigIntMSB(const BIGINT *n)
 *
 * PreCondition:    None
 *
 * Input:           *n: a pointer to the number
 *					
 * Output:          n->ptrMSB points to the MSB of n
 *
 * Side Effects:    None
 *
 * Overview:        Updates the ptrMSB.  Use after an operation in which
 *					the new MSB cannot be estimated.
 *
 * Note:            Supports at least 2048 bits
 ********************************************************************/
BIGINT_DATA_TYPE *BigIntMSB(BIGINT * n)
{
	BIGINT_DATA_TYPE *toRet;
	//if cached value is valid, use it
	if (n->MSBvalid == 1)
		return n->ptrMSB;
	//otherwise find a new MSB and save it
	_iA = n->data;
	_msbBI();
	toRet = _iA;
	if (toRet > n->ptrLSB)
		toRet = n->ptrLSB;
	n->ptrMSB = toRet;
	n->MSBvalid = 1;
	return toRet;
}

/*********************************************************************
 * Function:        void BigIntMultiply(const BIGINT *a, const BIGINT *b, BIGINT *res)
 *
 * PreCondition:    res->size >= a->size + b->size, &res != &[a|b]
 *
 * Input:           *a: a pointer to the first number
 *					*b: a pointer to the second number
 *					*res: a pointer to memory to store the result
 *
 * Output:          *res contains the result of a * b
 *
 * Side Effects:    None
 *
 * Overview:        Call BigIntMultiply() to multiply two really big numbers.
 *
 * Note:            Supports at least 2048 result bits.
 *					This essentially implements elementary school long
 *					multiplication in base 256 (byte).  This is the fastest
 *					algorithm until you pass about 1024 bits.  This is O(n^2).
 *					res CANNOT be A or B.
 ********************************************************************/
void BigIntMultiply(BIGINT * a, BIGINT * b, BIGINT * res)
{
	//clear out the result
	BigIntZero(res);
	//load the start and stop pointers
	_xB = mMSB(b);
	_xA = mMSB(a);
	_iR = mLSB(res);
	_iA = mLSB(a);
	_iB = mLSB(b);
	//perform the multiplication
	_mulBI();
	//invalidate the MSB ptr
	res->MSBvalid = 0;
}

void BigIntMultiplyROM(BIGINT * a, BIGINT_ROM * b, BIGINT * res)
{
	//clear out the result
	BigIntZero(res);
	//load the start and stop pointers
	_xBr = mMSBR(b);
	_xA = mMSB(a);
	_iR = mLSB(res);
	_iA = mLSB(a);
	_iBr = mLSBR(b);
	//perform the multiplication
	_mulBIROM();
	//invalidate the MSB ptr
	res->MSBvalid = 0;
}

/*********************************************************************
 * Function:        void BigIntSquare(const BIGINT *a, BIGINT *res)
 *
 * PreCondition:    res->size >= 2 * a->size, &res != &a
 *
 * Input:           *a: a pointer to the number
 *					*res: a pointer to memory to store the result
 *
 * Output:          *res contains the result of a * a
 *
 * Side Effects:    None
 *
 * Overview:        Call BigIntSquare() to square two really big numbers.
 *
 * Note:            Supports at least 2048 result bits.
 *					Functionally equivalent to BigIntMultiply, except
 *					an optimization is made for the case of square that
 *					allows us to skip ~1/2 the iterations.
 *					res CANNOT be A.
 ********************************************************************/
void BigIntSquare(BIGINT * a, BIGINT * res)
{
	BigIntZero(res);
	_xA = mMSB(a);
	_iR = mLSB(res);
	_iA = mLSB(a);
	_sqrBI();
	//invalidate the MSB ptr
	res->MSBvalid = 0;
}

/*********************************************************************
 * Function:        void BigIntAdd(BIGINT *a, const BIGINT *b)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to the first number
 *					*b: a pointer to the second number
 *
 * Output:          a = a + b
 *
 * Side Effects:    None
 *
 * Overview:        Call BigIntAdd() to add two really big numbers
 *
 * Note:            Supports at least 2048 result bits.
 ********************************************************************/
void BigIntAdd(BIGINT * a, BIGINT * b)
{
	_xB = mMSB(b);
	_iA = mLSB(a);
	_iB = mLSB(b);
	_xA = a->data;
	_addBI();
	//invalidate MSB pointer
	a->MSBvalid = 0;
}

void BigIntAddROM(BIGINT * a, BIGINT_ROM * b)
{
	_xBr = mMSBR(b);
	_iA = mLSB(a);
	_iBr = mLSBR(b);
	_xA = a->data;
	_addBIROM();
	//invalidate MSB pointer
	a->MSBvalid = 0;
}

/*********************************************************************
 * Function:        void BigIntSubtract(BIGINT *a, const BIGINT *b)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to the first number
 *					*b: a pointer to the second number
 *
 * Output:          a = a - b
 *
 * Side Effects:    None
 *
 * Overview:        Call BigIntSubtract() to subtract two really big numbers
 *
 * Note:            Supports at least 2048 result bits.
 ********************************************************************/
void BigIntSubtract(BIGINT * a, BIGINT * b)
{
	_xB = mMSB(b);
	_iA = mLSB(a);
	_iB = mLSB(b);
	_xA = a->data;
	_subBI();
	//invalidate MSB pointer
	a->MSBvalid = 0;
}

void BigIntSubtractROM(BIGINT * a, BIGINT_ROM * b)
{
	_xBr = mMSBR(b);
	_iA = mLSB(a);
	_iBr = mLSBR(b);
	_xA = a->data;
	_subBIROM();
	//invalidate MSB pointer
	a->MSBvalid = 0;
}

/*********************************************************************
 * Function:        void BigIntCopy(BIGINT *a, const BIGINT *b)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to a BIGINT to copy into
 *					*b: a pointer to the data
 *
 * Output:          a = b
 *
 * Side Effects:    None
 *
 * Overview:        Call BigIntCopy() copy one BIGINT to another
 *
 * Note:            Supports at least 2048 bits.  Only data is copied, so
 *					if sizeof(b) > sizeof(a), only the least significant 
 *					sizeof(a) bytes are copied.
 ********************************************************************/
void BigIntCopy(BIGINT * a, BIGINT * b)
{
	_xB = b->data;
	_xA = a->data;
	_iA = mLSB(a);
	_iB = mLSB(b);
	_copyBI();
	//invalidate MSB pointer
	a->MSBvalid = 0;
}

void BigIntCopyROM(BIGINT * a, BIGINT_ROM * b)
{
	BIGINT_DATA_TYPE *pa;
	const BIGINT_DATA_TYPE *pb;
	for (pa = mLSB(a), pb = mLSBR(b); pa >= a->data && pb >= b->data;pa--, pb--)
		*pa = *pb;
	//zero fill remainder
	while (pa >= a->data)
	{
		*pa = 0;
		pa--;
	}
	//invalidate MSB pointer
	a->MSBvalid = 0;
}

/*********************************************************************
 * Function:        unsigned char* BigIntMSBptr(const BIGINT *a)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to the number
 *					
 * Output:          Returns a pointer to the most significant byte
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BIGINT_DATA_TYPE *BigIntMSBptr(BIGINT * a)
{
	return mMSB(a);
}

/*********************************************************************
 * Function:        unsigned char* BigIntLSBptr(const BIGINT *a)
 *
 * PreCondition:    None
 *
 * Input:           *a: a pointer to the number
 *					
 * Output:          Returns a pointer to the least significant byte
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BIGINT_DATA_TYPE *BigIntLSBptr(BIGINT * a)
{
	return mLSB(a);
}

#endif
