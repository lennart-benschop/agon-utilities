/* 12AM COmmander, a Midnight Commander Lookalike for Agon Light.

   Display functions.

   12/05/2024
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mos_api.h"
#include <agon/vdp_vdu.h>
#include "mc.h"

uint8_t scr_cols, scr_rows;
uint8_t font_width, font_height;
uint8_t n_colours;

uint8_t dirwin_width;
uint8_t dirwin_height;
uint8_t cmdwin_height;



typedef struct {
  uint8_t top_x, top_y, width, height;
} display_win_t;




display_win_t display_wins[3];

/* Initialize the display.
   - select an at least 80-column mode. Store old mode
   - setup text viewport parameters.
   - Set cursor behaviour, logical coordinates.
 */
void display_init(void)
{
  static char disp_str[] = {
    12, // Clear screen
    23, 0, 192, 0, // Set physical coordinates.
    23, 16, 1, 0, // Scroll protection.
    23, 0, 152, 0, // No actions from control keys.
    15,            // No paged mode.
    18, 0, 15      // Graphics colour to white
  };
  if (getsysvar_scrCols() < 80 || getsysvar_scrRows()<25) {
    uint32_t time;
    vdp_mode(3);
    time = getsysvar_time();
    while (getsysvar_time() == time) {
    }
  }
  display_setattr(false,false);
  vdp_cursor_enable(false);
  mos_puts(&disp_str[0],sizeof disp_str, 0);
  scr_cols = getsysvar_scrCols();  
  scr_rows = getsysvar_scrRows();
  font_width = getsysvar_scrwidth() / scr_cols;  
  font_height = getsysvar_scrheight() / scr_rows;
  dirwin_width = scr_cols / 2 - 2;
  dirwin_height = scr_rows * 2 / 3;
  cmdwin_height = scr_rows - dirwin_height - 6;
  n_colours = getsysvar_scrColours();
  display_wins[0].top_x = 1;
  display_wins[0].top_y = 2;
  display_wins[0].width = dirwin_width;
  display_wins[0].height = dirwin_height;
  display_wins[1].top_x = scr_cols/2 + 1;
  display_wins[1].top_y = 2;
  display_wins[1].width = dirwin_width;
  display_wins[1].height = dirwin_height;
  display_wins[2].top_x = 0;
  display_wins[2].top_y = dirwin_height+5;
  display_wins[2].width = scr_cols;
  display_wins[2].height = scr_rows-dirwin_height-6;
}

/* Finalize the display.
   - Cancel text viewport.
   - Reset cursor behaviour, logical coordinates.
   - switch back to original video mode.
*/
void display_finish(void)
{
  static char disp_str[] = {
    26, // Cancel text viewport
    12, // Clear screen
    23, 0, 192, 1, // Set logical coordinates.
    23, 16, 0, 0, // Scroll protection off
    23, 0, 152, 1, // Re-enable control keys.
  };
  display_setattr(false,false);
  vdp_cursor_enable(true);
  mos_puts(&disp_str[0],sizeof disp_str, 0);
}
  
/* display_frame(void);
   Display the frame on the screen.
   - Top row for current directory name.
   - Line frames aorund text 
   
 */
void display_frame(void)
{
  static char * labels[] = {
    "Help", "Cmd", "View", "Edit", "Copy", "Move", "Mkdir", "Delete",
    "Cmd+F", "Quit"};

  unsigned int i;
  vdp_move_to(0,font_height+4);
  vdp_line_to(scr_cols*font_width-1, font_height+4);
  vdp_move_to(0,font_height*(2+dirwin_height)+4);
  vdp_line_to(scr_cols*font_width-1, font_height*(2+dirwin_height)+4);
  vdp_move_to(0,font_height*(4+dirwin_height)+4);
  vdp_line_to(scr_cols*font_width-1, font_height*(4+dirwin_height)+4);

  vdp_move_to(0,font_height+4);
  vdp_line_to(0,font_height*(4+dirwin_height)+4);
  vdp_move_to(scr_cols*font_width-1,font_height+4);
  vdp_line_to(scr_cols*font_width-1,font_height*(4+dirwin_height)+4);
  vdp_move_to(scr_cols*font_width/2-1,font_height+4);
  vdp_line_to(scr_cols*font_width/2-1,font_height*(4+dirwin_height)+4);
  vdp_move_to(scr_cols*font_width/2+font_width-1,font_height+4);
  vdp_line_to(scr_cols*font_width/2+font_width-1,font_height*(4+dirwin_height)+4);
  
  vdp_cursor_tab(scr_rows-1,0);
  for (i=0;i<10;i++ ) {
    display_setattr(false,true);
    printf("%2d",i+1);
    display_setattr(true,false);
    printf("%-6s",labels[i]);
  }
}

/* Select display attributes for inverse (cursor selected) and marked */
void display_setattr(bool inv, bool marked)
{
  int col = marked ? (n_colours==4?2:11) : 15;
  if (inv) {
    putch(17);putch(0);putch(17);putch(128+col);
  } else {
    putch(17);putch(col);putch(17);putch(128);
  }
}


/* Select one of the text windows
   1 left directory window
   2 right directory window.
   3 command window
 */
void display_setwin(uint8_t w)
{
  putch(28);
  putch(display_wins[w-1].top_x);
  putch(display_wins[w-1].top_y+display_wins[w-1].height-1);
  putch(display_wins[w-1].top_x+display_wins[w-1].width-1);
  putch(display_wins[w-1].top_y);
}

/* Display name of currently selected directory, show only rightmost portion
   of it if longer than window width.
   which: 1 for left, 2 for right.
   s: is name of path.
   hilight: must this be hilighted?
*/
void display_curdir(uint8_t which, char *s, bool hilight)
{
  putch(26);
  vdp_cursor_tab(0,(scr_cols/2)*(which-1)+1);
  display_setattr(hilight,false);
  printf("%-38.38s",s);
  display_setattr(false,false);
}

/* Display the currently selected file name at the bottom of directory window
   Gets more space, less likely to be truncated.
*/
void display_curfile(uint8_t which, char *s)
{
  putch(26);
  vdp_cursor_tab(dirwin_height+3,(scr_cols/2)*(which-1)+1);
  display_setattr(false,false);
  printf("%-38.38s",s);
}
