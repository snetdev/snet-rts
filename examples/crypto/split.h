#ifndef SPLIT_H_
#define SPLIT_H_

#include <C4SNet.h>

void *split( void *hnd, 
	     c4snet_data_t *entries, 
	     int num_entries);

#endif /* SPLIT_H_ */
