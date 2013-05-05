#include "split.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memfun.h"


void *split(void *hnd, c4snet_data_t *entries, int num_entries)
{
  c4snet_data_t *hash_cdata;
  c4snet_data_t *salt_cdata;
  int i;
  char *data, *hash, *salt, *first, *middle, *last;

  first = data = C4SNetGetData(entries);

  for (i = 0; i < num_entries; i++) {
    last = strchr(first, '\n');

    if (last == NULL) last = &data[strlen(data)];

    middle = strchr(first + 1, '$') + 1;
    middle = strchr(middle, '$') + 1;

    salt_cdata = C4SNetAlloc(CTYPE_char, middle - first + 1, (void **) &salt);
    strncpy(salt, first, middle - first);
    salt[middle - first] = '\0';

    hash_cdata = C4SNetAlloc(CTYPE_char, last - first + 1, (void **) &hash);
    strncpy(hash, first, last - first);
    hash[last - first] = '\0';

    C4SNetOut(hnd, 1, hash_cdata, salt_cdata, i);

    first = last + 1;
  }

  C4SNetFree(entries);

  return hnd;
}
