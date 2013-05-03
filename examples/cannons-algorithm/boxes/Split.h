/*
 * Split.h
 *
 *  Created on: Feb 25, 2013
 *      Author: vicentsanzmarco
 */

#ifndef SPLIT_H_
#define SPLIT_H_

#include <C4SNet.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void *Split (void* hnd, c4snet_data_t *matrix_A, c4snet_data_t *matrix_B, c4snet_data_t *size_matrix, c4snet_data_t *division);

#endif /* SPLIT_H_ */
