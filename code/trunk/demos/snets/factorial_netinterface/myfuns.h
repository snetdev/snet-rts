#ifndef _myfuns_h_
#define _myfuns_h_

void myfree( void *ptr);
void *mycopyChar( void *ptr);
void *mycopyInt( void *ptr);
char *myserializeChar(void* value);
char *myserializeInt(void* value);
void *mydeserialize(char* value);

#endif
