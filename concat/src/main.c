/* find
 * search the entire file system for file names conforming to a certain pattern
 */

#include <stdio.h>


#define BUF_SIZE 1024
unsigned char buf[BUF_SIZE];

int
main(int argc, char *argv[])
{
  FILE *f;
  int i,j;
  int nbytes;
  if (argc < 2) {
    fprintf(stderr,"Usage: concat <pattern+>\n");
    return 19;
  }

  for (i=1; i<argc; i++) {
    f=fopen(argv[i],"rb");
    if (f==NULL) {
      fprintf(stderr,"Cannot open file %s\n",argv[i]);
      continue;
    }
    while ((nbytes=fread(buf,1,BUF_SIZE,f)) > 0) {
      for (j=0; j<nbytes; j++) {
	if (buf[j] != '\r') fputc(buf[j],stdout);
      }
    }
    fclose(f);
  }
  return 0;	 
}
