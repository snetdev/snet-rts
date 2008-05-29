#ifndef _myfuns_h_
#define _myfuns_h_

void C2SNetFree( void *ptr);

void *C2SNetCopyFloat( void *ptr);
void *C2SNetCopyInt( void *ptr);

int C2SNetSerializeFloat(FILE *ptr, void *value);
int C2SNetSerializeInt(FILE *ptr, void *value);

void *C2SNetDeserialize(FILE *ptr);

#endif
