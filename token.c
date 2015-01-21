//
// token.c
//

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

// Line number - note static linkage not visible outside this file
static int line_no = 0;

token current_token;

int get_token (FILE *theFile) {

  // Static variables used to parse the file
  static char line[MAX_LINE_LENGTH];
  static char *ptr = NULL;

  int i;
  char *str = current_token.str;

  while (1) {

    // Get a new line
    if ( (ptr == NULL) || (*ptr == '\0') ) {
      if (fgets (line, MAX_LINE_LENGTH, theFile)) {

	// Find the first ; and replace with 0x00
	for (i=0; i < MAX_LINE_LENGTH; ++i)
	  if (line[i] == ';') {
	    line[i] = 0x00;
	    break;
	  }
	
	ptr = line;
	++line_no;
      } else {
	current_token.type = END_OF_FILE;
	return 1;
      }
    }

    // Advance ptr until you hit a character or the end of the line
    while ( isspace(*ptr) && (*ptr != '\0') ) ++ptr;

    if (*ptr == '\0') continue;

    if (*ptr == '(') {
      current_token.type = LPAREN;
      ++ptr;
      return 0;
    }

    if (*ptr == ')') {
      current_token.type = RPAREN;
      ++ptr;
      return 0;
    }

    // Copy the characters into the token
    for (i=0; i < MAX_LINE_LENGTH; ++i, ++ptr) {
      if (isgraph(*ptr) && (*ptr != ')') && (*ptr != '('))
	current_token.str[i] = *ptr;
      else
	break;
    }

    if (i < MAX_LINE_LENGTH) {
      current_token.str[i] = '\0';
    } else {
      printf ("Token too long on line %d\n", line_no);
      exit(1);
    }

    // Check for keywords and operators
    if (strcmp (current_token.str, "defun") == 0) {
      current_token.type = DEFUN;
      return 0;
    }

    if (strcmp (current_token.str, "let") == 0) {
      current_token.type = LET;
      return 0;
    }

    if (strcmp (current_token.str, "if") == 0) {
      current_token.type = IF;
      return 0;
    }

    if (strcmp (current_token.str, "+") == 0) {
      current_token.type = PLUS;
      return 0;
    }
    
    if (strcmp (current_token.str, "-") == 0) {
      current_token.type = MINUS;
      return 0;
    }

    if (strcmp (current_token.str, "*") == 0) {
      current_token.type = MPY;
      return 0;
    }
    
    if (strcmp (current_token.str, "/") == 0) {
      current_token.type = DIV;
      return 0;
    }
    
    if (strcmp (current_token.str, "%") == 0) {
      current_token.type = MOD;
      return 0;
    }
    
    if (strcmp (current_token.str, ">") == 0) {
      current_token.type = GT;
      return 0;
    }

    if (strcmp (current_token.str, "<") == 0) {
      current_token.type = LT;
      return 0;
    }
    
    if (strcmp (current_token.str, "<=") == 0) {
      current_token.type = LEQ;
      return 0;
    }

    if (strcmp (current_token.str, ">=") == 0) {
      current_token.type = GEQ;
      return 0;
    }
    
    if (strcmp (current_token.str, "==") == 0) {
      current_token.type = EQ;
      return 0;
    }

    // Check if it's a hex literal
    if ( (str[0] == '0') && (str[1] == 'x') ) {
      for (i=2; i < strlen(str); ++i) {
	if (!isxdigit(str[i])) {
	  printf ("Bad hex value on line %d\n", line_no);
	  return 2;
	}
      }

      current_token.type = NUMBER;
      sscanf(str, "%x", &(current_token.value));
      return 0;
    }

    // Check if it's a decimal literal
    if ((*str == '-') || (isdigit(*str))) {
      for (i=1; i < strlen(str); ++i) {
	if (!isdigit(str[i])) {
	  printf ("Bad decimal value on line %d\n", line_no);
	  return 2;
	}
      }

      current_token.type = NUMBER;
      sscanf(str, "%d", &(current_token.value));
      return 0;
    }
      
    // Check if it's a legal identifier
    if (isalpha(*str)) {
      for (i=1; i < strlen(str); ++i) {
	if ( !isalnum(str[i]) && (str[i] != '_') ) {
	  printf ("Bad identifier %s on line %d\n", str,  line_no);
	  return 3;
	}
      }

      // If we get here the token is fine
      current_token.type = IDENT;
      return 0;
    }

    // If we get here we have a bogus token
    printf ("Bogus token %s line %d\n", str, line_no);
    return -1;
  }
}

// A utility function for translating the enumeration values to strings
// this is useful for printing status messages and for debugging.
char *tokenType_to_string (tokenType sym) {
  switch (sym) {
    case LPAREN: return "LPAREN";
    case RPAREN: return "RPAREN";
    case IDENT: return "IDENT";
    case NUMBER: return "NUMBER";
    case DEFUN: return "DEFUN";
    case LET: return "LET";
    case IF: return "IF";
    case PLUS: return "ADD";
    case MINUS: return "SUB";
    case MPY: return "MUL";
    case DIV: return "DIV";
    case MOD: return "MOD";
    case GT: return "BRp";
    case LT: return "BRn";
    case GEQ: return "BRzp";
    case LEQ: return "BRnz";
    case EQ: return "BRz";
    case END_OF_FILE: return "END_OF_FILE";

    default:  return "UNKNOWN TOKEN";
  }
}


int accept (FILE *theFile, tokenType sym) {
  if (current_token.type == sym) {
    get_token (theFile);
    return 1;
  } else
    return 0;
}

int expect (FILE *theFile, tokenType sym) {
  if (accept(theFile, sym)) {
    return 1;
  } else {
    printf ("Unexpected symbol on line %d\n", line_no);
    exit (1);
  }
}

void parse_error (char *msg) {
  printf ("parse error on line %d : %s\n", line_no, msg);
  exit (1);
}
