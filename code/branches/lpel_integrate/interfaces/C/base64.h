#ifndef _BASE64_H_
#define _BASE64_H_

/** <!--********************************************************************-->
 *
 * $Id$
 *
 * file base64.h
 *
 * Base64 encoding encodes binary data to character values. 
 * The encoding can be used in XML data as it doesn't contain any
 * restricted characters that could appear in binary data.
 *
 * Base64 encoding codes each block of 3 bytes (24 bits) to 
 * 4 bytes (32 bits).
 * 
 *****************************************************************************/

#include <stdio.h>

/* Encode 'size' bytes from memory location 'src' to file 'dst' 
 * using base64 encoding. 
 *
 */
#ifdef __cplusplus
extern "C" {
#endif 

int Base64encode(FILE *dst, void *src, int size);


/* Decode max 'size' bytes base64 encoded data from 
 * file 'src' to memory location 'dst'. 'size' is the
 * size of the decoded data.
 * 
 */

int Base64decode(FILE *src, void *dst, int size);


/* Encode data type ID 'type' to file 'dst'. */

int Base64encodeDataType(FILE *dst, int type);


/* Decode data type ID from file 'src' to memory location 'type'. */

int Base64decodeDataType(FILE *src, int *type);

#ifdef __cplusplus
}
#endif 

#endif /* _BASE64_H_ */
