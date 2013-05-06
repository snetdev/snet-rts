/*
 * ColorConversion.c
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#include "ColorConversion.h"

void RGB2YCrCb(
        void *hnd,
        signed char pixelmatrix[MACRO_BLOCK_SIZE][MACRO_BLOCK_SIZE*3],
        unsigned int sample,
        int int_row,
        int int_col)
{
    unsigned int row, col, rowoffset, coloffset, xoffset, yoffset;

    signed char char_YMatrix[MATRIX_SIZE * MATRIX_SIZE];
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

                signed char ele_YMatrix = RGB2Y(pixelmatrix[row+rowoffset][(col+coloffset)*3+2],pixelmatrix[row+rowoffset][(col+coloffset)*3+1],pixelmatrix[row+rowoffset][(col+coloffset)*3]) - 128;

                set_array_char( row, MATRIX_SIZE, col, ele_YMatrix, char_YMatrix );
            }
        }

        c4snet_data_t *YMatrix;
        YMatrix     = C4SNetCreate (CTYPE_char, (MATRIX_SIZE * MATRIX_SIZE), &char_YMatrix);

        int color_matrixY = 0;


        C4SNetOut(hnd, 2,
                  YMatrix,
                  C4SNetCreate(CTYPE_int, 1, &color_matrixY),
                  C4SNetCreate(CTYPE_int, 1, &int_row),
                  C4SNetCreate(CTYPE_int, 1, &int_col),
                  C4SNetCreate(CTYPE_int, 1, &sample));

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


        for (iter1 = 0; iter1 < MATRIX_SIZE; iter1++)
        {
            for (iter2 = 0; iter2 < MATRIX_SIZE; iter2++)
            {
                //printf("CCY -> [%d,%d,%d] %04X\n", int_row, int_col, sample, (unsigned char) get_array_char(iter1, MATRIX_SIZE, iter2, char_YMatrix));
            }
        }

        if (sample == 3)
        {
            for (iter1 = 0; iter1 < MATRIX_SIZE; iter1++)
            {
                for (iter2 = 0; iter2 < MATRIX_SIZE; iter2++)
                {
                    //printf("CCR -> [%d,%d,%d] %04X\n", int_row, int_col, sample, (unsigned char) get_array_char(iter1, MATRIX_SIZE, iter2, char_CrMatrix));
                }
            }
            for (iter1 = 0; iter1 < MATRIX_SIZE; iter1++)
            {
                for (iter2 = 0; iter2 < MATRIX_SIZE; iter2++)
                {
                    //printf("CCB -> [%d,%d,%d] %04X\n", int_row, int_col, sample, (unsigned char) get_array_char(iter1, MATRIX_SIZE, iter2, char_CbMatrix));
                }
            }
        }

        c4snet_data_t *CrMatrix;
        c4snet_data_t *CbMatrix;

        CrMatrix    = C4SNetCreate (CTYPE_char, (MATRIX_SIZE * MATRIX_SIZE), &char_CrMatrix);
        CbMatrix    = C4SNetCreate (CTYPE_char, (MATRIX_SIZE * MATRIX_SIZE), &char_CbMatrix);

        int color_matrixCr = 1;
        C4SNetOut(hnd, 3,
                  CrMatrix,
                  C4SNetCreate(CTYPE_int, 1, &color_matrixCr),
                  C4SNetCreate(CTYPE_int, 1, &int_row),
                  C4SNetCreate(CTYPE_int, 1, &int_col),
                  C4SNetCreate(CTYPE_int, 1, &sample));


        int color_matrixCb = 2;
        sample = 5;
        C4SNetOut(hnd, 4,
                  CbMatrix,
                  C4SNetCreate(CTYPE_int, 1, &color_matrixCb),
                  C4SNetCreate(CTYPE_int, 1, &int_row),
                  C4SNetCreate(CTYPE_int, 1, &int_col),
                  C4SNetCreate(CTYPE_int, 1, &sample));
    }
}

