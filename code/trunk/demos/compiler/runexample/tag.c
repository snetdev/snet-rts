/*****************************************************************
 *
 * External net source file of run-example from SNet compiler implementation guide.
 * Just for testing compilation/linking of compiler.
 *
 * Author: Kari Keinanen, VTT Technical Research Centre of Finland
 *
 * Date:   21.02.2007
 *
 * Make library:
 * 
 *         $ gcc -shared -Wl,-soname,libtag.so -o libtag.so tag.c
 * 
 ****************************************************************/

#include <stdio.h>

void getSignature(FILE *f)
{
  fprintf(f, "net tag({A,B} -> {A,B,<T>});\n");
}

