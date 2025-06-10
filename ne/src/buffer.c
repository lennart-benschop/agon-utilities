/* Nano-style editor
   Copyright 2025, L.C. Benschop, Vught, The Netherlands.
   MIT license
*/

#include <string.h>
#include <stdbool.h>
#include "edit.h"


void EDT_BufStartLine(void)
{
  while (EDT.gap_start > EDT.text_start &&
	 EDT.gap_start[-1] != '\n') {
    EDT_BufPrevChar();
  }
}

void EDT_BufEndLine(void)
{
  while (*EDT.gap_end != '\n') {
    EDT_BufNextChar();
  }  
}


int EDT_BufLenCurLine(void)
{
  int i=0;
  while (EDT.gap_end[i] != '\n') {
    i++;
  }
  return i;
}



void EDT_BufInsertChar(unsigned char c)
{
  if (EDT.curline_len < 254 &&
      EDT.gap_start < EDT.gap_end-1) {
    EDT.is_changed=1;
    *EDT.gap_start++ = c;
    EDT.curline_len++;
    EDT.curline_pos++;
  }
}

void EDT_BufInsertNL(void)
{
  if (EDT.gap_start < EDT.gap_end-1) {
    *EDT.gap_start++ = '\n';    
    EDT.is_changed=1;
    EDT.lineno++;
    EDT.total_lines++;
  }
}


void EDT_BufDeleteChar(void)
{
  EDT.gap_end++;
  EDT.curline_len--;
  EDT.is_changed=1;
}

/*
 * 
 * Check that length current line + length next line <= 254.
 * If so, delete newline char. decrease total number of lines,
 *
 * Pre: gap_end points to (non-final) newline.
 */
void EDT_BufJoinLines(void)
{
  int nextlen;
  if (EDT.gap_end < EDT.text_end) {
    EDT_BufNextChar();
    nextlen = EDT_BufLenCurLine();
    if (EDT.curline_len + nextlen < 255) {
      EDT.is_changed=1;
      EDT.curline_len += nextlen;
      EDT.gap_start--;
      EDT.total_lines--;
    }
  }
}


void EDT_BufNextLine(void)
{
  EDT_BufEndLine();
  EDT_BufNextChar();
}

void EDT_BufPrevLine(void)
{
  EDT_BufStartLine();
  EDT_BufPrevChar();
  EDT_BufStartLine();
}


void EDT_BufNextChar(void)
{
  if (EDT.gap_end < EDT.text_end) {
    *EDT.gap_start++ = *EDT.gap_end++;
  }
}

void EDT_BufPrevChar(void)
{
  if (EDT.gap_start > EDT.text_start) {
    *--EDT.gap_end = *--EDT.gap_start;
  }
}

void EDT_BufAdjustCol(void)
{
  int targetcol = EDT.cursor_col_max;
  int col=0;
  int tabstop = EDT.tab_stop;
  EDT.curline_len=EDT_BufLenCurLine();
  if (targetcol == 0) {
    targetcol = EDT.cursor_col;
  }
  EDT.curline_pos = 0;
  while (col < targetcol) {
    if (*EDT.gap_end=='\n') return;
    if (*EDT.gap_end=='\t') {
      col = (col + tabstop) & (-tabstop);
    } else {
      col++;
    }
    EDT_BufNextChar();  
    EDT.curline_pos++;
  }
  if (col > EDT.cursor_col_max) {
    EDT.cursor_col_max = col;
  }    
}

/* Remove current line from buffer.
   Pre: gap_end points to start of current line.
   Post: gap_end points to start of line after the one deleted.
 */
void EDT_BufDeleteLine(void)
{
  EDT.gap_end += EDT.curline_len;
  if (EDT.gap_end < EDT.text_end - 1) {
    /* Remove the trailing newline and decrease the number of lines.\
       Do not do this at the end of the file, always keep the trailing newline.
     */
    EDT.total_lines--;
    EDT.gap_end++;
  }
}

bool EDT_BufCopyLine(void)
{
  if (EDT.curline_len < EDT.mem_end-EDT.cut_end) {
    memcpy(EDT.cut_end, EDT.gap_end, EDT.curline_len+1);
    EDT.cut_end += EDT.curline_len+1;
    EDT.cut_lines++;
    return true;
  } else {
    return false;
  }
}

void EDT_BufPaste(void)
{
  if (EDT.gap_end - EDT.gap_start > EDT.cut_end-EDT.text_end &&
      EDT.cut_lines>0) {
    memcpy(EDT.gap_start, EDT.text_end, EDT.cut_end-EDT.text_end);
    EDT.lineno += EDT.cut_lines;
    EDT.total_lines += EDT.cut_lines;
    EDT.gap_start += EDT.cut_end-EDT.text_end;
  }
}
