#ifndef ALGORITHM_H_
#define ALGORITHM_H_

#include <C4SNet.h>

void *algorithm( void *hnd, 
		 c4snet_data_t *password, 
		 c4snet_data_t *salt,
		 c4snet_data_t *dictionary,
		 int dictionary_size);

#endif /* ALGORITHM_H_ */
