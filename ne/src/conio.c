/* Nano-style editor
   Copyright 2025, L.C. Benschop, Vught, The Netherlands.
   MIT license
*/

#include <mos_api.h>
#include <agon/vdp_vdu.h>
#include <stdio.h>
#include "edit.h"


unsigned int  EDT_GetKey(void)
{
  int ch,vk;
  ch = getch();
  vk = getsysvar_vkeycode();
  if (vk == VKEY_UP || vk == VKEY_DOWN || vk == VKEY_LEFT || vk == VKEY_RIGHT ||
      vk == VKEY_PAGEUP || vk == VKEY_PAGEDOWN || vk == VKEY_DELETE ||
      vk == VKEY_HOME || vk == VKEY_END) {
    return vk + 0x100;
  } else {
    return ch & 0xff;
  }
}


void EDT_InitScreen(void)
{
  /* Disable control keys */
  putch(23); putch(0); putch(0x98); putch(0);
  /* paged mode off */
  putch(15);
  /* Text viwport off */
  putch(26);
  EDT.scr_rows = getsysvar_scrRows();
  EDT.scr_cols = getsysvar_scrCols();
  if (EDT.scr_cols < 80) {
    putch(22); putch(3);
    waitvblank();
    vdp_get_scr_dims(true);
    EDT.scr_rows = getsysvar_scrRows();
    EDT.scr_cols = getsysvar_scrCols();
  }
  EDT.fgcolour = vdp_return_palette_entry_index(128);
  EDT.bgcolour = vdp_return_palette_entry_index(129);;
}

void EDT_ExitScreen(void)
{
  /* Re-enable control keys */
  putch(23); putch(0); putch(0x98); putch(1);
}

  void EDT_InvVideo(void)
{
  putch(17);putch(EDT.bgcolour);putch(17);putch(128+EDT.fgcolour);
}

void EDT_TrueVideo(void)
{
  putch(17);putch(EDT.fgcolour);putch(17);putch(128+EDT.bgcolour);
}

void EDT_SetCursor(int x, int y)
{
  putch(31);putch(x);putch(y);
}

void EDT_ReadLine(unsigned char* buf, int len)
{
  int i=0;
  int c;
  for(;;) {
    if (buf[i]==0) break;
    putch(buf[i++]);
  }
  for(;;) {
    c=EDT_GetKey();
    switch(c) {
    case -1:
      return;
    case 13:
      return;
    case 127:
      if (i>0) {
	putch(127);
	i--;
	buf[i]=0;
      }
      break;
    default:
      if (i<len && c>=32) {
	putch(c);
	buf[i++]=c;
	buf[i]=0;
      }
    }
  }
}

void VDUGetCursor(int *x,int *y)
{
  uint8_t xb,yb;
  vdp_request_text_cursor_position(true);
  vdp_return_text_cursor_position(&xb,&yb);
  *x = xb;
  *y = yb;
}

void EDT_ClrEOL(void)
{
  int x,y,n;
  VDUGetCursor(&x,&y);
  n = SCR_COLS-x;
  if (y==EDT.scr_rows-1)
    n--;
  for (int i=0; i<n; i++) putch(32);
}

unsigned char * EDT_RenderLine(unsigned char *p, bool is_current)
{
  unsigned char c;
  bool is_scrolled=false;
  int col = 0;
  unsigned char *q=p;
  int tabstop = EDT.tab_stop;
  if (is_current) {
    /* First find out how much to scroll the line to the left if the current
       edit position is beyond the width of the screen */
    for(;;) {
      if (q==EDT.gap_start) {
	break;
      }
      if (*q=='\n') {
	break;
      }
      if (*q++=='\t') {
	col = (col + tabstop) & (-tabstop);
	if (col>SCR_COLS-1)col=SCR_COLS-1;
      } else {
	col++;
      }
      if (col==SCR_COLS-1) {
	col=1;
	p = q;
	is_scrolled=true;
      }
    }
    EDT.cursor_col=col;
  }
  if (is_scrolled) {
    EDT_InvVideo();
    putch('<');
    EDT_TrueVideo();
    col=1;
  } else {
    col=0;
  }
  for(;;) {
    if (p==EDT.gap_start) {
      p=EDT.gap_end;
    }
    c=*p++;
    if (c=='\n') {
      if (col==SCR_COLS) {
	EDT_InvVideo();
	putch('>');
	EDT_TrueVideo();	
      }  else if (is_current) {   
	EDT_ClrEOL();
      } else {
	putch(13);putch(10);
      }
      break;
    } else if (c=='\t' && col<SCR_COLS-1) {
      do {
	putch(' ');
	col++;
      } while ( (col & (tabstop-1)) && col < SCR_COLS-1);
    } else if (col<SCR_COLS-1) {
      putch(c);
      col++;
    } else {
      col = SCR_COLS;
    }
  }
  return p;
}

void EDT_RenderCurrentLine(void)
{
  EDT_SetCursor(0,
		EDT.cursor_row+1);
  EDT_RenderLine(EDT.gap_start-EDT.curline_pos,true);
  EDT_ShowCursor();
}

void EDT_LeaveCurrentLine(void)
{
  EDT_SetCursor(0,
		EDT.cursor_row+1);
  EDT_RenderLine(EDT.gap_start-EDT.curline_pos,false);
}


void EDT_ShowCursor(void)
{
  EDT_SetCursor(EDT.cursor_col,
		EDT.cursor_row+1);
  if (EDT.cursor_col_max < EDT.cursor_col) {
    EDT.cursor_col_max = EDT.cursor_col;
  }
}


void EDT_ShowBottom(void)
{
  EDT_SetCursor(0,EDT.scr_rows-1);
  EDT_InvVideo(); 
  printf("Line %d/%d, %d/%d bytes -- ESC to exit, ^G for help %c Cut %d",
		 EDT.lineno,
		 EDT.total_lines,
		 EDT.gap_start-EDT.text_start + EDT.text_end-EDT.gap_end,
		 EDT.text_end - EDT.text_start,
		 EDT.is_changed?'*':' ',
		 EDT.cut_lines
		 );
  EDT_ClrEOL();
  EDT_TrueVideo();
}

void EDT_ShowScreen(void)
{
  unsigned char *p = EDT.top_line;
  putch(12);
  EDT_InvVideo();
  printf("Nano Extended: %s",EDT.mem_start+FILENAME_OFFS);
  EDT_ClrEOL();
  EDT_TrueVideo();

  for (int i=0; i<EDT.scr_rows-2 && p!=EDT.text_end; i++) {
    p = EDT_RenderLine(p,i==EDT.cursor_row);
  }
  EDT_ShowBottom();
  EDT_ShowCursor();
}

void EDT_AdjustTop(bool always_redraw)
{
  if (EDT.top_line > EDT.gap_start ||
      EDT.cursor_row >= EDT.scr_rows -2) {
    int lines_to_move = (EDT.scr_rows -2)/2;
    EDT.cursor_row = 0;
    EDT.top_line = EDT.gap_start - EDT.curline_pos;
    while (lines_to_move && EDT.top_line > EDT.text_start) {
      lines_to_move--; 
      EDT.cursor_row++;
      EDT.top_line--;
      while (EDT.top_line > EDT.text_start && EDT.top_line[-1] != '\n') {
	EDT.top_line--;
      }
    }
    EDT_ShowScreen();
  } else if (always_redraw) {
    EDT_ShowScreen();
  } else {
    EDT_ShowBottom();
    EDT_RenderCurrentLine();
  }
}

