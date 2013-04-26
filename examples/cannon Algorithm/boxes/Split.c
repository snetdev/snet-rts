/*
 * Split.c
 *
 *  Created on: Feb 25, 2013
 *      Author: vicentsanzmarco
 */


#include "Split.h"


void *Split (void* hnd, c4snet_data_t *matrix_A, c4snet_data_t *matrix_B, c4snet_data_t *size_matrix, c4snet_data_t *division)
{
	 int *int_matrix_A  = C4SNetGetData(matrix_A);
	 int *int_matrix_B  = C4SNetGetData(matrix_B);

	 int int_size_matrix = *(int *) C4SNetGetData(size_matrix);
	 int int_division = *(int *) C4SNetGetData(division);


	 int *int_result = malloc(sizeof(int) * int_size_matrix * int_size_matrix);

	 int **int_submatrix_collect_A = malloc(sizeof(int *) * int_division);
	 int **int_submatrix_collect_B = malloc(sizeof(int *) * int_division);

	 int i, j, k, z;

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

	 int *int_submatrix_A;
	 int *int_submatrix_B;
	 int *int_copy_matrix_A = malloc(sizeof(int) * int_size_matrix*int_size_matrix);
	 int *int_copy_matrix_B = malloc(sizeof(int) * int_size_matrix*int_size_matrix);

	 int_submatrix_A = malloc(sizeof(int) * size_submatrix*size_submatrix);
	 int_submatrix_B = malloc(sizeof(int) * size_submatrix*size_submatrix);

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
					 int_copy_matrix_A[(i*int_size_matrix) + j] = int_matrix_A[ ( (i * int_size_matrix) + (j + row + repetition) )];
				 }
				 else
				 {
					 int_copy_matrix_A[(i*int_size_matrix) + j] = int_matrix_A[ ( (i * int_size_matrix) + ((j + row + repetition) % int_size_matrix) )];
				 }

				 if ( ( (i*int_size_matrix) + (col * int_size_matrix) + (repetition * int_size_matrix) ) < ( int_size_matrix * int_size_matrix ) )
				 {
					 int_copy_matrix_B[(i*int_size_matrix) + j] = int_matrix_B[ j + ( (i + col + repetition ) * int_size_matrix) ];
				 }
				 else
				 {
					 int_copy_matrix_B[(i*int_size_matrix) + j] = int_matrix_B[ j + (( (i + col + repetition ) * int_size_matrix) % (int_size_matrix * int_size_matrix)) ];
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
					 int_submatrix_A[(i*size_submatrix) + j] = int_copy_matrix_A[(i + (varrow * size_submatrix))*int_size_matrix + (j + (varcol * size_submatrix))];
					 int_submatrix_B[(i*size_submatrix) + j] = int_copy_matrix_B[(i + (varrow * size_submatrix))*int_size_matrix + (j + (varcol * size_submatrix))];
				 }
			 }

			 // right
			 if ( (position + 1)% (int_size_matrix/size_submatrix) == 0 )
			 {
				 varcol = 0;
				 varrow++;
			 }
			 else //down and begin
			 {
				 varcol++;
			 }

			 // send data
			 C4SNetOut(hnd, 2, C4SNetCreate (CTYPE_int, size_submatrix*size_submatrix, int_submatrix_A),
					 C4SNetCreate (CTYPE_int, size_submatrix*size_submatrix, int_submatrix_B),
					 C4SNetCreate(CTYPE_int, 1, &size_submatrix), C4SNetCreate(CTYPE_int, 1, &position));
		 }
	 }

	 // Second message
	 // int_size_matrix * int_size_matrix * submatrix_in_one_row = number of operations of submatrix
	 int total_ele = int_size_matrix * int_division;

	 int count = 0;

	 c4snet_data_t *result;
	 for (i = 0; i < int_size_matrix; i++)
	 {
		 for (j = 0; j < int_size_matrix; j++)
		 {
			 int_result[(i*int_size_matrix) + j] = 0;
		 }
	 }

	 result 	= C4SNetCreate (CTYPE_int, int_size_matrix*int_size_matrix, int_result);


	 C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &total_ele), C4SNetCreate(CTYPE_int, 1, &count), C4SNetCreate(CTYPE_int, 1, &submatrix_in_one_row), size_matrix, C4SNetCreate(CTYPE_int, 1, &size_submatrix), result);

	return hnd;
}
