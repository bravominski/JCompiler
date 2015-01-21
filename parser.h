//
// parser.h
//

#include <stdio.h>
#include "hash_table.h"
void program (FILE *theFile, char *asmName);
void function (FILE *theFile);
void expr (FILE *theFile);
void sexpr (FILE *theFile);
void setUp();
int eq (void *id1, void *id2);
int hash_function (void *id_no);
void add_entry (hash_table_ptr hash_table, char *name, int offset);