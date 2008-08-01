/** <!--********************************************************************-->
 *
 * $Id$
 *
 * file base64.c
 *
 * Base64 encoding encodes binary data to character values. 
 * The encoding can be used in XML data as it doesn't contain any
 * restricted characters that could appear in binary data.
 *
 * Base64 encoding codes each block of 3 bytes (24 bits) to 
 * 4 bytes (32 bits).
 * 
 *****************************************************************************/

#include "base64.h"

/* TODO: - Enc/dec should also handle byte ordering?
 *       - What about incorrect length? (for example: incoming 64bit int to 32bit int) 
 *         -> Because of the padding we know the real length of the data
 *            which could be compared to given length.
 */

/* Bits per byte. */
#define BITS_PER_BYTE 8

/* Bits per encoded byte. */
#define BITS_PER_ENCODED_BYTE 6

/* Bits per encoding block. */
#define BITS_PER_BLOCK 24

/* Macro for telling if encoding block is full */
#define BLOCK_NOT_FULL(i) (((i) * BITS_PER_ENCODED_BYTE) % BITS_PER_BLOCK != 0)

/* base64 encoding table for bit patterns */
static const char base64[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
			      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
			      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 
			      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 
			      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			      'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
			      'w', 'x', 'y', 'z', '0', '1', '2', '3', 
			      '4', '5', '6', '7', '8', '9', '+', '/',
	/* (padding char)*/   '='}; 

/* Index of the panding character in the encoding table */
#define PADDING_BYTE 64

/* Encode data type 'type' to 'file'. */
int Base64encodeDataType(FILE *file, int type) 
{
  int i = 0;

  while(type >= 64) {
    i += fputc(base64[PADDING_BYTE], file);
    type -= 64;
  }

  i += fputc(base64[type], file);

  return i;
}

/* Encode 'len' bytes from 'src' to 'file'. */
int Base64encode(FILE *file, void *src, int len)
{
  int size = 0;
  int i, j;
  int offset = 0;
  unsigned char cur = 0;
  unsigned char last = 0;
  unsigned char next = 0;
  unsigned char *from = (unsigned char *)src;

  size = (len * BITS_PER_BYTE + (BITS_PER_ENCODED_BYTE - 1)) / BITS_PER_ENCODED_BYTE;

  for(i = 0, j = 0, offset = 0; j < size; i++, j++) {

    if(i < len) {
      next = from[i];
    } else {
      next = 0;
    }

    if(offset == 0) {
      cur = (next >> 2);
      offset = 2;
      last = next;
    }else if(offset == 2) {
      cur = ((last & 3) << 4) | (next >> 4);
      offset = 4;
      last = next;
    }else if(offset == 4) {
      cur = ((last & 15) << 2) | (next >> 6);
      offset = 6;
      last = next;
    } else if(offset == 6) {
      cur = (last & 63);
      i--;
      offset = 0;  
    } 

    fputc(base64[(unsigned int)cur], file);
  }
  
  /* Add padding to the stream. */
  while(BLOCK_NOT_FULL(size)) {
    fputc(base64[PADDING_BYTE], file);
    size++; 
  }

  return size;
}

/* Decode data type enconding from 'file' to 'type'. */
int Base64decodeDataType(FILE *file, int *type)
{
  int i = 0;
  int j = 0;
  char next;

  *type = 0;

  while((next = fgetc(file)) == base64[PADDING_BYTE]) {
    *type += 64;
    i++;
  }
  i++;

  for(j = 0; j < PADDING_BYTE; j++) {
    if(base64[j] == next) {
      break;
    }
  }

  *type += j;

  if(j == PADDING_BYTE) {
    ungetc(next, file);
    return 0;
  }

  return i;
}

/* Decode 'len' bytes to 'dst' from 'file'.
 * 'len' is the number of bytes in 'dst' not the number 
 * of bytes to be taken from 'file'.
 */
int Base64decode(FILE *file, void *dst, int len)
{
  int i, j;
  int offset = 0;
  char cur;
  unsigned char next = 0;
  unsigned char *to = (unsigned char *)dst;

  j = 0;
  i = 0;
  while(j < len) {
    cur = fgetc(file);
    i++;

    /* Direct table instead of searching? */
    for(next = 0; next <= PADDING_BYTE; next++) {
      if(base64[(unsigned int)next] == cur) {
	break;
      }
    }

    if(next == PADDING_BYTE) {
      while(j < len) {
	to[j++] = 0;
      }
      break;
    }
    
    /* Unknown character! */
    if(next > PADDING_BYTE) {
      ungetc(cur, file);
      while(j < len) {
	to[j++] = 0;
      }
      return 0;
    }

    if(offset == 0) {
      to[j] = (next << 2);
      offset = 2;
    }else if(offset == 2) {
      to[j] |= (next >> 4);
      j++;
      if(j >= len)
	break;
      to[j] = (next & 15) << 4;
      offset = 4;
    }else if(offset == 4) {
      to[j] |= (next >> 2);
      j++;
      if(j >= len)
	break;
      to[j] = next << 6;
      offset = 6;
    } else if(offset == 6) {
      to[j] |= next;
      j++;
      if(j >= len)
	break;
      offset = 0;  
    }
  }

  /* Remove padding from the stream. */
  while(BLOCK_NOT_FULL(i)) {
    cur = fgetc(file);
    if(cur != base64[PADDING_BYTE]) {
      ungetc(cur, file);
      while(j < len) {
	to[j++] = 0;
      }
      return i;
    }
    i++;
  }

  return i;
}
