/*
 * $ gcc -shared -Wl,-soname,libtag.so -o libtag.so tag.c
 *
 */

#include <stdio.h>

void getSignature(FILE *f)
{
  fprintf(f, "net tag({A,B} -> {A,B,<T>});\n");
}

