/*
 * Multiplication.c
 *
 *  Created on: Feb 25, 2013
 *      Author: vicentsanzmarco
 */

#include "Multiplication.h"

void *Multiplication (
        void* hnd,
        c4snet_data_t *submatrix_A,
        c4snet_data_t *submatrix_B,
        c4snet_data_t *size_submatrix,
        c4snet_data_t *position)
{

        int *int_submatrix_A = C4SNetGetData(submatrix_A);
        int *int_submatrix_B = C4SNetGetData(submatrix_B);
        int  int_size_matrix = *(int *) C4SNetGetData(size_submatrix);
        int *int_submatrix_result;
        int  i, j;
        void *vptr = NULL;
        c4snet_data_t *submatrix_result = C4SNetAlloc(CTYPE_int,
                                              int_size_matrix * int_size_matrix,
                                              &vptr);
        int_submatrix_result = (int *) vptr;

        /*
         * Elements are moved from their initial positions to align them
         * so that the correct submatrix are multiplied with one another.
         * Note that submatrix on the diagonal don't actually require alignment.
         * Alignment is done by shifting the -th row of positions left
         * and the -th column of positions up.
         */
        for (i = 0; i < int_size_matrix; i++)
        {
                for (j = 0; j < int_size_matrix; j++)
                {
                        int_submatrix_result[(i*int_size_matrix) + j] =
                             int_submatrix_A[(i*int_size_matrix) + j] *
                             int_submatrix_B[(i*int_size_matrix) + j];
                }
        }

        C4SNetOut(hnd, 1, submatrix_result, position);

        C4SNetFree(submatrix_A);
        C4SNetFree(submatrix_B);
        C4SNetFree(size_submatrix);

        return hnd;
}

