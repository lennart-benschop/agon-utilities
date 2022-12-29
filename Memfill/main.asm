;
; Title:	Memfill - Main
; Author:	Lennart Benschop
; Created:	29/12/2022

			.ASSUME	ADL = 1			

			INCLUDE	"equs.inc"
			INCLUDE "mos_api.inc"

			SEGMENT CODE
			XREF	ASC_TO_NUMBER		
			XDEF	_main


; This program fills a memory region with the same byte. 
; 
; The main routine
; IXU: argv - pointer to array of parameters
;   C: argc - number of parameters
; Returns:
;  HL: Error code, or 0 if OK
;
_main:			LD	A, C
			CP 	#4
			JR  	Z, main1
			LD 	HL, s_USAGE		; Number of args not 3 or 4, print usage string and exit
			CALL	PRSTR
			LD	HL, 19
			RET
main1:			LD	HL, (IX+3)		; first parameter, address.
			CALL	ASC_TO_NUMBER
			JR	NC, main_badnum
			PUSH 	DE
			LD	HL, (IX+6)		; second parameter, length.
			CALL	ASC_TO_NUMBER
			JR	NC, main_badnum_pop1
			PUSH	DE
			LD	HL, (IX+9)		;
mein2:			CALL	ASC_TO_NUMBER
			JR	NC, main_badnum_pop2
			LD	A, E
			POP	BC
			POP	DE
			LD	HL, 0
			ADD	HL, DE			; Copy DE to HL
			LD	(HL), A
			INC	DE
			DEC	BC
			LDIR				; The classic way to fill a memory regain on Z80		
			LD	HL, 0
			RET
main_badnum_pop2:	POP	HL
main_badnum_pop1:	POP	HL
main_badnum:		LD	HL, s_BADNUM
			CALL	PRSTR
			LD	HL, 19
			RET
			
; Print a zero-terminated string
; Parameters:
;  HL: Address of string (24-bit pointer)
;
PRSTR:			LD	A,(HL)
			OR	A
			RET	Z
			RST.LIL	10h
			INC	HL
			JR	PRSTR

; Text messages
;
s_USAGE:		DB	" Usage: memfill <addr> <size> <byteval>\n\r", 0
s_BADNUM		DB	" Bad number\n\r", 0	
	
; RAM
; 
			DEFINE	LORAM, SPACE = ROM
			SEGMENT LORAM
			