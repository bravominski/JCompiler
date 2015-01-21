//
// jc.c
//

#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "parser.h"
#include <string.h>

int main (int argc, char *argv[]) {

  FILE *theFile;

  if (argc < 2) {
    printf ("First argument should be a filename\n");
    exit(1);
  }

  theFile = fopen (argv[1], "r");

  if (!theFile) {
    printf ("Problem opening the file\n");
    exit (1);
  }

  char *a = argv[1];
  
  a[strlen(a) - 1] = '\0';
  char str[strlen(a) + 3];
  strcpy(str, a);
  strcat(str, "asm");

  // Parse the program
  program (theFile, str);

  fclose (theFile);
  
}
