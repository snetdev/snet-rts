#ifndef _C4SNETTYPES_H_
#define _C4SNETTYPES_H_

void C4SNetFree( void *ptr);
void *C4SNetCopy( void *ptr);
int C4SNetSerialize( FILE *file, void *ptr);
int C4SNetEncode( FILE *file, void *ptr);
void *C4SNetDeserialize(FILE *file);
void *C4SNetDecode(FILE *file);

#endif
