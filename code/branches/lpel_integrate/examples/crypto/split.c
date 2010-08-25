#include "split.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"


void *split( void *hnd, c4snet_data_t *entries, int num_entries)
{
  c4snet_data_t *hash_cdata;
  c4snet_data_t *salt_cdata;

  char *data;
  char *hash;
  char *salt;

  int i, j;

  char *first;
  char *middle;
  char *last;

  data = (char *)C4SNetDataGetData(entries);


  first = data;

  for(i = 0; i < num_entries; i++) {

    last = strchr(first, '\n');
    
    if(last == NULL) {
      last = &data[strlen(data)];
    }
    
    for(j = 0, middle = first; j < 3; j++) {
      middle = strchr(middle, '$') + 1;
    } 
    
    salt = (char *)SNetMemAlloc(sizeof(char) * (middle - first + 1)); 
    hash = (char *)SNetMemAlloc(sizeof(char) * (last - first + 1));  

    strncpy(salt, first, (middle - first));
    salt[middle - first] = '\0';
    
    strncpy(hash, first, (last - first));
    hash[last - first] = '\0';
    
    hash_cdata = C4SNetDataCreateArray(CTYPE_char, strlen(hash) + 1, hash);
    salt_cdata = C4SNetDataCreateArray(CTYPE_char, strlen(salt) + 1, salt);

    C4SNetOut( hnd, 1, hash_cdata, salt_cdata, i);

    first = last + 1;
  }
 
  C4SNetDataFree(entries);

  return (hnd);
}
