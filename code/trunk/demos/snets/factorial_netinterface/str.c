#include "str.h"
#include <string.h>
#include <memfun.h>

char *STRcpy(const char *text){
  char *t = SNetMemAlloc(sizeof(char) * (strlen(text) + 1));
  return strcpy(t, text);
}

int STRcmp(const char *a, const char *b){
  return strcmp(a, b);
}

char *STRcat(const char *a, const char *b){
  char *t = SNetMemAlloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
  strcpy(t, a);
  strcat(t, b);
  return t;
}
