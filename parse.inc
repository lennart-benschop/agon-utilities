;
; Title:	Number Parsing Functions
; Author:	Dean Belfield
; Created:	15/11/2022
; Last Updated:	15/11/2022
; Last Updated: 29/12/2022 Lennart Benchop adapt for 24-bit mode.
; Modinfo:
; 2024/04/08    Adapt for ez80asm
				
; Read a number and convert to binary
; If prefixed with &, will read as hex, otherwise decimal
;   Inputs: HL: Pointer in string buffer
;  Outputs: HL: Updated text pointer
;           DE: Value
;            A: Terminator (spaces skipped)
;            F: Carry set if valid number, otherwise reset
; Destroys: A,D,E,H,L,F
;
ASC_TO_NUMBER:		LD		DE, 0			; Initialise DE
			CALL		_skip_spaces		; Skip whitespace
			LD		A, (HL)			; Read first character
			OR		A			; Check for end of string
			RET		Z			; Return with no carry if not
			PUSH		BC			; Preserve BC
			CP		'&'			; Is it prefixed with '&' (HEX number)?
			JR		NZ, ASC_TO_NUMBER3	; Jump to decimal parser if not
			INC		HL			; Otherwise fall through to ASC_TO_HEX
;
ASC_TO_NUMBER1:		LD		A, (HL)			; Fetch the character
			CALL   	 	UPPRC			; Convert to uppercase  
			SUB		'0'			; Normalise to 0
			JR 		C, ASC_TO_NUMBER4	; Return if < ASCII '0'
			CP 		10			; Check if >= 10
			JR 		C, ASC_TO_NUMBER2	; No, so skip next bit
			SUB 		7			; Adjust ASCII A-F to nibble
			CP 		16			; Check for > F
			JR 		NC, ASC_TO_NUMBER4	; Return if out of range
;
ASC_TO_NUMBER2:		PUSH		HL			; Stack HL
			PUSH		DE			; LD HL, DE
			POP		HL
			ADD		HL, HL	
			ADD		HL, HL	
			ADD		HL, HL	
			ADD		HL, HL	
			PUSH		HL			; LD DE, HL
			POP		DE
			POP		HL			; Restore HL			
			OR      	E			; OR the new digit in to the least significant nibble
			LD      	E, A
;			
			INC		HL			; Onto the next character
			JR      	ASC_TO_NUMBER1		; And loop
;
ASC_TO_NUMBER3:		LD		A, (HL)
			SUB		'0'			; Normalise to 0
			JR		C, ASC_TO_NUMBER4	; Return if < ASCII '0'
			CP		10			; Check if >= 10
			JR		NC, ASC_TO_NUMBER4	; Return if >= 10
;
			PUSH		HL			; Stack HL
			PUSH		DE			; LD HL, DE
			POP		HL
			PUSH		HL			; LD BC, HL
			POP		BC
			ADD		HL, HL 			; x 2 
			ADD		HL, HL 			; x 4
			ADD		HL, BC 			; x 5
			ADD		HL, HL 			; x 10
			LD		BC, 0
			LD 		C, A			; LD BCU, A
			ADD		HL, BC			; Add BCU to HL
			PUSH		HL			; LD DE, HL
			POP		DE
			POP		HL			; Restore HL
;						
			INC		HL
			JR		ASC_TO_NUMBER3
ASC_TO_NUMBER4:		POP		BC 			
			SCF					; We have a valid number so set carry
			RET


; Convert a character to upper case
;  A: Character to convert
;
UPPRC:  		AND     	7FH
			CP      	'`'
			RET     	C
			AND     	5FH			; Convert to upper case
			RET