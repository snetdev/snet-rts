#ifndef _C2SNetTypes_h_
#define _C2SNetTypes_h_

void *C2SNet_deserialize(FILE *file);
void *C2SNet_decode(FILE *file);

void C2SNetFree( void *ptr);

void *C2SNetCopyInt( void *ptr);
int C2SNetSerializeInt(FILE *ptr, void *value);
int C2SNetEncodeInt(FILE *ptr, void *value);

void *C2SNetCopyFloat( void *ptr);
int C2SNetSerializeFloat(FILE *ptr, void *value);
int C2SNetEncodeFloat(FILE *ptr, void *value);

#endif
