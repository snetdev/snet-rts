/*
 * dct.c
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 *
 *
 *    Function Name: dct
 *    Operation: Find the 8x8 DCT of an array using separable DCT
 *    First, finds 1-d DCT along rows, storing the result in inter[][]
 *    Then, 1-d DCT along columns of inter[][] is found
 *
 *    Input: pixels is the 8x8 input array
 *
 *    Output: dct is the 8x8 output array
 *
 */


#include "dct.h"

void *dct(
        void *hnd,
        c4snet_data_t *Matrix,
        c4snet_data_t *color,
        c4snet_data_t *row,
        c4snet_data_t *col,
        c4snet_data_t *sample)
{
    signed char *char_matrix  = C4SNetGetData(Matrix);

    signed short short_dctresult [ MATRIX_SIZE * MATRIX_SIZE];

    int intr, intc;     /* rows and columns of intermediate image */
    int outr, outc;     /* rows and columns of dct */
    int f_val;      /* cumulative sum */

    int inter[8][8];    /* stores intermediate result */

    int i,k;
        k=0;

    for (intr=0; intr<8; intr++)
    {
        for (intc=0; intc<8; intc++)
        {
            for (i=0,f_val=0; i<8; i++)
            {
                f_val +=((get_array_char(intr, MATRIX_SIZE, i, char_matrix))* weights[k]);//cos((double)(2*i+1)*(double)intc*PI/16);
                k++;
            }
            if (intc!=0)
            {
                inter[intr][intc] =  f_val>>15;
            }
            else
            {
                inter[intr][intc] =  (11585*(f_val>>14))>>15;
            }
        }
    }

    /* find 1-d dct along columns */

    for (outc=0, k=0; outc<8; outc++)
    {
        for (outr=0; outr<8; outr++)
        {
            for (i=0,f_val=0; i<8; i++)
            {
                f_val += (inter[i][outc] *weights[k]);
                k++;
            }
            if (outr!=0)
            {
                set_array_short(outr, MATRIX_SIZE, outc, (f_val>>15),short_dctresult);
            }
            else
            {
                set_array_short(outr, MATRIX_SIZE, outc, (11585*(f_val>>14)>>15), short_dctresult);
            }
        }
    }


    c4snet_data_t *dctresult;

    dctresult   = C4SNetCreate (CTYPE_short, (MATRIX_SIZE * MATRIX_SIZE), &short_dctresult);

    C4SNetOut(hnd,  1, dctresult, color, row, col, sample);
    C4SNetFree(Matrix);

    return hnd;
}


