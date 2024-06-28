;
; Title:	Screen initialization functions.
; Author:	Lennart Benschop
;
; Created:      27/06/2024 Adapted to ez80asm


; Put VDU in 80 column mode, if not already there.
Setup_Screen:		PUSH 	IX
			LD 	HL, c_FIXSETTINGS
			CALL	Print_CString		; Cancel text window control keys, page mode	
			MOSCALL mos_sysvars
			RES 	4, (IX+sysvar_vpd_pflags)	; Clear mode flag
			LD 	HL, c_GETMODE
			CALL	Print_CString  		; Get screen mode parameters.
@@:			BIT	4, (IX+sysvar_vpd_pflags)			
			JR 	Z, @B		        ; Wait until received.	
			LD	A, (IX+sysvar_scrMode)
			LD	(Saved_Mode),A
			LD	A, (IX+sysvar_scrCols)
			CP	80
			JR	NC, Setup_End		; We have a mode with at least 80 columns, keep that.
			LD	A, 22
			RST.LIL 10h
			LD	A, 3
			RST.LIL 10h			; Enter Mode 3 (80x30)
			LD	A,(IX+0)
@@:			CP	(IX+0)
			JR	Z, @B			; Wait for VBLANK
Setup_End:		RES 	4, (IX+sysvar_vpd_pflags)	; Clear mode flag
			LD 	HL, c_GETMODE
			CALL	Print_CString  		; Get screen mode parameters.
@@:			BIT	4, (IX+sysvar_vpd_pflags)			
			JR 	Z, @B		        ; Wait until received.	
			LD	A, (IX+sysvar_scrRows)
			LD	(Current_Rows),A
			LD	A, (IX+sysvar_scrCols)
			LD	(Current_Cols),A
			LD 	A, (IX+sysvar_scrMode)
			LD      (Current_Mode),A
			LD 	A, 128	     ; Obtain foreground colour.
			CALL	get_colour
			LD	(current_fg),A
			LD 	A, 129	     ; Obtain background colour.
			CALL	get_colour
			LD      (current_bg),A
			POP	IX
			RET
			
; Input A, index to get colour (128 = foreground, 129 = background).
; output: A scolour index of fg or bg colour.
get_colour:	    	RES     2,(IX+sysvar_vpd_pflags)
			LD  	L, A
			LD	A, 23
			RST.L   10h
			XOR	A
			RST.L   10h
			LD	A, 148
			RST.L   10h
			LD	A, L
			RST.L   10h
@@:			BIT	2, (IX+sysvar_vpd_pflags)			
			JR 	Z, @B		        ; Wait until received.	
			LD	A,(IX+sysvar_scrpixelIndex)
			RET
			