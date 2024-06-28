;
; Title:	Output functions.
; Author:	Lennart Benschop
; Created:	27/12/2022
;
; Modified:     09/04/2024 Adapted to ez80asm

Print_Decimal:	
; Print a decimal number (less than 1000000)';
;
; Input: HL 24-bit number to print.
			PUSH	IY
			PUSH	DE
			PUSH 	BC
			LD	IY, Num_Table
			LD	B, 6		; Consider 6 digits
			LD	C, 6		; Leading zero counter, reaches 1 if sixth digit sill 0.		; 
Print_Dec1:		LD	DE, (IY+0) 	; Take next power of 10.
			LEA	IY, IY+3
			LD	A, '0'-1
@@:			INC	A
			AND 	A	
			SBC 	HL, DE		; Repeatedly subtract power of 10 until carry
			JR	NC, @B
			ADD	HL, DE		; Undo the last subtract that caused carry					
			CP	'0'
			JR	NZ, @F		; Don't print leading zero			
			DEC	C	
			JR	NZ, Print_Dec2	; But do print 0 if it's the units digit.
@@:			RST.LIL	10h
			LD	C, 1		; Make sure the next digit will be printed.
Print_Dec2:		DJNZ	Print_Dec1
			POP 	BC
			POP	DE
			POP	IY
			RET

Num_Table:		DL	100000
			DL	10000
			DL	1000
			DL	100
			DL	10
			DL	1

; Print a counted string (preceded by a count byte)
; Parameters:
;  HL: Address of string (24-bit pointer)
;
Print_CString:          PUSH	BC  					     
			LD 	B, (HL)
			XOR	A
			CP	B
			JR	Z, Print_CString_End
			INC 	HL
@@:			LD 	A, (HL)
			RST.LIL 10h
			INC 	HL
			DJNZ 	@B	
Print_CString_End:	POP	BC	
			RET	
			
; Print Carriage-return and linefeed.			
Print_CRLF:		LD	HL, s_CRLF			
; Print a zero-terminated string
; Parameters:
;  HL: Address of string (24-bit pointer)
;
Print_String:		LD	A,(HL)
			OR	A
			RET	Z
			RST.LIL	10h
			INC	HL
			JR	Print_String
		

; Set display to inverse video	
Inverse_Video:	 	LD 	A, 17
			RST.L	10h
			LD	A, (current_bg)
			RST.L   10h
			LD 	A, 17
			RST.L	10h
			LD	A, (current_fg)
			ADD	A, $80
			RST.L   10h
			RET
			
; Set display to true video	
True_Video:   	        LD 	A, 17
			RST.L	10h
			LD	A, (current_fg)
			RST.L   10h
			LD 	A, 17
			RST.L	10h
			LD	A, (current_bg)
			ADD	A, $80
			RST.L   10h
			RET
			
; Restore original video mode.
Restore_Screen:	   	LD 	HL, c_ENABLECTRL        ; Re-enable control keys
			CALL	Print_CString

			LD	A, (Saved_Mode)
			LD      L, A
			LD 	A, (Current_Mode)
			CP      L
			RET	Z			; Do nothing if we had not changed modes.
			LD 	A, 22
			RST.LIL 10h
			LD 	A, L
			RST.LIL 10h
			RET
			
; Clear to the end of line by printing spaces. so cursor will wrap to next line.
; When at the bottom row, do not print the rightmost column to avoid scrolling.
			
Clear_EOL:		PUSH 	IX
			PUSH	BC
			MOSCALL mos_sysvars
			RES	0, (IX+sysvar_vpd_pflags)
			LD	A, 23	
			RST.LIL 10h
			XOR	A
			RST.LIL 10h
			LD	A, vdp_cursor
			RST.LIL 10h	; Ask the VDP to send cursor position.
@@:			BIT	0, (IX+sysvar_vpd_pflags)
			JR	Z, @B	; Wait for result.
			LD	A, (Current_Cols)
			LD	B, A	; Number of columns to B
			LD      A, (Current_Rows)
			DEC     A
                        CP      (IX+sysvar_cursorY)
                        JR      NZ, @F  ; Are we on bottom row?
                        DEC     B       ; Then only fill out up to last column, do not write a space there.
	
@@:			LD	A, (IX+sysvar_cursorX)
			SUB	B
			NEG	
			LD	B, A	; B contains desired column position - current cursor.
@@:			LD	A, ' '
			RST.LIL	10h
			DJNZ	@B	
			POP	BC
			POP 	IX
			RET

s_CRLF:			DB	13,10,0

c_FIXSETTINGS:		DB	6, 23,0,$98,0,15,26 ; Switch off control keys, cancel page mode, text window
c_ENABLECTRL:		DB	4, 23,0,$98,1       ; Switch on control keys
c_GETMODE:  		DB	3, 23,0,vdp_mode		; Get screen mode parameters. Switch off paged mode, cancels text window.			
; RAM
; 
