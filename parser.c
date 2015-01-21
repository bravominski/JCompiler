//
// parser.c : Implements a recursive descent parser for the J language
//

#include "token.h"
#include "parser.h"
#include <string.h>
#include <stdlib.h>

#define NUMBER_OF_HASH_BUCKETS  23

// Variables used to measure s-expression depth
static int s_expr_depth;

// Boolean used to disallow lets in bad places
// If this is non-zero lets will trigger a parse error
static int no_more_lets = 0;

int i; // for FOR loop
// # of local variables
int local_no;
int inStack;
int branch_no;
// Assembly output file
char *asmName;
FILE *asmFile;

// Hashtable
hash_table_ptr hash_table;
hash_table_ptr function_name_table;

void sexpr (FILE *theFile)
{
  int arg_no = 0;
  tokenType operator;
  char label1[10];
  char label2[10];
  char label3[10];

  expect (theFile, LPAREN);
  ++s_expr_depth;

  switch (current_token.type) {

    // Handle built in operators
    case PLUS:
    case MINUS:
    case MPY:
    case DIV:
    case MOD:

      if (s_expr_depth == 1) no_more_lets = 1;
      
      // record the operation type for later use
      operator = current_token.type;
      // get the next token
      get_token (theFile);
      // handle the first argument
      expr (theFile);
        
      // handle the second argument
      expr (theFile);
        
     //take out two arguments and do operation and put it back to stack
      fprintf(asmFile, "\tLDR R3, R6, #0\n");
      fprintf(asmFile, "\tLDR R7, R6, #1\n");
      fprintf(asmFile, "\tADD R6, R6, #2\n");
      fprintf(asmFile, "\t%s R7 R7 R3\n", tokenType_to_string(operator));
      
      // Putting the result of the operation back to stack unless it's the last one
      if (s_expr_depth != 1) { 
        fprintf(asmFile, "\tSTR R7, R6, #-1\n");
        fprintf(asmFile, "\tADD R6, R6, #-1\n");
      }
      break;

      // Handle comparison
    case GT:
    case LT:
    case GEQ:
    case LEQ:
    case EQ:  
      operator = current_token.type;

      // get the next token
      get_token (theFile);
      // handle the first argument
      expr (theFile);
      // handle the second argument
      expr (theFile);
      
      fprintf(asmFile, "\tLDR R3, R6, #0\n");
      fprintf(asmFile, "\tLDR R7, R6, #1\n");
      fprintf(asmFile, "\tADD R6, R6, #2\n");

      fprintf(asmFile, "\tCMP R7, R3\n");

      sprintf(label1, "Loop_%d", branch_no);
      branch_no++;
      sprintf(label2, "Loop_%d", branch_no);
      branch_no++;
      fprintf(asmFile, "\t%s %s\n", tokenType_to_string(operator), label2);
      fprintf(asmFile, "\tCONST R7, #0\n");
      fprintf(asmFile, "\tJMP %s\n\n", label1);
      fprintf(asmFile, "%s\n", label2);
      fprintf(asmFile, "\tCONST R7, #1\n\n");
      fprintf(asmFile, "%s\n", label1);

      if (s_expr_depth != 1) { 
        fprintf(asmFile, "\tSTR R7, R6, #-1\n");
        fprintf(asmFile, "\tADD R6, R6, #-1\n");
      }

      memset(label1, 0, sizeof(label1)); // reset labels
      memset(label2, 0, sizeof(label2));
      break;
      // Handle let statements
    case LET:

      if (s_expr_depth != 1)
       parse_error ("Let's cannot be nested inside other s-expressions");

      if (no_more_lets)
       parse_error ("All let's must occur at the start of the function");

      expect (theFile, LET);
      
      // Add local variable to hashtable
      local_no++;
      fprintf(asmFile, "\tADD R6, R6, #-1 ;; allocate stack space for local variables\n");
      add_entry(hash_table, current_token.str, -local_no); 
      
      /*if (current_token.type == IDENT) {
       printf ("Binding %s to a value\n", current_token.str);
      }*/

      // get the identifier being bound
      expect (theFile, IDENT);

      // expression value to be associated
      expr (theFile);

      fprintf(asmFile, "\tLDR R7, R6, #0\n");
      fprintf(asmFile, "\tADD R6, R6, #1\n");
      fprintf(asmFile, "\tSTR R7, R5, #%d\n", -local_no);
      fprintf(asmFile, "\tADD R6, R5, #%d\n", -local_no);

      break;

      // Handle if statements
    case IF:
      
      if (s_expr_depth == 1) no_more_lets = 1;

      expect (theFile, IF);
      //branch_no++;
      //printf ("Handling an IF statement \n");

      // get test expr
      expr (theFile);
      sprintf(label1, "Loop_%d", branch_no);
      branch_no++;
      sprintf(label2, "Loop_%d", branch_no);
      branch_no++;

      fprintf(asmFile, "\tLDR R7, R6, #0\n");
      fprintf(asmFile, "\tADD R6, R6, #1\n");
      fprintf(asmFile, "\tCMPI R7, #0\n");
      fprintf(asmFile, "\tBRz %s\n", label2);
      // get true clause - to be returned if test is non-zero
      expr (theFile);
      fprintf(asmFile, "\tJMP %s\n\n", label1);
      fprintf(asmFile, "%s\n", label2);
      // get false clause - to be returned if test is zero
      expr (theFile);
      fprintf(asmFile, "\n%s\n", label1);      

      if (s_expr_depth != 1) { 
        fprintf(asmFile, "\tSTR R7, R6, #-1\n");
        fprintf(asmFile, "\tADD R6, R6, #-1\n");
      }

      break;

      // Handle function calls
    case IDENT:

      if (s_expr_depth == 1) no_more_lets = 1;

      // Note the function name
      char *functionName;
      functionName = malloc(sizeof(current_token.str));
      strcpy(functionName, current_token.str);
      //printf ("Handling a function call to %s\n", current_token.str);
      // Handle the arguments
      
      expect (theFile, IDENT);

      while ((current_token.type == NUMBER) ||
       (current_token.type == IDENT)  ||
       (current_token.type == LPAREN)) { 
          /*if (arg_no != 0) {
            fprintf(asmFile, "\tLDR R7, R6, #0\n");
            fprintf(asmFile, "\tSTR R7, R6, #-1\n");
            for (i = 0; i < arg_no - 1; i++) {
              fprintf(asmFile, "\tADD R6, R6, #1\n");
              fprintf(asmFile, "\tLDR R7, R6, #0\n");
              fprintf(asmFile, "\tSTR R7, R6, #-1\n");
            }
          }*/

          arg_no++;
          expr (theFile); // LDR an argument to R7
          /*if (arg_no == 0) {
            fprintf(asmFile, "\tSTR R7, R6, #-1\n");
            fprintf(asmFile, "\tADD R6, R6, #-1\n");
          }
          else {
            fprintf(asmFile, "\tSTR R7, R6, #0\n");
            fprintf(asmFile, "\tADD R6, R6, #-%d\n", arg_no); 
          }*/
      }

      for (i = 0; i < (arg_no / 2); i++) {
        fprintf(asmFile, "\tLDR R7, R6, #%d\n", i);
        fprintf(asmFile, "\tLDR R3, R6, #%d\n", arg_no - i - 1);
        fprintf(asmFile, "\tSTR R7, R6, #%d\n", arg_no - i - 1);
        fprintf(asmFile, "\tSTR R3, R6, #%d\n", i);
      }

      fprintf(asmFile, "\tJSR %s\n", functionName);
      fprintf(asmFile, "\tLDR R7, R6, #-1 ;; grap return value\n");
      fprintf(asmFile, "\tADD R6, R6, #%d ;; free space for arguments\n\n", arg_no);

      if (s_expr_depth != 1) { 
        fprintf(asmFile, "\tSTR R7, R6, #-1\n");
        fprintf(asmFile, "\tADD R6, R6, #-1\n");
      }

      free(functionName);
      break;

    default:
      parse_error ("Bad sexpr");
      break;
  };
  expect (theFile, RPAREN);
  --s_expr_depth;
}

void expr (FILE *theFile)
{
  switch (current_token.type) {
    case NUMBER:

      if (s_expr_depth == 0) no_more_lets = 1;
      
      if (current_token.value <= 255) {
        fprintf(asmFile, "\tCONST R7, #%d\n", current_token.value);
      }
      else {
        int con = current_token.value % 256;
        int hicon = current_token.value / 256;
        fprintf(asmFile, "\tCONST R7, #%d\n", con);
        fprintf(asmFile, "\tHICONST R7, #%d\n", hicon);
      }
      fprintf(asmFile, "\tSTR R7, R6, #-1\n");
      fprintf(asmFile, "\tADD R6, R6, #-1\n");

      expect (theFile, NUMBER);
      
      break;

    case IDENT:

      if (s_expr_depth == 0) no_more_lets = 1;
      if (lookup(hash_table, current_token.str) == NULL) {
        printf("The variable %s is undefined\n", current_token.str);
        exit(1);
      }
   
      fprintf(asmFile, "\tLDR R7, R5, #%d\n", *(int*)lookup(hash_table, current_token.str));        
      fprintf(asmFile, "\tSTR R7, R6, #-1\n");
      fprintf(asmFile, "\tADD R6, R6, #-1\n");

      expect (theFile, IDENT);
      break;
      
    case LPAREN:
      sexpr (theFile);
      break;
      
    default:
      parse_error ("Bad expression");
      break;
  }
}

void function (FILE *theFile) {
  int arg_no = 0;
  expect (theFile, LPAREN);
  expect (theFile, DEFUN);
  
  if (lookup(function_name_table, current_token.str) == NULL) {
    add_entry (function_name_table, current_token.str, 1);
  }
  else {
    printf("Function %s is already defined before\n", current_token.str);
    exit(2);
  }

  local_no = 0; // Reset the number of local variable to 0
  inStack = 0;
  hash_table = init_hash_table (hash_function, eq, NUMBER_OF_HASH_BUCKETS);
  
  s_expr_depth = 0;
  no_more_lets = 0;
  
  if (current_token.type == IDENT) {
    printf ("Parsing a function called %s\n", current_token.str); // Parser
  }
    
  // Parse the function name
  expect (theFile, IDENT);
  fprintf(asmFile, ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;%s;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n", current_token.str); //Compiler
  fprintf(asmFile, "\t\t.CODE\n");
  fprintf(asmFile, "\t\t.FALIGN\n");
  fprintf(asmFile, "%s\n", current_token.str);
  fprintf(asmFile, "\t;;prologue\n");
  fprintf(asmFile, "\tSTR R7, R6, #-2 ;; save return address\n");
  fprintf(asmFile, "\tSTR R5, R6, #-3 ;; save base pointer\n");
  fprintf(asmFile, "\tADD R6, R6, #-3\n\tADD R5, R6, #0\n\n");
  fprintf(asmFile, "\t;;function body\n");
  
  // Parse the argument list
  expect (theFile, LPAREN);
  while (current_token.type == IDENT) {
     
    // Do something with each argument
    ++arg_no;
    add_entry (hash_table, current_token.str, arg_no + 2);   
    expect (theFile, IDENT);
  }

  expect (theFile, RPAREN);

  do {
    // Done with arguements of defun Function,
    // Now handle expressions(real function bodies
    expr (theFile);
    
  } while ((current_token.type == NUMBER) ||
     (current_token.type == IDENT)  ||
     (current_token.type == LPAREN));

  fprintf(asmFile, "\t;; epilogue\n"); // Compiler
  fprintf(asmFile, "\tADD R6, R5, #0  ;; pop locals off stack\n");
  fprintf(asmFile, "\tADD R6, R6, #3  ;; free space for return address, base pointer, and return value\n");
  fprintf(asmFile, "\tSTR R7, R6, #-1 ;; store return value\n");
  fprintf(asmFile, "\tLDR R5, R6, #-3 ;; restore base pointer\n");
  fprintf(asmFile, "\tLDR R7, R6, #-2 ;; restore return address\n");
  fprintf(asmFile, "\tRET\n\n");

  free_hash_table(hash_table);
  // End of function
  expect (theFile, RPAREN);
}

void program (FILE *theFile, char* asmFileName)
{
  if (get_token(theFile) != 0) {
    parse_error ("problem reading first token");
  }

  asmName = asmFileName;
  asmFile = fopen(asmName, "w");
  printf ("Parsing a J program into %s\n", asmName);

  branch_no = 1;
  function_name_table = init_hash_table (hash_function, eq, NUMBER_OF_HASH_BUCKETS);
  while (current_token.type != END_OF_FILE) {
    function (theFile);
  }
  free_hash_table(function_name_table);
  fclose(asmFile);  
}


///////////////////////hash_table helping functions//////////////////////

int eq (void *id1, void *id2) {
  int i;
  // Compare length
  if (strlen(id1) != strlen(id2)) {
    return 0;
  }

  // Compare each character
  for (i = 0; i < strlen(id1); i++) {
    if (((char*)id1)[i] != ((char*)id2)[i]) {
      return 0;
    }
  }
  return 1;
}

int hash_function (void *id_no) {
  return ((char*)id_no)[0] % NUMBER_OF_HASH_BUCKETS;
}

void add_entry (hash_table_ptr hash_table, char *name, int offset) {
  int *offset_ptr;
  char *name_ptr;

  offset_ptr = malloc (sizeof(int));
  name_ptr = malloc (strlen(name) + 1);

  if (offset_ptr == NULL || name_ptr == NULL) {
    printf ("Malloc failed\n");
    exit (1);
  }

  *offset_ptr = offset;
  strcpy(name_ptr, name);
  add (hash_table, name_ptr, offset_ptr);
}
