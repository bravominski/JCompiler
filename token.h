//
// token.h
//

#include <stdio.h>

#define MAX_LINE_LENGTH 256

typedef enum {
  LPAREN, RPAREN, IDENT, NUMBER, DEFUN, LET,
  IF, PLUS, MINUS, MPY, DIV, MOD, GT, LT, GEQ, LEQ, EQ, END_OF_FILE
} tokenType;

typedef struct {
  tokenType type;
  char str[MAX_LINE_LENGTH];
  int value;
} token;

extern token current_token;

int get_token (FILE *theFile);

int accept (FILE *theFile, tokenType sym);

int expect (FILE *theFile, tokenType sym);

void parse_error (char *msg);

char *tokenType_to_string (tokenType sym);
