;
; Title:	Copy - Initialisation Code
; Author:	Dean Belfield, Lennart Benschop
; Created:	06/11/2022
; Last Updated:	26/12/2022
;
; Modinfo:
; 17/12/2022:	Added parameter processing
; 26/12/2022:   Adapted to Copy program, use LEA instead of 3x INC IX, Save/restore MB
; Changed:      08/04/2924 adapt to ez80asm

			
argv_ptrs_max:		EQU	16			; Maximum number of arguments allowed in argv
			
;
; Start in ADL mode
;
			JP	_start			; Jump to start			
;
; The header stuff is from byte 64 onwards
;

_exec_name:
			PROGNAME			; The executable name, only used in argv

			ALIGN	64
			
			DB	"MOS"			; Flag for MOS - to confirm this is a valid MOS command
			DB	00h			; MOS header version 0
			DB	01h			; Flag for run mode (0: Z80, 1: ADL)
;
; And the code follows on immediately after the header
;
_start:			PUSH	AF			; Preserve the registers
			PUSH	BC
			PUSH	DE
			PUSH	IX
			PUSH	IY
			LD	A, MB			; Save MB
			PUSH 	AF
			XOR 	A
			LD 	MB, A                   ; Clear to zero so MOS API calls know how to use 24-bit addresses.
			
			LD	IX, argv_ptrs		; The argv array pointer address
			PUSH	IX
			CALL	_parse_params		; Parse the parameters
			POP	IX			; IX: argv	
			LD	B, 0			;  C: argc
			CALL	_main			; Start user code
			
			POP 	AF
			LD	MB, A
			POP	IY			; Restore registers
			POP	IX
			POP	DE
			POP	BC
			POP	AF
			RET
	
; Parse the parameter string into a C array
; Parameters
; - HL: Address of parameter string
; - IX: Address for array pointer storage
; Returns:
; -  C: Number of parameters parsed
;
_parse_params:		LD	BC, _exec_name
			LD	(IX+0), BC		; ARGV[0] = the executable name
			LEA     IX, IX+3
			CALL	_skip_spaces		; Skip HL past any leading spaces
;
			LD	BC, 1			; C: ARGC = 1 - also clears out top 16 bits of BCU
			LD	B, argv_ptrs_max - 1	; B: Maximum number of argv_ptrs
;
_parse_params_1:	
			PUSH	BC			; Stack ARGC	
			PUSH	HL			; Stack start address of token
			CALL	_get_token		; Get the next token
			LD	A, C			; A: Length of the token in characters
			POP	DE			; Start address of token (was in HL)
			POP	BC			; ARGC
			OR	A			; Check for A=0 (no token found) OR at end of string
			RET	Z
;
			LD	(IX+0), DE		; Store the pointer to the token
			PUSH	HL			; DE=HL
			POP	DE
			CALL	_skip_spaces		; And skip HL past any spaces onto the next character
			XOR	A
			LD	(DE), A			; Zero-terminate the token
			LEA  	IX, IX+3			; Advance to next pointer position
			INC	C			; Increment ARGC
			LD	A, C			; Check for C >= A
			CP	B
			JR	C, _parse_params_1	; And loop
			RET

; Get the next token
; Parameters:
; - HL: Address of parameter string
; Returns:
; - HL: Address of first character after token
; -  C: Length of token (in characters)
;
_get_token:		LD	C, 0			; Initialise length
@@:			LD	A, (HL)			; Get the character from the parameter string
			OR	A			; Exit if 0 (end of parameter string in MOS)
			RET 	Z
			CP	13			; Exit if CR (end of parameter string in BBC BASIC)
			RET	Z
			CP	' '			; Exit if space (end of token)
			RET	Z
			INC	HL			; Advance to next character
			INC 	C			; Increment length
			JR	@B
	
; Skip spaces in the parameter string
; Parameters:
; - HL: Address of parameter string
; Returns:
; - HL: Address of next none-space character
;    F: Z if at end of string, otherwise NZ if there are more tokens to be parsed
;
_skip_spaces:		LD	A, (HL)			; Get the character from the parameter string	
			CP	' '			; Exit if not space
			RET	NZ
			INC	HL			; Advance to next character
			JR	_skip_spaces		; Increment length

; Storage for the argv array pointers
;
argv_ptrs:		BLKP	argv_ptrs_max, 0			