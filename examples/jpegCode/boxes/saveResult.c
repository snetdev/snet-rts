/*
 * saveResult.c
 *
 *  Created on: Feb 14, 2013
 *      Author: vicentsanzmarco
 */


#include "saveResult.h"
#include "huffman.h"

void *saveResult(
        void *hnd,
        c4snet_data_t *count,
        c4snet_data_t *total_ele,
        c4snet_data_t *bitstream,
        c4snet_data_t *color,
        c4snet_data_t *row,
        c4snet_data_t *col,
        c4snet_data_t *sample)
{
    int  int_count = * (int *) C4SNetGetData(count);
    int  int_elements = * (int *) C4SNetGetData(total_ele);

    int  int_row = * (int *) C4SNetGetData(row);
    int  int_col = * (int *) C4SNetGetData(col);
    int  int_sample = * (int *) C4SNetGetData(sample);

    signed char *dataunit   = C4SNetGetData(bitstream);

    int Total_cols = bmpheader->width>>4;
    int iter1;

    for (iter1 = 0; iter1 < 64; iter1++ )
    {
        lastMatrix[ (6 * 64 * Total_cols * int_row) + (6 * 64 * int_col)  +  (64 * int_sample)  + iter1 ] = get_array_char(0, 64, iter1, dataunit);
    }

    int_count = int_count + 1;
    if ( int_count < int_elements)
    {
        C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int,1, &int_count), total_ele);
    }
    else {
        unsigned int  cols, rows;

        rows = bmpheader->height>>4;
        cols = bmpheader->width>>4;
        printf("Processed more than %d %dx%d-blocks.\n",(rows-1)*cols,MATRIX_SIZE,MATRIX_SIZE);  // +col


        int row, col, sample, element;

        char dataunit[64];

        for (row=0; row < rows; row++)
        {
            for (col = 0; col < cols; col++)
            {
                for (sample = 0; sample < 6; sample++)
                {
                    for (element = 0; element < 64; element++)
                    {
                        dataunit[element] = (unsigned char) (lastMatrix + ((6 * 64 * cols * row) + (6 * 64 * col)  +  (64 * sample) ))[element];
                    }

                    if (sample < 4)
                    {
                        EncodeDataUnit(dataunit,  0, row, col, 'Y', sample);
                    }
                    else
                    {
                        if ( sample == 4)
                        {
                            EncodeDataUnit(dataunit,  1, row, col, 'R', sample);
                        }else
                        {
                            EncodeDataUnit(dataunit,  2, row, col, 'B', sample);

                        }
                    }
                }

            }
        }

        vlc_stop_done(rows, cols, 'F');

        closeBMPJPG();
        C4SNetOut(hnd, 2, 0);

        C4SNetFree(total_ele);
        free(lastMatrix);
    }

    C4SNetFree(count);
    C4SNetFree(bitstream);
    C4SNetFree(color);
    C4SNetFree(row);
    C4SNetFree(col);
    C4SNetFree(sample);

    return hnd;
}

