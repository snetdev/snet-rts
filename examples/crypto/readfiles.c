#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "readfiles.h"

static void pexit(const char *s) { perror(s); exit(1); }
static void assure(int b, const char *s) { if (!b) pexit(s); }

static char* array_to_string(c4snet_data_t *c4data)
{
  size_t size = C4SNetArraySize(c4data);
  char* str = malloc(size + 1);

  assure(C4SNetSizeof(c4data) == CTYPE_char, __func__);
  strncpy(str, C4SNetGetData(c4data), size);
  str[size] = '\0';
  return str;
}

static c4snet_data_t *slurp_file( c4snet_data_t *filename )
{
  struct stat st;
  char *namecopy = array_to_string(filename);
  void *vptr = NULL;
  char *entries;
  c4snet_data_t *entries_cdata;
  int fd = open(namecopy, O_RDONLY);

  assure(fd >= 0, namecopy);
  fstat(fd, &st);
  entries_cdata = C4SNetAlloc(CTYPE_char, st.st_size + 1, &vptr);
  entries = (char *) vptr;
  (void) read(fd, entries, st.st_size);
  entries[st.st_size] = '\0';
  close(fd);

  free(namecopy);

  return entries_cdata;
}

static c4snet_data_t *read_pass( c4snet_data_t *pass_file, int *pass_size )
{
  c4snet_data_t *entries_cdata = slurp_file( pass_file );
  char *entries = C4SNetGetData( entries_cdata );
  int count = 0;
  char *nl;

  for (nl = strchr(entries, '\n'); nl; nl = strchr(nl + 1, '\n')) {
    ++count;
  }

  *pass_size = count;
  return entries_cdata;
}

static c4snet_data_t *read_dict( c4snet_data_t *dict_file, int *dict_size )
{
  c4snet_data_t *dict_cdata = slurp_file( dict_file );
  char *dict = C4SNetGetData( dict_cdata );
  int count = 0;
  char *nl;

  for (nl = strchr(dict, '\n'); nl; nl = strchr(nl + 1, '\n')) {
    *nl = '\0';
    ++count;
  }

  *dict_size = count;
  return dict_cdata;
}

/* IMPLEMENT:
 *
 *     box read_files ( (dictionary_filename, passwords_filename)
 *                   -> (dictionary, entries, <dictionary_size>, <num_entries>) )
 *
 */
void *read_files( void *hnd, c4snet_data_t *dict_file, c4snet_data_t *pass_file )
{
  int dict_size = 0;
  int num_entries = 0;
  c4snet_data_t *dict, *pass;

  dict = read_dict( dict_file, &dict_size );
  pass = read_pass( pass_file, &num_entries );
  C4SNetOut( hnd, 1, dict, pass, dict_size, num_entries );
  return hnd;
}

