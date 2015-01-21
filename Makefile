
token.o : token.c token.h
	clang -c token.c

parser.o : parser.c parser.h token.h
	clang -c parser.c

hash_table.o : hash_table.c hash_table.h
	clang -c hash_table.c

jc : jc.c parser.o token.o hash_table.o 
	clang -o jc jc.c parser.o token.o hash_table.o 
