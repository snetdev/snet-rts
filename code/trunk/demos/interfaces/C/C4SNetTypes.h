#ifndef _C4SNETTYPES_H_
#define _C4SNETTYPES_H_

void *C4SNet_deserialize(FILE *file);
void *C4SNet_decode(FILE *file);

void C4SNetFree( void *ptr);

void *C4SNetCopyInt( void *ptr);
int C4SNetSerializeInt(FILE *ptr, void *value);
int C4SNetEncodeInt(FILE *ptr, void *value);

void *C4SNetCopyFloat( void *ptr);
int C4SNetSerializeFloat(FILE *ptr, void *value);
int C4SNetEncodeFloat(FILE *ptr, void *value);

#endif
