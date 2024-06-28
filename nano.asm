;
; Title:	Nano - Main
; Author:	Lennart Benschop
; Created:	02/01/2023
; Modified	06/01/2023 version 0.01 basic edit functions working.
;               05/03/2023 version 0.04 Adapted to new VDP codes./

			.ASSUME	ADL = 1			

			INCLUDE "mos_api.inc"

			ORG $b0000 ; Is a moslet
	
			MACRO PROGNAME
			ASCIZ "nano.bin"
			ENDMACRO
			
  			include "init.inc"
			include "parse.inc"
			include "output.inc"

; This is a simple text editor, with a user interface similar to "GNU nano" on Linux.
; But this editor runs on Agon and is written in assembly.
; 
; Features:
;	- Can run as a MOS command
;	- Optionally takes a buffer address to allow editing larger files (up to 450kB).
;	- Optionally takes a line number argument, so we can look at a file at a specific line.
;	- Edits single file only.
;	- Renames old file into backup file when saving.
;	- Normal behaviour of cursor keys, home, end, page up, page down, backspace, delete and Enter.
;	- Forward/backward search.
;	- can insert other file into text.
;	- Cut/past of whole lines.
;	- Normalizes all files to DOS-style CR-LF, removes control characters (except TAB), last line always had CR-LF line ending.
;
; Design decisions:
;	- Correctness over user-friendliness or efficiency. Enforce some invariants.
;		- All lines contain no control characters except TAB.
;		- All lines are 253 bytes or less (excluding terminationg CR-LF). During loading insert extra
;		  line terminators in longer lines, during editing refuse to add more characters.
;		- All lines, including the last line are terminated by CR-LF.
;	'	- cur_lineaddr/top_lineaddr always point to the start of a line in the edit buffer, 
;		  the corresponding line number variables are always correct.
;	- Edit file is contiguous text in file buffer.
;	- Lines are copied to line buffer while being edited, moved back into text when done editing that line.
;	  When line of different size is moved back into text, all text after it needs to be moved up/down,
;	  but that is acceptable as it is only done once per edited line.
;	- Cut/paste only of whole lines, cut lines are collected at end of edit buffer.
;	- Minimize redraw of whole screen. Not while editing on same line, not while moving cursor inside visible screen area.
;         Screen redraw is fast enough if only done when lines are added/removed or when curor moves outside visible screen area.
;
; The main routine
; IXU: argv - pointer to array of parameters
;   C: argc - number of parameters
; Returns:
;  HL: Error code, or 0 if OK
;
_main:			LD	HL, 1
			LD	(start_lineno), HL
			LD	A, C
			CP 	#2
			JR  	Z, main_defaultbuf
			CP	#3
			JR	Z, main_bufparam
			CP	#4			
			JR	Z, main_linenum
			LD 	HL, s_USAGE		; Number of args not 1, 2 or 3, print usage string and exit
			CALL	Print_String
			LD	HL, 19
			RET
			
main_defaultbuf:	LD 	HL, default_buf
			LD	(buf_start), HL
			LD	HL, default_buf_end
			LD 	(buf_end), HL
			JR	main1
main_linenum:		LD	HL, (IX+9)
			CALL 	ASC_TO_NUMBER
			LD	(start_lineno), DE			
main_bufparam:		LD	HL, (IX+6)
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
			CALL	Print_String
			LD	HL, 19
			RET
main1:			LD 	HL, (IX+3)	
			
			; Copy the file name to a local buffer 'filename', so we can edit it on saving
			LD	DE, filename	
@@:			LD 	A, (HL)
			LD	(DE), A
			INC	HL
			INC 	DE
			AND 	A
			JR 	NZ, @B
			CALL 	Setup_Screen
			
			LD	HL, filename
			LD	DE, (buf_start)
			CALL 	Load_File
			CP	0ffh
			JP 	Z, main_full
			AND 	A
			JP	NZ, main_fatal
			LD	HL, (buf_end)
			LD	(cutbuf_start), HL  	; Empty cut buffer
			LD	HL, (buf_start)
			LD	(cur_lineaddr), HL
			LD	(top_lineaddr), HL
			LD	DE, (load_filesize)
			ADD 	HL, DE
			LD 	(text_end), HL
			LD	HL, (cutbuf_lines)
			LD 	(num_lines), HL
			LD	HL, 1
			LD	(cur_lineno), HL
			LD	(top_lineno), HL
			DEC	HL
			LD	(cutbuf_lines), HL
			XOR	A
			LD	(cursor_row), A
			LD	(cursor_col), A
			LD	(cursor_col_max), A
			LD	(linebuf_scrolled), A
			LD	(file_modified), A
			LD	(curline_stat), A
			LD	(findstring), A
			CALL	Show_Screen
			LD	BC, (start_lineno)
			DEC	BC
			LD	IX, cur_lineaddr			
			CALL	Lines_Forward
			CALL	Adjust_Top
			LD	(save_sp), SP		; Save the stack pointer so we can return here in case of error.
			; Main editor loop
main_loop:		XOR	A
			LD	(cut_continue), A	; No continued cut/copy by default.
			; Return her after cut/copy, so repeated keypress will add more lines to buffer.
main_loop_continue:	CALL	Read_Key
			CP 	127
			JP 	Z, do_backspace		; Backspace key.
			CP 	32
			JP 	NC, do_ins_char		; Insert all caracters 32..255, except delete.
			LD	E, A
			ADD 	A, A
			ADD	A, E
			LD 	DE, 0
			LD 	E, A
			LD 	HL, main_dispatch
			ADD 	HL, DE
			
			LD  	HL, (HL)
			JP 	(HL)
			
main_full:		CALL	Restore_Screen
			LD	HL, s_ERROR_BUF
			CALL    Print_String
			LD 	HL, 12
			RET
main_fatal:		LD 	HL, 0
			LD 	L, A			; Pass error returned by mos_save
			PUSH 	HL
			CALL	Restore_Screen
			POP 	HL
			EX	DE, HL
			LD	HL, s_FATAL
			CALL 	Print_String
			EX	DE, HL
			RET	


main_dispatch:		; Dispatch table for all control characters/
			DL	main_loop		; NULL character, cannot happen
			DL	do_line_start		; ^A start of line
			DL	do_cur_left		; ^B cursor left
			DL	do_copy			; ^C copy current line		; 
			DL	do_delete		; ^D delete to right
			DL	do_line_end		; ^E end of line 
			DL	do_cur_right		; ^F cursor right
			DL	do_help			; ^G show help
			DL	do_goto			; ^H goto line
			DL	do_tab			; ^I TAB
			DL	main_loop		; 
			DL	do_cut			; ^K cut current line
			DL	do_center		; ^L screen center
			DL	do_enter		; ^M Enter, 
			DL	do_cur_down		; ^N cursor down
			DL	do_save			; ^O write file
			DL	do_cur_up		; ^P cursor up
			DL	do_rfind		; ^Q find backwards
			DL	do_readfile		; ^R read file
			DL	main_loop		; 
			DL	do_special		; ^T insert special character
			DL	do_paste		; ^U paste
			DL	do_page_down		; ^V page down
			DL	do_find			; ^W search forward
			DL	do_exit			; ^X Exit
			DL	do_page_up		; ^Y page up
			DL	main_loop		; 
			DL	do_exit			; ESC Exit
			DL	main_loop		; 28
			DL	main_loop		; 29
			DL	main_loop		; 30
			DL	main_loop		; 31 ^/ should be goto line but impossible to get this key.
						

; Handlers for all keys
; Backspace key, delete to left, can join lines if at start of line.
do_backspace:		CALL 	Enter_Current
			LD	A, (linebuf_editpos)
			AND	A
			JR	Z, backspace_joinlines	; At start of line: join lines together. 
			DEC	A
			LD	(linebuf_editpos), A	; move one position beck before deleting the character.
			CALL	Delete_Char
			CALL	Col_From_Editpos
			CALL 	Render_Current_Line
			;CALL	Show_Status
			CALL 	Show_Cursor
			JP 	main_loop
backspace_joinlines:	; Move to the previous line and delete the newline at its end.	
			LD	HL, (cur_lineno)
			LD	DE, 1
			AND 	A
			SBC 	HL, DE
			JP	Z, main_loop		; Don't move if already at start.	
			CALL	Leave_Current
			LD	IX, cur_lineaddr
			LD	BC, 1
			CALL 	Lines_Backward
			CALL	Adjust_Top		; Move to previous line.	
			CALL 	Enter_Current		; Copy that to the line buffer
			LD	A, (curline_length) 
			DEC	A
			DEC	A
			LD	(linebuf_editpos), A 	; Move to the end of it
			CALL	Col_From_Editpos
			JR	delete_joinlines

; Delete key (^D): delete to right, can join lines if at end of line.
do_delete:		CALL	Enter_Current
			LD	A, (curline_length)
			DEC	A
			DEC	A
			LD	C, A
			LD	A, (linebuf_editpos)
			CP	C
			JR	Z, delete_joinlines	; At end of line: join lines together
			CALL	Delete_Char
			CALL	Col_From_Editpos
			CALL 	Render_Current_Line
			;CALL	Show_Status
			CALL 	Show_Cursor
			JP 	main_loop
delete_joinlines:	; About to join current and next line. First check if it is allowed
			LD	HL, (cur_lineno)
			LD	DE, (num_lines)
			AND	A
			SBC	HL, DE
			JP	Z, main_loop		; Not allowed if last line of file.
			LD	HL, (cur_lineaddr)
			CALL	Next_LineAddr		; Step to end of current line.
			PUSH 	HL		
			CALL	Next_LineAddr		; Step to end of next line
			POP	DE
			AND	A
			SBC 	HL, DE			; Find length of next line.
			LD	DE, 0
			LD	A, (curline_length)
			LD	E, A
			ADD	HL, DE			; Add current line length.
			DEC	HL
			DEC	HL			; Subtract two, we are going to remove CR-LF
			LD	A, H
			AND	A			; Check that HL >= 256, joined line would be too long.
			JR	NZ, Line_Too_Long
			; Now we are allowed to join the lines
			LD	A, (curline_length)
			DEC	A
			DEC	A
			LD	(curline_length), A	; Sutract 2 fron length, effectively removing the trailing CRLF
			LD	A, 2
			LD	(curline_stat), A	; Mark current line as modified.
			CALL	Leave_Current		; Put the truncated line back into text (without its trailing CRLF)
			LD	HL, (num_lines)
			DEC	HL
			LD	(num_lines), HL		; Decrement number of lines.
			CALL	Show_Screen
			JP	main_loop

; TAB key, insert TAB character
do_tab:			LD	A, 9
; Keys corresponding to printable characters, insert into text
do_ins_char:		PUSH	AF
			CALL	Enter_Current
			POP	AF
			EX	AF, AF'
			LD	A, (curline_length)
			CP	255
			JR	Z, Line_Too_Long
			EX	AF, AF'
			CALL	Insert_Char
			CALL	Col_From_Editpos
			CALL 	Render_Current_Line
			;CALL	Show_Status
			CALL 	Show_Cursor
			JP 	main_loop
	

Line_Too_Long:		CALL 	Clear_BottomRow
			LD 	HL, s_LINETOOLONG
			CALL	Print_String
			JP	main_loop

Memory_Full:		LD 	SP, (save_sp)		; Can get here from within a subroutine, restore stack.
			CALL	Adjust_Top
			CALL	Show_Screen
			CALL 	Clear_BottomRow
			LD 	HL, s_NOMEM
			CALL	Print_String
			JP	main_loop	
			
Load_Error:		CALL 	Clear_BottomRow
			LD 	HL, s_LOADERR
			CALL	Print_String
			JP	main_loop
		
			
; ENTER key, add new line after current line/split current line.
; Simply insert CR-LF into the current line, put the line back into the text
; and increase nun_lines..
; One issue with this is: if the current length is greater than 253, we cannot insert CRLF,
; so we cannot split the line when we might need it most.
do_enter:		CALL	Enter_Current
			LD	A, (curline_length)
			CP	254
			JR	NC, Line_Too_Long
			LD	A, 13
			CALL	Insert_Char
			LD	A, 10
			CALL	Insert_Char
			CALL	Leave_Current		; Causes the line with the additional CR-LF to be put in the text.
			LD	HL, (num_lines)
			INC	HL
			LD	(num_lines), HL		; Increment number of lines.
			XOR	A
			LD	(cursor_col_max), A	; Move to the start of the line.
			LD 	IX, cur_lineaddr
			LD	BC, 1			; Move to the next line.
			CALL	Lines_Forward
			CALL	Adjust_Top
			CALL	Show_Screen		; Redraw the entire screen.
			JP	main_loop

; Exit key (^X or ESC). Prompt to save file if modified and exit editor.
do_exit:		CALL 	Leave_Current
			LD 	A, (file_modified)	
			AND 	A
			JR 	Z, exit_nosave
			LD      A, 12
			RST.LIL 10h
			LD	HL, s_ASKSAVE
			CALL	Print_String
			CALL	Read_Key
			CP	'n'
			JR	Z, exit_nosave
			CP	'N'
			JR	Z, exit_nosave
			CALL 	Save_File	
exit_nosave:		CALL	Restore_Screen
			LD	A, 12
			RST.LIL 10h			; Clear the screen
			LD	HL, 0
			RET

; Save key (^O). Save file, allow altering file name (save-as).
do_save:		CALL 	Leave_Current
			LD      A, 12
			RST.LIL 10h
			CALL	Save_File
			CALL	Show_Screen
			JP	main_loop
			
			
; Help key, show help screen
do_help:		CALL	Leave_Current
			LD	HL, s_HELP_Large
			CALL 	Print_String
			CALL 	Read_Key
			CALL	Show_Screen
			JP 	main_loop

; Cursor left key (^B), move one position to the left, Go to end of previous line if at start
do_cur_left:		CALL	Enter_Current
			LD	A, (linebuf_editpos)
			AND	A
			JR	Z, left_previous
			DEC	A
			LD	(linebuf_editpos),A	; We can decrement cursor position.
			CALL	Col_From_Editpos
			CALL 	Render_Current_Line
			;CALL    Show_Status
			CALL	Show_Cursor
			JP	main_loop
left_previous:		; From leftmost column: Move to the end of the previous line.
			LD	HL, (cur_lineno)
			LD	DE, 1
			AND 	A
			SBC 	HL, DE
			JP	Z, main_loop	; Don't move if already at start.	
			CALL	Leave_Current
			LD	IX, cur_lineaddr
			LD	BC, 1
			CALL 	Lines_Backward
			CALL	Adjust_Top
			JR 	do_line_end	; Go to end of now current line.	
			

; Cursor right key (^F), move one position to the right, Go to start of next line if at end
do_cur_right:		CALL	Enter_Current
			LD	A, (curline_length)
			DEC	A
			DEC	A
			LD	B, A
			LD	A, (linebuf_editpos)
			CP	A, B
			JR	Z, right_next
			INC	A
			LD	(linebuf_editpos),A	; We can increment cursor position.
			CALL	Col_From_Editpos
			CALL 	Render_Current_Line
			;CALL    Show_Status
			CALL	Show_Cursor
			JP	main_loop
right_next:		; from end of line, move to the start of the next line.	
			LD	HL, (cur_lineno)
			LD	DE, (num_lines)
			AND	A
			SBC 	HL, DE
			JP 	Z, main_loop				
			CALL	Leave_Current
			LD	IX, cur_lineaddr
			LD	BC, 1
			CALL 	Lines_Forward
			CALL	Adjust_Top
			; Continue into do_line_start
; Home key (^A), move to start of line
do_line_start:		XOR	A
			LD (cursor_col_max), A
			LD (cursor_col), A
			LD (linebuf_editpos), A
			LD (linebuf_scrolled), A
			CALL	Render_Current_Line	; Need this in case a scrolled line was shown.
			;CALL    Show_Status
			CALL	Show_Cursor
			JP	main_loop

; End key (^E), move to end of line.
do_line_end:		CALL 	Enter_Current
			LD	A, (curline_length) 
			DEC	A
			DEC	A
			LD	(linebuf_editpos), A
			CALL	Col_From_Editpos
			CALL	Render_Current_Line	; Can potentially scroll the current line.
			;CALL    Show_Status
			CALL	Show_Cursor
			JP	main_loop			

; Cursor up key (^P), move to previous line
do_cur_up:		CALL	Leave_Current
			LD	IX, cur_lineaddr
			LD	BC, 1
			CALL 	Lines_Backward
			CALL	Adjust_Top
			JP 	main_loop

; Cursor doen key (^N), move to next line
do_cur_down:		CALL	Leave_Current
			LD	IX, cur_lineaddr
			LD	BC, 1
			CALL 	Lines_Forward
			CALL	Adjust_Top
			JP 	main_loop

; Page Up key (^Y), move one screenful up
do_page_up:		CALL	Leave_Current
			LD	IX, cur_lineaddr
			LD	BC, 0
			LD	A, (Current_Rows)
			DEC	A
			DEC	A			; Current_Rows-2, number of lines on screen	
			LD	C, A
			CALL	Lines_Backward
			CALL	Adjust_Top
			JP	main_loop

; Page Down key (^V), move one screenful down
do_page_down:		CALL	Leave_Current
			LD	IX, cur_lineaddr
			LD	BC, 0
			LD	A, (Current_Rows)
			DEC	A
			DEC	A
			LD	C, A			; Current_Rows-2, number of lines on screen
			CALL	Lines_Forward
			CALL 	Adjust_Top
			JP	main_loop
			
; Adjust screen (^L) such that current line is shown in the centre.			
do_center:		CALL	Leave_Current
			LD	HL, (cur_lineaddr)
			LD	(top_lineaddr), HL
			LD	HL, (cur_lineno)
			LD	(top_lineno), HL	
			LD	IX, top_lineaddr	; Set top line to current.
			LD	BC, 0
			LD	A, (Current_Rows)
			DEC	A
			DEC	A
			SRL	A
			LD	C, A
			CALL	Lines_Backward		; Move top line half screen above current line.
			LD	HL, (cur_lineno)
			LD	DE, (top_lineno)
			AND	A				
			SBC     HL, DE				; Recompute cursor row as top may have hit line 1.
			LD	A, L
			LD 	(cursor_row),A
			CALL	Show_Screen
			JP	main_loop
			
; Goto line (^H), enter line number and move to that line.
do_goto:		CALL	Leave_Current
			CALL	Clear_BottomRow
			LD	HL, s_GOTO
			CALL	Print_String
			LD	HL, linebuf
			LD	E, 1
			LD 	BC, 10
			MOSCALL mos_editline
			LD	HL, linebuf
			CALL	ASC_TO_NUMBER
			PUSH	DE
			POP 	BC
			DEC  	BC
		
			LD	IX, cur_lineaddr
			LD	HL, (buf_start)
			LD	(IX+0), HL
			LD	HL, 1
			LD	(IX+3), HL		; Set line number to 1 at start of buffer
			CALL	Lines_Forward		; forwarn n-1 lines 
			XOR	A
			LD	(cursor_col), A
			CALL	Adjust_Top
			JP	main_loop

; Cut (^K) single line	
do_cut:			LD	A, (cut_continue)
			AND	A
			JR	NZ, @F
			LD	HL, (buf_end)
			LD	(cutbuf_start), HL	; Clear the old cutbuffer if not continuing.
			LD	HL, 0
			LD	(cutbuf_lines), HL
@@:			CALL	Enter_Current
			LD	A, (curline_length)
			PUSH 	AF
			XOR	A
			LD	(curline_length), A	; Set curline_length to 0, causing Leave_Current to replace current line by emptiness,
			CALL	Leave_NoRender		; Move current line (now of length 0) back.
			LD	HL, (cur_lineno)
			LD	DE, (num_lines)
			AND	A
			SBC	HL, DE			; Check if we are on last line
			JR	Z, cut_lastline
			DEC	DE
			LD	(num_lines), DE		; Store decreased number of lines.
			JR	cut_move
cut_lastline:		LD	HL, (text_end)		; We have deleted the last lien, which is not allowed, put an empty line in its place.
			LD	(HL), 13
			INC	HL
			LD	(HL), 10
			INC 	HL
			LD	(text_end), HL
cut_move:		POP 	AF			; Get original line length back.
			LD	(curline_length), A
			CALL	Cutbuffer_Add		; Add the line to the cut buffer.
			CALL	Show_Screen
			LD	A, 1
			LD 	(cut_continue), A
			JP	main_loop_continue

; Copy (^C) single line
do_copy:		LD	A, (cut_continue)
			AND	A
			JR	NZ, @F
			LD	HL, (buf_end)
			LD	(cutbuf_start), HL	; Clear the old cutbuffer if not continuing.
			LD	HL, 0
			LD	(cutbuf_lines), HL
@@:			CALL	Enter_Current
			CALL	Cutbuffer_Add		; Add the line to the cutbuffer.	
			CALL	Leave_Current		; CALL Leave_Current because we move to next line.
			LD	IX, cur_lineaddr
			LD	BC, 1
			CALL 	Lines_Forward		; Move to next line.
			CALL	Adjust_Top
			LD	A, 1
			LD 	(cut_continue), A
			JP	main_loop_continue

; Paste (^U) contents of cutbuffer
do_paste:		CALL	Leave_Current
			LD	HL, (cutbuf_start)
			PUSH 	HL			; Keep current line address in cut buffer on the stack during the loop.	
do_paste1:		POP	HL
			PUSH 	HL
			LD	DE, (buf_end)
			AND	A
			SBC	HL, DE
			JR	Z,  paste_end		; Did we reach the end of the paste buffer?  
			POP	HL
			PUSH 	HL			; stack: old_lineaddr 
			
			CALL	Next_LineAddr
			POP	DE						;
			PUSH	HL
			PUSH    DE			; stack: new_lineaddr old_lineaddr
			AND	A
			SBC	HL, DE
			LD	BC, 0
			LD	C, L			; Determine length of first line in cut buffer.
			LD	A, L			; Length goes to BC for LDIR and to curline_length	
			LD 	(curline_length), A
			XOR 	A
			LD	(oldcurline_length), A	; by setting oldcurline_length to 0, nothing gets overwritten, line gets inserted.
			POP 	HL			; source address
			LD	DE, linebuf		
			LDIR				; Copy line of cutbuf to linebuf.
			CALL	Leave_NoRender		; Cause line to be inserted in text.
			LD	HL, (num_lines)
			INC	HL
			LD	(num_lines), HL
			LD	BC, 1
			LD	IX, cur_lineaddr
			CALL	Lines_Forward		; Skip past line that got just inserted.
			JR	do_paste1					
paste_end:		POP 	HL
			CALL	Adjust_Top
			CALL	Show_Screen
			JP	main_loop

; Read a file (^R) and isert it into the text.
do_readfile:		LD	HL, (buf_end)
			LD	(cutbuf_start), HL	; Clear the cut buffer.
			CALL	Leave_Current
			CALL	Clear_BottomRow
			LD	HL, s_READFILE
			CALL	Print_String
			LD	HL, linebuf
			LD	E, 1
			LD 	BC, 40
			MOSCALL mos_editline
			LD 	HL, linebuf
			LD	DE, (text_end)
			CALL	Load_File
			CP 	0ffh
			JP	Z, Memory_Full
			AND	A
			JP	NZ, Load_Error
			LD	HL, (text_end)
			PUSH	HL
do_read1:		LD	HL, (cutbuf_lines)
			LD	A, H
			OR 	L
			JR	Z, paste_end		; Did cutbuf_lines reach zero?
			DEC	HL
			LD	(cutbuf_lines), HL	; Decrement cutbuf_lines
			POP	HL
			PUSH 	HL			; stack: old_lineaddr 			
			CALL	Next_LineAddr
			POP	DE						;
			PUSH	HL
			PUSH    DE			; stack: new_lineaddr old_lineaddr
			AND	A
			SBC	HL, DE
			LD	BC, 0
			LD	C, L			; Determine length of first line in inserted file
			LD	A, L			; Length goes to BC for LDIR and to curline_length	
			LD 	(curline_length), A
			XOR 	A
			LD	(oldcurline_length), A	; by setting oldcurline_length to 0, nothing gets overwritten, line gets inserted.
			POP 	HL			; source address
			LD	DE, linebuf		
			LDIR				; Copy line of read file to linebuf.
			CALL	Leave_NoRender		; Cause line to be inserted in text.
			LD	HL, (num_lines)
			INC	HL
			LD	(num_lines), HL
			LD	BC, 1
			LD	IX, cur_lineaddr
			CALL	Lines_Forward		; Skip past line that got just inserted.
			JR	do_read1					
		
; Find forward (^W)		
do_find:		CALL	Enter_Current
			CALL	Leave_Current
			CALL	Clear_BottomRow
			LD	HL, s_FIND
			CALL	Print_String
			LD	HL, findstring
			LD	E, 0
			LD 	BC, 40
			MOSCALL mos_editline
			LD	HL, (cur_lineaddr)
			LD	A, (linebuf_editpos)
			LD	DE, 0
			LD	E, A
			ADD	HL, DE			; start position to find the string.
			EXX
			LD	HL, (cur_lineno)	
			LD	DE, (cur_lineaddr)
			EXX
			LD	BC, (text_end)
			; The following loop starts seaching just after the current cursor position and wraps around 
			; at the start of the text file when reaching the end. It terminates when either the string is found or 
			; the original cursor position is reached again.
			; Register usage in the find loop:
			; HL pointer in text.
			; BC, text_end to compare against.
			; DE, scratch used in compare function.
			; HL' line number, DE' start address of current line.			
find_loop:		INC 	HL
			AND	A
			SBC 	HL, BC
			JR	NC, find_notfound
			; Reached end of text?	
			ADD	HL, BC			; Undo the last subtract.
			LD 	A, (HL)
			CP 	13			; At end of line?
			JR	NZ, @F
			INC	HL			; Increment past CR
			PUSH 	HL
			EXX	
			POP	DE			; Remember current line start
			INC	DE
			INC	HL			; Increment line number.
			EXX
			JR	find_loop
@@:			CALL	Compare
			JR 	NZ, find_loop			
find_found:		PUSH 	HL			; Stack find position
			EXX	
			LD	IX, cur_lineaddr
			LD	(IX+0), DE
			LD	(IX+3), HL
			EXX				; Set line number where string was found.
			CALL	Adjust_Top
			CALL	Enter_Current
			POP	HL
			LD	DE, (cur_lineaddr)
			AND	A
			SBC	HL, DE
			LD	A, L
			LD	(linebuf_editpos), A	; Compute editpos from find position.
			CALL	Col_From_Editpos
			CALL	Render_Current_Line
			CALL 	Show_Cursor
			JP	main_loop
find_notfound:		CALL	Clear_BottomRow
			LD	HL, s_NOTFOUND
			CALL	Print_String
			CALL	Show_Cursor
			JP	main_loop
			
; Find backward (^Q)
do_rfind:		CALL	Enter_Current
			CALL	Leave_Current
			CALL	Clear_BottomRow
			LD	HL, s_FIND
			CALL	Print_String
			LD	HL, findstring
			LD	E, 0
			LD 	BC, 40
			MOSCALL mos_editline
			LD	HL, (cur_lineaddr)
			LD	A, (linebuf_editpos)
			LD	DE, 0
			LD	E, A
			ADD	HL, DE			; start position to find the string.
			EXX
			LD	DE, (cur_lineno)	
			LD	HL, (cur_lineaddr)
			CALL	Next_LineAddr
			INC	DE
			EX	DE, HL
			EXX
			LD	BC, (buf_start)
			; The following loop starts seaching just before the current cursor position and terminates
			; when either the start of the text is reached (not found) or the string is found.
			; Register usage in the find loop:
			; HL pointer in text.
			; BC, buf_start to compare against.
			; DE, scratch used in compare function.
			; HL' line number, DE' start address of current line.			
rfind_loop:		DEC 	HL
			AND	A
			SBC 	HL, BC
			JR	C, find_notfound
			; Reached start of text?	
			ADD	HL, BC			; Undo the last subtract.
			LD 	A, (HL)
			CP 	10			; At end of previous line?
			JR	NZ, @F
			PUSH 	HL
			DEC	HL			; decrement past LF
			EXX	
			POP	DE			; Remember current line start
			INC	DE
			DEC	HL			; Decrement line number.
			EXX
			JR	rfind_loop
@@:			CALL	Compare
			JR 	NZ, rfind_loop			
rfind_found:		PUSH 	HL			; Stack find position
			EXX	
			LD	IX, cur_lineaddr
			LD	(IX+0), DE
			LD	(IX+3), HL
			EXX				; Set line number where string was found.
			LD	BC, 1
			CALL	Lines_Backward		; But go one line back.
			CALL	Adjust_Top
			CALL	Enter_Current
			POP	HL
			LD	DE, (cur_lineaddr)
			AND	A
			SBC	HL, DE
			LD	A, L
			LD	(linebuf_editpos), A	; Compute editpos from find position.
			CALL	Col_From_Editpos
			CALL	Render_Current_Line
			CALL 	Show_Cursor
			JP	main_loop	
			
; Insert special character (^T). Type two hex characters.
do_special:		CALL	Read_Key
			CALL 	UPPRC
			SUB	'0'
			JP	C, main_loop
			CP	10
			JR	C, @F
			SUB	7
@@:			ADD	A,A		; Convert character to hex digit
			ADD	A,A
			ADD	A,A
			ADD	A,A
			LD	E, A		; shift first digit 4 positions to left.
			CALL	Read_Key
			CALL	UPPRC
			SUB	'0'
			JP	C, main_loop
			CP	10
			JR	C, @F
			SUB	7		; convert character to hex digit
@@:			ADD	A,E		; Add to first digit.	
			CP 	07Fh
			JP	Z, main_loop	; Skip deelete
			CP	' '
			JP	C, main_loop
			JP	do_ins_char
			
			
; Clear the bottom row for a prompt, reset the cursor to start of that line.
Clear_BottomRow:	LD	A, 31
			RST.LIL	10h
			XOR 	A
			RST.LIL 10h
			LD	A, (Current_Rows)
			DEC	A
			RST.LIL 10h
			CALL	Clear_EOL
			LD	A, 13
			RST.LIL 10h
			RET

; Draw the entire edit screen, including top and bottom status lines.
Show_Screen:		LD	A, 12
			RST.LIL 10h			; Clear the screen
			CALL 	Inverse_Video		; Print top row
			LD	HL, s_NAME
			CALL    Print_String
			LD	HL, filename
			CALL	Print_String
			CALL	Clear_EOL
			CALL 	True_Video
			
			LD	HL, (top_lineaddr)
			LD	A, (Current_Rows)
			DEC	A
			DEC	A
			LD	B, A ; Number of lines to draw.
			LD	E, 0
Show_loop:		CALL	Render_Line
			INC	E
			CALL	Next_LineAddr
			JR	C, Show_Status			; Exit loop if no more lines in buffer
			DJNZ	Show_loop					
; Call this when you want to update the status line, but not redraw the whole screen.
Show_Status:			; Show status line at bottom of screen
			LD	A, 31
			RST.LIL 10h
			LD	A, 0
			RST.LIL 10h
			LD	A, (Current_Rows)
			DEC	A
			RST.LIL 10h			; Bottom row
			CALL 	Inverse_Video
			LD	HL, s_LINE
			CALL	Print_String
			LD	HL, (cur_lineno)
			CALL	Print_Decimal
			LD 	A, '/'
			RST.LIL 10h
			LD	HL, (num_lines)
			CALL	Print_Decimal
			LD 	A, ','
			RST.LIL 10h
			LD 	A, ' '
			RST.LIL 10h			
			LD	HL, (text_end)
			LD	DE, (buf_start)
			AND	A
			SBC 	HL, DE			; Text size in bytes
			CALL	Print_Decimal
			LD 	A, '/'
			RST.LIL 10h
			LD	HL, (cutbuf_start)
			LD	DE, (buf_start)
			AND	A
			SBC	HL, DE			; Total buffer size in bytes
			CALL	Print_Decimal
			LD 	HL, s_HELP_Small
			CALL 	Print_String
			LD	A, (file_modified)
			AND	A
			JR	Z, @F
			LD	A, '*'			; Show an asterisk when file is modified.
			RST.LIL	10h
			LD	A, ' '
			RST.LIL 10h
@@:			LD	HL, (cutbuf_lines)
			CALL	Print_Decimal
			CALL	Clear_EOL
			CALL	True_Video
; Position (blinking) text cursor at correct location.			
Show_Cursor:		LD	A, 31
			RST.LIL 10h
			LD	A, (cursor_col)
			RST.LIL 10h
			LD	A, (cursor_row)
			INC	A
			RST.LIL 10h
			RET
			
; Render a text line on the screen.
;	Inputs: HL pointer to line in memory.
;		E row index. Will be used to determine if this is the current line.
Render_Line:		PUSH 	HL
			PUSH 	BC
			LD	C, 0
			LD	A, (cursor_row)
			CP	E
			JR	Z, Render_Current_Line2	; Separate routine to render the current line.
			LD	A, (Current_Cols)
			LD	B, A			
Render_Loop:		LD	A, (HL)
			INC	HL
			CP	A, 13     ; At end of line.
			JR	Z, Render_CR
			CP	A, 9
			JR	NZ, Render_Notab
Render_Tab:		DEC	B
			JR	Z, Render_Rightmost
			LD	A, 32
			RST.LIL	10h		; Print a space
			INC	C
			LD	A, C
			AND	07h
			JR	NZ, Render_Tab
			JR	Render_Loop
Render_Notab:		DEC     B
			JR	Z, Render_Rightmost
			INC 	C
			RST.LIL	10h
			JR 	Render_Loop			
			; Reached rightmost column before all characters printed:
			; Print inverse > to indicate that there's more.
Render_Rightmost:	CALL 	Inverse_Video
			LD	A, '>'
			RST.LIL 10h
			CALL	True_Video
			POP	BC
			POP	HL
			RET
Render_CR:		CALL	Print_CRLF
			POP 	BC
			POP 	HL
			RET

; Render the current line.
; Determine cursor colum and also allow to scroll the line to the left
; if the cursor position is beyond the end of the screen.

Render_Current_Line:	PUSH 	HL
			PUSH 	BC
			LD	A, 31
			RST.LIL 10h
			XOR 	A
			RST.LIL 10h				; Position the cursor at start of current row.
			LD	A, (cursor_row)
			INC	A
			RST.LIL 10h
Render_Current_Line2:	LD	A, (curline_stat)
			AND 	A
			LD	HL, (cur_lineaddr)
			JR	Z, @F
			LD	HL, linebuf
			LD	A, (linebuf_scrolled)
			LD	BC, 0
			LD	C, A
			ADD	HL, BC				; Take the linebuf address, add the number of bytes the line was scrolled left.
@@:			LD	C, 0
			LD	A, (Current_Cols)
			LD	B, A
			LD	A, (linebuf_scrolled)
			AND	A
			JR	Z, Render_Loop2
			PUSH 	HL
			INC	C
			DEC	B				; Put inverted < marker in leftmost column if line scrolled horizontally.	
			CALL	Inverse_Video
			LD	A, '<'
			RST.LIL 10h
			CALL  	True_Video
			POP	HL
Render_Loop2:		LD	A, (HL)
			INC	HL
			CP	A, 13     ; At end of line.
			JR	Z, Render_CR2
			CP	A, 9
			JR	NZ, Render_Notab2
Render_Tab2:		DEC	B
	
			JR	Z, Render_Rightmost2
			LD	A, 32
			RST.LIL	10h		; Print a space
			INC	C
			LD	A, C
			AND	07h
			JR	NZ, Render_Tab2
			JR	Render_Loop2
Render_Notab2:		DEC     B
			JR	Z, Render_Rightmost2
			INC 	C
			RST.LIL	10h
			JR 	Render_Loop2			
Render_Rightmost2:	LD	A, (cursor_col_max)
			LD	(cursor_col), A
			JP	Render_Rightmost
Render_CR2:		LD	A, (cursor_col_max)
			CP	C
			JR	C, @F
			LD	A, C
			
@@:			LD	(cursor_col), A		; Curor col is minimum of cursor_col_max and end-of-line column.
@@:			LD	A, ' '			
			RST.LIL 10h			; Clear with spaces to end of line instead of CR
			DJNZ   	@B
			POP 	BC
			POP 	HL
			RET
			
; Advance HL to next line address
; Return carry if at end of file
Next_LineAddr:		PUSH 	DE
			PUSH 	HL			; Keep HL just in case we are at end.
@@:			LD 	A, (HL)	
			INC	HL
			CP 	10
			JR 	NZ, @B
			LD  	DE, (text_end)
			AND 	A
			SBC 	HL, DE
			JR 	Z, Next_AtEnd
			ADD 	HL, DE
			POP	DE
			POP	DE
			AND 	A
			RET
			
Next_AtEnd:		POP	HL
			POP	DE			
			SCF	
			RET
; Move the combination of line addres and line number n lines forward in the text.
; Never go beyond the last line.
;
; Inputs: IX is the address of either cur_lineaddr or top_lineaddr
;	  BC is the number of lines to move forward (limited to 16 bits).
Lines_Forward:		LD	A, C
			OR	B
			RET	Z			; Do nothing if 0 lines to forward
			PUSH 	HL
			PUSH 	BC
			PUSH 	DE
			LD	HL, (IX+0) 		; Get line address
			LD	DE, (IX+3)		; Get line number
@@:			CALL 	Next_LineAddr
			JR	C, Forward_End
			INC	DE
			DEC	BC
			LD	A, C
			OR	B
			JR	NZ, @B
Forward_End:		LD	(IX+0), HL
			LD	(IX+3), DE
			POP	DE
			POP	BC
			POP	HL
			RET

; Move the combination of line addres and line number n lines backward in the text.
; Never go beyond line 1.
;
; Inputs: IX is the address of either cur_lineaddr or top_lineaddr
;	  BC is the number of lines to move backward (limited to 16 bits)
Lines_Backward:		PUSH 	HL
			PUSH	BC
			PUSH	DE
			LD	HL, (IX+0)		; Get line address
			EXX
			LD	HL, (IX+3)		; Get line number
			LD	DE, 2			; Value to compare line number with.
			EXX		
Backward_Loop:		EXX
			AND	A
			SBC	HL, DE
			JR	NZ, @F			
			; Line number equals two, special case: previous line is at buffer start.
			LD	HL, 1
			EXX
			LD	HL, (buf_start)	
			JR	Backward_End
@@:			JR	NC, @F
			; Line number equals one, special case: do not move back
			LD	HL, 1	
			EXX
			JR	Backward_End
@@:			; Regular case, scan for the previous line ending.
			INC 	HL			; subtract had decremented by 2, add 1 to decrement by just 1.
			EXX
			DEC	HL			; Decement the pointer till before the LF immediately preceding the line.
@@:			DEC	HL			
			LD	A, (HL)
			CP	A, 10
			JR	NZ, @B
			INC	HL			; Increment just beyond LF of previous line.			
			DEC	BC
			LD	A, C
			OR 	B
			JR 	NZ, Backward_Loop
Backward_End:		
			LD	(IX+0), HL
			EXX	
			LD	(IX+3), HL
			EXX
			POP 	DE
			POP	BC
			POP	HL
			RET

; After moving the current line, adjust the top line, so the current line will be in view. 
; Also recompute cursor_row
Adjust_Top:		LD	HL, (cur_lineno)
			LD	DE, (top_lineno)
			AND	A
			SBC     HL, DE
			JR	C, Adjust_Top_Change		; Current line no is less than top
			LD	A, (Current_Rows)
			LD	DE, 0
			DEC 	A
			DEC     A
			LD	E, A
			AND	A
			SBC	HL, DE
			JR	NC, Adjust_Top_Change		; Current line is too far ahead of top.
			ADD	HL, DE
			LD	A, L
			LD 	(cursor_row),A
			CALL	Render_Current_Line
			CALL	Show_Status
			RET
			
Adjust_Top_Change:	; Current line is out of visible screen, adjust top line.
			LD	HL, (cur_lineno)
			LD	(top_lineno), HL
			LD	HL, (cur_lineaddr)
			LD	(top_lineaddr), HL		; Make top equal to current line addr
			LD	A, (Current_Rows)
			DEC	A
			DEC	A
			SRL	A
			LD	BC, 0
			LD	C, A				; Move the top half a screen down.
			LD 	IX, top_lineaddr
			CALL	Lines_Backward
			LD	HL, (cur_lineno)
			LD	DE, (top_lineno)
			AND	A				
			SBC     HL, DE				; Recompute cursor row as top may have hit line 1.
			LD	A, L
			LD 	(cursor_row),A
			CALL	Show_Screen
			RET
	
; Compare text at HL against zero terminated string in findstring.
; Return Z if strings match, NZ otherwise.
; Clobbers: A, DE
Compare:		PUSH 	HL
			LD	DE, findstring
@@:			LD	A, (DE)
			AND	A
			JR	Z, Compare_End	; reached the end of findstring, compare succeeded.
			CP	(HL)
			JR	NZ, Compare_End	; mismatch found, compare failed.
			INC	HL
			INC	DE
			JR	@B
Compare_End:		POP	HL
			RET
	
; Copy the current line to the line buffer, to be edited there.	
Enter_Current:		LD	A, (curline_stat)
			AND	A
			RET	NZ	; Line already in line buffer, do nothing.
			LD	B, 0
			LD	HL, (cur_lineaddr)
			LD	DE, linebuf
@@:			LD	A, (HL)
			LD 	(DE), A
			INC	HL
			INC 	DE
			INC	B
			CP	10
			JR	NZ, @B
			LD	A, B
			LD	(curline_length), A
			LD	(oldcurline_length), A
			LD	A, 1
			LD	(curline_stat), A
			; Now derive the edit position from the current cursor column.
			; This is somewhat the inverse of Col_From_Editpos
			; HL pointer into linebuf
			; C is screen column
			; B is editpos
			; D is cursor column
			LD	HL, linebuf
			LD	A, (cursor_col_max)
			LD	D, A
			LD	C, 0
			LD	B, 0
Enter_Current_Loop:	LD	A, (HL)	
			INC	HL
			CP	13
			JR	Z, Enter_Current_End
			CP	9
			JR	NZ, Enter_Current_Notab
@@:			LD	A, D
			CP	C
			JR	Z, Enter_Current_End
			INC	C
			LD	A, C
			AND 	7
			JR	NZ, @B
			INC	B
			JR	Enter_Current_Loop
Enter_Current_Notab:	LD	A, D
			CP	C
			JR	Z, Enter_Current_End
			INC	B
			INC	C
			JR	Enter_Current_Loop
Enter_Current_End:	LD 	A, B
			LD	(linebuf_editpos), A
			RET
	
; Put any locally edited line in linebuf back into the text buffer.
Leave_Current:		LD 	A, (curline_stat)
			AND	A
			RET	Z			; Line not in edit buffer, do nothing.	
			XOR	A
			LD 	(linebuf_scrolled), A
			CALL	Render_Current_Line	; Re-render line with no horizontal scroll.
			LD 	A, (curline_stat)
			CP	A, 1
			LD	A, 0
			LD	(curline_stat), A
			RET	Z			; Return if line not modified.
Leave_NoRender:		; Get here if we do know the line buffer was modified aand we do not want to render
			; At this point we will put the line from linebuf back into the main text.
			; curline_length can be zero, for example when deleting an empty line.
			XOR	A
			LD	(curline_stat), A
			INC	A
			LD	(file_modified), A	; Mark file as modified
			LD	A, (oldcurline_length)	
			LD	C, A
			LD	A, (curline_length)
			SUB	C
			JP	Z, Leave_Moveline 	; Lengths are the same, ready to move the line.
			JP	C, Leave_Movedown 	; New line smaller than old one, move down
			;  New line longer than old one, move text up, but first check it it fits.
			LD	BC, 0
			LD	C, A			; Length difference in BC
			LD	HL, (text_end)
			ADD	HL, BC
			EX	DE, HL
			LD	HL, (cutbuf_start)	
			AND	A
			SBC	HL, DE			; cutbuf_start < text_end + size_increse?
			JP	C, Memory_Full
			LD	HL, (cur_lineaddr)
			LD	DE, 0
			LD	A, (curline_length)
			LD	E, A
			ADD	HL, DE			; Destination address.
			PUSH	HL
			LD	HL, (cur_lineaddr)
			LD	DE, 0
			LD	A, (oldcurline_length)
			LD	E, A
			ADD	HL, DE			; Source address.
			PUSH 	HL
			LD	DE, (text_end)
			EX	DE, HL
			AND	A
			SBC	HL, DE			; Length
			JR	Z, Leave_Move_Pop2	; Do not move data if length=0
			PUSH 	HL
			POP	BC
			POP	HL
			POP	DE
			ADD	HL, BC
			DEC	HL
			EX	DE, HL
			ADD	HL, BC
			DEC 	HL
			EX	DE, HL
			LDDR	
			JR	Leave_Moveline
			
Leave_Movedown:		; Move text down
			LD	HL, (cur_lineaddr)
			LD	DE, 0
			LD	A, (curline_length)
			LD	E, A
			ADD	HL, DE			; Destination address.
			PUSH	HL
			LD	HL, (cur_lineaddr)
			LD	DE, 0
			LD	A, (oldcurline_length)
			LD	E, A
			ADD	HL, DE			; Source address.
			PUSH 	HL
			LD	DE, (text_end)
			EX	DE, HL
			AND	A
			SBC	HL, DE			; Length
			JR	Z, Leave_Move_Pop2	; Do not move data if length=0
			PUSH 	HL
			POP	BC
			POP	HL
			POP 	DE			; Get registers right.
			LDIR	
			JR	Leave_Moveline
Leave_Move_Pop2:	POP 	HL
			POP	HL
Leave_Moveline:		; Move the line from linebuf to text.  
			LD	A, (oldcurline_length)	; But first adjust text_end
			LD	BC, 0
			LD	C, A
			LD	A, (curline_length)
			LD	DE, 0
			LD	E, A
			LD	HL, (text_end)
			ADD	HL, DE
			AND	A
			SBC	HL, BC
			LD	(text_end), HL	
			LD	A, (curline_length)
			AND 	A
			RET	Z		; Zero length line, so do not copy anything
			LD	DE, (cur_lineaddr)
			LD	HL, linebuf
			LD	BC, 0
			LD	C, A
			LDIR
			RET
	
; Compute the screen column from the edit position. 
;	Take into account tab characters and lines that have to be scrolled left.
; Pre: 	Line is in line buffer.
; Post: cursor_col_max contains the desired cursor position.
;       linebuf_scrolled contains the number of positions the current line is scrolled to the left.  
;	Register usage:
;	A character to read, temporary register
;	B edit position, count down
;	C screen column
;	D screen width usable for characters (with - 1).
;	E count of characters since last scroll left.
	
Col_From_Editpos:	LD	HL, linebuf
			XOR 	A
			LD	(linebuf_scrolled), A
			LD	A, (Current_Cols)
			LD	D, A
			DEC	D
			LD	C, 0
			LD	E, 0
			LD	A, (linebuf_editpos)
			LD	B, A
			AND 	A
			JR	Z, Col_From_Editpos_End
Col_From_Editpos_Loop:	LD	A, (HL)
			INC	HL
			INC	E
			CP	9			; Need to read character only to find out it's a tab
			JR	NZ, Col_From_Editpos_Notab
@@:			INC	C
			LD	A, D
			CP	C
			JR	Z, Col_From_Editpos_Scroll
			LD	A, C
			AND     7
			JR 	NZ, @B
			JR	Col_From_Editpos_Cont
Col_From_Editpos_Notab:	INC	C
			LD 	A, D
			CP	C
			JR	Z, Col_From_Editpos_Scroll			
Col_From_Editpos_Cont:	DJNZ	Col_From_Editpos_Loop
Col_From_Editpos_End:	LD	A, C
			LD	(cursor_col_max), A
			RET
Col_From_Editpos_Scroll: ;Reached the end of the screen.scroll line left
			LD	A, (linebuf_scrolled)
			ADD	A, E
			LD	(linebuf_scrolled), A
			LD	E, 0
			LD	C, 1			; Start next line in column 1
			JR	Col_From_Editpos_Cont
			
; Insert character (in A) into linebuf at edit position. Increment both current_length and linebuf_editpos.
; Mark line as modified.
; Pre: current_length < 255
;      linebuf_editpos is strictly less than current_length
;      current line is in linebuf
Insert_Char:		EX	AF, AF'
			LD	BC, 0
			LD	A, (linebuf_editpos)
			LD	C, A
			LD	A, (curline_length)
			SUB	C
			LD	C, A			; Number of bytes to move in BC.
			LD	HL, linebuf
			LD	DE, 0
			LD	A, (linebuf_editpos)
			LD	E, A
			ADD	HL, DE
			PUSH 	HL			; Keep byte position to insert character
			ADD	HL, BC
			PUSH 	HL
			POP	DE
			DEC	HL
			LDDR				; Move the bytes
			POP	HL
			EX	AF, AF'
			LD	(HL), A
			LD	A, (linebuf_editpos)
			INC	A
			LD	(linebuf_editpos), A
			LD	A, (curline_length)
			INC	A
			LD	(curline_length), A
			LD	A, 2
			LD	(curline_stat), A
			RET

; Delete character in linebuf at edit position. Decrement linebuf_editpos.
; Mark line as modified
; Pre: linebuf_editpos is not at the end of the line.(not pointing to CR)
;      current line is in linebuf
Delete_Char:		LD	BC, 0		
			LD	A, (linebuf_editpos)
			LD	C, A
			LD	A, (curline_length)
			SUB	C
			DEC	C
			LD	C, A			; Number of bytes to move in BC.
			LD	HL, linebuf
			LD	DE, 0
			LD	A, (linebuf_editpos)
			LD 	E, A
			ADD	HL, DE
			PUSH	HL
			POP	DE
			INC	HL
			LDIR				; Move the bytes 
			LD	A, (curline_length)
			DEC	A
			LD	(curline_length), A
			LD	A, 2
			LD	(curline_stat), A
			RET


; Append line in linebuf to the cut buffer. Move existing cut buffer lines down in RAM (first check space).
; Pre: linebuf contains the line to be inserted, curline_Lnegth is its length.

Cutbuffer_Add:		lD	HL, (cutbuf_start)
			LD	DE, 0
			LD	A, (curline_length)
			LD	E, A
			AND	A
			SBC 	HL, DE
			PUSH	HL				; Push new start of cut buffer
			LD	DE, (text_end)
			AND	A
			SBC	HL, DE	
			JP	C, Memory_Full			; Check for memory full.
			POP	DE
			LD	HL, (buf_end)
			LD	BC, (cutbuf_start)
			AND	A
			SBC 	HL, BC
			PUSH 	HL
			POP 	BC
			LD	HL, (cutbuf_start)
			LD	(cutbuf_start), DE
			JR	Z, Cutbuffer_Add_Linebuf
			LDIR					; Move cutbuffer down			
Cutbuffer_Add_Linebuf:	; Note: DE points to the address at the end of the cut buffer, just where we want it.
			LD	HL, linebuf
			LD	BC, 0
			LD	A, (curline_length)
			LD	C, A
			LDIR	
			LD	HL, (cutbuf_lines)
			INC	HL
			LD	(cutbuf_lines), HL
			RET

; Load a file into the edit buffer.
; Remove control characters and normalize all line endings into CR-LF. Make sure a trailing CR-LF is present.
; This function is used at the start to load the file initially and also by the 'insert' file command.
;
; Input: HL is address of filename
;	 DE is start address of buffer to use (end is buf_end variable).
; Output: cutbuf_lines contains the number of lines loaded (including any appended trailing CR-LF).
;         load_filesize contains the size of the loaded text (after normalization) in bytes.'
;	  A=0 if the file is loaded correctly of if the file was not found (new file to be created)
;	      path not found will be fatal as file cannot be saved later.
;	  A=0FFh fs buffer was full

;	  Other A: fatal error during load.	
Load_File:		PUSH 	HL
			LD (load_start), DE
			; First clear buffer with zeros.
			LD	HL, (buf_end)
			AND 	A
			SBC	HL, DE		; Compute the length of the buffer.
			AND 	A
			LD	BC, 2
			SBC	HL, BC
			JP 	C, Load_Small_Pop; Buffer is too small.
			ADD	HL, BC		; Undo subtraction.
			PUSH 	HL
			POP	BC		; Buffer size to BC
			PUSH    BC
			LD 	HL, 0
			ADD 	HL, DE		; HL and DE point to start of buffer
			XOR 	A
			LD 	(HL), A
			INC	DE
			DEC	BC
			LDIR			; Clear the buffer
			; Load file
			POP	BC
			LD	DE, (load_start)
			POP	HL
			PUSH    BC
			LD 	C,fa_read
			MOSCALL	mos_fopen
			EX	DE, HL
			POP	DE
			LD	C, A
			AND 	A 
			JP	Z, Load_Empty	; File not found, return just an empty buffer.			
			MOSCALL mos_fread
			MOSCALL mos_fclose
			; scan from top of buffer down to find the text and move it to the end of buffer.
			; doing all translation (LF to CR-LF, remove control chars) on-the fly. Count lines.
			; Also ensure that no lines are >255 chars including CR-LF by inserting extra line endings.
			; 
			; Registers used during this loop:
			;     HL source pointer
			;     DE destination pointer. It has to remain strictly greater than HL
			;     BC start of buffer.
			;     HL' line start
			;     C'  flags: bit 0 set when last character is LF, do not treat next CR as separate line.
			;     B'  down counter to force insertion of CRLF when decrementd to 0.
			;     AF' used by helper function while insertin CRLF
			LD	HL, (buf_end)
			DEC 	HL
			LD	A,(HL)
			AND 	A
			JP 	NZ, Load_Small	; If no zero at end of buffer, buffer is too small.
			LD	DE, 0
			EX	DE, HL
			ADD	HL, DE		; Copy HL to DE
			LD	BC, (load_start)
			DEC	HL
			EXX
			LD	HL, 0
			LD	BC, 0100h
			EXX	
Load_Scan_EOF:		LD	A, (HL)			
			AND	A
			JR	NZ, Load_Scan_Loop
			AND	A
			SBC	HL, BC			; Check if we reached the start of the buffer.
			JR	Z, Load_Empty
			ADD	HL, BC
			DEC	HL
			JR	Load_Scan_EOF
			; We found the last non-null character in the loaded file. Loop start with checking character in A
Load_Scan_Loop:		CP	A, 10
			JR	NZ, @F
			EXX	
			LD	C, 1
			EXX
			CALL	Load_Insert_CRLF
			JR	Load_Skip_Char
@@:			CP	A, 13
			JR	NZ, @F
			EXX
			BIT	0, C
			LD	C, 0
			EXX
			JR	NZ, Load_Skip_Char
			CALL	Load_Insert_CRLF
			JR	Load_Skip_Char
@@:			EXX
			DEC	B
			EXX
			CALL	Z, Load_Insert_CRLF	; Force CRLF if line too long.
			; This will also be called when file did not end in CRLF.
			CP	127
			JR	Z, Load_Skip_Char	; Skip DEL
			CP	9
			JR	Z, @F			; TAB is okay.
			CP	32
			JR	C, Load_Skip_Char	; Skip control characters.
@@:			LD	(DE), A
			DEC	DE
			AND	A
			SBC	HL, DE
			JR	Z, Load_Small		; Check that destination does not run into source.
			ADD	HL, DE
Load_Skip_Char:		DEC	HL
			AND	A
			SBC	HL, BC			; Check we reached the bottom of the buffer.
			JR	C, Load_End
			ADD	HL, BC
			LD	A, (HL)			; Load next character
			JR 	Load_Scan_Loop
			
			
Load_End:		; We normalized the entire file and stored it at the end of the buffer.
			EXX
			LD (cutbuf_lines), HL		; Store number of lines
			EXX
			INC 	DE
			; Move text back to start of buffer.
			LD	HL, (buf_end)
			AND 	A
			SBC	HL, DE			; Compute text size
			LD	(load_filesize), HL
			PUSH	HL
			POP 	BC
			LD	HL, (load_start)
			EX	DE, HL
			LDIR
			XOR	A
			RET
Load_Small_Pop:		POP	HL			
Load_Small:		LD	A, 0ffh		; Buffer too small, we did not load.
Load_Fatal:		RET
Load_Empty:		; File not found, create a text with just an empty line with trailing CR-LF
			LD 	DE, (load_start)
			LD	A, 13
			LD	(DE), A
			INC	DE
			LD	A, 10
			LD	(DE), A
			LD	HL, 1
			LD	(cutbuf_lines), HL
			INC	HL
			LD	(load_filesize), HL
			XOR     A
			RET
			
; Helper function of Load_File
Load_Insert_CRLF:	EX	AF, AF'		; Make sure to keep character in A
			LD	A, 10
			LD 	(DE), A
			DEC	DE
			AND	A
			SBC	HL, DE
			JR	Z, Load_Insert_CRLF_Full
			ADD	HL, DE
			LD	A, 13
			LD 	(DE), A
			DEC	DE
			AND	A
			SBC	HL, DE
			JR	Z, Load_Insert_CRLF_Full
			ADD	HL, DE
			EXX	
			INC 	HL		; increment line counter
			LD	B, 0FDh		; Allow 253 chars to be inserted before next forced newline.
			EXX
			EX	AF, AF'
			RET
			
Load_Insert_CRLF_Full:	POP 	HL 		; Get rid of return address
			JR 	Load_Small

; Save the curreent edit buffer to a file.
; Prompt for a file name (the one originally loaded is the default).
; Delete teh old *.bak file, rename the original file into *.bak and save the new file under the original name.
Save_File:		; Let the user edit the file name to be saved.
			LD	HL, s_FILENAME
			CALL	Print_String
			LD	HL, filename
			lD	E, 0
			LD	BC, 256
			MOSCALL mos_editline
			CALL	Print_CRLF
			; Check if the file exists
			LD	HL, filename
			LD      C, fa_read
			MOSCALL	mos_fopen
			AND	A
			JR	Z, Save_Nobackup
			MOSCALL mos_fclose
			; Create the backup filename.
			LD	HL, filename
			LD	DE, linebuf
			LD	BC, 0
Save_Loop1:		LD	A, (HL)
			CP 	'.'
			JR	NZ, @F
			LD	C, B		; Save the offset of the last '.' char
@@:			INC 	B
			LD	(DE),A
			INC	HL
			INC	DE
			AND	A
			JR	NZ, Save_Loop1
			LD	A, C
			AND 	A
			JR 	Z, Save_Nosuffix
			LD	DE, linebuf
			LD	HL, 0
			LD	L, A
			ADD	HL, DE
			EX  	DE, HL
			INC 	DE
Save_Nosuffix:		DEC   	DE
			LD	HL, s_BAKSFX
@@:			lD	A, (HL)
			LD	(DE), A
			INC	HL
			INC 	DE
			AND	A
			JR 	NZ, @B
			; Try to delete the backup file.
			LD 	HL, linebuf
			MOSCALL mos_del
			; Try to rename the old file into the backup file.
			LD	HL, filename
			LD	DE, linebuf
			MOSCALL mos_ren
			; Save the file
Save_Nobackup:		LD	DE, (buf_start)
			LD	HL, (text_end)
			AND	A
			SBC	HL, DE
			PUSH 	HL
			POP	BC
			LD	HL, filename
			MOSCALL	mos_save
			AND 	A
			JR      Z, Save_End
			LD      HL, s_SAVEERR
			CALL    Print_String
			CALL    Read_Key
			CP	'n'
			JR	Z, Save_End
			CP	'N'
			JR	Z, Save_End
			JP	Save_File
Save_End:		XOR	A
			LD	(file_modified), A		; Mark file as not modiffied.
			RET

; Wait for a keypress to be available and read a key from the keyaboard.
; Use modifier bits to translate cursor keys to different control codes from the ones
; they ware mapped to.
Read_Key:		PUSH	IX
			PUSH 	BC
			MOSCALL mos_sysvars
Read_Key1:
			MOSCALL mos_getkey
			LD      B,   A                  ; Store returned ASCII code.
			LD      A, (IX+sysvar_vkeycode)
			LD      C, 2			; Want to translate cursor-left to ^B
			CP	154			
			JR	Z, Key_Translate
			LD	C, 6                    ; Want to translate cursor-right to ^F
			CP	156                      
			JR	Z, Key_Translate            
			LD	C, 16			 ; Want to translate cursor-up to ^P
			CP      150
			JR	Z, Key_Translate            
			LD	C, 14			 ; Want to translate cursor-down to ^N
			CP      152
			JR	Z, Key_Translate            
			LD	C, 1			 ; Want to translate Home to ^A
			CP      133
			JR	Z, Key_Translate            
			LD	C, 5			 ; Want to translate End to ^E
			CP      135
			JR	Z, Key_Translate            
			LD	C, 25			 ; Want to translate Page-Up to ^Y
			CP      146
			JR	Z, Key_Translate            
			LD	C, 22			 ; Want to translate Page-Down to ^V
			CP      148
			JR	Z, Key_Translate            
			LD	C, 4			 ; Want to translate Delete to ^D
			CP      130
			JR	Z, Key_Translate            
			; None of the special keys.
			LD	A, B                     ; Used the saved 'ascii' value.
			OR      A
			JR 	Z, Read_Key1             ; Is 'ascii' code 0?
			JR	Key_End
Key_Translate:		LD	A, C
Key_End:		PUSH    AF
			LD	A, 15			  ; Switch off paged mode, in case we pressed ^N
			RST.L   10h
			POP     AF  
			POP	BC
			POP 	IX
			RET

; Text messages
;
s_ERROR_BUF:		DB 	"File buffer full,\r\n", 0	
s_FATAL:		DB	"Fatal error loading file\r\n", 0
s_USAGE:		DB	"Usage: nano <file> [<bufaddr> [<startline.]]\r\n", 0
s_BADADDR:		DB	"Bad buffer address\r\n", 0	

s_NAME:			DB	"Editor for Agon ",0
s_LINE:			DB	"Line ",0
s_HELP_Small:		DB	" bytes -- Esc to exit, Ctrl-G for help ",0

s_HELP_Large:		DB      12, "Text editor for Agon v0.11, Copyright 2023-2024, L.C. Benschop\r\n"
			DB	"\r\n"
			DB  	"Cursor movement:\r\n"	
			DB	"Ctrl-B or cursor left, Ctrl-F or cursor right\r\n"
			DB	"Ctrl-P or cursor up, Ctrl-N or cursor down\r\n"
			DB	"Ctrl-Y: page up, Ctrl-V: page down\r\n"
			DB	"Ctrl-A; start of line, Ctrl-E: end of line\r\n"
			DB	"Ctrl-L, redraw screen with current line in centre\r\n"
			DB	"Ctrl-H goto line (enter number)\r\n"
			DB	"\r\n"
			DB	"Delete:\r\n"
			DB	"Backspace: delete to left, Control-D: delete to right\r\n"
			DB	"\r\n"
			DB	"Cut and paste:\r\n"
			DB	"Ctrl-K cut current line, repeat to cut block of multiple lines\r\n"
			DB	"Ctrl-C copy current line, repeat to copy block of multiple lines\r\n"
			DB	"Ctrl-U paste cut/copied lines, can be repeated to paste multiple times\r\n"
			DB	"\r\n"
			DB	"Find:\r\n"
			DB	"Ctrl-W: find forward, Ctrl-Q: find backward\r\n"
			DB	"\r\n"
			DB	"Other:\r\n"
			DB	"Ctrl-R: insert file before current line, Ctrl-O to save file\r\n"
			DB	"Ctrl-X or ESC: Exit editor (ask to save)\r\n"
			DB	"Ctrl-T followed by two hex digits: insert special character\r\n"
			DB	"\r\n"
			DB	"Press any key to return to editor.",0
s_ASKSAVE:		DB	"Save file (Y/n) \r\n",0
s_FILENAME:		DB	"Save file name: ",0
s_READFILE:		DB	"Read file name: ",0
s_LOADERR:		DB	"Error loading file.",0
s_SAVEERR:		DB	"Error saving file, retry? (Y/n)\r\n",0	
s_BAKSFX:		DB	".bak",0
s_GOTO:			DB	"Goto line: ",0
s_FIND:			DB	"Find: ",0
s_NOTFOUND:		DB	"Not found!",0	
s_LINETOOLONG:		DB	"Line too long!",0
s_NOMEM:		DB	"Buffer full!",0
; RAM variables

Saved_Mode:		DS	1		; video mode in which editor started, so we can return to that mode.
Current_Mode:           DS      1               ; Current video mode.
Current_Rows:		DS 	1		; Number of rows in current video mode
Current_Cols:		DS	1		; Number of columns in current video mode.
filename:		DS 	256		; Name of file being edited.		
linebuf:		DS 	256		; Buffer to store current line being edited, or filename.
findstring:		DS 	40		; Buffer to store find string
; Test buffer layout
;
; buf_start...text_end-1       Text to be edited.
; text_end...cutbuf_sart-1     Free space
; cutbuf_start...cutbuf_end-1  Cut buffer
;
; We maintain the edited text as a contiguous string. An edited line is moved into the line buffer and edited there.
; So as long as you type characters on the same line, the main text is not modified and no large data moves are required.
; When you visit another line, the modified line is inserted back into the main text and then it may need moving up/down.
buf_start:		DS 3			; Start address of edit buffer
buf_end:		DS 3			; End address of edit buffer
text_end:		DS 3			; End of main text in edit buffer
cutbuf_start:		DS 3			; Start of cut buffer 
num_lines:		DS 3			; Number of lines in main text.
cur_lineaddr:		DS 3			; Address of current line in buffer (where the cursor is)
cur_lineno:		DS 3			; Current line number within text
top_lineaddr:		DS 3			; Address of top line (shown on the top of screen).
top_lineno:		DS 3			; Line number of line at top of screen.
start_lineno:		DS 3			; Line number to start up with.
cursor_row:		DS 1			; Row of cursor on screen (0 = first row to display text).
cursor_col:		DS 1	;		; Column of cursor on screen actually shown.(0 = leftmost column).
cursor_col_max:		DS 1			; Column of cursor to show if line is long enough.
						; The cursor will never be shown beyond the end of current line, but will return to 
						; a position up to cursor_col_max if you move vertially to a longer line.
linebuf_editpos:	DS 1			; position in linebuf where character should be inserted.
linebuf_scrolled:	DS 1			; number of positions the current line is scrolled to the left.
curline_stat:		DS 1			; Status of the current line: 
						;	0=not in linebuf,
						;	1=in linebuf unmodified, 
						; 	2=in linebuf and modified.
curline_length:		DS 1			; Length of current line in linebuf (including CRLF).
oldcurline_length:	DS 1			; Length of the current line in edit buffer
load_start:		DS 3			; Start address for loading the file.	
cutbuf_lines:		DS 3			; Number of lines in cut buffer	
load_filesize:		DS 3			; Number of bytes of file loaded after CRLF normalization
file_modified:		DS 1			; Flag that tells if a file is modified.
cut_continue:		DS 1			; Flag to indicate that we want to continue cut/copy the next line and add to cut buffer.
save_sp:		DS 3			; Saved stack pointer for error return.
; The default Edit buffer is in the space between the end of the program and the top of the MOS command space.
current_fg:    	   	DS 1 	    	  	; Foreground colour.
current_bg:    	   	DS 1 	    	  	; Background colour.
default_buf:
			; Initialization code can overlap with text buffer, no longer required
			; after initialization.
			include "scr_init.inc"
default_buf_end:	EQU 0B8000h
			
