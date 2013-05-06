/*
 * merger.c
 *
 *  Created on: Feb 25, 2013
 *      Author: vicentsanzmarco
 */


#include "Merger.h"

void *Merger (void *hnd,
              c4snet_data_t *total_ele,
              c4snet_data_t *count,
              c4snet_data_t *submatrix_in_one_row,
              c4snet_data_t *size_matrix,
              c4snet_data_t *size_submatrix,
              c4snet_data_t *result,
              c4snet_data_t *submatrix_result,
              c4snet_data_t *position)
{
        int int_total_ele = * (int *) C4SNetGetData(total_ele);
        int int_count = * (int *) C4SNetGetData(count);

        int *int_submatrix_result = C4SNetGetData(submatrix_result);
        int *int_result = C4SNetGetData(result);

        int int_submatrix_in_one_row = * (int *) C4SNetGetData(submatrix_in_one_row);
        int int_size_matrix = * (int *) C4SNetGetData(size_matrix);
        int int_size_submatrix = * (int *) C4SNetGetData(size_submatrix);
        int int_position = * (int *) C4SNetGetData(position);

        int i, j;


        int positioni, positionj;
        int position_in_result;

        int_count = int_count + 1;

        positioni = int_position / int_submatrix_in_one_row;
        positionj = int_position % int_submatrix_in_one_row;


        for (i = 0; i < int_size_submatrix; i++)
        {
                for (j = 0; j < int_size_submatrix; j++)
                {
                        position_in_result = (int_size_matrix * (i + (positioni * int_size_submatrix)) + j + (positionj * int_size_submatrix));
                        int_result[position_in_result] = int_result[position_in_result] + int_submatrix_result[(int_size_submatrix * i) + j];
                }
        }

        if ( int_count < int_total_ele)
        {

                c4snet_data_t *new_result = C4SNetCreate(CTYPE_int,
                                                int_size_matrix*int_size_matrix,
                                                int_result);
                c4snet_data_t *c4_count = C4SNetCreate(CTYPE_int, 1, &int_count);
                C4SNetOut(hnd, 1, total_ele, c4_count, submatrix_in_one_row,
                                  size_matrix, size_submatrix, new_result);
        }
        else
        {
                printf("SOLUTION:\n");
                for (i = 0; i < int_size_matrix; i++)
                {
                        for (j = 0; j < int_size_matrix; j++)
                        {
                                printf("%d\t", int_result[(i*int_size_matrix) + j]);
                        }
                        printf("\n");
                }
                C4SNetOut(hnd, 2, 0);

                C4SNetFree(total_ele);
                C4SNetFree(submatrix_in_one_row);
                C4SNetFree(size_matrix);
                C4SNetFree(size_submatrix);
        }

        C4SNetFree(submatrix_result);
        C4SNetFree(result);
        C4SNetFree(count);
        C4SNetFree(position);

        return hnd;
}
