/* grep
 * search a file for a certain string.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <mos_api.h>

static char *my_index(char *s, char c)
{
  while (*s) {
    if (*s == c)
      return s;
    s++;
  }
  return NULL;
}

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

bool nocase = false;
bool singlefile = false;

static char buf[1024];
static char linebuf[256];
int file_idx = 0;
int buf_filled = 0;
FILE *f;
static int nextchar(void)
{
  if (file_idx == buf_filled) {
    buf_filled = fread(buf, 1, 1024, f);
    if (buf_filled == 0) {
      return -1;
    }
    file_idx = 0;
  }
  return buf[file_idx++];
}

static bool nextline(void)
{
  int linelength = 0;
  int c;
  for (;;) {
    c=nextchar();
    if (c==-1) {
      linebuf[linelength] = 0;
      return linelength != 0;
    }
    else if (c=='\n') {
      linebuf[linelength] = 0;
      return true;
    }
    else if (linelength==255) {
      linebuf[linelength] = 0;
      return true;      
    } else {
      linebuf[linelength++]=c;
    }
  }
}

static bool str_match(char *line,char *pat)
{
  char *p=line;
  char *q, *r;
  char c1, c2;
  for (;;) {
    q=p; p++;
    r=pat;
    for (;;) {
      c1=*q++;
      c2=*r++;
      if (c2==0) return true;
      if (c1==0) return false;
      if (nocase) {
	if (c1 >= 'a' && c1 <= 'z') c1-=0x20; 
	if (c2 >= 'a' && c2 <= 'z') c2-=0x20;
      }
      if (c1 != c2) break;
    }
  }  
}

static void search_file(char *fname, char *string_pat)
{
  f=fopen(fname,"r");
  if (!f) {
    fprintf(stderr,"Error opening file %s\n",fname);
    return;
  }
  file_idx = 0;
  buf_filled = 0;
  while (nextline()) {
    if (str_match(linebuf,string_pat)) {
      if (!singlefile)
	printf("%s:",fname);
      printf("%s\n",linebuf);
    }
  }
  fclose(f);
}


int
main(int argc, char *argv[])
{
  int nopts = 0;
  DIR dir_struct;
  FILINFO file_struct;
  int res;
  char *pat;

  for (;;) {
    if (argc < nopts+3) {
      fprintf(stderr,"Usage: grep [-i] <string> <files>\n");
      return 19;
    }
    if (strcmp(argv[nopts+1],"-i")==0) {
      nocase = true;
      nopts += 1;
    } else {
      break;
    }
  }
  pat = argv[1+nopts];
  if (nopts+3 == argc &&
      my_index(argv[nopts+2],'*')==0 &&
      my_index(argv[nopts+2],'?')==0) { /* single file no wildcard */
    singlefile = true;
  }
  for (; nopts<argc-2; nopts++) {
    res = ffs_dopen(&dir_struct,".");
    if (res == 0) {
      for (;;) {
	res = ffs_dread(&dir_struct,&file_struct);
	if (res != 0 || file_struct.fname[0]==0)
	  break;
	if ((file_struct.fattrib & 0x10) == 0 &&
	    glob_is_match(file_struct.fname,argv[2+nopts])) {
	  search_file(file_struct.fname,pat);
	}
      }
      ffs_dclose(&dir_struct);
    }
  }
  return 0;	 
}
