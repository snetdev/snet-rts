/*
 * saveResult.h
 *
 *  Created on: Feb 14, 2013
 *      Author: vicentsanzmarco
 */

#ifndef SAVERESULT_H_
#define SAVERESULT_H_

#include "ejpgl.h"

void *saveResult(void *hnd, c4snet_data_t * count, c4snet_data_t * total_ele, c4snet_data_t * bitstream, c4snet_data_t * color, c4snet_data_t * row, c4snet_data_t * col, c4snet_data_t * sample);

#endif /* SAVERESULT_H_ */
