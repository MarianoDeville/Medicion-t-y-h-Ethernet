; *********************************************************************
; *
; *	 Big Integer Assembly Helpers
; *  Library for Microchip TCP/IP Stack
; *	  - Accelerates processing for BigInt functions
; *
; *********************************************************************
; * FileName:        BigInt_helper.asm
; * Dependencies:    None
; * Processor:       PIC18
; * Complier:        Microchip C18 v3.13 or higher
; * Company:         Microchip Technology, Inc.
; *
; * Software License Agreement
; *
; * Copyright © 2002-2007 Microchip Technology Inc.  All rights 
; * reserved.
; *
; * Microchip licenses to you the right to use, modify, copy, and 
; * distribute: 
; * (i)  the Software when embedded on a Microchip microcontroller or 
; *      digital signal controller product (“Device”) which is 
; *      integrated into Licensee’s product; or
; * (ii) ONLY the Software driver source files ENC28J60.c and 
; *      ENC28J60.h ported to a non-Microchip device used in 
; *      conjunction with a Microchip ethernet controller for the 
; *      sole purpose of interfacing with the ethernet controller. 
; *
; * You should refer to the license agreement accompanying this 
; * Software for additional information regarding your rights and 
; * obligations.
; *
; * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT 
; * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT 
; * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A 
; * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL 
; * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR 
; * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF 
; * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS 
; * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE 
; * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER 
; * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT 
; * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
; *
; *
; * Author               Date		Comment
; *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
; * Elliott Wood	     6/20/07	Original
; ********************************************************************/
 
BIGINT_VARS		UDATA
_iA			RES	2	;*_iA, starting index for A
	GLOBAL _iA
_iB			RES	2	;*_iB, starting index for B
	GLOBAL _iB
_iBr		RES	3	;*_iBr, starting index for B
	GLOBAL _iBr		;  when B is a ROM pointer
_iR			RES	2	;*_iR, starting index for Res
	GLOBAL _iR
_xA			RES	2	;*_xA, end index for A
	GLOBAL _xA
_xB			RES	2	;*_xB, end index for B
	GLOBAL _xB
_xBr		RES	3	;*_xBr, end index for B
	GLOBAL _xBr		;  when B is a ROM pointer
_B			RES 1	;value of B for _mul and _sqr
	GLOBAL _B
CarryByte	RES 1	;value of carry for _mul and _sqr
CarryH		RES 1	;high value of carry for _sqr

BIGINT_CODE		CODE

#include p18cxxx.inc
#include P18MACRO.INC

;loads a literal value into a memory location
#ifndef mMOVLF
mMOVLF  macro  literal,dest
        movlw  literal
        movwf  dest
        endm
#endif


;***************************************************************************
; Function: 	void _addBI()
;
; PreCondition: _iA and _iB are loaded with the LSB of each BigInt
;				_xA and _xB are loaded with the MSB of each BigInt
;				a.size >= b.magnitude
;
; Input: 		A and B, the BigInts to add
;
; Output: 		none
;
; Side Effects: A = A + B
;
; Stack Req: 	none
;
; Overview: 	Quickly performs the bulk addition of two BigInts
;***************************************************************************

	GLOBAL	_addBI
_addBI:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank
	movff	_iA+0x0,FSR0L		;Put iA in FSR0
	movff	_iA+0x1,FSR0H
	movff	_iB+0x0,FSR2L		;Put iB in FSR2
	movff	_iB+0x1,FSR2H
	bcf		STATUS,C			;clear carry bit

;perform addition of all but the last byte
aLoop: 							;check B end, xor preserves carry bit
	movf	FSR2L,W				;check if low byte of B ptr
	xorwf	_xB+0x0,W			;  equals low byte of stop
	bnz		aDoAdd				;if not, do addition
	movf	FSR2H,W				;check if high byte of B ptr
	xorwf	_xB+0x1,W			;  equals high byte of stop
	bz		aLastBByte			;if so, process last byte
aDoAdd:
	movf	POSTDEC2,W			;load value of B
	addwfc	POSTDEC0,F			;add to value of A
	bra		aLoop				;repeat loop

;handle addition of last byte of B, whether A is done or not
aLastBByte:
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		aNotDone			;add to A, and continue
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bnz		aNotDone			;if A is not at MSB, we're not done
	movf	INDF2,W				;load value of B
	addwfc	INDF0,F				;add to value of A
	bra		aDone				;terminate addition
aNotDone:
	movf	INDF2,W				;load value of B
	addwfc	POSTDEC0,F			;add to value of A

;continue the carrying process as needed
aCarryOut:
	bnc		aDone				;if no carry occurs, we're done
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		aDoCarryOut			;continue carry
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bz		aFinalCarry			;if A is at MSB, we're done
aDoCarryOut:
	movlw	1					;load a 1 for the carry
	addwf	POSTDEC0,F			;add in carry bit
	bra		aCarryOut			;test for no carry or overflow

;process final carry if necessary
aFinalCarry:
	movlw	1
	addwf	INDF0,F

;return
aDone:
	Stk2PopToReg	FSR2L		;restore FSR2 from stack
	return

;***************************************************************************
; Function: 	void _addBIROM()
;
; PreCondition: _iA and _iBr are loaded with the LSB of each BigInt
;				_xA and _xBr are loaded with the MSB of each BigInt
;				a.size >= b.magnitude
;
; Input: 		A and B, the BigInts to add
;
; Output: 		none
;
; Side Effects: A = A + B
;
; Stack Req: 	none
;
; Overview: 	Quickly performs the bulk addition of two BigInts
;***************************************************************************

	GLOBAL	_addBIROM
_addBIROM:
	banksel _xA					;select the assembly pointer bank
	movff	_iA+0x0,FSR0L		;Put iA in FSR0
	movff	_iA+0x1,FSR0H
	movff	_iBr+0x0,TBLPTRL	;Put iB in TBLPTR
	movff	_iBr+0x1,TBLPTRH
	movff	_iBr+0x2,TBLPTRU
	bcf		STATUS,C			;clear carry bit

;perform addition of all but the last byte
;  skip checking of TBLPTRU since no BIGINTS are that large
aRLoop: 						;check B end, xor preserves carry bit
	movf	TBLPTRL,W			;check if low byte of B ptr
	xorwf	_xBr+0x0,W			;  equals low byte of stop
	bnz		aRDoAdd				;if not, do addition
	movf	TBLPTRH,W			;check if high byte of B ptr
	xorwf	_xBr+0x1,W			;  equals high byte of stop
	bz		aRLastBByte			;if so, process last byte
aRDoAdd:
	tblrd	*-					;load value of B
	movf	TABLAT,W
	addwfc	POSTDEC0,F			;add to value of A
	bra		aRLoop				;repeat loop

;handle addition of last byte of B, whether A is done or not
aRLastBByte:
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		aRNotDone			;add to A, and continue
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bnz		aRNotDone			;if A is not at MSB, we're not done
	tblrd	*
	movf	TABLAT,W			;load value of B
	addwfc	INDF0,F				;add to value of A
	bra		aRDone				;terminate addition
aRNotDone:
	tblrd	*
	movf	TABLAT,W			;load value of B
	addwfc	POSTDEC0,F			;add to value of A

;continue the carrying process as needed
aRCarryOut:
	bnc		aRDone				;if no carry occurs, we're done
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		aRDoCarryOut			;continue carry
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bz		aRFinalCarry		;if A is at MSB, we're done
aRDoCarryOut:
	movlw	1					;load a 1 for the carry
	addwf	POSTDEC0,F			;add in carry bit
	bra		aRCarryOut			;test for no carry or overflow

;process final carry if necessary
aRFinalCarry:
	movlw	1
	addwf	INDF0,F

;return
aRDone:
	return

;***************************************************************************
; Function: 	void _subBI()
;
; PreCondition: _iA and _iB are loaded with the LSB of each BigInt
;				_xA and _xB are loaded with the MSB of each BigInt
;
; Input: 		A and B, the BigInts to subtract
;
; Output: 		none
;
; Side Effects: A = A - B
;
; Stack Req: 	none
;
; Overview: 	quickly performs the bulk subtraction of two BigInts
;***************************************************************************

	GLOBAL	_subBI
_subBI:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank
	movff	_iA+0x0,FSR0L		;Put iA in FSR0
	movff	_iA+0x1,FSR0H
	movff	_iB+0x0,FSR2L		;Put iB in FSR2
	movff	_iB+0x1,FSR2H
	bsf		STATUS,C			;clear borrow bit

;perform subtraction of all but the last byte
sLoop: 							;check B end, xor preserves carry bit
	movf	FSR2L,W				;check if low byte of B ptr
	xorwf	_xB+0x0,W			;  equals low byte of stop
	bnz		sDoSub				;if not, do addition
	movf	FSR2H,W				;check if high byte of B ptr
	xorwf	_xB+0x1,W			;  equals high byte of stop
	bz		sLastBByte			;if so, process last byte
sDoSub:
	movf	POSTDEC2,W			;load value of B
	subwfb	POSTDEC0,F			;add to value of A
	bra		sLoop				;repeat loop

;handle subtraction of last byte of B, whether A is done or not
sLastBByte:
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		sNotDone			;add to A, and continue
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bnz		sNotDone			;if A is not at MSB, we're not done
	movf	INDF2,W				;load value of B
	subwfb	INDF0,F				;add to value of A
	bra		sDone				;terminate addition
sNotDone:
	movf	INDF2,W				;load value of B
	subwfb	POSTDEC0,F			;subtract from value of A

;continue the carrying process as needed
sBorrowUp:
	bc		sDone				;if no borrow occurs, we're done
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		sDoBorrowUp			;continue borrow
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bz		sFinalBorrow		;if A is at MSB, we're done
sDoBorrowUp:
	movlw	1					;load a 1 for the borrow
	subwf	POSTDEC0,F			;subtract that borrow bit
	bra		sBorrowUp			;test for no borrow or overflow

;process final borrow if necessary
sFinalBorrow:
	movlw	1
	subwf	INDF0,F

;return
sDone:
	Stk2PopToReg	FSR2L		;restore FSR2 from stack
	return

;***************************************************************************
; Function: 	void _subBIROM()
;
; PreCondition: _iA and _iBr are loaded with the LSB of each BigInt
;				_xA and _xBr are loaded with the MSB of each BigInt
;
; Input: 		A and B, the BigInts to subtract
;
; Output: 		none
;
; Side Effects: A = A - B
;
; Stack Req: 	none
;
; Overview: 	quickly performs the bulk subtraction of two BigInts
;***************************************************************************

	GLOBAL	_subBIROM
_subBIROM:
	banksel _xA					;select the assembly pointer bank
	movff	_iA+0x0,FSR0L		;Put iA in FSR0
	movff	_iA+0x1,FSR0H
	movff	_iBr+0x0,TBLPTRL	;Put iB in TBLPTR
	movff	_iBr+0x1,TBLPTRH
	movff	_iBr+0x2,TBLPTRU
	bsf		STATUS,C			;clear borrow bit

;perform subtraction of all but the last byte
;  skip checking of TBLPTRU since no BIGINTS are that large
sRLoop: 						;check B end, xor preserves carry bit
	movf	TBLPTRL,W			;check if low byte of B ptr
	xorwf	_xBr+0x0,W			;  equals low byte of stop
	bnz		sRDoSub				;if not, do addition
	movf	TBLPTRH,W			;check if high byte of B ptr
	xorwf	_xBr+0x1,W			;  equals high byte of stop
	bz		sRLastBByte			;if so, process last byte
sRDoSub:
	tblrd	*-
	movf	TABLAT,W			;load value of B
	subwfb	POSTDEC0,F			;add to value of A
	bra		sRLoop				;repeat loop

;handle subtraction of last byte of B, whether A is done or not
sRLastBByte:
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		sRNotDone			;add to A, and continue
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bnz		sRNotDone			;if A is not at MSB, we're not done
	tblrd	*
	movf	TABLAT,W			;load value of B
	subwfb	INDF0,F				;add to value of A
	bra		sRDone				;terminate addition
sRNotDone:
	tblrd	*
	movf	TABLAT,W			;load value of B
	subwfb	POSTDEC0,F			;subtract from value of A

;continue the carrying process as needed
sRBorrowUp:
	bc		sRDone				;if no borrow occurs, we're done
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		sRDoBorrowUp			;continue borrow
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bz		sRFinalBorrow		;if A is at MSB, we're done
sRDoBorrowUp:
	movlw	1					;load a 1 for the borrow
	subwf	POSTDEC0,F			;subtract that borrow bit
	bra		sRBorrowUp			;test for no borrow or overflow

;process final borrow if necessary
sRFinalBorrow:
	movlw	1
	subwf	INDF0,F

;return
sRDone:
	return

;***************************************************************************
; Function: 	void _zeroBI()
;
; PreCondition: _iA is loaded with the LSB of the BigInt
;				_xA is loaded with the MSB the BigInt
;
; Input: 		none
;
; Output: 		none
;
; Side Effects: A = 0
;
; Stack Req: 	none
;
; Overview: 	Sets all bytes from _iA to _xA to zero
;***************************************************************************

	GLOBAL	_zeroBI
_zeroBI:
	banksel _xA					;select the assembly pointer bank
	movff	_iA+0x0,FSR0L		;Put iA in FSR0
	movff	_iA+0x1,FSR0H
zLoop:
	clrf	POSTDEC0			;set byte to zero
	movf	FSR0L,W				;check if A is at MSB
	xorwf	_xA+0x0,W			;test low byte first
	bnz		zLoop				;add to A, and continue
	movf	FSR0H,W				;check high byte
	xorwf	_xA+0x1,W			;
	bnz		zLoop				;if A is not at MSB, we're not done
	
;handle the last byte and return
	clrf	INDF0				;clear last byte
	return

;***************************************************************************
; Function: 	void _msbBI()
;
; PreCondition: _iA is loaded with the top byte of the BigInt
;				_xA is loaded with the LSB the BigInt
;
; Input: 		none
;
; Output: 		none
;
; Side Effects: _iA is now the MSB of the BigInt
;
; Stack Req: 	none
;
; Overview: 	Finds the MSB of the BigInt
;***************************************************************************

	GLOBAL	_msbBI
_msbBI:
	banksel _xA					;select the assembly pointer bank
	movff	_iA+0x0,FSR0L		;Put iA in FSR0
	movff	_iA+0x1,FSR0H
	movf	POSTDEC0,W			;subtract one from pointer
msbLoop:
	movf	PREINC0,W			;load the next value
	bz		msbLoop				;if value isn't zero, this is the MSB
;	movf	FSR0L,W				;check if pointer is at LSB
;	xorwf	_xA+0x0,W			;test low byte first
;	bnz		msbLoop				;
;	movf	FSR0H,W				;check high byte
;	xorwf	_xA+0x1,W			;
;	bnz		msbLoop
	
;copy FSR0 back to _iA and return
msbDone:
	movff	FSR0L,_iA+0x0		;move FSR0 back to iA
	movff	FSR0H,_iA+0x1		
	return

;***************************************************************************
; Function: 	void _mulBI()
;
; PreCondition: _iA and _iB are loaded with the LSB of each BigInt
;				_xA and _xB are loaded with the MSB of each BigInt
;				_iR is the LSB of the accumulator BigInt
;				_iR is zeroed, and has enough space
;
; Input: 		A and B, the BigInts to multiply
;
; Output: 		none
;
; Side Effects: R = A * B
;
; Stack Req: 	none
;
; Overview: 	Performs the bulk multiplication of two BigInts
;***************************************************************************

	GLOBAL	_mulBI
_mulBI:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank

	;decrement xA, xB (to match termination case)
	clrf 	WREG				;load zero to W
	decf	_xA,F				;decrement xA
	subwfb	_xA+0x1,F			;borrow if needed
	decf	_xB,F				;decrement xB
	subwfb	_xB+0x1,F

	;set up for loop over all values in B
	Stk2PushFromReg	_iR			;save initial iR to stack
	Stk2PushFromReg	_iB			;save initial iB to stack

mLoopB:
	Stk2PopToReg	FSR0L		;pop next iB from stack
	Stk2PopToReg	FSR2L		;pop next iR from stack

	;check if B is done
	movf	FSR0L,W				;check if low byte of B ptr
	xorwf	_xB+0x0,W			;  equals low byte of stop ptr
	bnz		mNextB				;if not, work on next byte of B
	movf	FSR0H,W				;check if high byte of B ptr
	xorwf	_xB+0x1,W			;  equals high byte of stop ptr
	bz		mDone				;if so, terminate

mNextB:
	;save next value of iR
	movf	POSTDEC2,W			;decrement iR for next value
	Stk2PushFromReg	FSR2L		;save next value of iR to stack
	movf	POSTINC2,W			;restore iR value

	;load B with value of B[iB--]
	;check B==0, if so then we can skip this byte
	movf	POSTDEC0,W			;copy value of B
	Stk2PushFromReg	FSR0L		;save next value of iB to stack
	bz		mLoopB				;if B==0, continue
	movwf	_B					;save value of B
	
	;set up for loop over all values in A
	clrf	CarryByte			;clear carry byte
	movff	_iA+0x0,FSR0L		;load FSR0 with iA
	movff	_iA+0x1,FSR0H

mLoopA:
	movf	CarryByte,W			;load carry byte
	addwf	INDF2,F				;add to accumulator
	clrf	CarryByte			;clear carry byte
	btfsc	STATUS,C			;if a carry occurred
	incf	CarryByte,F			;  save 1 to the carry byte
	movf	_B,W				;load B
	mulwf	POSTDEC0			;calculate B * A[iA--]
	movf	PRODL,W				;load low result byte
	addwf	POSTDEC2,F			;add to accumulator, and move ptr
	movf	PRODH,W				;load high result byte
	addwfc	CarryByte,F			;add to carry byte, along with
								;  any carry from previous addition
	
	;if A isn't complete, keep looping
	movf	FSR0L,W				;check if low byte of A ptr
	xorwf	_xA+0x0,W			;  equals low byte of stop ptr
	bnz		mLoopA				;if not, continue looping
	movf	FSR0H,W				;check if high byte of A ptr
	xorwf	_xA+0x1,W			;  equals high byte of stop ptr
	bnz		mLoopA				;if not, continue looping

	;if A is complete, finish out the carrying
	movff	CarryByte,INDF2		;save carry byte (always adding to zero)
	bra		mLoopB

mDone:
	;movff	FSR2L,_iR+0x0
	;movff	FSR2H,_iR+0x1
	Stk2PopToReg	FSR2L		;restore old FSR2
	return

;***************************************************************************
; Function: 	void _mulBIROM()
;
; PreCondition: _iA and _iBr are loaded with the LSB of each BigInt
;				_xA and _xBr are loaded with the MSB of each BigInt
;				_iR is the LSB of the accumulator BigInt
;				_iR is zeroed, and has enough space
;
; Input: 		A and B, the BigInts to multiply
;
; Output: 		none
;
; Side Effects: R = A * B
;
; Stack Req: 	none
;
; Overview: 	Performs the bulk multiplication of two BigInts
;***************************************************************************

	GLOBAL	_mulBIROM
_mulBIROM:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank

	;decrement xA, xBr (to match termination case)
	clrf 	WREG				;load zero to W
	decf	_xA,F				;decrement xA
	subwfb	_xA+0x1,F			;borrow if needed
	decf	_xBr,F				;decrement xBr
	subwfb	_xBr+0x1,F
	subwfb	_xBr+0x2,F

	;load the TBLPTR
	movff	_iBr+0x0,TBLPTRL
	movff	_iBr+0x1,TBLPTRH
	movff	_iBr+0x2,TBLPTRU

	;set up for loop over all values in B
	Stk2PushFromReg	_iR			;save initial iR to stack

mRLoopB:
	Stk2PopToReg	FSR2L		;pop next iR from stack

	;check if B is done
	movf	TBLPTRL,W			;check if low byte of B ptr
	xorwf	_xBr+0x0,W			;  equals low byte of stop ptr
	bnz		mRNextB				;if not, work on next byte of B
	movf	TBLPTRH,W			;check if high byte of B ptr
	xorwf	_xBr+0x1,W			;  equals high byte of stop ptr
	bnz		mRNextB				;if not, work on next byte of B
	movf	TBLPTRU,W			;check if upper byte of B ptr
	xorwf	_xBr+0x2,W			;  equals high byte of stop ptr
	bz		mRDone				;if so, terminate

mRNextB:
	;save next value of iR
	movf	POSTDEC2,W			;decrement iR for next value
	Stk2PushFromReg	FSR2L		;save next value of iR to stack
	movf	POSTINC2,W			;restore iR value

	;load B with value of B[iB--]
	;check B==0, if so then we can skip this byte
	tblrd	*-
	movf	TABLAT,W			;copy value of B
	bz		mRLoopB				;if B==0, continue
	movwf	_B					;save value of B
	
	;set up for loop over all values in A
	clrf	CarryByte			;clear carry byte
	movff	_iA+0x0,FSR0L		;load FSR0 with iA
	movff	_iA+0x1,FSR0H

mRLoopA:
	movf	CarryByte,W			;load carry byte
	addwf	INDF2,F				;add to accumulator
	clrf	CarryByte			;clear carry byte
	btfsc	STATUS,C			;if a carry occurred
	incf	CarryByte,F			;  save 1 to the carry byte
	movf	_B,W				;load B
	mulwf	POSTDEC0			;calculate B * A[iA--]
	movf	PRODL,W				;load low result byte
	addwf	POSTDEC2,F			;add to accumulator, and move ptr
	movf	PRODH,W				;load high result byte
	addwfc	CarryByte,F			;add to carry byte, along with
								;  any carry from previous addition
	
	;if A isn't complete, keep looping
	movf	FSR0L,W				;check if low byte of A ptr
	xorwf	_xA+0x0,W			;  equals low byte of stop ptr
	bnz		mRLoopA				;if not, continue looping
	movf	FSR0H,W				;check if high byte of A ptr
	xorwf	_xA+0x1,W			;  equals high byte of stop ptr
	bnz		mRLoopA				;if not, continue looping

	;if A is complete, finish out the carrying
	movff	CarryByte,INDF2		;save carry byte (always adding to zero)
	bra		mRLoopB

mRDone:
	Stk2PopToReg	FSR2L		;restore old FSR2
	return

;***************************************************************************
; Function: 	void _sqrBI()
;
; PreCondition: _iA is loaded with the LSB of the BigInt
;				_xA is loaded with the MSB of the BigInt
;				_iR is the LSB of the accumulator BigInt
;				_iR is zeroed, and has enough space
;
; Input: 		A: Source BigInts to square
;				R: Output location
;
; Output: 		R = A * A
;
; Side Effects: None
;
; Overview: 	Performs the bulk multiplication of two BigInts
;***************************************************************************

	GLOBAL	_sqrBI
_sqrBI:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank

	;decrement xA, xB (to match termination case)
	clrf 	WREG				;load zero to W
	decf	_xA,F				;decrement xA
	subwfb	_xA+0x1,F			;borrow if needed

	;set up for outer loop over all values of A
	Stk2PushFromReg	_iR			;save initial iR to stack
	Stk2PushFromReg _iA			;save initial iA to stack

qOuterLoop:
	Stk2PopToReg	FSR0L		;pop next iA from stack
	Stk2PopToReg	FSR2L		;pop next iR from stack

	;check if outer loop is done
	movf	FSR0L,W				;check if low byte of A ptr
	xorwf	_xA+0x0,W			;  equals low byte of stop ptr
	bnz		qNextOut			;if not, work on next byte of A
	movf	FSR0H,W				;check if high byte of A ptr
	xorwf	_xA+0x1,W			;  equals high byte of stop ptr
	bz		qDone				;if so, terminate

qNextOut:
	;save next value of iR
	movf	POSTDEC2,W			;decrement iR twice for next value
	movf	POSTDEC2,W
	Stk2PushFromReg	FSR2L		;save next value of iR to stack
	movf	POSTINC2,W			;restore iR value
	movf	POSTINC2,W

	;save next value of iA
	movf	POSTDEC0,W			;decrement iA for next value
	Stk2PushFromReg	FSR0L		;save next value of iA to stack

	;load B with value of A[iA--]
	;check B==0, if so then we can skip this byte
	movf	PREINC0,W,			;restore iA and copy value
	movwf	_B					;  into temporary byte
	bz		qOuterLoop			;if B==0, continue to next byte
	
	;set up for inner loop over all remaining values in A
	clrf	CarryByte			;clear carry bytes
	clrf	CarryH

	;first result only gets accumulated once
	mulwf	POSTDEC0			;square first byte (W = B above)
	movf	PRODL,W				;load PRODL
	addwf	POSTDEC2,F			;accumulate
	movf	PRODH,W				;load PRODH
	addwfc	CarryByte,F			;save carry byte (with prev carry)

qInnerLoop:
	;if A isn't complete, keep looping
	movf	FSR0L,W				;check if low byte of A ptr
	xorwf	_xA+0x0,W			;  equals low byte of stop ptr
	bnz		qInnerByte		;if not, continue looping
	movf	FSR0H,W				;check if high byte of A ptr
	xorwf	_xA+0x1,W			;  equals high byte of stop ptr
	bz		qInnerDone			;if not, continue looping

qInnerByte:
	;all future bytes get accumulated twice
	movf	CarryByte,W			;load carry byte
	addwf	INDF2,F				;add to accumulator
	movff	CarryH,CarryByte	;move high carry byte down
	clrf	CarryH
	btfsc	STATUS,C			;if a carry occurred
	incf	CarryByte,F			;  add 1 to the carry byte
	movf	_B,W				;load B
	mulwf	POSTDEC0			;calculate B * A[iA--]
	bcf		STATUS,C			;multiply product by 2
	rlcf	PRODL,F				
	rlcf	PRODH,F
	btfsc	STATUS,C			;if a carry occurrs
	incf	CarryH,F			;  save 1 to the CarryH byte
	movf	PRODL,W				;load low result byte
	addwf	POSTDEC2,F			;add to accumulator, and move ptr
	movf	PRODH,W				;load high result byte
	addwfc	CarryByte,F			;add to carry byte, along with
								;  any carry from previous addition
	btfsc	STATUS,C			;if a carry occurrs
	incf	CarryH,F			;  save 1 to the CarryH byte
	bra		qInnerLoop
	

qInnerDone
	;A is complete, finish out the carrying
	movf	CarryByte,W			;accumulate the carry bytes
	addwf	POSTDEC2,F
	movf	CarryH,W
	addwfc	INDF2
	bra		qOuterLoop

qDone:
	;movff	FSR2L,_iR+0x0
	;movff	FSR2H,_iR+0x1
	Stk2PopToReg	FSR2L		;restore old FSR2
	return

;***************************************************************************
; Function: 	void _masBI()
;
; PreCondition: _iA is loaded with the LSB of the modulus BigInt
;				_xA is loaded with the MSB of the modulus BigInt
;				_B is loaded with the 8 bit integer by which to multiply
;				_iR is the starting LSB of the decumulator BigInt
;
; Input: 		A and B, the BigInts to multiply
;
; Output: 		none
;
; Side Effects: R = R - A * B
;
; Stack Req: 	none
;
; Overview: 	Performs a Multiply And Subtract function.  This is used in
;				the modulus calculation to save several steps.  A BigInt (A)
;				is multiplied by a single byte and subtracted rather than
;				accumulated.  By adjusting the LSB of R, the the operation
;				is shifted.  Given qHat may cause R to go negative, in which
;				case 1*A should be added again at the shifted position.
;
; Note:			Decumulator is the opposite of an accumulator,
;				if that wasn't obvious
;
;***************************************************************************

	GLOBAL	_masBI
_masBI:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank

	;decrement xA (to match termination case)
	clrf 	WREG				;load zero to W
	decf	_xA,F				;decrement xA
	subwfb	_xA+0x1,F			;borrow if needed

	;load initial values of iA and iR
	movff	_iA+0x0,FSR0L		;load iA into FSR0
	movff	_iA+0x1,FSR0H
	movff	_iR+0x0,FSR2L		;load iR into FSR2
	movff	_iR+0x1,FSR2H
	
	;set up for loop over all values in A
	clrf	CarryByte			;clear carry byte

masLoop:
	movf	CarryByte,W			;load carry byte
	subwf	INDF2,F				;subtract from decumulator
	clrf	CarryByte			;clear carry byte
	btfss	STATUS,C			;if a borrow occurred
	incf	CarryByte,F			;  save 1 to the carry byte
	movf	_B,W				;load B
	mulwf	POSTDEC0			;calculate B * A[iA--]
	movf	PRODL,W				;load low result byte
	subwf	POSTDEC2,F			;subtract from decumulator, and move ptr
	movf	PRODH,W				;load high result byte
	btg		STATUS,C			;toggle carry bit (borrow = ~carry)
	addwfc	CarryByte,F			;add to carry byte, along with
								;  any borrow from previous subtraction
	
	;if A isn't complete, keep looping
	movf	FSR0L,W				;check if low byte of A ptr
	xorwf	_xA+0x0,W			;  equals low byte of stop ptr
	bnz		masLoop				;if not, continue looping
	movf	FSR0H,W				;check if high byte of A ptr
	xorwf	_xA+0x1,W			;  equals high byte of stop ptr
	bnz		masLoop				;if not, continue looping

	;if A is complete, finish out the borrow and return
	movf	CarryByte,W			;subtract remaining borrow byte
	subwf	INDF2,F
	Stk2PopToReg	FSR2L		;restore old FSR2
	return
	
;***************************************************************************
; Function: 	void _masBIROM()
;
; PreCondition: _iBr is loaded with the LSB of the modulus BigInt
;				_xBr is loaded with the MSB of the modulus BigInt
;				_B is loaded with the 8 bit integer by which to multiply
;				_iR is the starting LSB of the decumulator BigInt
;
; Input: 		R, B, and _B
;
; Output: 		none
;
; Side Effects: R = R - B * _B
;
; Stack Req: 	none
;
; Overview: 	Performs a Multiply And Subtract function.  This is used in
;				the modulus calculation to save several steps.  A BigInt (B)
;				is multiplied by a single byte (_B) and subtracted rather than
;				accumulated.  By adjusting the LSB of R, the the operation
;				is shifted.  Given qHat may cause R to go negative, in which
;				case 1*A should be added again at the shifted position.
;
; Note:			Decumulator is the opposite of an accumulator,
;				if that wasn't obvious.
;				Assuming n = n % m, B is a BIGINT representing m, and _B is
;				a byte from n.  The nomenclature is confusing.
;***************************************************************************

	GLOBAL	_masBIROM
_masBIROM:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank

	;decrement xB (to match termination case)
	clrf 	WREG				;load zero to W
	decf	_xBr,F				;decrement xBr
	subwfb	_xBr+0x1,F			;borrow if needed
	subwfb	_xBr+0x2,F
	
	;load initial values of iB and iR
	movff	_iBr+0x0,TBLPTRL	;load iB into TBLPTR
	movff	_iBr+0x1,TBLPTRH
	movff	_iBr+0x2,TBLPTRU
	movff	_iR+0x0,FSR2L		;load iR into FSR2
	movff	_iR+0x1,FSR2H
	
	;set up for loop over all values in B
	clrf	CarryByte			;clear carry byte

masRLoop:
	movf	CarryByte,W			;load carry byte
	subwf	INDF2,F				;subtract from decumulator
	clrf	CarryByte			;clear carry byte
	btfss	STATUS,C			;if a borrow occurred
	incf	CarryByte,F			;  save 1 to the carry byte
	movf	_B,W				;load _B
	tblrd	*-					;load B[iB--]
	mulwf	TABLAT				;calculate _B * B[iB--]
	movf	PRODL,W				;load low result byte
	subwf	POSTDEC2,F			;subtract from decumulator, and move ptr
	movf	PRODH,W				;load high result byte
	btg		STATUS,C			;toggle carry bit (borrow = ~carry)
	addwfc	CarryByte,F			;add to carry byte, along with
								;  any borrow from previous subtraction
	
	;if B isn't complete, keep looping
	;  save clock cycles by not checking TBLPTRU since BIGINTS
	;  that large will never be used
	movf	TBLPTRL,W			;check if low byte of A ptr
	xorwf	_xBr+0x0,W			;  equals low byte of stop ptr
	bnz		masRLoop			;if not, continue looping
	movf	TBLPTRH,W			;check if high byte of A ptr
	xorwf	_xBr+0x1,W			;  equals high byte of stop ptr
	bnz		masRLoop			;if not, continue looping

	;if B is complete, finish out the borrow and return
	movf	CarryByte,W			;subtract remaining borrow byte
	subwf	INDF2,F
	;restore xBr
	clrf 	WREG				;load zero to W
	incf	_xBr,F				;increment xBr
	addwfc	_xBr+0x1,F			;borrow if needed
	addwfc	_xBr+0x2,F
	Stk2PopToReg	FSR2L		;restore old FSR2
	return

;***************************************************************************
; Function: 	void _copyBI()
;
; PreCondition: _iA and _iB are loaded with the LSB of each BigInt
;				_xA and _xB are loaded with the MSB of each BigInt
;
; Input: 		A: the destination
;				B: the source
;
; Output: 		A = B
;
; Side Effects: None
;
; Stack Req: 	none
;
; Overview: 	Copies a value from one BigInt to another
;***************************************************************************

	GLOBAL	_copyBI
_copyBI:
    Stk2PushFromReg FSR2L		;Save FSR2 on the stack
	banksel _xA					;select the assembly pointer bank

	;decrement xA, xB (to match termination case)
	clrf 	WREG				;load zero to W
	decf	_xA,F				;decrement xA
	subwfb	_xA+0x1,F			;borrow if needed
	decf	_xB,F				;decrement xB
	subwfb	_xB+0x1,F

	movff	_iA+0x0,FSR0L		;load iA into FSR0
	movff	_iA+0x1,FSR0H
	movff	_iB+0x0,FSR2L		;load iB into FSR2
	movff	_iB+0x1,FSR2H
	
cLoop:
	movff	POSTDEC2,POSTDEC0	;copy B to A
	movf	FSR2L,W				;check if low byte of B ptr
	xorwf	_xB+0x0,W			;  equals low byte of stop ptr
	bnz		cChkA				;if not, continue looping
	movf	FSR2H,W				;check if high byte of B ptr
	xorwf	_xB+0x1,W			;  equals high byte of stop ptr
	bz		cZeroLoop			;if so, B is done, so clear rest of A
cChkA:
	movf	FSR0L,W				;check if low byte of A ptr
	xorwf	_xA+0x0,W			;  equals low byte of stop ptr
	bnz		cLoop				;if not, continue
	movf	FSR0H,W				;check if high byte of A ptr
	xorwf	_xA+0x1,W			;  equals high byte of stop ptr
	bnz		cLoop				;if so, terminate

cZeroLoop:
	movf	FSR0L,W				;check if low byte of A ptr
	xorwf	_xA+0x0,W			;  equals low byte of stop ptr
	bnz		cZero				;if not, continue
	movf	FSR0H,W				;check if high byte of A ptr
	xorwf	_xA+0x1,W			;  equals high byte of stop ptr
	bz		cDone				;if so, terminate
cZero:
	clrf	POSTDEC0			;set byte to zero
	bra		cZeroLoop			;continue

cDone:
	Stk2PopToReg	FSR2L		;restore old FSR2
	return

  end
