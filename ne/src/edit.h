/* Nano-style editor
   Copyright 2025, L.C. Benschop, Vught, The Netherlands.
   MIT license
*/

#include <mos_api.h>
#include <stdbool.h>

#define VKEY_UP  150
#define VKEY_DOWN 152
#define VKEY_LEFT 154
#define VKEY_RIGHT 156
#define VKEY_PAGEUP 146
#define VKEY_PAGEDOWN 148
#define VKEY_DELETE 130
#define VKEY_HOME 133
#define VKEY_END  135

#define VDUWrite(c) putch(c)

#define EDIT_BUF_SIZE (400*1024)
#define CUT_BUF_SIZE (32*1024)
#define MAX_NAME_LENGTH 128

/* Global editor state is only the _EditState structure and the
   emeory space (allocated at startup).
*/

struct _EditState {
  unsigned char *mem_start; /* start of memory allocated to editior */
  unsigned char *text_start; /* Start of text being edited */
  unsigned char *top_line; /* Top line visible on screen */
  unsigned char *gap_start;
  unsigned char *gap_end;
  unsigned char *text_end;
  unsigned char *cut_end;  
  unsigned char *mem_end;
  unsigned int lineno; /* Line number at cursor (1-based) */
  unsigned int total_lines; /* Number of lines of text in buffer */
  unsigned int cut_lines;
  unsigned char is_changed;   /* Flag to indicate if file is changed */
  unsigned char curline_pos;   /* Position within current line of cursor */
  unsigned char curline_len;    /*  Lenght of current line in bytes */
  unsigned char cursor_col_max;   /* Cursor column to which we may return if we move to a longer line */
  unsigned char cursor_col;   /*  Cursor column (can be >= 80) for long lines. */
  unsigned char cursor_row;    /* Cursor row 0 is top of text area */
  unsigned char tab_stop;        /* Tab stops at 4 or 8 chars? */
  unsigned char scr_rows;        /* Number of text rows on the screen (30 or 60) */
  unsigned char crlf_flag;       /* flag to indicate that file should be saved with CR-LF instead of LR */
  unsigned char fgcolour;       /* foreground colour */
  unsigned char bgcolour;       /* background colour */
  unsigned char scr_cols;       /* Number of text columns on the screen (>=80)*/
};

extern struct _EditState EDT;

/* Memory space *
   mem_start <= addr < text_start: various byte-size and string vartables
                                   (see defines below).
   mem_start <= addr < gap_start:  text before cursor.
   gap_start <= addr < gap_end:    free space (gap).
   gap_end <= addr < text_end:     text at cursor and after (file always ends
                                   with newline that cursor cannot move past).
   text_end <= addr < cut_end      text in cut buffer.
   cut_end <= addr < mem_end       free space after cut buffer.
*/
   
#define FILENAME_OFFS 0
  /* n+1 bytes n-byte null-terminaed filename */
#define BACKFILENAME_OFFS (FILENAME_OFFS+MAX_NAME_LENGTH+1)
  /* n+1 bytes n-byte null-terminaed backup filename */
#define SEARCHSTRING_OFFS (BACKFILENAME_OFFS+MAX_NAME_LENGTH+1)
  /* n+1 bytes n-byte null-terminated search string */
#define LINENOSTRING_OFFS (SEARCHSTRING_OFFS+41)
  /* 5 bytes 4-byte type buffer for line number */
#define VAR_END_OFFS (LINENOSTRING_OFFS + 5)
  /* Offset where we can have the text_start pointer */

#define SCR_COLS (EDT.scr_cols) /* The number of columns is set fixed to 80 */

void EDT_EditCore(void);
unsigned int  EDT_GetKey(void);
void EDT_InitScreen(void);
void EDT_ExitScreen(void);
void EDT_LoadFile(unsigned char *name);
bool EDT_SaveFile(unsigned char *name);
void EDT_LoadConfig(char *name);
void EDT_InvVideo(void);
void EDT_TrueVideo(void);
void EDT_SetCursor(int x, int y);
void EDT_ClrEOL(void);
void EDT_ReadLine(unsigned char* buf, int len);
unsigned char * EDT_RenderLine(unsigned char *p, bool is_current);
void EDT_RenderCurrentLine(void);
void EDT_LeaveCurrentLine(void);
void EDT_ShowScreen(void);
void EDT_ShowCursor(void);
void EDT_ShowBottom(void);
void EDT_AdjustTop(bool always_redraw);

void EDT_BufStartLine(void);
void EDT_BufEndLine(void);
int EDT_BufLenCurLine(void);
void EDT_BufInsertChar(unsigned char c);
void EDT_BufInsertNL(void);
void EDT_BufDeleteChar(void);
void EDT_BufJoinLines(void);
void EDT_BufNextLine(void);
void EDT_BufPrevLine(void);
void EDT_BufNextChar(void);
void EDT_BufPrevChar(void);
void EDT_BufAdjustCol(void);
void EDT_BufDeleteLine(void);
bool EDT_BufCopyLine(void);
void EDT_BufPaste(void);
