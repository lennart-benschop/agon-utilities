;
; Title:	Comp - Main
; Author:	Lennart Benschop
; Created:	27/12/2022
;

			.ASSUME	ADL = 1			

			INCLUDE "mos_api.inc"
			ORG $b0000 ; Is a moslet
	
			MACRO PROGNAME
			ASCIZ "comp.bin"
			ENDMACRO
			
  			include "init.inc"
			include "outhex.inc"
	
BUFSIZE:		equ    4096

; File compare utility. Compares two files and shows byte offset of any difference. 
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
			LD 	HL, s_USAGE		; Number of args != 2, print usage string and exit
			CALL	Print_String
			LD	HL, 19
			RET
main1:			LD 	HL, (IX+3)		; source file name
			LD	C,  fa_read
			MOSCALL mos_fopen
			OR	A
			JR	NZ, main2	
			LD	HL, s_ERROR_OPEN	; source file not opened, error
			CALL	Print_String
			LD 	HL, (IX+3)
			CALL 	Print_String
			LD	HL, s_CRLF
			CALL	Print_String
			LD	HL, 4			
			RET
main2: 			LD 	E, A    		; Store file1 handle to E.
			LD 	HL, (IX+6)		; source file name
			LD	C,  fa_read
			MOSCALL mos_fopen
			OR	A
			JR	NZ, main3	
			LD	HL, s_ERROR_OPEN	; source file not opened, error
			CALL	Print_String
			LD 	HL, (IX+6)
			CALL 	Print_String
			LD	HL, s_CRLF
			CALL	Print_String
			LD	C, E
			MOSCALL mos_fclose		; close the file that was already open
			LD	HL, 4			
			RET
main3: 			LD 	D, A    		; Store file2 handle to D
			PUSH 	DE			; Druing the main loop, keep file handles stacked.
			LD 	DE, 0
			LD 	(File_Offs), DE
			XOR	A
			LD	(At_End), A
main_comp_loop:		POP 	DE
			LD	C, E
			PUSH 	DE
			LD 	HL, Buf1
			LD	DE, BUFSIZE
			CALL	read_block		; Read block from file1
			LD	(Blk1_Size), HL
			LD	DE, BUFSIZE
			AND 	A
			SBC	HL, DE
			JR	Z, @F
			LD	A, 1
			LD 	(At_End), A		; Check if bytes read < buf size. At end of file
@@:			POP	DE
			LD	C, D
			PUSH	DE
			LD	HL, Buf2
			LD	DE, BUFSIZE
			CALL	read_block		; Read block from file 2
			LD	(Blk2_Size), HL
			LD	DE, BUFSIZE
			AND 	A
			SBC	HL, DE
			JR	Z, @F
			LD	A, 1
			LD 	(At_End), A		;  Check if bytes read < buf size. At end of file
@@:			LD	HL, (Blk1_Size)				
			LD	DE, (Blk2_Size)
			AND 	A
			SBC	HL, DE
			JR	Z, main_same_size
			JR	NC, main_file1_larger
main_file2_larger:	LD	BC, (Blk1_Size)
			CALL	comp_blocks
			JR 	NZ, main_blocks_differ    
			LD	DE, (IX+6)
			JP 	main_size_differ
main_file1_larger:	LD	BC, (Blk2_Size)
			CALL	comp_blocks
			JR 	NZ, main_blocks_differ    
			LD	DE, (IX+3)
			JP 	main_size_differ	
main_same_size:		LD	BC, (Blk1_Size)
			CALL	comp_blocks
			JR	NZ, main_blocks_differ
			LD	A, (At_End)		
			AND	A
			JP	Z, main_comp_loop
main_file_same:		LD	HL, s_SAME
			CALL	Print_String
			LD	HL, 0
			JR 	main_close_all
main_blocks_differ:	LD	HL, s_DIFFER
			CALL	Print_String
			LD	HL, (File_Offs)
			CALL	Print_Hex24
			LD 	HL, s_CRLF
			CALL	Print_String
			JR	main_close_all
main_size_differ:	LD	HL, s_LARGER
			CALL	Print_String
			EX	DE, HL
			CALL	Print_String
			LD 	HL, s_CRLF
			CALL	Print_String
main_close_all:		POP DE
			LD C, E
			MOSCALL mos_fclose
			LD C, D
			MOSCALL mos_fclose
			LD HL, 0
			RET

; read_block
;
; Read blcok from open file
;
; input:	C = file handle.
;		HL = buffer position.
;		DE = max nof bytes to read (less than 0x10000, not 0)..
; output:	HL = nof bytes actually read, less than max (possibly 0) if EOF.
;		There should be a MOS API function to do this, but currently not implemented
read_block:		PUSH 	DE			; Store original max size
			MOSCALL mos_fread
			EX	DE, HL		
			POP	DE
			RET	
			
; comp_blocks.
;
; Compare blocks in Buf1 and Buf2. Adjust File_Offs for number of bytes compared.
;
; input 	BC is number of bytes to compare.
; output	Z flag is set when blocks are equal, cleared otherwise.
			
comp_blocks:		LD	HL, Buf1			
			LD	DE, Buf2
			JR	comp2
comp1:			LD 	A, (DE)
			CP	(HL)
			JR	NZ, comp_ne
			INC	HL
			INC 	DE
			DEC     BC
comp2:			LD	A, B			; We are equal if block length = 0.
			OR	C
			JR	NZ, comp1
comp_ne:		PUSH	AF			; Keep zero flag to return.
			AND 	A
			LD	DE, Buf1
			SBC	HL, DE 
			LD	DE, (File_Offs)
			ADD	HL, DE
			LD	(File_Offs), HL		; Add compared bytes to offset.
			POP 	AF
			RET
			
; Text messages
;
s_ERROR_OPEN:		DB 	" Cannot open file ", 0
s_USAGE:		DB	" Usage: comp <file1> <file2>\r\n", 0                                 
s_DIFFER:		DB	" Files differ at offset ",0
s_LARGER:		DB	" File is larger: ",0	
s_SAME:			DB	" Files are identical"	
s_CRLF:			DB 	13,10,0
; Variable space
File_Offs:		DS 3		
Blk1_Size:		DS 3
Blk2_Size:		DS 3
At_End:			DS 1
Buf1:			DS BUFSIZE
Buf2:			DS BUFSIZE