#include "base64.h"

/* Todo: enc/dec could also change byte order to network byte order
 *       for transfer? */

static const char base64[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
			      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
			      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 
			      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 
			      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			      'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
			      'w', 'x', 'y', 'z', '0', '1', '2', '3', 
			      '4', '5', '6', '7', '8', '9', '+', '-'};

int Base64encode(FILE *file, unsigned char *from, int len)
{
  int size = 0;
  int i, j;
  int offset = 0;
  unsigned char cur = 0;
  unsigned char last = 0;
  unsigned char next = 0;

  i = len * 8;
  size = i % 6; 
  size = (size == 0) ? (i / 6) : ((i / 6 + 1)); 

  for(i = 0, j = 0, offset = 0; j < size; i++, j++) {

    if(i < len) {
      next = from[i];
    }else {
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
  
  return size;
}

int Base64decode(FILE *file, unsigned char *to, int len)
{
  int i, j;
  int offset = 0;
  unsigned char cur;
  unsigned char next = 0;

  j = 0;
  i = 0;
  while(j < len) {
    cur = fgetc(file);
    i++;

    /* Direct table instead of searching? */
    for(next = 0; next < 64; next++) {
      if(base64[(unsigned int)next] == cur) {
	break;
      }
    }

    if(next == 64) {
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

  return i;
}
