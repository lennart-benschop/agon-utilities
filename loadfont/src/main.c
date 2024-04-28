/* 
   loadfont -- Load font into a VDP buffer.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <mos_api.h>

#include "codepages.h"

#define VERBOSE 0

#define MAX_FONTBUF_SIZE 36000

#define SYSFONT_ID 0xffff

#define CODE_PAGE_DEFAULT 0
#define CODE_PAGE_NONE 1
#define CODE_PAGE_UPPER 2


static unsigned int
parse_int(char *p)
{
  unsigned char base=10;
  unsigned char digit,c;
  unsigned int res = 0;
  if (*p=='&') {
    base=16;
    p++;
  }
  while (*p > 32) {
    c = toupper(*p++);
    if (base == 16 && c >='A' && c <= 'F')
      digit = c - 'A' + 10;
    else if (c >= '0' && c <= '9')
      digit = c - '0';
    else
      break;
    res = res * base + digit;
  }
  
  return res;
}


unsigned char fontbuf[MAX_FONTBUF_SIZE];
unsigned int font_bytes;
bool is_psf;
unsigned int n_glyphs;
unsigned char font_width;
unsigned char font_height;
unsigned char code_page = CODE_PAGE_DEFAULT;
char *cp_name = "cp1252";
unsigned int glyphs_start;
unsigned int glyph_bytes;
unsigned int utable_ptr;
unsigned char hdr_type;

uint8_t nullglyph[64];

uint8_t *glyphptrs[256];

static int
parse_header(void)
{
  uint32_t *p=(uint32_t*)fontbuf;
  if (!is_psf) {
    code_page = CODE_PAGE_NONE;
    n_glyphs = 256;
    font_width = 8;
    if (font_bytes < 2048 || font_bytes % 256 != 0)
      return 1;
    font_height = font_bytes/n_glyphs;
    glyph_bytes = font_height;
  } else if (fontbuf[0]==0x36 && fontbuf[1]==0x04) {
    hdr_type = 1;
    glyphs_start = 4;
    if (fontbuf[2] & 0x1)
      n_glyphs = 512;
    else
      n_glyphs = 256;
    font_width = 8;
    font_height = fontbuf[3];
    glyph_bytes = font_height;
    if ((fontbuf[2] & 0x6) != 0)
      utable_ptr = glyphs_start + n_glyphs * glyph_bytes;
  } else if (p[0] == 0x864ab572) {
    hdr_type = 2;
    glyphs_start = 32;
    n_glyphs = p[4];
    glyph_bytes = p[5];
    font_height = p[6];
    font_width = p[7];
    if ((p[3] & 0x1) != 0)
      utable_ptr = glyphs_start + n_glyphs * glyph_bytes;
  } else {
    return 1;
  }
  if (utable_ptr >= font_bytes)
    return 1;
  return 0;
}

/* Decode the next number from the Unicode table of the font */
static uint16_t
next_ucode(void)
{
  uint16_t w;
  uint8_t c;
  if (utable_ptr >= font_bytes) {
    return 0xffff;
    } else if (hdr_type == 1) { /* 16-bit little-endian */
    w = fontbuf[utable_ptr] + 256*fontbuf[utable_ptr+1];
    utable_ptr+=2;
    return w;
  } else { /* Header type 2, decode UTF-8 bytes, special cases 0xfe and 0xff */
    c = fontbuf[utable_ptr++];
    if (c<128) {
      return c;
    } else if (c==0xfe) {
      return 0xfffe;
    } else if (c>=192 && c<224) {
      w = (c & 0x1f) << 6;
      c = fontbuf[utable_ptr++];
      w = w | (c & 0x3f);
      return w;
    } else if (c>=224 && c<240) {
      w = (c & 0x0f) << 12;
      c = fontbuf[utable_ptr++];
      w = w | ((c & 0x3f) << 6);
      c = fontbuf[utable_ptr++];
      w = w | (c & 0x3f);
      return w;
    } else if (c>=240 && c<248) {
      /* Consume non-BMP codepoints but don't care about value */
      c = fontbuf[utable_ptr++]; 
      c = fontbuf[utable_ptr++];
      c = fontbuf[utable_ptr++];
      return 0xfffd;
    } else {
      return 0xffff;
    }
  }
}

static int
translate_codepoints(void)
{
  unsigned int i,j;
  unsigned int n_translated = 0;
  const uint16_t *cp_p = NULL;
  uint16_t w;
  if (code_page == CODE_PAGE_UPPER && n_glyphs >= 512) {
    /* Want to load upper 256 glyphs from 512 glyphs font */
    for (i=0; i<256; i++) {
      glyphptrs[i] = fontbuf + glyphs_start + (256+i)*glyph_bytes;
    }
  } else if (code_page == CODE_PAGE_NONE || utable_ptr == 0) {
    /* No translation ordered and/or no table available.*/
    for (i=0; i<256; i++) {
      glyphptrs[i] = fontbuf + glyphs_start + i*glyph_bytes;
    }
  } else {
    for (i=0; i<N_CODEPAGES; i++) {
      if (strcmp(codetable[i].name,cp_name)==0) {
	cp_p = codetable[i].cp;
	break;
      }
    }
    if (cp_p == NULL) {
      printf("Unknown code page\n");
      return 1;
    }
    for (i=0; i<256; i++)
      glyphptrs[i] = &nullglyph[0];
    i=0;
    for (;;)  {
      if (i==n_glyphs) break;
      w=next_ucode();
      if (w==0xfffe) { /* Don't want to parse sequences */
	do {w=next_ucode();} while(w != 0xffff);
      }
      if (w==0xffff) {
	i++;         /* Next glyph */
      } else {
	/* Glyph i represents unicode point w */
	for (j=0; j<256; j++) {
	  if (cp_p[j] == w) {
#if VERBOSE	
	    printf("Glyph %d unicode 0x%04x at code page index %d\n",i,w,j);
#endif	    
	    glyphptrs[j] = fontbuf + glyphs_start + i*glyph_bytes;
	    n_translated ++;
	    break;
	  }
	}
#if VERBOSE	
	if (j==256) printf("Glyph %d unicode 0x%04x not used\n",i,w);
#endif	
      }
    }    
    printf("Number of glyphs used: %d\n",n_translated);
  }
  return 0;  
}


static void
program_buffer(unsigned int bufid)
{
  uint8_t ascent = font_height*3/4+1;
  uint16_t n_bytes = 256 * glyph_bytes;
  const uint8_t *p;
  unsigned int i,j;
  if (bufid == SYSFONT_ID) {
    for (i=0; i<256; i++) {
      p = glyphptrs[i];
      putch(23);putch(0);putch(0x90);putch(i);
      for (j=0; j<8; j++) {
	putch(*p++);
      }
    }
  } else {
    putch(23);putch(0);putch(0xa0);putch(bufid & 0xff);putch(bufid >> 8);putch(2);
    putch(23);putch(0);putch(0xa0);putch(bufid & 0xff);putch(bufid >> 8);putch(0);
    putch(n_bytes & 0xff);putch(n_bytes >> 8);
    for (i=0; i<256; i++) {
      p = glyphptrs[i];
      for (j=0; j<glyph_bytes; j++) {
	putch(*p++);
      }
    }
    putch(23);putch(0);putch(0x95);putch(1);putch(bufid & 0xff);putch(bufid >> 8);
    putch(font_width);putch(font_height);putch(ascent);putch(0);
  }
}

int
main(int argc, char**argv)
{
  unsigned int bufid;
  FILE *fontfile;
  unsigned int namelen;
  unsigned int i;
  if (argc < 3) {
    printf("Usage: loadfont <bufid> <file> [<codepage>]\n"
           "    bufid can be 'sys' or an integer in range 0..65534\n"
           "    file can be raw binary or (uncompressed) psf\n"
           "    codepage can be none, upper or any of:\n");
    for (i=0; i<N_CODEPAGES; i++) {
      printf("%s ",codetable[i].name);
    }
    printf("\n\n    none = no translation, upper is upper range of 512-char font\n");
    return 19;
  }
  if (strcmp(argv[1],"sys") == 0)
    bufid = SYSFONT_ID;
  else
    bufid = parse_int(argv[1]);
  namelen = strlen(argv[2]);
  if (namelen>3 && (strcmp(argv[2]+namelen-4,".psf") == 0 ||
		    strcmp(argv[2]+namelen-4,".PSF") == 0)) {
    is_psf = true;
    printf("PSF file\n");
  } else {
    is_psf = false;
    printf("Raw binary file\n");
  }
  fontfile = fopen(argv[2],"rb");
  if (fontfile == NULL) {
    return 4;
  }
  font_bytes = fread(fontbuf, 1, MAX_FONTBUF_SIZE, fontfile);
  fclose(fontfile);
  if (argc > 3) {
    if (strcmp(argv[3],"none") == 0) {
      code_page = CODE_PAGE_NONE;
    } else if (strcmp(argv[3],"upper") == 0) {
      code_page = CODE_PAGE_UPPER;
    } else {
      code_page = CODE_PAGE_DEFAULT;
      cp_name = argv[3];
    }
  }
  
  if (parse_header() != 0) {
    printf("No valid PSF header\n");
    return 19;
  }
  printf("Font has %d glyphs, width=%d height=%d\n",n_glyphs,font_width,font_height);
  if (bufid == SYSFONT_ID && (font_width != 8 || font_height != 8)) {
    printf("System font only accepts 8x8 size chars\n");
    return 19;
  }
  if (font_width > 16 || font_height > 32) {
    printf("Font dimensions out of range\n");
    return 19;
  }
  if (translate_codepoints() != 0) {
    printf("Translation failed\n");
    return 19;
  }
  program_buffer(bufid);
  if (bufid == SYSFONT_ID) {
    printf("Successfully programmed the system font\n");
  } else {
    printf("Successfully programmed the font, type the following command to select it:\n"
	   "fontctl %s\n",argv[1]);
  }
  return 0;
}
