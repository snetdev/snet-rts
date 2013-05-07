#ifndef READFILES_H_
#define READFILES_H_

#include <C4SNet.h>

/* IMPLEMENT:
 *
 *     box read_files ( (dictionary_filename, passwords_filename)
 *                   -> (dictionary, entries, <dictionary_size>, <num_entries>) )
 *
 */
void *read_files( void *hnd, c4snet_data_t *dict_file, c4snet_data_t *pass_file );

#endif
