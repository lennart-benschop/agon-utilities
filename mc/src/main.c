/* 12AM COmmander, a Midnight Commander Lookalike for Agon Light.
   11/05/2024
   23/06/2024: added mode/font/video mode config
   21/07/2024: do case-insensitive compare. do not add path to filename in
               commands.
   15/06/2025: fix external command execution.
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mos_api.h>
#include <agon/vdp_vdu.h>
#include "mc.h"


int my_strcasecmp(char *p,char *q)
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

char dirname_left[256];
char dirname_right[256];
struct dirlist * dirlist_left;
struct dirlist * dirlist_right;
uint8_t which_dir = 1;

bool fQuit;

#define N_EXTENSIONS 20

/* Addresses in internal RAM for launcher */
#define LAUNCHER_EXT_EXEC     0xb7f000
#define LAUNCHER_EXT_CMDLINE  0xb7f100
#define LAUNCHER_OWN_CMDLINE  0xb7f200

static char viewer_cmd[256];
static char editor_cmd[256];

uint8_t video_mode = 3;
uint16_t font_nr = 65535; 
uint8_t fgcol = 15; /* Foreground colour */
uint8_t bgcol = 0;  /* Background colour */
uint8_t hlcol = 11; /* Hilite colour */


void execute_command(char *cmdline,bool fWait)
{
  int res;
  display_finish();
  cmdline[strlen(cmdline)+1] = 0;
  res = mos_oscli(cmdline,NULL,0); /* OSCLI will do internal commands and moslets*/
  //putch(12);printf("Command line: \'%s\'\n",cmdline);getch();
  if (res == 20) { /* Try it as an external command (execute from /bin) */
    char *q = cmdline;
    char *p = (char*)LAUNCHER_EXT_EXEC;
    /* Put full name of executable at LAUNCHER_EXT_EXEC  */
    if (q[0]=='/') {
      while (*q!=0 && *q!=' ') {
	*p++ = *q++;
      }
      *p = 0;
      q++;
    } else {
      strcpy(p,"/bin/");
      p+=strlen(p);
      while (*q!=0 && *q!=' ') {
	*p++ = *q++;
      }
      *p = 0;
      q++;
      strcat(p,".bin");
    }
    /* Prepare the run command in the launcher */
    p = (char*)LAUNCHER_EXT_CMDLINE;
    strcpy(p,"run . ");
    strcat(p,q);
    /* When re-launching the commander afterwards, pass it the currently 
       selected directories*/
    p = (char*)LAUNCHER_OWN_CMDLINE;  
    strcpy(p,dirname_left);
    strcat(p," ");
    strcat(p,dirname_right);
    if (which_dir == 2)
      strcat(p," r");
    /* Will quit the commander now */
    fQuit = true;
  } else {
    /* Internal command or moslet returned */
    if (fWait) {
      printf("Press any key to return\n");getch();
    }
    /* Rebuild the commander screen */
    display_init();
    display_frame();
    display_curdir(1,dirname_left,which_dir==1);
    display_curdir(2,dirname_right,which_dir==2);
    dirlist_show(1, dirlist_left);
    dirlist_show(2, dirlist_right);
  }
}

char cmdbuf[256];
char pathbuf[256];

void set_pathbuf(bool fFullPath)
{
  if (fFullPath) {
    strcpy(pathbuf,which_dir==1?dirname_left:dirname_right);
    if (strcmp(pathbuf,"/")!=0) {
      strcat(pathbuf,"/");
    }
  } else {
    pathbuf[0]=0;
  }
  strcat(pathbuf,dirlist_get_name(which_dir==1?dirlist_left:dirlist_right));
}

bool has_ext(char *name, char* ext)
{
  unsigned int i;
  char *p,*q;
  if (strlen(name) <= strlen(ext)) {
    return false;
  } else if (name[strlen(name)-strlen(ext)-1] != '.') {
    return false;
  } else {
    p=name+strlen(name)-strlen(ext);
    q=ext;
    for (i=0; i<strlen(ext); i++) {
      if (toupper(*p) != toupper(*q)) return false;
      p++; q++;
    }
    return true;
  }
}


static void cmd_chdir(uint8_t which_dir,
		     struct dirlist **dir_list,
		     char *dirname)
{
  char *newdirname = dirlist_get_name(*dir_list);
  char *p;
  if (strcmp(newdirname,"..")==0) {
    /* We are moving our way up in the directory tree */
    p = dirname + strlen(dirname);
    while (*--p != '/')
      ;
    if (p==dirname)
      dirname[1]=0;
    else
      *p = 0;
  } else {
    /* Add subdir name to current directory path */
    if (strcmp(dirname, "/") != 0) {
      strcat(dirname, "/");
    }
    strcat(dirname, newdirname);
  }
  display_curdir(which_dir, dirname, true);
  dirlist_free(*dir_list);
  *dir_list = dirlist_read(dirname);
  dirlist_show(which_dir, *dir_list);
}

void cmd_reload_dir(void)
{
  dirlist_free(dirlist_left);
  dirlist_free(dirlist_right);
  dirlist_left = dirlist_read(dirname_left);
  dirlist_right = dirlist_read(dirname_right);
  dirlist_show(1, dirlist_left);
  dirlist_show(2, dirlist_right);
}

struct {
  char *ext;
  char *cmdline;
  bool do_pause;
} cmd_extensions[N_EXTENSIONS];
unsigned int n_extensions;

char * scan_word(char*dest, char *src)
{
  char *p = src;
  unsigned int i=0;
  dest[0]=0;
  while (*p++ == ' ')
    ;
  p--;
  while (*p != ' ' && *p != 0) {
    dest[i++] =*p++;
  }
  dest[i] = 0;
  return p;     
}

/* Replacement for fgets as the one in AgDev appears to be broken,
   does not detect EOF on real machine */
static char * my_fgets(char *s, unsigned int maxlen, FILE *f)
{
  unsigned int i;
  char c;
  for(i=0; i<maxlen-1;) {
    if (fread(&c, 1, 1, f) <= 0) break;
    if (c=='\r') continue;
    s[i] = c;
    i++;
    if (c=='\n') break;
  }
  s[i] = 0;
  if (i==0)
    return 0;
  else
    return s;
}

void read_cfg_file(void)
{
  FILE * infile = fopen("/bin/12amc.cfg","r");
  char *p;
  bool do_pause;
  if (infile != 0) {
    while(my_fgets(cmdbuf, 256, infile) != NULL) {
      if (cmdbuf[0] == '\n' || cmdbuf[0] == '#') continue;
      cmdbuf[strlen(cmdbuf)-1] = 0; /* Get rid of trailing newline */
      p = cmdbuf;
      p = scan_word(pathbuf,p);
      if (my_strcasecmp(pathbuf,"view") == 0) {
	while (*p==' ') p++;
	strcpy(viewer_cmd,p);
      } else if (my_strcasecmp(pathbuf,"edit") == 0) {
	while (*p==' ') p++;
	strcpy(editor_cmd,p);
      } else if (my_strcasecmp(pathbuf,"exec") == 0 || my_strcasecmp(pathbuf,"execp") == 0) {
	do_pause = my_strcasecmp(pathbuf,"execp") == 0;
	p = scan_word(pathbuf,p);
	while (*p++==' ')
	  ;
	p--;
	if (n_extensions < N_EXTENSIONS && strlen(pathbuf) != 0) {
	  cmd_extensions[n_extensions].ext = strdup(pathbuf);
	  cmd_extensions[n_extensions].cmdline = strdup(p);
	  cmd_extensions[n_extensions].do_pause = do_pause;

	  n_extensions++;
	}
      } else if (my_strcasecmp(pathbuf,"mode") == 0) {
	video_mode = strtol(p,&p,10);
      } else if (my_strcasecmp(pathbuf,"font") == 0) {
	while (*p++ == ' ')
	  ;
	p--;
	if (memcmp(p,"sys",3)==0) {
	  font_nr = 65535;
	}else {
	  font_nr = strtol(p,&p,10);
	}
      } else if (my_strcasecmp(pathbuf,"color") == 0 || my_strcasecmp(pathbuf,"colour") == 0) {
	fgcol = strtol(p,&p,10);
	bgcol = strtol(p,&p,10);
	hlcol = strtol(p,&p,10);
      }
    }
    fclose(infile);
  }
}


int
main(int argc, char *argv[])
{
  fQuit = false;
  uint8_t ch,vk;
  uint8_t *p = (uint8_t*)LAUNCHER_EXT_EXEC;
  *p=0;

  read_cfg_file();
  
  if (argc < 2) {
    strcpy(dirname_left,"/");
  } else {
    if (argv[1][0] == '/') {
      strcpy(dirname_left,argv[1]);
    } else {
      dirname_left[0]='/'; 
      strcpy(dirname_left+1,argv[1]);
   }
  }
  if (argc < 3) {
    strcpy(dirname_right,"/");
  } else {
    if (argv[2][0] == '/') {
      strcpy(dirname_right,argv[2]);
    } else {
      dirname_right[0]='/'; 
      strcpy(dirname_right+1,argv[2]);
   }
  }
  display_init();
  
  if (argc >= 4 && argv[3][0]=='r')
    which_dir = 2;
  display_init();
  display_frame();
  display_curdir(1,dirname_left,which_dir==1);
  display_curdir(2,dirname_right,which_dir==2);
  dirlist_left = dirlist_read(dirname_left);
  dirlist_right = dirlist_read(dirname_right);
  dirlist_show(1, dirlist_left);
  dirlist_show(2, dirlist_right);
  do {    
    ch = getch();
    vk = getsysvar_vkeycode();
    switch(vk) {
    case 142: /* Tab */
      if (which_dir == 2) which_dir = 1; else which_dir = 2;
      display_curdir(1,dirname_left,which_dir==1);
      display_curdir(2,dirname_right,which_dir==2);      
      break;
    case 150: /* cursor up */
      dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,-1);
      break;
    case 152: /* cursor down */
      dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,1);
      break;
    case 146: /* Page up */
      dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,-dirwin_height+2);
      break;
    case 148: /* Page down */
      dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,dirwin_height-2);
      break;
    case 133: /* Home */
      dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,-16384);
      break;
    case 135: /* End */
      dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,16384);
      break;      
    case 128 : /* INS for tag */
      dirlist_select_file(which_dir==1?dirlist_left:dirlist_right);
      dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,1);
      break;
    case 143: /* Enter */
      if (dirlist_is_dir(which_dir==1?dirlist_left:dirlist_right)) {
	/* Any directory, try to change to it */
	cmd_chdir(which_dir,
		  which_dir==1?&dirlist_left:&dirlist_right,
		  which_dir==1?dirname_left:dirname_right);
      } else {
	mos_cd(which_dir==1?dirname_left:dirname_right);
	set_pathbuf(false);
	if (has_ext(pathbuf,"bin")) {
	  set_pathbuf(true);
	  execute_command(pathbuf,false);
	} else {
	  unsigned int i;
	  for (i=0; i<n_extensions; i++) {
	    if (has_ext(pathbuf,cmd_extensions[i].ext)) {
	      sprintf(cmdbuf, cmd_extensions[i].cmdline, pathbuf);
	      execute_command(cmdbuf,cmd_extensions[i].do_pause);
	      break;
	    }
	  }
	}
      }
      break;
    case 159: /* F1 Help */
      sprintf(cmdbuf,viewer_cmd,"/bin/12amc.hlp");
      execute_command(cmdbuf,false);
      break;
    case 160: /* F2  command line full screen */
      display_finish();
      printf("%s *",which_dir==1?dirname_left:dirname_right);
      mos_cd(which_dir==1?dirname_left:dirname_right);
      mos_editline(cmdbuf, 256, 1);
      execute_command(cmdbuf,true);
      break;
    case 161: /* F3 view */
      set_pathbuf(false);
      mos_cd(which_dir==1?dirname_left:dirname_right);
      sprintf(cmdbuf,viewer_cmd,pathbuf);
      execute_command(cmdbuf,false);
      break;
    case 162: /* F4 Edit */
      set_pathbuf(false);
      mos_cd(which_dir==1?dirname_left:dirname_right);
      sprintf(cmdbuf,editor_cmd,pathbuf);
      execute_command(cmdbuf,false);
      break;
    case 163: /* F5 Copy */
      mos_cd(which_dir==1?dirname_left:dirname_right);
      display_setwin(3);
      putch(12);
      {
	unsigned int n_selected = dirlist_n_selected(which_dir==1?dirlist_left:dirlist_right);
	char *nm;
	if (n_selected == 0) {
	  strcpy(cmdbuf,dirlist_get_name(which_dir==1?dirlist_left:dirlist_right));
	  printf("Copy file: %s to:",cmdbuf);
	  vdp_cursor_enable(true);
	  mos_editline(pathbuf,256,true);
	  vdp_cursor_enable(false);
	  putch(12);
	  mos_copy(cmdbuf,pathbuf);
	} else {
	  printf("Copy %d selected files to %s? (Y/N)\n",n_selected,which_dir==1?dirname_right:dirname_left);
	  ch = getch();
	  putch(12);
	  if (ch=='y' || ch=='Y') {
	    nm = dirlist_first_selected(which_dir==1?dirlist_left:dirlist_right);
	    while (nm != NULL) {	      
	      strcpy(pathbuf,which_dir==1?dirname_right:dirname_left);
	      if (strcmp(pathbuf,"/") != 0)
		strcat(pathbuf,"/");
	      strcat(pathbuf,nm);
	      mos_copy(nm,pathbuf);
	      nm = dirlist_next_selected(which_dir==1?dirlist_left:dirlist_right);
	    }
	  }
	}
      }
      cmd_reload_dir();
      break;
    case 164: /* F6 Move */
      mos_cd(which_dir==1?dirname_left:dirname_right);
      display_setwin(3);
      putch(12);
      {
	unsigned int n_selected = dirlist_n_selected(which_dir==1?dirlist_left:dirlist_right);
	char *nm;
	if (n_selected == 0) {
	  strcpy(cmdbuf,dirlist_get_name(which_dir==1?dirlist_left:dirlist_right));
	  printf("Rename file: %s to:",cmdbuf);
	  vdp_cursor_enable(true);
	  mos_editline(pathbuf,256,true);
	  vdp_cursor_enable(false);
	  putch(12);
	  mos_ren(cmdbuf,pathbuf);
	} else {
	  printf("Move %d selected files to %s? (Y/N)\n",n_selected,which_dir==1?dirname_right:dirname_left);
	  ch = getch();
	  putch(12);
	  if (ch=='y' || ch=='Y') {
	    nm = dirlist_first_selected(which_dir==1?dirlist_left:dirlist_right);
	    while (nm != NULL) {	      
	      strcpy(pathbuf,which_dir==1?dirname_right:dirname_left);
	      if (strcmp(pathbuf,"/") != 0)
		strcat(pathbuf,"/");
	      strcat(pathbuf,nm);
	      mos_ren(nm,pathbuf);
	      nm = dirlist_next_selected(which_dir==1?dirlist_left:dirlist_right);
	    }
	  }
	}
      }
      cmd_reload_dir();
      break;
    case 165: /* F7 Mkdir */
      mos_cd(which_dir==1?dirname_left:dirname_right);
      display_setwin(3);
      putch(12);
      printf("Directory to create: ");
      vdp_cursor_enable(true);
      mos_editline(cmdbuf,256,true);
      vdp_cursor_enable(false);
      putch(12);
      mos_mkdir(cmdbuf);
      cmd_reload_dir();
      break;
    case 166: /* F8 Delete */
      mos_cd(which_dir==1?dirname_left:dirname_right);
      display_setwin(3);
      putch(12);
      {
	unsigned int n_selected = dirlist_n_selected(which_dir==1?dirlist_left:dirlist_right);
	char *nm;
	if (n_selected == 0) {
	  strcpy(cmdbuf,dirlist_get_name(which_dir==1?dirlist_left:dirlist_right));
	  printf("Delete file: %s? (Y/N)\n",cmdbuf);
	  ch = getch();
	  putch(12);
	  if (ch=='y' || ch=='Y') {
	    mos_del(cmdbuf);
	  }
	} else {
	  printf("Delete %d selected files? (Y/N)\n",n_selected);
	  ch = getch();
	  putch(12);
	  if (ch=='y' || ch=='Y') {
	    nm = dirlist_first_selected(which_dir==1?dirlist_left:dirlist_right);
	    while (nm != NULL) {
	      mos_del(nm);
	      nm = dirlist_next_selected(which_dir==1?dirlist_left:dirlist_right);
	    }
	  }
	}
      }
      cmd_reload_dir();
      break;
    case 167: /* F9 */      
      display_finish();
      printf("%s *",which_dir==1?dirname_left:dirname_right);
      mos_cd(which_dir==1?dirname_left:dirname_right);
      strcpy(cmdbuf,dirlist_get_name(which_dir==1?dirlist_left:dirlist_right));
      mos_editline(cmdbuf, 256, 0);
      execute_command(cmdbuf,true);
      break;
    case 168: /* F10 */
      fQuit = true;
      break;
    default:
      switch (ch) {
      case 0: /* Ignore non-ASCII */
	break; 
      case 20: /* Control-T for tag */
	  dirlist_select_file(which_dir==1?dirlist_left:dirlist_right);
	  dirlist_move_cursor(which_dir, which_dir==1?dirlist_left:dirlist_right,1);
	  break;
      case 27: /* ESC for quit */
	fQuit = true;
	break;
      case '+': /* Select files with a pattern */
	display_setwin(3);
	putch(12);
	printf("WIldcard pattern of files to select: ");
	vdp_cursor_enable(true);
	mos_editline(cmdbuf,256,true);
	vdp_cursor_enable(false);
	putch(12);
	dirlist_select_pattern(which_dir,which_dir==1?dirlist_left:dirlist_right,cmdbuf);	
	break;
      case '\\':
      case '-': /* Deselect files with a pattern */
	display_setwin(3);
	putch(12);
	printf("WIldcard pattern of files to deselect: ");
	vdp_cursor_enable(true);
	mos_editline(cmdbuf,256,true);
	vdp_cursor_enable(false);
	putch(12);
	dirlist_deselect_pattern(which_dir,which_dir==1?dirlist_left:dirlist_right,cmdbuf);	
	break;
      case 12: /* Ctrl-L, reload directories */
	cmd_reload_dir();
	break;
      default:
	if (ch >= 32) {
	  mos_cd(which_dir==1?dirname_left:dirname_right);
	  display_setwin(3);
	  putch(12);
	  printf("* ");
	  cmdbuf[0] = ch;
	  cmdbuf[1] = 0;
	  vdp_cursor_enable(true);
	  mos_editline(cmdbuf, 256, 0);
	  vdp_cursor_enable(false);
	  printf("\n");
	  execute_command(cmdbuf,true);
	}
      }     
    }
  } while (!fQuit);
  display_finish();
  return 0;
}
