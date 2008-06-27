#include "base64.h"

/* TODO: - Enc/dec should also handle byte ordering?
 *       - What about incorrect length? (for example: incoming 64bit int to 32bit int) 
 *         -> Because of the padding we know the real length of the data
 *            which could be compared to given length.
 */

#define PADDING_CHAR 64
#define BITS_PER_CHAR 8
#define BITS_PER_ENCODED_CHAR 6
#define BITS_PER_BLOCK 24
#define BLOCK_NOT_FULL(i) (((i) * BITS_PER_ENCODED_CHAR) % BITS_PER_BLOCK != 0)

static const char base64[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
			      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
			      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 
			      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 
			      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			      'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
			      'w', 'x', 'y', 'z', '0', '1', '2', '3', 
			      '4', '5', '6', '7', '8', '9', '+', '/',
	/* (padding char)*/   '='}; 

int Base64encodeDataType(FILE *file, int type) 
{
  int i = 0;

  while(type >= 64) {
    i += fputc(base64[PADDING_CHAR], file);
    type -= 64;
  }

  i += fputc(base64[type], file);

  return i;
}


int Base64encode(FILE *file, unsigned char *from, int len)
{
  int size = 0;
  int i, j;
  int offset = 0;
  unsigned char cur = 0;
  unsigned char last = 0;
  unsigned char next = 0;

  size = (len * BITS_PER_CHAR + (BITS_PER_ENCODED_CHAR - 1)) / BITS_PER_ENCODED_CHAR;

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
    fputc(base64[PADDING_CHAR], file);
    size++; 
  }

  return size;
}

int Base64decodeDataType(FILE *file, int *type)
{
  int i = 0;
  int j;
  char next;

  while((next = fgetc(file)) == base64[PADDING_CHAR]) {
    *type += 64;
    i++;
  }
  i++;

  for(j = 0; j < PADDING_CHAR; j++) {
    if(base64[j] == next) {
      break;
    }
  }

  *type += j;

  if(j == PADDING_CHAR) {
    ungetc(next, file);
    return 0;
  }

  return i;
}

int Base64decode(FILE *file, unsigned char *to, int len)
{
  int i, j;
  int offset = 0;
  char cur;
  unsigned char next = 0;

  j = 0;
  i = 0;
  while(j < len) {
    cur = fgetc(file);
    i++;

    /* Direct table instead of searching? */
    for(next = 0; next <= PADDING_CHAR; next++) {
      if(base64[(unsigned int)next] == cur) {
	break;
      }
    }

    if(next == PADDING_CHAR) {
      while(j < len) {
	to[j++] = 0;
      }
      break;
    }
    
    /* Unknown character! */
    if(next > PADDING_CHAR) {
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
    if(cur != base64[PADDING_CHAR]) {
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

#undef PADDING_CHAR
#undef BITS_PER_CHAR
#undef BITS_PER_ENCODED_CHAR
#undef BITS_PER_BLOCK
#undef BLOCK_NOT_FULL
