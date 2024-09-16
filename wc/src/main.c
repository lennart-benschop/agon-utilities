/* wc
 * count lines, words and characters in one or more files.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <mos_api.h>


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

bool count_lines, count_words, count_chars;
unsigned long lines,words,chars, total_lines, total_words, total_chars;


static char buf[1024];
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

static void show_line(unsigned long lines,
		      unsigned long words,
		      unsigned long chars,
		      const char *fname)
{
  if(count_lines) printf("%9lu ",lines);
  if(count_words) printf("%9lu ",words);
  if(count_chars) printf("%9lu ",chars);
  printf("%s\n",fname);
}

static void count_file(char *fname)
{
  int c,prev_c;
  f=fopen(fname,"r");
  if (!f) {
    fprintf(stderr,"Error opening file %s\n",fname);
    return;
  }
  lines=0;
  words=0;
  chars=0;
  file_idx = 0;
  buf_filled = 0;
  prev_c=0;
  while ((c=nextchar()) != -1) {
    chars++;
    if(c=='\n') lines++;
    if (c>32 && prev_c <= 32) words++;
    prev_c = c;
  }
  fclose(f);
  total_lines +=lines;
  total_words +=words;
  total_chars +=chars;
  show_line(lines,words,chars,fname);
}


int
main(int argc, char *argv[])
{
  int nopts = 0;
  DIR dir_struct;
  FILINFO file_struct;
  int res;
  unsigned int nfiles=0;
  
  for (;;) {
    if (argc < nopts+2) {
      fprintf(stderr,"Usage: wc [-l] [-w] [-c] <files>\n");
      return 19;
    }
    if (strcmp(argv[nopts+1],"-l")==0) {
      count_lines = true;
      nopts += 1;
    } else if (strcmp(argv[nopts+1],"-w")==0) {
      count_words = true;
      nopts += 1;
    } else if (strcmp(argv[nopts+1],"-c")==0) {
      count_chars = true;
      nopts += 1;
    } else {
      break;
    }
  }
  if (!count_lines && !count_words && !count_chars) {
    count_lines = true;
    count_words = true;
    count_chars = true;
  }
  res = ffs_dopen(&dir_struct,".");
  if (res == 0) {
    for (;;) {
      res = ffs_dread(&dir_struct,&file_struct);
      if (res != 0 || file_struct.fname[0]==0)
	break;
      if ((file_struct.fattrib & 0x10) == 0 &&
	  glob_is_match(file_struct.fname,argv[1+nopts])) {
	nfiles++;
	count_file(file_struct.fname);
      }
    }
    ffs_dclose(&dir_struct);
  }
  if (nfiles>1) show_line(total_lines, total_words, total_chars, "total");
  return 0;	 
}

