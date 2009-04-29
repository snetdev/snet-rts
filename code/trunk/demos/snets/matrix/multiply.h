#ifndef MULTIPLY_H_
#define MULTIPLY_H_

#include <C4SNet.h>

void *multiply( void *hnd, c4snet_data_t *a, c4snet_data_t *B, int A_width, int A_height, int B_width, int B_height, int first, int last);

#endif /* MULTIPLY_H_ */
