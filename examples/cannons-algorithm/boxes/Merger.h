/*
 * Merger.h
 *
 *  Created on: Feb 25, 2013
 *      Author: vicentsanzmarco
 */

#ifndef MERGER_H_
#define MERGER_H_

#include "C4SNet.h"
#include <stdio.h>
#include <stdlib.h>

void *Merger (void *hnd, c4snet_data_t *total_ele, c4snet_data_t *count, c4snet_data_t *submatrix_in_one_row, c4snet_data_t *size_matrix, c4snet_data_t *size_submatrix, c4snet_data_t *result, c4snet_data_t *submatrix_result, c4snet_data_t *position);


#endif /* MERGER_H_ */
