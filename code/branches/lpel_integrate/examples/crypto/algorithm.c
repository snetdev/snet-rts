#include <algorithm.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>
#define __USE_GNU
#include <crypt.h>
#include <string.h>
#include "memfun.h"
#include "id.h"


void *algorithm( void *hnd, 
		 c4snet_data_t *password, 
		 c4snet_data_t *salt,
		 c4snet_data_t *dictionary,
		 int dictionary_size)
{
  c4snet_data_t *ret;

  char *pw;
  char *slt;
  char *dict;
  char *result;

  int i,j;

  bool found = false;

  char *word;

  struct crypt_data cdata;

  cdata.initialized = 0;

  pw = (char *)C4SNetDataGetData(password);
  slt = (char *)C4SNetDataGetData(salt);
  dict = (char *)C4SNetDataGetData(dictionary);

  for(i = 0, j=0; i < dictionary_size; i++) {

    result = crypt_r (&dict[j], slt, &cdata);

    if(strcmp(result, pw) == 0) {

      word = SNetMemAlloc(sizeof(char) * strlen(&dict[j]) + 1);
      
      strcpy(word, &dict[j]);

      ret = C4SNetDataCreateArray(CTYPE_char, strlen(&dict[j]) + 1, word);

      C4SNetOut( hnd, 1, ret);

      found = true;

      break;
    } 

    while(dict[j] != '\0') {
      j++;
    }   
    j++;
  }

  if(!found) {
    C4SNetOut( hnd, 2, 0);
  }

  C4SNetDataFree(password);
  C4SNetDataFree(salt);
  C4SNetDataFree(dictionary);

  return( hnd);
}

