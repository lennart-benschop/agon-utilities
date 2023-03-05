;
; Title:	More - Main
; Author:	Lennart Benschop
; Created:	27/12/2022
;

			.ASSUME	ADL = 1			

			INCLUDE	"equs.inc"
			INCLUDE "mos_api.inc"

			SEGMENT CODE
			
			XDEF	_main


; A simple program to view text files. After each page, show --More--
;
; The main routine
; IXU: argv - pointer to array of parameters
;   C: argc - number of parameters
; Returns:
;  HL: Error code, or 0 if OK
;
_main:			LD	A, C
			CP 	#2
			JR  	Z, main1
			LD 	HL, s_USAGE		; Number of args != 3, print usage string and exit
			CALL	PRSTR
			LD	HL, 19
			RET
main1:			LD 	HL, (IX+3)		; source file name
			LD	C,  fa_read
			MOSCALL mos_fopen
			OR	A
			JR	NZ, main2	
			LD	HL, s_ERROR_SRC		; source file not opened, error
			CALL	PRSTR
			LD	HL, 4			
			RET
main2: 			LD 	C, A    		; Store source file handle to C.
			MOSCALL mos_sysvars
			RES 	4, (IX+sysvar_vpd_pflags)		; Clear mode flag
			LD 	HL, c_GETMODE
			CALL	PRCNTSTR  		; Get screen mode parameters.
main_waitmode:		BIT	4, (IX+sysvar_vpd_pflags)			
			JR 	Z, main_waitmode        ; Wait until received.	
			XOR 	A
			LD (prev_cr), A
			LD (current_col), A
			LD (num_lines), A
 			; Load one byte per character.
main_byte_loop		MOSCALL mos_feof
			CP 	1
			JR 	Z, main_load_end
			MOSCALL mos_fgetc		; Fortunately fgetc transparently passes all bytes.
			; mos_fgetc does not set carry flag at EOF in MOS 1.02, bummer, therefore separate mos_feof call in the loop.
			CP	9
			JR 	NZ, main3
			; Character is a TAB, expand into spaces, at least one space printed, until column pos is 8-aligned.
$$:			LD	A, 32
			RST.LIL 10h
			CALL	do_advance
			LD	A,(current_col)
			AND 	07h
			JR 	NZ, $B
			JR	main_not_cr
main3:			CP      10
			JR	NZ, main4
			; Character is LF
			LD	A, (prev_cr)
			AND 	A
			JR	NZ, main_not_cr		; Skip LF when immediately following a CR
			CALL	do_newline
			JR	main_not_cr
main4:			CP	13
			JR 	NZ, main5
			; Character is CR
			CALL	do_newline
			LD 	A, 1			; Mark last char as CR, so any LF will be skipped.
			LD	(prev_cr), A
			JR 	main_byte_loop
main5			CP	127				
			JR	Z, main_not_cr		; Ignore DEL char
			CP 	32
			JR	C, main_not_cr		; Ignore control chars 0..31, exeept for TAB, CR, LF, tested earlier
			RST.LIL	10h			; Print the character, all 322--127, 128-255 are printable.
			CALL	do_advance
main_not_cr:		XOR 	A
			LD 	(prev_cr),A
			JR	main_byte_loop
main_load_end		MOSCALL mos_fclose 
			LD	A, (current_col)
			AND 	A
			JR 	Z, main6		; Print CRLF if file does not end in newline.
			LD 	A, 13		
			RST.LIL 10h
			LD 	A, 10
			RST.LIL 10h
main6:			LD 	HL, 0
			RET

; Print a counted string (preceded by a count byte)
; Parameters:
;  HL: Address of string (24-bit pointer)
;
PRCNTSTR             					     
			LD 	B, (HL)
			XOR	A
			CP	B
			RET 	Z
			INC 	HL
$$:			LD 	A, (HL)
			RST.LIL 10h
			INC 	HL
			DJNZ 	$B
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

; Advance the column position. If at end of line, do a newline.
do_advance:		LD 	A, (current_col)
			INC	A
			LD	(current_col), A
			CP 	(IX+sysvar_scrCols)	; Are we at the end of the screen line?
			JR	Z, do_newline			
			RET
; Do a newline, if rews-1 lines were printe since last pause, do a pause.			
do_newline:		LD	A, (current_col)
			CP	(IX+sysvar_scrCols)
			JR	Z, nl_nocrlf		; Skip printing the newline if cursor has advanced to last screen position.
			LD	A, 13
			RST.LIL 10h
			LD	A, 10
			RST.LIL 10h
nl_nocrlf:		XOR 	A
			LD 	(current_col),A		; Clear column position
			LD	A, (num_lines)
			INC 	A
			LD	(num_lines),A
			INC	A
			CP	(IX+sysvar_scrRows)
			RET	NZ			; Do we need a pause?
			XOR 	A
			LD	(num_lines), A		; Clear line counter
			LD 	HL, c_MORE
			CALL 	PRCNTSTR		; Print More message
$$:			LD      A, (IX+sysvar_keycode)
			AND 	A
			JR	Z, $B			; Do this instead of MOSCALL mos_getkey because it has a race condition.
			LD	(IX+sysvar_keycode), 0
			CP	'Q'
			JR	Z, do_quit
			CP	'q'
			JR	Z, do_quit
			CP	27
			JR	Z, do_quit
			LD	HL, c_CLEAR
			CALL	PRCNTSTR
			RET
do_quit:		LD	HL, c_CLEAR
			CALL	PRCNTSTR
			POP 	HL			; Remove caller address
			JP	main_load_end
			
; Text messages
;
s_ERROR_SRC:		DB 	" Cannot open source file\n\r", 0
s_USAGE:		DB	" Usage: more <txtfile>\n\r", 0                                 
c_MORE:			DB  	16 ;String length
			DB	17, 0  ;foregrond black
			DB 	17, 143 ; background white
			DB      "--More--"
			DB 	17, 15 ; foreground white
			DB	17, 128 ; background black
c_CLEAR			DB      10,13,32,32,32,32,32,32,32,32,13 ; Clear the --More-- output
c_GETMODE               DB	3, 23,0,6	; Get screen mode parameters.

current_col:		DS	1			; Current column on the screen.
num_lines:		DS 	1			; Number of lines printed since last pause
prev_cr:		DS	1			; flag to indicate previous char was CR.

; RAM
; 
			DEFINE	LORAM, SPACE = ROM
			SEGMENT LORAM
			