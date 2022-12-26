;
; Title:	Copy - Main
; Author:	Lennart Benschop
; Created:	26/12/2022
;

			.ASSUME	ADL = 1			

			INCLUDE	"equs.inc"
			INCLUDE "mos_api.inc"

			SEGMENT CODE
			
			XDEF	_main

BUF_START		EQU 090000h
BUF_END			EQU 0B0000h

; This program sopies a single file, command line has source file and destination file name.
; 
; Collects the file in a buffer outside code space, currently limited to 128kB.
;	- Read via mos_fgetc loop, as mos_load does not return the file length.
;	- Write via mos_save. Loop with mos_fputc does not work as it converts LF to CRLF. 
;
; The main routine
; IXU: argv - pointer to array of parameters
;   C: argc - number of parameters
; Returns:
;  HL: Error code, or 0 if OK
;
_main:			LD	A, C
			CP 	#3
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
			; Load one byte per character. not possible to determine file length from MOS load command.
main_load		LD 	HL, BUF_START
			LD	DE, BUF_END
main_load_loop		MOSCALL mos_feof
			CP 	1
			JR 	Z, main_load_end
			MOSCALL mos_fgetc		; Fortunately fgetc transparently passes all bytes.
			; mos_fgetc does not set carry flag at EOF in MOS 1.02, bummer, therefore separate mos_feof call in the loop.
			LD	(HL),A			; Store loaded char into buffer.
			INC	HL
			AND	A
			SBC 	HL, DE			; Compare against limit.
			JR	NC, main_buffer_full
			ADD	HL, DE			; undo subtraction
			JR	main_load_loop
main_load_end		MOSCALL mos_fclose
			LD 	DE, BUF_START
			AND 	A
			SBC	HL, DE			; HL has file length.
			PUSH 	HL
			POP	BC			
			LD   	HL, (IX+6)
			MOSCALL mos_save
			JR	NC, main_disk_full
			AND 	A
			JR	NZ, main_file_error
			LD	HL, 0			; All OK
			RET			
main_buffer_full:	
			LD	HL, s_ERROR_BUF
			CALL    PRSTR
			LD 	HL, 12
			RET
main_file_error:	LD 	HL, 0
			LD 	L, A			; Pass error returned by mos_save
			EX	DE, HL
			LD	HL, s_ERROR_SAVE
			CALL 	PRSTR
			EX	DE, HL
			RET	
main_disk_full:		LD 	HL, s_ERROR_DF
			CALL	PRSTR
			LD 	HL, 12
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
s_ERROR_SRC:		DB 	" Cannot open source file\n\r", 0
s_ERROR_BUF:		DB 	" File buffer full\n\r", 0	
s_ERROR_DF:		DB	" Disk full\n\r", 0
s_ERROR_SAVE		DB	" Cannot save file\n\r", 0
s_USAGE:		DB	" Usage: copy <srcfile> <dstfile>\n\r", 0
	
; RAM
; 
			DEFINE	LORAM, SPACE = ROM
			SEGMENT LORAM
			