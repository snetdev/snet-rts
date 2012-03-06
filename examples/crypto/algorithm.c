#include "algorithm.h"
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>
#define __USE_GNU
#include <crypt.h>
#include <string.h>
#include "memfun.h"
#include "C4SNet.h"


void *algorithm( void *hnd,
                 c4snet_data_t *password,
                 c4snet_data_t *salt,
                 c4snet_data_t *dictionary,
                 int dictionary_size)
{
  struct crypt_data cdata;
  char *pw, *slt, *dict, *result;

  bool found = false;

  cdata.initialized = 0;

  pw = C4SNetGetData(password);
  slt = C4SNetGetData(salt);
  dict = C4SNetGetData(dictionary);

  for (int i = 0, j = 0; i < dictionary_size; i++) {
    result = crypt_r(&dict[j], slt, &cdata);

    if (!strcmp(result, pw)) {
      C4SNetOut(hnd, 1,
          C4SNetCreate(CTYPE_char, strlen(&dict[j]) + 1, &dict[j]));

      found = true;
      break;
    }

    while (dict[j] != '\0') j++;
    j++;
  }

  if (!found) C4SNetOut(hnd, 2, 0);

  C4SNetFree(password);
  C4SNetFree(salt);
  C4SNetFree(dictionary);

  return hnd;
}
