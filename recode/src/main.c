/* Recode utility
   Convert text files between differnt code pages and Unicode.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <mos_api.h>

#define BUF_SIZE 4096
#define NAMELEN_MAX 64
#define RECODE_EOF 0xffffff


#include "../../loadfont/src/codepages.h"

const uint16_t *inpage, *outpage;

unsigned int in_bytes,out_bytes,in_chars,out_chars,bad_chars,untr_chars;

#define LINE_CONV_NONE 0
#define LINE_CONV_LF   1
#define LINE_CONV_CR   2
#define LINE_CONV_CRLF 3


FILE *infile;
FILE *outfile;

uint8_t inbuffer[BUF_SIZE];
uint8_t outbuffer[BUF_SIZE];
unsigned int inptr, insize, outptr;

// Dummy code pages for the Unicode variants.
const uint16_t dummy_utf8,dummy_utf16,dummy_utf16be;

#define LOW_UCODE_RANGE 0x500
#define HIGH_UCODE_BASE 0x1e00
#define HIGH_UCODE_RANGE 0x900

uint8_t ucode_to_bytes_low[LOW_UCODE_RANGE]; // Code points 0..0x4ff (latin,greek,cyrillic)
uint8_t ucode_to_bytes_high[HIGH_UCODE_RANGE]; // Code points 0x1e00..0x26ff (Celtic chars and various symbols) 

/* Table with pairs of Unicode alternatives. If one could not be found in the
   target code page and the other can be found, use the alternative. 
   
   Currently used to mitigate the Romanian S/T comma versus cedilla debacle.
*/
struct {
  uint16_t a1;
  uint16_t a2;
} alternative_table[] = {
  {0x15e, 0x218}, // Capital S with cedilla, comma
  {0x162, 0x21a}, // Capital T with cedilla, comma
  {0x15f, 0x219}, // lowercase S with cedilla, comma
  {0x163, 0x21b}, // lowercase T with cedilla, comma
};
#define N_ALTERNATIVES (sizeof(alternative_table)/sizeof(alternative_table[0]))

const uint16_t *set_code_page(char *name)
{
  unsigned int i;
  if (strcmp(name,"utf8")==0) {
    return &dummy_utf8;
  } else if (strcmp(name,"utf16")==0) {
    return &dummy_utf16;
  } else {
    for (i=0; i<N_CODEPAGES; i++) {
      if (strcmp(codetable[i].name,name)==0) {
	return codetable[i].cp;
      }
    }
    return NULL;
  }
}

uint8_t get_reverse_table_entry(unsigned int idx)
{
  if (idx < LOW_UCODE_RANGE)
    return ucode_to_bytes_low[idx];
  else if (idx >= HIGH_UCODE_BASE && idx < HIGH_UCODE_BASE+HIGH_UCODE_RANGE)
    return ucode_to_bytes_high[idx-HIGH_UCODE_BASE];
  else
    return 0;
}

void set_reverse_table_entry(unsigned int idx, uint8_t val)
{
  if (idx < LOW_UCODE_RANGE)
    ucode_to_bytes_low[idx] = val;
  else if (idx >= HIGH_UCODE_BASE && idx < HIGH_UCODE_BASE+HIGH_UCODE_RANGE)
    ucode_to_bytes_high[idx-HIGH_UCODE_BASE] = val; 
}

void setup_reverse_table(void)
{
  unsigned int i;
  for (i=0; i<32; i++) {
    set_reverse_table_entry(i,i); // control chars straight-through.
  }
  for (i=32; i<256; i++) {
    set_reverse_table_entry(outpage[i], i);
  }
  for (i=0; i<N_ALTERNATIVES; i++) {
    if (get_reverse_table_entry(alternative_table[i].a1) == 0) {
      set_reverse_table_entry(alternative_table[i].a1,
			      get_reverse_table_entry(alternative_table[i].a2));
    }
    if (get_reverse_table_entry(alternative_table[i].a2) == 0) {
      set_reverse_table_entry(alternative_table[i].a2,
			      get_reverse_table_entry(alternative_table[i].a1));
    }
  }
}

int open_files(char *filename)
{
  char namebuf[NAMELEN_MAX];
  int lastdot=-1;
  unsigned int res;
  int i=0;
  do {
    namebuf[i] = filename[i];
    if (filename[i]=='.') lastdot=i;    
  } while (filename[i++] !=0 && i<NAMELEN_MAX);
  
  if(lastdot < 0) lastdot=i-1;

  if (lastdot >= NAMELEN_MAX-5) {
    printf("File name too long\n");
    return 19;
  }
  strcpy(namebuf+lastdot,".bak");

  mos_del(namebuf); // Delete old backup file.
  if ((res = mos_ren(filename, namebuf)) != 0) // rename source file to backup.
    return res;
  infile=fopen(namebuf, "rb");
  outfile=fopen(filename,"wb");
  inptr = 0;
  outptr = 0;
  return 0;
}

void close_files(void)
{
  if (outptr > 0) {
    fwrite(outbuffer, 1, outptr, outfile);
  }
  fclose(infile);
  fclose(outfile);
}

unsigned int in_getc(void)
{
  if (inptr ==  insize) {
    inptr = 0;
    insize = fread(inbuffer, 1, BUF_SIZE, infile);
    if (insize <= 0) {
      return RECODE_EOF;
    }
  }
  in_bytes++;
  return inbuffer[inptr++];
}

void in_ungetc(uint8_t ch)
{
  inbuffer[--inptr] = ch;
  in_bytes--;
}

void out_putc(uint8_t ch)
{
  if (outptr == BUF_SIZE) {
    fwrite(outbuffer, 1, outptr, outfile);
    outptr = 0;
  }
  out_bytes++;
  outbuffer[outptr++] = ch;
}

uint8_t expect_trailer(void)
{
  unsigned int c = in_getc();
  if (c==RECODE_EOF) {
    printf("Unexpected end of file\n");
    bad_chars++;
    return 0;
  } else if (c>=0x80 && c<0xc0) {
    return c;
  } else {
    in_ungetc(c);
    printf("Missing UTF8 trailer\n");
    bad_chars++;
    return 0;
  }
}

unsigned int nextchar(void)
{
  unsigned int w1,w2;
  unsigned int c1,c2;
  if (inpage==&dummy_utf16||inpage==&dummy_utf16be) {
    w2=0xffffff;w1=0xffffff;
    do {
      if (w1 != 0xffffff) {
	printf("Two high surrogates in a row\n");
	bad_chars++;
      }
      w1 = w2;
      c1=in_getc();
      if (c1==RECODE_EOF) return RECODE_EOF;
      c2=in_getc();  
      if (c2==RECODE_EOF) return RECODE_EOF;
      if (inpage==&dummy_utf16be) {
	w2 = (c1<<8)|c2;
      } else {
	w2 = (c2<<8)|c1;
      }	
      if (w2==0xfffe && in_bytes==2) { // Found byte order mark for big-endian
	inpage=&dummy_utf16be;
	return 0xfeff;
      }
    } while (w2>=0xd800 && w2<0xdc00); // Read until not a high surrogate
    if (w2>=0xdc00 && w2<0xe000) {
      // Have a low surrogate 
      if (w1 != 0xffffff) {
	// Correct non-BMP char, represented by high and low surrogate
	in_chars++;
	return 0x10000 + ((w1 & 0x3ff)<<10) + (w2 & 0x3ff);
      } else {
	printf("Low surrogate not following a high surrogate\n");
	bad_chars++;
	return 0xfffd;
      }
    } else {
      if (w1 != 0xffffff) {
	printf("High surrogate not followd by low surrogate\n");
	bad_chars++;
      }
      in_chars++;
      return w2;
    }    
  } else if (inpage==&dummy_utf8) {
    c1 = in_getc();
    if (c1 < 0x80) {
      in_chars++;
      return c1;
    } else if (c1 < 0xc0) {
      printf("UTF-8 trailer byte unexpected\n");
      bad_chars++;
      return 0xfffd;
    } else if (c1 < 0xE0) {
      c2 = expect_trailer();
      if (c2==0) return 0xfffd;
      w1 = ((c1-0xc0) << 6) | (c2 & 0x3f);
      if (w1 < 0x80) {
	printf("Overly long encoding\n");
	bad_chars++;
      }
      in_chars++;
      return w1;
    } else if (c1 < 0xf0) {
      c2 = expect_trailer();
      if (c2==0) return 0xfffd;
      w1 = ((c1-0xe0) << 12) | ((c2 & 0x3f) << 6);
      c2 = expect_trailer();
      if (c2==0) return 0xfffd;
      w1 |= (c2 & 0x3f);
      if (w1 < 0x800) {
	printf("Overly long encoding\n");
	bad_chars++;
      }
      in_chars++;
      return w1;      
    } else if (c1 < 0xf8) {
      c2 = expect_trailer();
      if (c2==0) return 0xfffd;
      w1 = ((c1-0xf0) << 18) | ((c2 & 0x3f) << 12);
      c2 = expect_trailer();
      if (c2==0) return 0xfffd;
      w1 |= (c2 & 0x3f) << 6;
      c2 = expect_trailer();
      if (c2==0) return 0xfffd;
      w1 |= (c2 & 0x3f);
      if (w1 < 0x10000) {
	printf("Overly long encoding\n");
	bad_chars++;
      } else if (w1 > 0x10ffff) {
	printf("Outside unicode range\n");
	bad_chars++;
	return 0xfffd;
      }
      in_chars++;
      return w1;      
    } else if (c1 == RECODE_EOF) {
      return c1;
    } else {
      printf("Unexpected char in UTF-8\n");
      bad_chars++;
      return 0xfffd;
    }
  } else {
    c1=in_getc();
    if (c1<32) { 
      in_chars++;
      return c1;
    } else if (c1==RECODE_EOF) {
      return RECODE_EOF;
    } else if (inpage[c1] != 0) { 
      in_chars++;
      return inpage[c1];
    } else {
      bad_chars++;
      return 0xfffd;
    }
  }
}
  
void writechar(unsigned int ch)
{
  unsigned int w1,w2;
  if (ch == 0xfeff && out_chars==0) {
    return; // Do not write BOM 
  } else if (outpage == &dummy_utf16) {
    out_chars++;
    if (out_bytes==0) {
      out_putc(0xff);out_putc(0xfe); // Write initial BOM
    }
    if (ch >= 0x10000) {
      w1 = 0xd800 + ((ch>>10) & 0x3ff);
      w2 = 0xdc00 + (ch & 0x3ff);
      out_putc(w1 & 0xff); out_putc(w1 >> 8);
      out_putc(w2 & 0xff); out_putc(w2 >> 8);
    } else {
      out_putc(ch & 0xff); out_putc(ch >> 8);      
    }
  } else if (outpage == &dummy_utf8) {
    out_chars++;
    if (ch < 0x80) {
      out_putc(ch);
    } else if (ch < 0x800) {
      out_putc(0xc0 + (ch>>6));
      out_putc(0x80 + (ch & 0x3f));
    } else if (ch < 0x10000) {
      out_putc(0xe0 + (ch>>12));
      out_putc(0x80 + ((ch>>6) & 0x3f));
      out_putc(0x80 + (ch & 0x3f));
    } else {
      out_putc(0xe0 + (ch>>18));
      out_putc(0x80 + ((ch>>12) & 0x3f));
      out_putc(0x80 + ((ch>>6) & 0x3f));
      out_putc(0x80 + (ch & 0x3f));
    }		   
  } else {
    w1 = get_reverse_table_entry(ch);
    if (w1==0) {
      untr_chars++;
      out_putc(0x3f);
    } else {
      out_chars++;
      out_putc(w1);
    }
  }     
}

void usage(void)
{
  unsigned int i;;
  printf("Usage: recode [cr|lf|crlf] <src-encoding> <dst-encoding> <file>\n"
	 "    or recode cr|lf|crlf <file>\n"
	 "    Original file is retained with *.bak suffix\n"
	 "    Convert text file to different encoding.\n"
	 "    Can convert the end-of-line convention by itself or combined\n"
	 "    with character recoding\n"
	 "    src-encoding, dst-encoding can be utf8, utf16 or any of:\n");
  for (i=0; i<N_CODEPAGES; i++) {
    printf("%s ",codetable[i].name);
  }
}

int main(int argc, char *argv[])
{
  unsigned int ch;
  unsigned int char_to_suppress;
  int res;
  int lineconv = LINE_CONV_NONE;
  char *fname;
  if (argc < 1) {
    usage(); return 19;
  }
  if (strcmp(argv[1],"cr") == 0) {
    lineconv = LINE_CONV_CR;
  } else if (strcmp(argv[1],"lf") == 0) {
    lineconv = LINE_CONV_LF;
  } else if (strcmp(argv[1],"crlf") == 0) {
    lineconv = LINE_CONV_CRLF;
  }
  if (lineconv == LINE_CONV_NONE && argc==4) {
      inpage = set_code_page(argv[1]);
      outpage = set_code_page(argv[2]);
      fname = argv[3];
  } else if (lineconv != LINE_CONV_NONE && argc==5) {
      inpage = set_code_page(argv[2]);
      outpage = set_code_page(argv[3]);
      fname = argv[4];
  } else if (lineconv != LINE_CONV_NONE && argc==3) {
    /* no source & destination encoding specified:
       Character encoding straight through */
    inpage = set_code_page("iso8859-1");
    outpage = inpage;
    fname = argv[2];
  } else {
    usage(); return 19;
  }      
  if (inpage == 0) {
    printf("Invalid source code page\n");
    return 19;
  }
  if (outpage == 0) {
    printf("Invalid destination code page\n");
    return 19;
  }
  if (outpage != &dummy_utf8 && outpage != &dummy_utf16) {
    setup_reverse_table();
  }
  if ((res=open_files(fname))!= 0) {
    return res;
  }
  char_to_suppress = RECODE_EOF;
  for (;;) {
    ch=nextchar();
    if (ch==RECODE_EOF) break;
    if (ch==char_to_suppress) {
      char_to_suppress = RECODE_EOF; // Next character will not be suppressed.
    } else if (lineconv != LINE_CONV_NONE) {
      if (ch == 10) {
	char_to_suppress = 13;
      } else if (ch == 13) {
	ch = 10;
	char_to_suppress = 10;
      } else {
	char_to_suppress = 0;
	writechar(ch);
      }
      if (ch == 10) {
	switch(lineconv) {
	case LINE_CONV_CR:
	  writechar(13);
	  break;
	case LINE_CONV_LF:
	  writechar(10);
	  break;
	case LINE_CONV_CRLF:
	  writechar(13);writechar(10);
	  break;
	}
      }
    } else {
      writechar(ch);
    }
  }
  close_files();
  printf("%u bytes read, %u bytes written, %u chars read, %u chars written\n",
	 in_bytes, out_bytes, in_chars, out_chars);
  printf("%u conversion errors, %u untranslated chars\n",bad_chars,untr_chars);
  return 0;
}
