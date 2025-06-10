/* Nano-style editor
   Copyright 2025, L.C. Benschop, Vught, The Netherlands.
   MIT license
*/

#include "edit.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct _EditState EDT;


int main(int argc, char** argv)
{
  EDT.mem_start=malloc(VAR_END_OFFS+EDIT_BUF_SIZE+CUT_BUF_SIZE);
  if (!EDT.mem_start) {
    printf("Not enough RAM\n");
    return 1;
  }
  EDT.mem_end=EDT.mem_start+VAR_END_OFFS+EDIT_BUF_SIZE+CUT_BUF_SIZE;
  if (argc != 2) {
    printf("Usage: ne <filename>\n");
    return 19;
  }
  strncpy((char*)EDT.mem_start + FILENAME_OFFS, argv[1], MAX_NAME_LENGTH);
  EDT_EditCore();
  return 0;
}
