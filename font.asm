;
; Title:	Font - Main
; Author:	Lennart Benschop
; Created:	30/12/2022
;
; Modified 09/04/2024 adapter to ez80asm

			.ASSUME	ADL = 1			

			INCLUDE "mos_api.inc"

			ORG $b0000 ; Is a moslet
	
			MACRO PROGNAME
			ASCIZ "font.bin"
			ENDMACRO
			
  			INCLUDE "init.inc"


; A program to load an 8x8 font from a file into the VDP. Only codes 32..126 and 128..255 will be loaded.
; Optionally show the font just loaded.
;
; The main routine
; IXU: argv - pointer to array of parameters
;   C: argc - number of parameters
; Returns:
;  HL: Error code, or 0 if OK
;
_main:			XOR 	A
			LD	(will_show), A
			LD	A, C
			CP 	#2
			JR  	Z, main1		; Only load font, don't show
			CP	#3
			JR	NZ, main_usage		
			LD	A, 1
			LD	(will_show),A		; Additional parameter present, do not check.
			JR 	main1
main_usage:			
			LD 	HL, s_USAGE		; Number of args != 3, print usage string and exit
			CALL	PRSTR
			LD	HL, 19
			RET
main1:			LD 	HL, (IX+3)		; source file name
			LD	DE, font_buf
			LD	BC, 4096
			MOSCALL mos_load
			OR	A
			JR	Z, main2
			PUSH 	AF
			LD	HL, s_ERROR_SRC		; source file not opened, error
			CALL	PRSTR
			POP	AF
			LD	HL, 0
			LD	L, A			
			RET
main2:			; Font file loaded, now try to get them into VDP. We will also write char 127, but it cannot be shown.
			LD	E, 8	; Assume font has 8 rows, always
			LD	C, 32
			LD 	HL, font_buf + $100 ; Start at space.
main_fontloop:		LD 	A, 23
			RST.LIL 10h		; VDU 23 
			LD 	A, C
			RST.LIL 10h		; followed by character code.
			LD 	B, E
@@:			LD 	A, (HL)
			RST.LIL 10h		; followd by 8/16 bitmap bytes from font.
			INC 	HL
			DJNZ 	@B			
			INC 	C
			LD 	A, C
			AND	A
			JR 	NZ, main_fontloop	; End loop if encremented to 0
			; Font is now loaded into VDP. 
			LD A, (will_show)
			AND A
			JR Z, main_end
			; Show the font in 7 rows of 32 chars each.
			LD C, 32
main_showloop:		LD B, 32
main_showloop2:		LD A, C
			CP 127
			JR Z, @F		; We will skip 127 when showing.
			RST.LIL 10h		; It's at the endo of the row, no need to show something else instead..
@@:			INC C
			DJNZ main_showloop2		; Inner loop, print 32 chars per row			
			LD A, 13
			RST.LIL 10h
			LD A, 10
			RST.LIL 10h		; print newline after row.
			LD A, C
			AND A
			JR NZ, main_showloop			
main_end:		LD 	HL, 0
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

;			
; Text messages
;
s_ERROR_SRC:		DB 	" Cannot load font file\r\n", 0
s_USAGE:		DB	" Usage: font <fontfile> [show]\r\n", 0                                 
c_MORE:			DB  	28 ;String length


; RAM
; 

will_show:		DS 	1
font_buf:		DS 	2048
			
