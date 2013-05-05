/*
 * Split.c
 *
 *  Created on: Feb 25, 2013
 *      Author: vicentsanzmarco
 */


#include "Split.h"


void *Split (
        void* hnd,
        c4snet_data_t *matrix_A,
        c4snet_data_t *matrix_B,
        c4snet_data_t *size_matrix,
        c4snet_data_t *division)
{
        int *int_matrix_A  = C4SNetGetData(matrix_A);
        int *int_matrix_B  = C4SNetGetData(matrix_B);
        int int_size_matrix = *(int *) C4SNetGetData(size_matrix);
        int int_division = *(int *) C4SNetGetData(division);
        int i, j;
        int repetition, row, col;

        /*if (int_division <= 0 || int_size_matrix % sqrt(int_division) != 0)
        {
                printf("THe number of division is not possible. ");
                return hnd;
        }*/

        int submatrix_in_one_row = sqrt(int_division);
        int size_submatrix = (int_size_matrix / sqrt(int_division));
        int varrow = 0;
        int varcol = 0;

        int *int_copy_matrix_A = malloc(sizeof(int) * int_size_matrix*int_size_matrix);
        int *int_copy_matrix_B = malloc(sizeof(int) * int_size_matrix*int_size_matrix);
        int *int_submatrix_A = malloc(sizeof(int) * size_submatrix*size_submatrix);
        int *int_submatrix_B = malloc(sizeof(int) * size_submatrix*size_submatrix);

        int position;

        for ( repetition = 0; repetition < int_size_matrix; repetition++ )
        {
                row = 0;
                col = 0;

                for (i = 0; i < int_size_matrix; i++)
                {
                        for (j = 0; j < int_size_matrix; j++)
                        {
                                if ( (j + row + repetition) < int_size_matrix )
                                {
                                        int_copy_matrix_A[(i*int_size_matrix) + j] =
                                             int_matrix_A[ ( (i * int_size_matrix) +
                                                           (j + row + repetition) )];
                                }
                                else
                                {
                                        int_copy_matrix_A[(i*int_size_matrix) + j] =
                                             int_matrix_A[ ( (i * int_size_matrix) +
                                                           ((j + row + repetition) %
                                                                 int_size_matrix) )];
                                }

                                if ( ( (i*int_size_matrix) + (col * int_size_matrix) +
                                     (repetition * int_size_matrix) ) <
                                     ( int_size_matrix * int_size_matrix ) )
                                {
                                        int_copy_matrix_B[(i*int_size_matrix) + j] =
                                         int_matrix_B[ j + ( (i + col + repetition ) *
                                                                  int_size_matrix) ];
                                }
                                else
                                {
                                        int_copy_matrix_B[(i*int_size_matrix) + j] =
                                         int_matrix_B[ j + (( (i + col + repetition ) *
                                                  int_size_matrix) %
                                                 (int_size_matrix * int_size_matrix)) ];
                                }

                                col++;
                        }
                        row++;
                }
                varrow = 0;
                varcol = 0;

                for ( position = 0; position < int_division; position++ )
                {
                        for (i = 0; i < size_submatrix; i++)
                        {
                                for (j = 0; j < size_submatrix; j++)
                                {
                                        int_submatrix_A[(i*size_submatrix) + j] =
                                         int_copy_matrix_A[(i + (varrow * size_submatrix)) *
                                         int_size_matrix + (j + (varcol * size_submatrix))];
                                        int_submatrix_B[(i*size_submatrix) + j] =
                                         int_copy_matrix_B[(i + (varrow * size_submatrix))*
                                         int_size_matrix + (j + (varcol * size_submatrix))];
                                }
                        }

                        // right
                        if ( (position + 1) % (int_size_matrix/size_submatrix) == 0 )
                        {
                                varcol = 0;
                                varrow++;
                        }
                        else //down and begin
                        {
                                varcol++;
                        }

                        // send data
                        c4snet_data_t *c4_subA = C4SNetCreate (CTYPE_int,
                                                   size_submatrix*size_submatrix,
                                                   int_submatrix_A);
                        c4snet_data_t *c4_subB = C4SNetCreate (CTYPE_int,
                                                   size_submatrix*size_submatrix,
                                                   int_submatrix_B);
                        c4snet_data_t *c4_size = C4SNetCreate(CTYPE_int, 1,
                                                   &size_submatrix);
                        c4snet_data_t *c4_pos  = C4SNetCreate(CTYPE_int, 1,
                                                   &position);
                        C4SNetOut(hnd, 2, c4_subA, c4_subB, c4_size, c4_pos);
                }
        }

        free(int_copy_matrix_A);
        free(int_copy_matrix_B);
        free(int_submatrix_A);
        free(int_submatrix_B);

        // Second message
        // int_size_matrix * int_size_matrix * submatrix_in_one_row =
        //                        number of operations of submatrix
        int total_ele = int_size_matrix * int_division;
        int count = 0;
        void *vptr = NULL;
        c4snet_data_t *c4_result = C4SNetAlloc (CTYPE_int,
                                                int_size_matrix*int_size_matrix,
                                                &vptr);
        int *int_result = (int *) vptr;

        for (i = 0; i < int_size_matrix; i++)
        {
                for (j = 0; j < int_size_matrix; j++)
                {
                        int_result[(i*int_size_matrix) + j] = 0;
                }
        }

        c4snet_data_t *c4_total_ele   = C4SNetCreate(CTYPE_int, 1, &total_ele);
        c4snet_data_t *c4_count       = C4SNetCreate(CTYPE_int, 1, &count);
        c4snet_data_t *c4_submatrix_1 = C4SNetCreate(CTYPE_int, 1, &submatrix_in_one_row);
        c4snet_data_t *c4_size_subm_1 = C4SNetCreate(CTYPE_int, 1, &size_submatrix);
        C4SNetOut(hnd, 1,
                  c4_total_ele,
                  c4_count,
                  c4_submatrix_1,
                  size_matrix,
                  c4_size_subm_1,
                  c4_result);

        C4SNetFree(matrix_A);
        C4SNetFree(matrix_B);
        C4SNetFree(division);

        return hnd;
}
