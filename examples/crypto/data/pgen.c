#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define __USE_GNU
#include <crypt.h>

/* Program to randomly generate test material for crypto */

const char const saltchars[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int main(int argc, char *argv[]) 
{
  FILE *fp;
  FILE *fpw;
  FILE *fpr;
  int n;
  int i, j;
  int *entries;
  int t;
  int num_entries;
  char c;
  int size;
  char *dict;
  char *result;
  char salt[12];

  struct crypt_data cdata;

  cdata.initialized = 0;

  if(argc != 5) {
    printf("USAGE: pgen <number of words to generate> <dictionary file> <file for crypted words> <file for clear words>\n");
    return 1;
  }
  
  if((fp = fopen(argv[2],"r")) == NULL) {
    return 1;
  }

  if((fpw = fopen(argv[3],"w")) == NULL) {
    return 1;
  }

  if((fpr = fopen(argv[4],"w")) == NULL) {
    return 1;
  }

  /* Count words, replace line changes with end of lines */
  fseek(fp, 0, SEEK_END);
  size = ftell(fp) + 2;
  fseek(fp, 0, SEEK_SET);
  
  dict = malloc(sizeof(char) * size);

  num_entries = 1;

  for(i = 0;(c = fgetc(fp)) != EOF; i++) {
    if(c == '\n') {
      dict[i] = '\0';
      num_entries++;
    } else {
      dict[i] = c;
    }
  }

  /* Generate n distinct random numbers */

  n = atoi(argv[1]);

  entries = malloc(sizeof(int) * n);

  srand(time(0));

  for(i = 0; i < n; i++) {
    t = 1;
    while(t != 0) {
      t = 0;
      entries[i] = rand() % num_entries;
      for(j = 0; j < i; j++) {
	if(entries[i] == entries[j]) {
	  t++;
	}
      }
    } 
  }

  /* Generate n crypted passwords */

  for(i = 0; i < n; i++) {
    fseek(fp, 0, SEEK_SET);

    salt[0] = '$';
    salt[1] = '1';
    salt[2] = '$';
    for(j = 3; j < 11; j++) {
      salt[j] = saltchars[rand() % 64];
    }
    salt[11] = '$';

    for(j = 0, t = 0;(j < size) && (t != entries[i]); j++) {
      if(dict[j] == '\0') {
	t++;
      }
    }
    
    result = crypt_r(&dict[j], salt, &cdata);

    fprintf(fpw, "%s\n",result);
    fprintf(fpr, "%s\n", &dict[j]);    
  }
  

  fclose(fp);
  fclose(fpw);
  fclose(fpr);

  free(dict);

  return 0;
}
