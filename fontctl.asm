;
; Title:	Memfill - Main
; Author:	Lennart Benschop
; Created:	19/04/2024

  			.ASSUME	ADL = 1			
			INCLUDE "mos_api.inc"
			ORG $b0000 ; Is a moslet
	
			MACRO PROGNAME
			ASCIZ "fontctl.bin"
			ENDMACRO
			
  			include "init.inc"
			include "parse.inc"
	

; This program selects a font previously loaded to a buffer or the system
; font. With the show parameter it can show all characters in the font.
; With the clear parameter it clears the font from teh buffer.
; (clearing the system font) will reset the default font.
;
; fontctl sys
; fontctl <num>
; fontctl <num< show
; fontctl <num> clear

; 
; The main routine
; IXU: argv - pointer to array of parameters
;   C: argc - number of parameters
; Returns:
;  HL: Error code, or 0 if OK
;
_main:			LD	A, C
			PUSH    BC
			CP 	#2
			JR  	NC, main1
			LD 	HL, s_USAGE		; Number of args not 3 or 4, print usage string and exit
			CALL	PRSTR
			POP	BC
			LD	HL, 19
			RET
main1:			LD	HL, (IX+3)		; first parameter, bufid.
			LD 	A,  (HL)
			CALL 	UPPRC
			CP 	'S'			; Assume it's sys
			JR	Z,   main2		 
			CALL	ASC_TO_NUMBER
			JR	main3
main2:			LD 	DE, $FFFF		; Use 65535 for sys
main3:			POP 	BC
			LD	A, C
			CP	3
			JR	C,  main4
			LD	HL, (IX+6)		; second parameter, leng
			LD 	A,  (HL)
			CALL  	UPPRC
			CP 	'C'			; Assume it's clear.
			JR 	Z, do_clear
main4:			LD	C, A			; Save second cmdline param.
			LD	A, 23			; Select the font.
			RST.L	10h
			XOR	A
			RST.L	10h
			LD	A,95h
			RST.L   10h
			XOR	A
			RST.L   10h
			LD	A, E
			RST.L   10h
			LD	A, D
			RST.L 	10h
			XOR 	A
			RST.L   10h
			; Font selected.
			LD	A, C
			CP	'S'		; Was the second param 'show'?
			JR	NZ, main_end
			; Show the font.
			LD     E, 0
			LD     C, 8
show_loop:		LD     A, 13
			RST.L  10h
			LD     A, 10
			RST.L  10h			
			LD     B, 32			
@@:			lD     A, E
			CP     127
			CALL   Z, do_escape			
			CP     32
			CALL   C, do_escape
			RST.L  10h
			INC    E
			DJNZ   @B
			DEC    C
			JR     NZ, show_loop
			LD     A, 13
			RST.L  10h
			LD     A, 10
			RST.L  10h
main_end:		; End with no error
			LD 	HL, 0
			RET
do_clear:		; Clear the font.
			LD	A, E
			AND	D
			INC	A
			JR	Z, do_clear_sys ; Use special case for system font
			LD	A, 23
			RST.L	10h
			XOR	A
			RST.L	10h
			LD	A,95h
			RST.L   10h
			LD	A,4
			RST.L   10h
			LD	A, E
			RST.L   10h
			LD	A, D
			RST.L 	10h      ; Clear the font
			LD	A, 23
			RST.L	10h
			XOR	A
			RST.L	10h
			LD	A,A0h
			RST.L   10h
			LD	A, E
			RST.L   10h
			LD	A, D
			RST.L 	10h
			LD	A,2
			RST.L   10h	; Clear the buffer
			JR	main_end
do_clear_sys:		; Clear (rest) system font.
			LD	A, 23
			RST.L	10h
			XOR	A
			RST.L	10h
			LD	A,91h
			RST.L   10h
			JR	main_end

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

do_escape:		PUSH	AF
			LD	A, 27
			RST.L	10h
			POP	AF
			RET

; Text messages
;
s_USAGE:		DB	"Usage: fontctl <bufid>|sys [show|claar]\r\n", 0
	
; RAM
;
