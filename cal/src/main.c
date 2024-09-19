/* cal
 * Unix style calendar program.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool start_sunday;
bool whole_year;

char *month_names[] = {
  "January",
  "February",
  "March",
  "April",
  "May",
  "June",
  "July",
  "August",
  "September",
  "October",
  "November",
  "December"
};

char days[] = "     1  2  3  4  5  6  7  8  9"
              " 10 11 12 13 14 15 16 17 18 19"
              " 20 21 22 23 24 25 26 27 28 29"
              " 30 31";
char daynames1[] = " Mo Tu We Th Fr Sa Su";
char daynames2[] = " Su Mo Tu We Th Fr Sa";

unsigned int month_lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char month_buf1[6*22];
char month_buf2[6*22];
char month_buf3[6*22];

void fill_month(char *buf, unsigned int startday, unsigned int month_length)
{
  unsigned int i;
  char *p;
  for (i=0; i<6; i++)
    memset(buf+i*22,32,21);
  p=buf+startday*3;
  for (i=0; i<month_length*3; i+=3) {
    memcpy(p,days+i+3,3);
    p+=3;
    if (*p==0)p++;
  }
}


unsigned int month_length(unsigned int m,unsigned int year)
{
  unsigned int ml=month_lengths[m-1];
  if (m==2) {
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
      ml++;
  }
  return ml;
}

int
main(int argc, char *argv[])
{
  int nopts=0;
  unsigned int i,j,y;
  unsigned int year, month;
  for (;;) {
    if (argc+nopts < 2) {
      fprintf(stderr,"Usage: cal [-s] [month] <year>\n");
      return 19;
    }
    if (strcmp(argv[1+nopts],"-s")==0) {
      start_sunday = true;
      nopts++;
    }
    else {
      break;
    }
  }
  if (argc == 2+nopts) {
    month = 0;
    year = atoi(argv[nopts+1]);
  }
  else {
    month = atoi(argv[nopts+1]);
    if (month <1 || month >12) {
      fprintf(stderr,"Invalid month\n");
      return 19;
    }
    year = atoi(argv[nopts+2]);    
  }
  y=year-1;
  y=y+y/4-y/100+y/400+start_sunday; // First day of current year.
  if (month==0) {
    printf("%36u\n",year);
    for (i=0; i<12; i+=3) {
      unsigned int k1,k2,k3;
      k1=strlen(month_names[i]);
      k2=strlen(month_names[i+1]);
      k3=strlen(month_names[i+2]);
      printf("%*s%*s%*s\n",12+k1/2,month_names[i],
	     22-k1/2+k2/2,month_names[i+1],
	     22-k2/2+k3/2,month_names[i+2]);
      printf("%s  %s  %s\n",start_sunday?daynames2:daynames1,
	     start_sunday?daynames2:daynames1,	     
	     start_sunday?daynames2:daynames1);
      fill_month(month_buf1, y%7, month_length(i+1,year));
      y+=month_length(i+1,year);
      fill_month(month_buf2, y%7, month_length(i+2,year));
      y+=month_length(i+2,year);
      fill_month(month_buf3, y%7, month_length(i+3,year));
      y+=month_length(i+3,year);
      for (j=0; j<6; j++)
	printf("%s  %s  %s\n",month_buf1+22*j,month_buf2+22*j,month_buf3+22*j);
      if (i<12) printf("\n");
    }
  } else {
    printf("%*s %u\n",9+strlen(month_names[month-1])/2,month_names[month-1],year);
    printf("%s\n",start_sunday?daynames2:daynames1);
    for (j=1; j<month; j++) y+= month_length(j,year); // skip past preceding months.
    fill_month(month_buf1, y%7, month_length(month,year));
    for (j=0; j<6; j++)
      printf("%s\n",month_buf1+22*j);
  }
  return 0;	 
}
