#ifndef _C4SNETTEXT_H_
#define _C4SNETTEXT_H_

int C4SNetSerialize( FILE *file, void *ptr);
void *C4SNetDeserialize(FILE *file);

#endif /* _C4SNETTEXT_H_ */
