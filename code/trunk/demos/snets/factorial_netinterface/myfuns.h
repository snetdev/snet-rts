#ifndef _myfuns_h_
#define _myfuns_h_


void myfree( void *ptr);
void *mycopy( void *ptr);

void *mydeserialize(const char* value);
char *myserialize(const void* value);

#endif
