/* 12AM COmmander, a Midnight Commander Lookalike for Agon Light.

   Directory list functions.

   12/05/2024
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <mos_api.h>
#include <agon/vdp_vdu.h>
#include "mc.h"

typedef struct {
  char * name;
  uint16_t date,time;
  uint32_t size;
  bool is_dir;
  bool is_tagged;
} dirinfo_t;

struct dirlist {
  dirinfo_t *entries;
  unsigned int n_entries;
  unsigned int top_idx; /* Index at top of directory window. */
  unsigned int sel_idx; /* Index at bottom of directory window */
};

static int entry_compare(const void *a, const void *b)
{
  const dirinfo_t *e1 = a;
  const dirinfo_t *e2 = b;
  if (e1->is_dir == e2->is_dir) {
    return my_strcasecmp(e1->name,e2->name);
  }
  else if (e1->is_dir) {
    return -1;
  } else {
    return 1;
  }
}

static unsigned int n_dentries(char *dirname)
{
  int res;
  unsigned int i;
  DIR dir_struct;
  FILINFO file_struct;
  
  res = ffs_dopen(&dir_struct,dirname);
  if (res == 0) {
    for (i=0;;i++) {
      res = ffs_dread(&dir_struct,&file_struct);
      if (res != 0 || file_struct.fname[0]==0)
	break;
    }
    ffs_dclose(&dir_struct);
    return i;
  } else {
    return 0;
  }
}


/* Read the current directory into a a list in memory and return a pointer to 
   that list.
   Dynamically allocate all the required space for it.
 */
struct dirlist * dirlist_read(char *dirname)
{
  int res;
  unsigned int i;
  DIR dir_struct;
  unsigned int is_sub = 1;
  FILINFO file_struct;
  struct dirlist * dir = malloc(sizeof(struct dirlist));
  if (dir==NULL)
    return NULL;
  /* Fabricate a .. directory-up entry unless at root*/
  if (strcmp(dirname,"/") == 0) 
    is_sub = 0;
  dir->top_idx = 0;
  dir->sel_idx = 0;
  dir->n_entries = n_dentries(dirname) + is_sub;
  dir->entries = malloc(dir->n_entries * sizeof(dirinfo_t));
  if (dir->entries == 0) {
    free(dir);
    return NULL;
  }
  res = ffs_dopen(&dir_struct,dirname);
  if (res != 0) {
    free(dir->entries);
    free(dir);
    return NULL;

  }
  for (i=0; i<dir->n_entries; i++) {
    dir->entries[i].is_tagged = false;
    if (i==0 && is_sub) {
      dir->entries[i].is_dir  = true;
      dir->entries[i].name = strdup("..");      
    } else {
      res = ffs_dread(&dir_struct,&file_struct);
      if (res == 0 && file_struct.fname[0]!=0) {
	dir->entries[i].name = strdup(file_struct.fname);
	dir->entries[i].size = file_struct.fsize;
	dir->entries[i].date = file_struct.fdate;
	dir->entries[i].time = file_struct.ftime;
	dir->entries[i].is_dir = (file_struct.fattrib & 0x10) != 0;	
      } else {
	dir->entries[i].name = NULL;
	dir->entries[i].is_dir = false;
      }
    }
  }
  qsort(dir->entries, dir->n_entries, sizeof(dirinfo_t), entry_compare); 
  return dir;
}


/* Free the in-meomory resources of a directory list */
void dirlist_free(struct dirlist  *dir)
{
  unsigned int i;
  if (dir && dir->entries) {
    for (i=0; i<dir->n_entries; i++) {
      if (dir->entries[i].name)
	free(dir->entries[i].name);
    }
    free(dir->entries);
  }
  if (dir)
    free(dir);
}

void dirlist_show_entry(dirinfo_t *entry, bool is_current)
{
  display_setattr(is_current, entry->is_tagged);
  printf("%-20.20s ",entry->name);
  if (entry->is_dir) {
    printf(" <DIR>");
  } else if (entry->size > 99999999) { 
    printf("%5luM",entry->size/1000000);
  } else if (entry->size > 999999) {
    printf("%5luk",entry->size/1000);
  } else {
    printf("%6lu", entry->size);
  }
  if (strcmp(entry->name,"..")!=0) {
    printf(" %04u-%02u-%02u",1980+(entry->date >> 9),(entry->date >> 5) & 0xf,
	   entry->date & 0x1f);
  }
}

/* Display the current directory list in one of the directory windows. */
void dirlist_show(uint8_t which, struct dirlist *dir)
{
  unsigned int row, idx;
  display_setwin(which);
  putch(12);
  if (dir==0 || dir->n_entries == 0) {
    printf("Invalid directory");
  } else {
    idx = dir->top_idx;
    for (row = 0; row<dirwin_height; row++) {
      if (idx >= dir->n_entries)
	break;
      vdp_cursor_tab(0,row);
      dirlist_show_entry(&dir->entries[idx], row == (dir->sel_idx - dir->top_idx));
      idx++;
    }      
    display_curfile(which,dir->entries[dir->sel_idx].name);
  }
}

/* Move the cursor inside the directory list, 
   direction: negative number is n positions up, positive = n positions down
   update the displayed directory. */
void dirlist_move_cursor(uint8_t which, struct dirlist *dir, int direction)
{
  int old_row, new_row,new_idx;
  display_setwin(which);
  if (dir==0 || dir->n_entries == 0) {
    return;
  }
  old_row = dir->sel_idx - dir->top_idx;
  vdp_cursor_tab(0,old_row);
  dirlist_show_entry(&dir->entries[dir->sel_idx], false);
  new_idx = (signed)dir->sel_idx + direction;
  if (new_idx < 0) {
    new_idx = 0;
  } else if ((unsigned)new_idx >= dir->n_entries) {
    new_idx = dir->n_entries -1;
  }
  dir->sel_idx = new_idx;
  new_row = new_idx - dir->top_idx;
  if (new_row < 0 || new_row >= dirwin_height) {
    if (dirwin_height/2 > (unsigned)new_idx) {
      dir->top_idx = 0;
    } else {
      dir->top_idx = new_idx - dirwin_height/2;
    }
    dirlist_show(which,dir);
  } else {
    vdp_cursor_tab(0,new_row);
    dirlist_show_entry(&dir->entries[dir->sel_idx], true);    
    display_curfile(which,dir->entries[dir->sel_idx].name);
  }
}

/* Select or deselect the directory entry under the cursor */
void dirlist_select_file(struct dirlist *dir)
{
  if (dir==0 || dir->n_entries == 0) {
    return;
  }
  if (!dir->entries[dir->sel_idx].is_dir) {
    dir->entries[dir->sel_idx].is_tagged ^= 1;
  }
}


static bool is_match(char *name, char *pat)
{
  char *p;
  if (*name == 0 && *pat == 0) {
    return true;
  } else if (*pat=='?' && *name != 0) {
    return is_match(name+1, pat+1);
  } else if (*pat=='*') {
    p = name;
    do {
      if (is_match(p, pat+1)) {
	return true;
      }
    } while (*p++);
    return false;
  } else if (toupper(*pat) == toupper(*name)) {
    return is_match(name+1, pat+1);
  } else {
    return false;
  }
}

/* Select files in the current directory list according to the indicated 
   pattern Update display with marked items. */
void dirlist_select_pattern(uint8_t which, struct dirlist *dir, char *pat)
{
  unsigned int i;
  if (dir==0 || dir->n_entries == 0) {
    return;
  }
  for (i=0; i<dir->n_entries; i++) {
    if (!dir->entries[i].is_dir && is_match(dir->entries[i].name, pat)) {
      dir->entries[i].is_tagged = true;
    }
  }
  dirlist_show(which, dir);
}

/* Select files in the current directory list according to the indicated \
   pattern Update display with marked items. */
void dirlist_deselect_pattern(uint8_t which, struct dirlist *dir, char *pat)
{
  unsigned int i;
  if (dir==0 || dir->n_entries == 0) {
    return;
  }
  for (i=0; i<dir->n_entries; i++) {
    if (!dir->entries[i].is_dir && is_match(dir->entries[i].name, pat)) {
      dir->entries[i].is_tagged = false;
    }
  }
  dirlist_show(which, dir);
}

/* Return the number of selected files */
unsigned int dirlist_n_selected(struct dirlist *dir)
{
  unsigned int i,n;
  if (dir==0 || dir->n_entries == 0) {
    return 0;
  } else {
    n=0;
    for (i=0; i<dir->n_entries; i++) {
      if (dir->entries[i].is_tagged) n++;
    }
    return n;
  }
}


/* return boolean true if and only if currently selected item in
   the indicated directory list represents a subdirectory. */
bool dirlist_is_dir(struct dirlist *dir)
{
  if (dir==0 || dir->n_entries == 0) {
    return false;
  }
  return dir->entries[dir->sel_idx].is_dir;
}

/* Get name of currently selected item in directory list */
char * dirlist_get_name(struct dirlist *dir)
{
  if (dir==0 || dir->n_entries == 0) {
    return NULL;
  }
  return dir->entries[dir->sel_idx].name;
}


static unsigned int select_index;
char * dirlist_first_selected(struct dirlist *dir)
{
  select_index = 0;
  return dirlist_next_selected(dir);
}

char * dirlist_next_selected(struct dirlist *dir)
{
  if (dir==0 || dir->n_entries == 0) {
    return NULL;
  }
  while (select_index < dir->n_entries) {
    if(dir->entries[select_index].is_tagged)
      return dir->entries[select_index++].name;
    select_index++;
  }
  return NULL;
}
