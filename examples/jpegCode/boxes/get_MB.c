/*
 * get_MB.c
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#include "get_MB.h"

void *get_MB (void *hnd, c4snet_data_t *rows, c4snet_data_t *cols, c4snet_data_t * infile, c4snet_data_t * outfile)
{
	int size_matrix = MACRO_BLOCK_SIZE * (MACRO_BLOCK_SIZE*3);

	void** p_infile  = (void **) C4SNetGetData(infile);
	void** p_outfile = (void **) C4SNetGetData(outfile);

	FILE *st_infile = (FILE *) *p_infile;
	FILE *st_outfile = (FILE *) *p_outfile;

	static unsigned char buffer[MACRO_BLOCK_SIZE*3];  // move array on main memory

	signed char pixelmatrix[MACRO_BLOCK_SIZE][MACRO_BLOCK_SIZE*3];

	int int_rows = *(int *) C4SNetGetData(rows);
	int int_cols = *(int *) C4SNetGetData(cols);

	int mb_row;
	int mb_col;
	int sample;

	for (mb_row = 0; mb_row < int_rows; mb_row++)
	{
	   for (mb_col = 0; mb_col < int_cols; mb_col++)
	   {
		   /* Start read bmp file */
		   unsigned int row, col;
		   for(row = 0;row < MACRO_BLOCK_SIZE; row++)
		   {
			   //Find first point of row in the matrix to be read.
			   int position = -(3*(int_cols<<4)*(row + 1 + mb_row*MACRO_BLOCK_SIZE)-(MACRO_BLOCK_SIZE*3)*mb_col);
			   long lposition = position;

			   fseek(st_infile,lposition,SEEK_END);
			   //Read row from matrix
			   fread(buffer, 1, MACRO_BLOCK_SIZE*3, st_infile);
			   for(col = 0; col < MACRO_BLOCK_SIZE*3; col++)
			   {
				   pixelmatrix[row][col] = buffer[col]- 128;
			   }
		   }
		   //readbmpfile(pixelmatrix, mb_row, mb_col, st_bmpheader);
		   /* End read bmp file */

		   for(sample=0;sample<5;sample++)
		   {
			   /* Start RGB2YCrCb */
			   unsigned int row, col, rowoffset, coloffset, xoffset, yoffset;
			   signed char char_YMatrix [MATRIX_SIZE * MATRIX_SIZE];
			   signed char char_CrMatrix[MATRIX_SIZE * MATRIX_SIZE];
			   signed char char_CbMatrix[MATRIX_SIZE * MATRIX_SIZE];

			   // Initializate the matrix
			   int iter1, iter2;
			   for (iter1 = 0; iter1 < MATRIX_SIZE; iter1++ )
			   {
				   for ( iter2 = 0; iter2 < MATRIX_SIZE; iter2++)
				   {
					   set_array_char( iter1, MATRIX_SIZE, iter2, 0x00, char_YMatrix );
					   set_array_char( iter1, MATRIX_SIZE, iter2, 0x00, char_CrMatrix );
					   set_array_char( iter1, MATRIX_SIZE, iter2, 0x00, char_CbMatrix );
				   }
			   }

			   if(sample<4)
			   {
				   for(row = 0;row < MATRIX_SIZE; row++)
				   {
					   for(col = 0; col < MATRIX_SIZE; col++)
					   {
						   coloffset = (sample&0x01)*8;
						   rowoffset = (sample&0x02)*4;

						   signed char ele_YMatrix = RGB2Y(
								   pixelmatrix[row+rowoffset][(col+coloffset)*3+2],
								   pixelmatrix[row+rowoffset][(col+coloffset)*3+1],
								   pixelmatrix[row+rowoffset][(col+coloffset)*3]
								                              ) - 128;

						   set_array_char( row, MATRIX_SIZE, col, ele_YMatrix, char_YMatrix );
					   }
				   }

				   c4snet_data_t *YMatrix;
				   YMatrix 	= C4SNetCreate (CTYPE_char, (MATRIX_SIZE * MATRIX_SIZE), &char_YMatrix);

				   int color_matrixY = 0;

				   C4SNetOut(hnd, 2, YMatrix, C4SNetCreate(CTYPE_int, 1, &color_matrixY)
						   , C4SNetCreate(CTYPE_int, 1, &mb_row), C4SNetCreate(CTYPE_int, 1, &mb_col)
						   , C4SNetCreate(CTYPE_int, 1, &sample));
			   } // end if
			   else
			   {
				   for(row = 0;row < MATRIX_SIZE; row++)
				   {
					   for(col = 0; col < MATRIX_SIZE; col++)
					   {
						   coloffset = (3&0x01)*8;
						   rowoffset = (3&0x02)*4;

						   signed char ele_CrMatrix = RGB2Cr(pixelmatrix[row+rowoffset][(col+coloffset)*3+2],pixelmatrix[row+rowoffset][(col+coloffset)*3+1],pixelmatrix[row+rowoffset][(col+coloffset)*3]) - 128;
						   signed char ele_CbMatrix = RGB2Cb(pixelmatrix[row+rowoffset][(col+coloffset)*3+2],pixelmatrix[row+rowoffset][(col+coloffset)*3+1],pixelmatrix[row+rowoffset][(col+coloffset)*3]) - 128;

						   if (col%2==0)
						   {
							   yoffset = (3&0x01)*4;
							   xoffset = (3&0x02)*2;
							   if (row%2==0)
							   {
								   set_array_char( xoffset+(row>>1), MATRIX_SIZE, (yoffset+(col>>1)), ele_CrMatrix, char_CrMatrix );
							   }
							   else
							   {
								   set_array_char( xoffset+(row>>1), MATRIX_SIZE, (yoffset+(col>>1)), ele_CbMatrix, char_CbMatrix );
							   }
						   }
					   }
				   }

				   c4snet_data_t *CrMatrix;
				   c4snet_data_t *CbMatrix;

				   CrMatrix 	= C4SNetCreate (CTYPE_char, (MATRIX_SIZE * MATRIX_SIZE), &char_CrMatrix);
				   CbMatrix 	= C4SNetCreate (CTYPE_char, (MATRIX_SIZE * MATRIX_SIZE), &char_CbMatrix);

				   int color_matrixCr = 1;
				   C4SNetOut(hnd, 3, CrMatrix, C4SNetCreate(CTYPE_int, 1, &color_matrixCr)
						   , C4SNetCreate(CTYPE_int, 1, &mb_row), C4SNetCreate(CTYPE_int, 1, &mb_col)
						   , C4SNetCreate(CTYPE_int, 1, &sample));

				   int color_matrixCb = 2;
				   sample = 5;
				   C4SNetOut(hnd, 4, CbMatrix, C4SNetCreate(CTYPE_int, 1, &color_matrixCb)
						   , C4SNetCreate(CTYPE_int, 1, &mb_row), C4SNetCreate(CTYPE_int, 1, &mb_col)
						   , C4SNetCreate(CTYPE_int, 1, &sample));
			   }

			   //RGB2YCrCb(hnd, pixelmatrix, sample, mb_row, mb_col);
			  /* End RGB2YCrCb */
		   }
	   }
    }

    int total_ele = 0;
    int count = 0;
	total_ele = int_rows * int_cols *6;
	unsigned char *lastMatrix;

	lastMatrix = malloc((int_rows * int_cols * 6 * 64) * sizeof(unsigned char));

	C4SNetOut(hnd,1,C4SNetCreate(CTYPE_int, 1, &count),
			C4SNetCreate(CTYPE_int, 1, &total_ele),
			cols,
			rows,
			//C4SNetCreate(CTYPE_uchar, (int_rows * int_cols * 6 * 64), lastMatrix),
			C4SNetCreate(CTYPE_uchar, sizeof(void*), &lastMatrix),
			outfile,
			infile, 0
	);

	return hnd;
}

