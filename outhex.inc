;
; Title:	Memory Dump - Output functions
; Author:	Dean Belfield
; Created:	15/11/2022
; Last Updated:	29/12/2022 Lennart Benschop
;		- Adapted to ADL=1
;
; Modinfo:
; Changed 09/04/2024 Adapted to ez80asm

				
; Print a zero-terminated string
;
Print_String:		LD	A,(HL)
			OR	A
			RET	Z
			RST.LIL	10h
			INC	HL
			JR	Print_String
			
; Print a 24-bit HEX number
; HLU: Number to print
;
Print_Hex24:		PUSH		HL
			LD		HL, 2
			ADD		HL, SP
			LD		A, (HL)
			POP		HL

			CALL		Print_Hex8			
			
; Print a 16-bit HEX number
; HL: Number to print
;
Print_Hex16:		LD	A,H
			CALL	Print_Hex8
			LD	A,L

; Print an 8-bit HEX number
; A: Number to print
;
Print_Hex8:		LD	C,A
			RRA 
			RRA 
			RRA 
			RRA 
			CALL	@F 
			LD	A,C 
@@:			AND	0Fh
			ADD	A,90h
			DAA
			ADC	A,40h
			DAA
			RST.LIL	10h
			RET
			