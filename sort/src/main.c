/* sort
 * sort a text file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <mos_api.h>

#define MAX_LINES 8000

int my_strcasecmp(const char *p,const char *q)
{
  char c1,c2;
  for(;;) {
    c1 = *p++;
    c2 = *q++;
    if (c1 >= 'a' && c1 <= 'z') c1-=0x20; 
    if (c2 >= 'a' && c2 <= 'z') c2-=0x20;
    if (c1 != c2) return c1-c2;
    if ((c1 | c2 ) == 0) return 0;
 }
}

bool nocase;
bool reverse;

static int line_compare(const void *a, const void *b)
{
  const char * const *l1 = a;
  const char * const *l2 = b;
  int v;
  if (nocase)
    v=my_strcasecmp(*l1,*l2);
  else
    v=strcmp(*l1,*l2);
  if (reverse)
    v=-v;
  return v;
}


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

char *lines[MAX_LINES];
unsigned int nlines=0;

int
main(int argc, char *argv[])
{
  int nopts = 0;
  unsigned int i;

  for (;;) {
    if (argc < nopts+2) {
      fprintf(stderr,"Usage: sort [-f][-r] <file>\n");
      return 19;
    }
    if (strcmp(argv[nopts+1],"-f")==0) {
      nocase = true;
      nopts += 1;
    } else if (strcmp(argv[nopts+1],"-r")==0) {
      reverse = true;
      nopts += 1;
    } else {
      break;
    }
  }
  f = fopen(argv[1+nopts],"r");
  while (nlines<MAX_LINES && nextline()) {
    lines[nlines++] = strdup(linebuf);    
  }
  fclose(f);
  qsort(lines, nlines, sizeof(char*),line_compare);
  for (i=0; i<nlines; i++) {
    printf("%s\n",lines[i]);	  
  }  
  return 0;	 
}
