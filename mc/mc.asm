; Title:	Startup code for Launcher for 12AM Commander
; Author:	Lennart Benschop
; Created:	11/05/2024

	       	ASSUME	ADL = 1			

	       	INCLUDE "../mos_api.inc"

      	       	ORG $40000 ; Is a normal binary
	
		JP _start
		DB "mc.bin",0
		ALIGN 64
		DB  "MOS",0,1
_start:		PUSH  HL
		PUSH  DE
		PUSH  BC
		; Move payload to internal RAM.
		LD HL,payload
		LD DE,$b7e000
		LD BC,payload_end-payload
		LDIR
		POP  BC
		POP  DE
		POP  HL
		JP $b7e000
		
payload:
		INCBIN "launcher.bin"
payload_end:		
