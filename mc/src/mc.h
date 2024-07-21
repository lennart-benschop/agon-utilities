/* 12AM COmmander, a Midnight Commander Lookalike for Agon Light.
   
   Common declarations and Function prototypes.

   11/05/2024

   23/06/2024: added mode/font/video mode config
 */

#ifndef MC_H_
#define MC_H_

#include <stdint.h>
#include <stdbool.h>

extern uint8_t video_mode;
extern uint16_t font_nr; 
extern uint8_t fgcol; /* Foreground colour */
extern uint8_t bgcol;  /* Background colour */
extern uint8_t hlcol; /* Hilite colour */


/* Replancement for strcasecmp */
int my_strcasecmp(char *p,char *q);


/* Number of bytes fitting into a single directory window*/
extern uint8_t dirwin_height;

struct dirlist;

/* Initialize the display.
   - select an at least 80-column mode. Store old mode
   - setup text viewport parameters.
   - Set cursor behaviour, logical coordinates.
 */
void display_init(void);

/* Finalize the display.
   - Cancel text viewport.
   - Reset cursor behaviour, logical coordinates.
   - switch back to original video mode.
*/
void display_finish(void);

/*  Display the frame on the screen.
   - Top row for current directory name.
   - Line frames aorund text 
   
 */
void display_frame(void);

/* Select display attributes for inverse (cursor selected) and marked */
void display_setattr(bool inv, bool marked);

/* Select one of the text windows
   1 left directory window
   2 right directory window.
   3 command window
 */
void display_setwin(uint8_t w);

/* Display name of currently selected directory, show only rightmost portion
   of it if longer than window width.
   which: 1 for left, 2 for right.
   s: is name of path.
   hilight: must this be hilighted?
*/
void display_curdir(uint8_t which, char *s, bool hilight);

/* Display the currently selected file name at the bottom of directory window
   Gets more space, less likely to be truncated.
*/
void display_curfile(uint8_t which, char *s);

/* Read the current directory into a a list in memory and return a pointer to 
   that list.
   Dynamically allocate all the required space for it.
 */
struct dirlist * dirlist_read(char *dirname);


/* Free the in-meomory resources of a directory list */
void dirlist_free(struct dirlist *dir);

/* Display the current directory list in one of the directory windows. */
void dirlist_show(uint8_t which, struct dirlist *dir);

/* Move the cursor inside the directory list, 
   direction: negative number is n positions up, positive = n positions down
   update the displayed directory. */
void dirlist_move_cursor(uint8_t which, struct dirlist *dir, int direction);

/* Select or deselect the directory entry under the cursor. */
void dirlist_select_file(struct dirlist *dir);

/* Select files in the current directory list according to the indicated 
   pattern Update display with marked items. */
void dirlist_select_pattern(uint8_t which, struct dirlist *dir, char *pat);

/* Select files in the current directory list according to the indicated \
   pattern Update display with marked items. */
void dirlist_deselect_pattern(uint8_t which, struct dirlist *dir, char *pat);


/* return boolean true if and only if currently selected item in
   the indicated directory list represents a subdirectory. */
bool dirlist_is_dir(struct dirlist*dir);

/* Get name of currently selected item in directory list */
char * dirlist_get_name(struct dirlist *dir);

/* Return the number of selected files */
unsigned int dirlist_n_selected(struct dirlist *dir);

/* Return the first tagged entry in the directory list */
char * dirlist_first_selected(struct dirlist *dir);

/* Return the next tagged entry in the directory list */
char * dirlist_next_selected(struct dirlist *dir);

#endif

