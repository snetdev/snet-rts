#ifndef _BASE64_H_
#define _BASE64_H_

#include <stdio.h>

int Base64encode(FILE *file, unsigned char *from, int size);

int Base64decode(FILE *file, unsigned char *to, int size);

int Base64encodeDataType(FILE *file, int type);

int Base64decodeDataType(FILE *file, int *type);

#endif /* _BASE64_H_ */
