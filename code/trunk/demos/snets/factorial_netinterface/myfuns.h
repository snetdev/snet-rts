#ifndef _myfuns_h_
#define _myfuns_h_

void myfree( void *ptr);
void *mycopyChar( void *ptr);
void *mycopyInt( void *ptr);
int myserializeChar(void* value, char **serialized);
int myserializeInt(void* value, char **serialized);
void *mydeserialize(char* value, int len);

#endif
