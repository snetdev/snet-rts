/*
 * dct.h
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#ifndef DCT_H_
#define DCT_H_

#include "globalSettings.h"

void *dct( void *hnd, c4snet_data_t *Matrix, c4snet_data_t *color, c4snet_data_t *row, c4snet_data_t *col, c4snet_data_t * sample);

#endif /* DCT_H_ */
