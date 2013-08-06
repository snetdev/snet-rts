/*
 * saveResult.h
 *
 *  Created on: Feb 14, 2013
 *      Author: vicentsanzmarco
 */

#ifndef SAVERESULT_H_
#define SAVERESULT_H_

#include "huffman.h"

void *saveResult(void *hnd, c4snet_data_t * count, c4snet_data_t * total_ele, c4snet_data_t * width,
		c4snet_data_t * height, c4snet_data_t *lastMatrix, c4snet_data_t *outfile, c4snet_data_t *infile,
		c4snet_data_t * bitstream, c4snet_data_t * color, c4snet_data_t * row, c4snet_data_t * col, c4snet_data_t * sample, int firstTime);
#endif /* SAVERESULT_H_ */
