/* Nano-style editor
   Copyright 2025, L.C. Benschop, Vught, The Netherlands.
   MIT license
*/

#include "edit.h"
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void EDT_StartLine(void)
{
  EDT_BufStartLine();
  EDT.curline_pos=0;
  EDT.cursor_col_max=0;
  EDT_RenderCurrentLine();
}

static void EDT_EndLine(void)
{
  EDT_BufEndLine();
  EDT.curline_pos=EDT.curline_len;
  EDT.cursor_col_max=0;
  EDT_RenderCurrentLine();
}

static void EDT_CursorLeft(void)
{
  EDT.cursor_col_max=0;
  if(EDT.curline_pos > 0) {
    EDT_BufPrevChar();
    EDT.curline_pos--;
    EDT_RenderCurrentLine();
  } else if (EDT.lineno > 1) {
    EDT.lineno--;
    EDT_LeaveCurrentLine();
    EDT.cursor_row--;
    EDT_BufPrevLine();
    EDT_BufAdjustCol();
    EDT_BufEndLine();
    EDT.curline_pos=EDT.curline_len;
    EDT_AdjustTop(false);
  }
}

static void EDT_CursorRight(void)
{
  EDT.cursor_col_max=0;
  if(EDT.curline_pos < EDT.curline_len) {
    EDT_BufNextChar();
    EDT.curline_pos++;
    EDT_RenderCurrentLine();
  } else if (EDT.lineno < EDT.total_lines) {
    EDT.lineno++;
    EDT_LeaveCurrentLine();
    EDT.cursor_row++;
    EDT_BufNextLine();
    EDT_BufAdjustCol();
    EDT_BufStartLine();
    EDT.curline_pos=0;
    EDT_AdjustTop(false);
  }
}

static void EDT_CursorUp(void)
{
  if (EDT.lineno > 1) {
    EDT.lineno--;
    EDT_LeaveCurrentLine();
    EDT.cursor_row--;
    EDT_BufPrevLine();
    EDT_BufAdjustCol();
    EDT_AdjustTop(false);
  }
}

static void EDT_CursorDown(void)
{
  if (EDT.lineno < EDT.total_lines) {
    EDT.lineno++;
    EDT_LeaveCurrentLine();
    EDT.cursor_row++;
    EDT_BufNextLine();
    EDT_BufAdjustCol();
    EDT_AdjustTop(false);
  }
}

static void EDT_Delete(void)
{
  if(EDT.curline_pos < EDT.curline_len) {
    EDT_BufDeleteChar();
    EDT_RenderCurrentLine();
  } else {
    EDT_BufJoinLines();
    EDT_AdjustTop(true);
  }
}

static void EDT_BackSpace(void)
{
  EDT.cursor_col_max=0;
  if(EDT.curline_pos>0) {
    EDT.curline_pos--;
    EDT_BufPrevChar();
    EDT_BufDeleteChar();
    EDT_RenderCurrentLine();
  } else if (EDT.gap_start > EDT.text_start) {
    EDT.lineno--;
    EDT.cursor_row--;
    EDT_BufPrevLine();
    EDT_BufAdjustCol();
    EDT_BufEndLine();
    EDT.curline_pos=EDT.curline_len;
    EDT_BufJoinLines();
    EDT_AdjustTop(true);
  }
}

static void EDT_NewLine(void)
{
  if (EDT.gap_start < EDT.gap_end-1) {
    EDT.cursor_row++;
    EDT_BufInsertNL();
    EDT_BufAdjustCol();
    EDT_BufStartLine();
    EDT.curline_pos=0;
    EDT_AdjustTop(true);
  }
}

static void EDT_InsertChar(int c)
{
  EDT_BufInsertChar(c);
  EDT.cursor_col_max=0;
  EDT_RenderCurrentLine();
}

static void EDT_CutLine(bool fContinue)
{
  if (!fContinue) {
    EDT.cut_end = EDT.text_end;
    EDT.cut_lines = 0;
  }
  EDT_BufStartLine();
  EDT.curline_pos=0;
  EDT.cursor_col_max=0;
  if (EDT_BufCopyLine()) {
    EDT_BufDeleteLine();
    EDT_BufAdjustCol();
    EDT_AdjustTop(true);
    EDT.is_changed=1;
  } else {
    EDT_RenderCurrentLine();
  }
}

static void EDT_CopyLine(bool fContinue)
{
  if (!fContinue) {
    EDT.cut_end = EDT.text_end;
    EDT.cut_lines = 0;
  }
  EDT_BufStartLine();
  EDT.curline_pos=0;
  EDT.cursor_col_max=0;
  if (EDT_BufCopyLine() && EDT.lineno<EDT.total_lines) {
    EDT.lineno++;
    EDT.cursor_row++;
    EDT_BufNextLine();
    EDT_BufAdjustCol();
    EDT_AdjustTop(true);
  } else {
    EDT_ShowBottom();
    EDT_RenderCurrentLine();
  }
}

static void EDT_Paste(void)
{
  EDT_BufStartLine();
  EDT.curline_pos=0;
  EDT.cursor_col_max=0;
  EDT_BufPaste();
  EDT.is_changed=1;
  EDT.cursor_row=127;
  EDT_BufAdjustCol();
  EDT_AdjustTop(true);
}

static void EDT_CenterScreen(void)
{
    EDT.cursor_row=127;
    EDT_AdjustTop(false);
}

static bool EDT_AskSave(void)
{
  printf("Save file: ");
  EDT_ReadLine(EDT.mem_start+FILENAME_OFFS,MAX_NAME_LENGTH);
  return EDT_SaveFile(EDT.mem_start+FILENAME_OFFS);
}

static void EDT_GotoLine(void)
{
  int i,n;
  EDT.mem_start[LINENOSTRING_OFFS]=0;
  EDT_SetCursor(0,EDT.scr_rows-1);
  EDT_ClrEOL();
  EDT_SetCursor(0,EDT.scr_rows-1);
  printf("Goto line: ");
  EDT_ReadLine(EDT.mem_start+LINENOSTRING_OFFS,4);
  n=atoi((char*)EDT.mem_start+LINENOSTRING_OFFS);
  if (n<1) n=1;
  if (n>EDT.total_lines) n=EDT.total_lines;
  if (n<EDT.lineno) {
    for (i=0; i<EDT.lineno-n; i++) {
      EDT_BufPrevLine();
    }
    EDT_BufAdjustCol();
  } else if (n>EDT.lineno) {
    for (i=0; i<n-EDT.lineno; i++) {
      EDT_BufNextLine();
    }
    EDT_BufAdjustCol();
  }
  EDT.lineno=n;
  EDT.cursor_row=127;
  EDT_AdjustTop(false);
}

static void EDT_PageUp(void)
{
  int i;
  int n=EDT.scr_rows -2;
  EDT_LeaveCurrentLine();
  for (i=0; i<n;i++) {
    if (EDT.lineno==1) break;
    EDT.lineno--;
    EDT.cursor_row--;
    EDT_BufPrevLine();
  }
  if (i>0)EDT_BufAdjustCol();
  EDT_AdjustTop(false);
}

static void EDT_PageDown(void)
{
  int i;
  int n=EDT.scr_rows -2;
  EDT_LeaveCurrentLine();
  for (i=0; i<n;i++) {
    if (EDT.lineno ==EDT.total_lines) break;
    EDT.lineno++;
    EDT.cursor_row++;
    EDT_BufNextLine();
  }
  if (i>0)EDT_BufAdjustCol();
  EDT_AdjustTop(false);
}

static void EDT_SearchF(void)
{
  unsigned int lines_fwd=0;
  unsigned int srch_len;
  unsigned char *p = EDT.gap_end+1;
  EDT_SetCursor(0,EDT.scr_rows-1);
  EDT_ClrEOL();
  EDT_SetCursor(0,EDT.scr_rows-1);
  printf("Find: ");
  EDT_ReadLine(EDT.mem_start+SEARCHSTRING_OFFS,MAX_NAME_LENGTH);
  srch_len = strlen((char*)EDT.mem_start+SEARCHSTRING_OFFS);
  if (srch_len>0) {
    while (EDT.text_end - p >= srch_len) {
      if (memcmp(p,EDT.mem_start+SEARCHSTRING_OFFS,srch_len)==0) {
	/* Found it! */
	while (p != EDT.gap_end) {
	  EDT_BufNextChar();
	}
	EDT_BufStartLine();
	EDT.cursor_col=0;
	EDT.cursor_col_max=0;
	EDT_BufAdjustCol();
	EDT.curline_pos=p-EDT.gap_end;
	while (p != EDT.gap_end) {
	  EDT_BufNextChar();
	}
	EDT.lineno+=lines_fwd;
	EDT.cursor_row=127;
	EDT_AdjustTop(true);
	return;
      }
      if (*p++=='\n') lines_fwd++;
    }
  }
  /* Got here, not found */
  EDT_SetCursor(0,EDT.scr_rows-1);
  printf("Not found!");
  EDT_ClrEOL();
  EDT_RenderCurrentLine();
}

static void EDT_SearchB(void)
{
  unsigned int lines_bwd=0;
  unsigned int srch_len;
  unsigned char *p = EDT.gap_start-1;
  EDT_SetCursor(0,EDT.scr_rows-1);
  EDT_ClrEOL();
  EDT_SetCursor(0,EDT.scr_rows-1);
  printf("Find: ");
  EDT_ReadLine(EDT.mem_start+SEARCHSTRING_OFFS,MAX_NAME_LENGTH);
  srch_len = strlen((char*)EDT.mem_start+SEARCHSTRING_OFFS);
  if (srch_len>0) {
    while (p > EDT.text_start) {
      if (memcmp(p,EDT.mem_start+SEARCHSTRING_OFFS,srch_len)==0) {
	/* Found it! */
	while (p != EDT.gap_start) {
	  EDT_BufPrevChar();
	}
	EDT_BufStartLine();
	EDT.cursor_col=0;
	EDT.cursor_col_max=0;
	EDT_BufAdjustCol();
	EDT.curline_pos=p-EDT.gap_start;
	while (p != EDT.gap_start) {
	  EDT_BufNextChar();
	}
	EDT.lineno-=lines_bwd;
	EDT.cursor_row=127;
	EDT_AdjustTop(true);
	return;
      }
      if (*p--=='\n') lines_bwd++;
    }
  }
  /* Got here, not found */
  EDT_SetCursor(0,EDT.scr_rows-1);
  printf("Not found!");
  EDT_ClrEOL();
  EDT_RenderCurrentLine();
}

static void EDT_ReadFile(void)
{
  unsigned int old_lines = EDT.total_lines;
  unsigned int added_lines,i;
  EDT_BufStartLine();
  EDT.curline_pos=0;
  EDT.cursor_col_max=0;
  EDT_SetCursor(0,EDT.scr_rows-1);
  EDT_ClrEOL();
  EDT_SetCursor(0,EDT.scr_rows-1);
  printf("Read file: ");
  EDT.mem_start[BACKFILENAME_OFFS]=0;
  EDT_ReadLine(EDT.mem_start+BACKFILENAME_OFFS,MAX_NAME_LENGTH);
  EDT_LoadFile(EDT.mem_start+BACKFILENAME_OFFS);
  added_lines = EDT.total_lines - old_lines;
  for (i=0; i<added_lines; i++) {
    EDT_BufNextLine();
    EDT.lineno++;
  }
  EDT.is_changed=1;
  EDT.cursor_row=127;
  EDT_BufAdjustCol();
  EDT_AdjustTop(true);
}

/* Read 2 keypresses and treat them as hex digits. Insert the corresponding
   character */
static void EDT_InsertHex(void)
{
  unsigned char d1,d2;
  d1=toupper(EDT_GetKey());
  d2=toupper(EDT_GetKey());
  if (d1>='A') d1-=7;
  d1-='0';
  if (d2>='A') d2-=7;
  d2-='0';
  d1 = (d1<<4)+d2;
  if (d1>=32 && d1 != 127)
    EDT_InsertChar(d1);
}

static unsigned int EDT_NextWordLength()
{
  unsigned char *p = EDT.gap_end;
  while (*p > ' ') {
    p++;
  }
  return p-EDT.gap_end;
}

#define EDT_JUSTIFY_SPACE 0
#define EDT_JUSTIFY_WORD 1
#define EDT_JUSTIFY_COLS (SCR_COLS - 1)

static void EDT_Justify(void)
{
  unsigned char *ret_p = EDT.gap_end;
  unsigned int cur_len = 0;
  bool crossed_cursor = false;
  unsigned int state = EDT_JUSTIFY_SPACE;
  /* Find start of paragraph */
  while (EDT.gap_start - EDT.text_start > 2 &&
	 (EDT.gap_start[-1] != '\n' || EDT.gap_start[-2] != '\n')) {
    if (EDT.gap_end[0] == '\n' && ret_p != EDT.gap_end) EDT.lineno--;
    EDT_BufPrevChar();
  }
  /* Reformat paragraph */
  while (EDT.gap_end < EDT.text_end - 2 &&
	 (EDT.gap_end[0] != '\n' || EDT.gap_end[1] != '\n')) {
    if (!crossed_cursor && ret_p <= EDT.gap_end) {
      crossed_cursor = true;
      ret_p = EDT.gap_start;
    }
    if (state == EDT_JUSTIFY_WORD) {
      if (EDT.gap_end[0] > ' ') {
	EDT_BufNextChar();
	cur_len++;
      } else {
	state = EDT_JUSTIFY_SPACE;
      }
    } else {
      if (EDT.gap_end[0] == '\n') {
	EDT.total_lines--;
	EDT.gap_end++;
      } else if (EDT.gap_end[0] <= ' ') {
	EDT.gap_end++;
      } else {
	unsigned int wlen = EDT_NextWordLength();
	if (cur_len > 0) {
	  if (cur_len + 1 + wlen > EDT_JUSTIFY_COLS) {
	    *EDT.gap_start++ = '\n';
	    EDT.total_lines++;
	    if (!crossed_cursor) {
	      EDT.lineno++;
	    }
	    cur_len=0;
	  } else {
	    *EDT.gap_start++ = ' ';
	    cur_len++;
	  }
	}
	state = EDT_JUSTIFY_WORD;
      }
    }
  }
  if (crossed_cursor) {
    /* Return to the original character */
    while (EDT.gap_start != ret_p) {
      EDT_BufPrevChar();
    }
  } else {
    ret_p = EDT.gap_start;
  }
  /* Reposition cursor */
  EDT.is_changed=1;
  EDT_BufStartLine();
  EDT.cursor_col=0;
  EDT.cursor_col_max=0;
  EDT_BufAdjustCol();
  EDT.curline_pos=ret_p-EDT.gap_start;
  while (ret_p != EDT.gap_start) {
    EDT_BufNextChar();
  }
  EDT.cursor_row=127;
  EDT_AdjustTop(true);
}

static char HelpText[] =
  "Nano Extended for Agon v0.20, Copyright 2025, L.C. Benschop\r\n"
  "\r\n"
  "Cursor movement:\r\n"
  "Ctrl-B or cursor left, Ctrl-F or cursor right\r\n"
  "Ctrl-P or cursor up, Ctrl-N or cursor down\r\n"
  "Ctrl-Y: page up, Ctrl-V: page down\r\n"
  "Ctrl-A; start of line, Ctrl-E: end of line\r\n"
  "Ctrl-L, redraw screen with current line in centre\r\n"
  "Ctrl-H goto line (enter number)\r\n"
  "\r\n"
  "Delete:\r\n"
  "Backspace: delete to left, Control-D: delete to right\r\n"
  "\r\n"
  "Cut and paste:\r\n"
  "Ctrl-K cut current line, repeat to cut block of multiple lines\r\n"
  "Ctrl-C copy current line, repeat to copy block of multiple lines\r\n"
  "Ctrl-U paste cut/copied lines, can be repeated to paste multiple times\r\n"
  "\r\n"
  "Find:\r\n"
  "Ctrl-W: find forward, Ctrl-Q: find backward\r\n"
  "\r\n"
  "Other:\r\n"
  "Ctrl-R: insert file before current line, Ctrl-O to save file\r\n"
  "Ctrl-X or ESC: Exit editor (ask to save) Ctrl-J Justify paragraph\r\n"
  "Ctrl-T followed by two hex digits: insert special character\r\n"
  "\r\n"
  "Press any key to return to editor.";

static void EDT_ShowHelp(void)
{
  char *p=HelpText;
  putch(12);
  while (*p)
    putch(*p++);
  EDT_GetKey();
  EDT_ShowScreen();
}

/* Main function of the editor.
   Pre: EDT.mem_start start of memory area.
	EDT.mem_end end of memory area.
	EDT.mem_start+FILENAME_OFFS: null terminated string filename to edit
*/
void EDT_EditCore(void)
{
  bool fCutContinue = false;
  EDT.text_start = EDT.mem_start + VAR_END_OFFS;
  EDT.text_end = EDT.mem_end - CUT_BUF_SIZE;
  EDT.gap_start = EDT.text_start;
  EDT.gap_end = EDT.text_end;
  EDT.cut_end = EDT.text_end;
  EDT.tab_stop = 8;
  EDT.crlf_flag = 0;
  EDT.scr_rows = 30;
  EDT.cursor_row = 0;
  EDT.cursor_col = 0;
  EDT.cursor_col_max = 0;
  EDT.curline_pos = 0;
  EDT.is_changed = 0;
  EDT.mem_start[SEARCHSTRING_OFFS]=0;
  EDT.total_lines = 0;
  EDT.cut_lines = 0;
  EDT_LoadConfig("/bin/ne.cfg");

  EDT_InitScreen();
  EDT_LoadFile(EDT.mem_start+FILENAME_OFFS);
  EDT.top_line = EDT.gap_start;
  EDT.curline_len=EDT_BufLenCurLine();
  EDT.lineno = 1;
  EDT_ShowScreen();
  for(;;){
    int k = EDT_GetKey();
    switch (k) {
    case 1:
    case 0x100+VKEY_HOME:
      EDT_StartLine();
      break;
    case 2:
    case 0x100+VKEY_LEFT:
      EDT_CursorLeft();
      break;
    case 3:
      EDT_CopyLine(fCutContinue);
      fCutContinue = true;
      break;
    case 4:
    case 0x100+VKEY_DELETE:
      EDT_Delete();
      break;
    case 5:
    case 0x100+VKEY_END:
      EDT_EndLine();
      break;
    case 6:
    case 0x100+VKEY_RIGHT:
      EDT_CursorRight();
      break;
    case 7:
      EDT_ShowHelp();
      break;
    case 8:
      EDT_GotoLine();
      break;
    case 9:
      EDT_InsertChar(9);
      break;
    case 10:
      EDT_Justify();
      break;
    case 11:
      EDT_CutLine(fCutContinue);
      fCutContinue = true;
      break;
    case 12:
      EDT_CenterScreen();
      break;
    case 13:
      EDT_NewLine();
      break;
    case 14:
    case 0x100+VKEY_DOWN:
      EDT_CursorDown();
      break;
    case 15:
      EDT_SetCursor(0,EDT.scr_rows-1);
      EDT_ClrEOL();
      EDT_SetCursor(0,EDT.scr_rows-1);
      if (!EDT_AskSave()) {
	EDT_SetCursor(0,EDT.scr_rows-1);
	EDT_ClrEOL();
	EDT_SetCursor(0,EDT.scr_rows-1);
	printf("File save failed!");
      } else {
	EDT_ShowBottom();
      }
      break;
    case 16:
    case 0x100+VKEY_UP:
      EDT_CursorUp();
      break;
    case 17:
      EDT_SearchB();
      break;
    case 18:
      EDT_ReadFile();
      break;
    case 20:
      EDT_InsertHex();
      break;
    case 21:
      EDT_Paste();
      break;
    case 22:
    case 0x100+VKEY_PAGEDOWN:
      EDT_PageDown();
      break;
    case 23:
      EDT_SearchF();
      break;
    case 24:
    case 27:
      putch(12);
      if (EDT.is_changed) {
	bool success;
	do {
	  success =true;
	  printf("Save file (Y/n)? ");
	  k = EDT_GetKey();
	  printf("\r\n");
	  if (k!='n' && k!='N') {
	    success=EDT_AskSave();
	    printf("\r\n");
	  }
	  if (!success) printf("File save failed!\r\n");
	} while (!success);
      }
      EDT_ExitScreen();
      return;
    case 25:
    case 0x100+VKEY_PAGEUP:
      EDT_PageUp();
      break;
    case 127:
      EDT_BackSpace();
      break;
    default:
      if ( k>=32)
	EDT_InsertChar(k);
    }
    if (k != 11 && k != 3)
      fCutContinue = false;
  }
}
