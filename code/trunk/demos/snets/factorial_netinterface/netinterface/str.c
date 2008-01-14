/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   10.1.2008
 * -----
 *
 * Description:
 * ------------
 *
 * String functions for S-NET network interface.
 *
 *******************************************************************************/

#include "str.h"
#include <string.h>
#include <memfun.h>

char *STRcpy(const char *text){
  char *t = SNetMemAlloc(sizeof(char) * (strlen(text) + 1));
  return strcpy(t, text);
}

char *STRcat(const char *a, const char *b){
  char *t = SNetMemAlloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
  strcpy(t, a);
  strcat(t, b);
  return t;
}
