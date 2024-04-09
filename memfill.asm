;
; Title:	Memfill - Main
; Author:	Lennart Benschop
; Created:	29/12/2022
; Changed:	30/12/2022 handle length = 0 and length = 1
; Changed:      08/04/2924 adapt to ez80asm

  			.ASSUME	ADL = 1			
			INCLUDE "mos_api.inc"
			ORG $b0000 ; Is a moslet
	
			MACRO PROGNAME
			ASCIZ "memfill.bin"
			ENDMACRO
			
  			include "init.inc"
			include "parse.inc"
	

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
			LD 	HL, 1
			AND 	A
			SBC	HL, BC
			JR	Z, main_fill1		; Special case if length=1
			JR	NC, main_end		; No carry only if BC is 1 or less. So it must be 0 now.
			LD	HL, 0
			ADD	HL, DE			; Copy DE to HL
			LD	(HL), A
			INC	DE
			DEC	BC
			LDIR				; The classic way to fill a memory regain on Z80		
main_end:		LD	HL, 0
			RET
main_fill1:		LD	(DE),A			; Fill only one byte
			JR	main_end
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
			RST.L	10h
			INC	HL
			JR	PRSTR

; Text messages
;
s_USAGE:		DB	" Usage: memfill <addr> <size> <byteval>\r\n", 0
s_BADNUM:		DB	" Bad number\r\n", 0	
	
; RAM
; 
			
