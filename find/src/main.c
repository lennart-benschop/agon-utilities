/* find
 * search the entire file system for file names conforming to a certain pattern
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <mos_api.h>


#define MAX_DIRS 200
char *dirnames[MAX_DIRS];
int write_idx=0;
int read_idx=0;

static bool glob_is_match(char *name, char *pat)
{
  char *p;
  if (*name == 0 && *pat == 0) {
    return true;
  } else if (*pat=='?' && *name != 0) {
    return glob_is_match(name+1, pat+1);
  } else if (*pat=='*') {
    p = name;
    do {
      if (glob_is_match(p, pat+1)) {
	return true;
      }
    } while (*p++);
    return false;
  } else if (toupper(*pat) == toupper(*name)) {
    return glob_is_match(name+1, pat+1);
  } else {
    return false;
  }
}

int
main(int argc, char *argv[])
{
  DIR dir_struct;
  FILINFO file_struct;
  int res;
  char namebuf[256];
  if (argc < 2) {
    fprintf(stderr,"Usage: find <pattern>\n");
    return 19;
  }
  dirnames[write_idx++] = strdup("/");
  while (dirnames[read_idx] != NULL) {
      res = ffs_dopen(&dir_struct,dirnames[read_idx]);
      if (res == 0) {
	for (;;) {
	  res = ffs_dread(&dir_struct,&file_struct);
	  if (res != 0 || file_struct.fname[0]==0)
	    break;
	  strcpy(namebuf, dirnames[read_idx]);
	  if (dirnames[read_idx][1] != 0)
	    strcat(namebuf,"/");
	  strcat(namebuf, file_struct.fname);
	  if ((file_struct.fattrib & 0x10) != 0) {
	    if (write_idx < MAX_DIRS-1)
	      dirnames[write_idx++] = strdup(namebuf);
	  } else {
	    if (glob_is_match(file_struct.fname,argv[1]))
		printf("%s\n",namebuf);
	  }
	}
      }
      read_idx++;
  }
  return 0;	 
}
