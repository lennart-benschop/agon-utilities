;
; Title:	Launcher for 12AM Commander
; Author:	Lennart Benschop
; Created:	11/05/2024


LAUNCHER_EXT_EXEC:	equ	0xb7f000
LAUNCHER_EXT_CMDLINE:  	equ 	0xb7f100
LAUNCHER_OWN_CMDLINE:  	equ 	0xb7f200
	
  		ASSUME ADL=1
		INCLUDE "../mos_api.inc"

; Launcher program is to run from internal RAM at $b7e0000
; HL points to command line. 
  	   	ORG $b7e000
relaunch:	PUSH	 BC
		PUSH	 DE
		PUSH  	 IX
		PUSH   	 IY
		PUSH     HL                     ; SAve program args
		LD    	 HL,progname
		LD    	 DE,$40000
		LD    	 BC,$b0000-$40000
		LD    	 A, mos_load
		RST.L 	 $08
		CP    	 $00
		JR    	 NZ, error
		POP   	 HL
		POP	 IY			; Restore registers
		POP	 IX
		POP	 DE
		POP	 BC
		
		CALL	 $40000
		LD	 A, (LAUNCHER_EXT_EXEC)
		AND	 A
		RET	 Z  ; Exit if no external command given
		PUSH	 BC
		PUSH	 DE
		PUSH     IX
  		PUSH     IY
		LD	 HL, LAUNCHER_EXT_EXEC
		LD	 DE, $40000
		LD    	 BC,$b0000-$40000
		LD    	 A, mos_load
		RST.L 	 $08
		CP    	 $00
		JR	 NZ, @1		
		LD	 HL, LAUNCHER_EXT_CMDLINE
		LD	 A, mos_oscli
		RST.L	 $08
@1:		POP	 IY
		POP	 IX
		POP	 DE
		POP	 BC
		LD 	 HL, LAUNCHER_OWN_CMDLINE
		JR	 relaunch	 

error:		XOR	 A
		LD HL,	 message
		LD BC,	 0
		RST.L	 $18
		POP   	 HL
		POP	 IY			; Restore registers
		POP	 IX
		POP	 DE
		POP	 BC
		LD  	 HL, 4
		RET

message:	DB "Overlay not found",13,10,0
progname:	DB "/bin/12amc.ovl",0
		



