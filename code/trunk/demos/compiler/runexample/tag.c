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
 * Run module loader:
 *
 *         $ snetc -b2 example.snet
 *
 *         or if libtag.so resides somewhere else than current directory
 *
 *         $ snetc -b2 example.snet -Llibdir1 -Llibdir2
 *
 *         or specifying environment variable for locations of libraries
 *
 *         $ export LIBRARY_PATH="libdir1 libdir2"; snetc -b2 example.snet
 * 
 ****************************************************************/

#include <stdio.h>

void getSignature(FILE *f)
{
  fprintf(f, "net tag({A,B} -> {A,B,<T>});\n");
}

