;
; Title:	Copy - Main
; Author:	Lennart Benschop
; Created:	26/12/2022
; Modified:	29/12/2022. 
;		- Don't be naughty and just steal some memory, only use if buffer address is in third parameter
;		- Check address before storing the byte.

			.ASSUME	ADL = 1			

			INCLUDE	"equs.inc"
			INCLUDE "mos_api.inc"

			SEGMENT CODE
			XREF	ASC_TO_NUMBER		
			XDEF	_main


; This program copies a single file, command line has source file and destination file name.
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
			JR  	Z, main_defaultbuf
			CP	#4
			JR	Z, main_bufparam
			
			LD 	HL, s_USAGE		; Number of args not 3 or 4, print usage string and exit
			CALL	PRSTR
			LD	HL, 19
			RET
			
main_defaultbuf:	LD 	HL, default_buf
			LD	(buf_start), HL
			LD	HL, default_buf_end
			LD 	(buf_end), HL
			JR	main1
main_bufparam		LD	HL, (IX+9)
			CALL	ASC_TO_NUMBER
			JR	NC, main_badbuf
			LD	HL, 03ffffh
			AND A
			SBC	HL, DE
			JR	NC, main_badbuf
			LD	HL, 0b0000h
			AND A
			SBC	HL, DE
			JR 	C,  main_badbuf
			LD	(buf_start), DE
			LD	HL, 0B0000h
			LD	(buf_end), HL
			JR	main1
main_badbuf:		LD 	HL, s_BADADDR
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
main_load		LD 	HL, (buf_start)
			LD	DE, (buf_end)
main_load_loop		MOSCALL mos_feof
			CP 	1
			JR 	Z, main_load_end
			MOSCALL mos_fgetc		; Fortunately fgetc transparently passes all bytes.
			; mos_fgetc does not set carry flag at EOF in MOS 1.02, bummer, therefore separate mos_feof call in the loop.
			AND	A
			SBC 	HL, DE			; Compare against limit.
			JR	NC, main_buffer_full
			ADD	HL, DE			; undo subtraction
			LD	(HL),A			; Store loaded char into buffer.
			INC	HL
			JR	main_load_loop
main_load_end		MOSCALL mos_fclose
			LD 	DE, (buf_start)
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
s_ERROR_BUF:		DB 	" File buffer full,\n\r", 0	
s_ERROR_DF:		DB	" Disk full\n\r", 0
s_ERROR_SAVE:		DB	" Cannot save file\n\r", 0
s_USAGE:		DB	" Usage: copy <srcfile> <dstfile> [<bufaddr>]\n\r", 0
s_BADADDR		DB	" Bad buffer address\n\r", 0	
	
; RAM
; 
			DEFINE	LORAM, SPACE = ROM
			SEGMENT LORAM
			
buf_start:		DS 3
buf_end:		DS 3
; The default Copy buffer is in the space between the end of the program and the top of the MOS command space.
default_buf:
default_buf_end		EQU 0B8000h
			