/*
 * zzq.h
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#ifndef ZZQ_H_
#define ZZQ_H_


#include "ejpgl.h"

unsigned char quantization_table[MATRIX_SIZE][MATRIX_SIZE] ={
        {4, 3, 3, 4, 4, 5, 6, 6},
        {3, 3, 4, 4, 5, 6, 6, 6},
        {4, 4, 4, 4, 5, 6, 6, 6},
        {4, 4, 4, 5, 6, 6, 6, 6},
        {4, 4, 5, 6, 6, 7, 7, 6},
        {4, 5, 6, 6, 6, 7, 7, 6},
        {6, 6, 6, 6, 7, 7, 7, 7},
        {6, 6, 6, 7, 7, 7, 7, 7}
    };

void *zzq_encode( void *hnd, c4snet_data_t *dctresult, c4snet_data_t *color, c4snet_data_t *row, c4snet_data_t *col,c4snet_data_t * sample);

#endif /* ZZQ_H_ */
