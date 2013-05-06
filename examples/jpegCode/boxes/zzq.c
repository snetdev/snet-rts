/*
 * zzq.c
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#include "zzq.h"


void *zzq_encode(
        void *hnd,
        c4snet_data_t *dctresult,
        c4snet_data_t *color,
        c4snet_data_t *row,
        c4snet_data_t *col,
        c4snet_data_t *sample)
{
    signed short *short_dctresult = C4SNetGetData(dctresult);

    signed char char_bitstream[NUMBER_OF_PIXELS] ;

    int i, x, y, jumped, deltax, deltay;

    x = y = deltax = deltay = jumped = 0;

    for(i=0; i < NUMBER_OF_PIXELS; i++)
    {
        if(get_array_short(y, MATRIX_SIZE, x, short_dctresult)>0)
        {
            char_bitstream[i] = (get_array_short(y,MATRIX_SIZE, x, short_dctresult)>>quantization_table[y][x]);
        }
        else
        {
            char_bitstream[i] = -((-get_array_short(y, MATRIX_SIZE, x, short_dctresult))>>quantization_table[y][x]);
        }
        if((y == 0) || (y == MATRIX_SIZE-1))
        { //on top or bottom side of matrix
            if(!jumped)
            { //first jump to element on the right
                x++;
                jumped = 1;
            }
            else
            { //modify direction
                if(i<(NUMBER_OF_PIXELS>>1))
                {
                    deltax = -1;
                    deltay = 1;
                }
                else
                {
                    deltax = 1;
                    deltay = -1;
                }

                x += deltax;
                y += deltay;
                jumped = 0;
            }
        }
        else if ((x == 0) || (x == MATRIX_SIZE-1))
        { //on left or right side of matrix
            if(!jumped)
            { //jump to element below
                y++;
                jumped = 1;
            }
            else
            { //modify direction
                if(i<(NUMBER_OF_PIXELS>>1))
                {
                    deltax = 1;
                    deltay = -1;
                }
                else
                {
                    deltax = -1;
                    deltay = 1;
                }
                x += deltax;
                y += deltay;
                jumped = 0;
            }
        }
        else
        {//not on the edges of the matrix
            x += deltax;
            y += deltay;
        }
    }

    c4snet_data_t *bitstream;

    bitstream   = C4SNetCreate (CTYPE_char, NUMBER_OF_PIXELS, char_bitstream);

    C4SNetOut(hnd,  1, bitstream, color, row, col, sample);
    C4SNetFree(dctresult);

    return hnd;
}


