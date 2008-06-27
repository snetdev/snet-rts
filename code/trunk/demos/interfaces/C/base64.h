#ifndef _BASE64_H_
#define _BASE64_H_

#include <stdio.h>

/* Encode 'size' bytes from memory location 'src' to file 'dst' 
 * using base64 encoding. 
 *
 */

int Base64encode(FILE *dst, void *src, int size);


/* Decode max 'size' bytes base64 encoded data from 
 * file 'src' to memory location 'dst'.
 * 
 */

int Base64decode(FILE *src, void *dst, int size);


/* Encode data type ID 'type' to file 'dst'. 
 * Encoding is XML -safe. 
 *
 */

int Base64encodeDataType(FILE *dst, int type);


/* Decode data type ID from file 'src' to memory location 'type'.
 * 
 */

int Base64decodeDataType(FILE *src, int *type);

#endif /* _BASE64_H_ */
