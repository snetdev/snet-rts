/*
 * saveResult.c
 *
 *  Created on: Feb 14, 2013
 *      Author: vicentsanzmarco
 */

#include "saveResult.h"

void *saveResult(void *hnd, c4snet_data_t * count, c4snet_data_t * total_ele, c4snet_data_t * width,
		c4snet_data_t * height, c4snet_data_t *lastMatrix, c4snet_data_t *outfile, c4snet_data_t *infile,
		c4snet_data_t * bitstream, c4snet_data_t * color, c4snet_data_t * row, c4snet_data_t * col, c4snet_data_t * sample, int firstTime)
{

	int  int_count 		= * (int *) C4SNetGetData(count);
	int  int_elements 	= * (int *) C4SNetGetData(total_ele);

	int  int_row 	= * (int *) C4SNetGetData(row);
	int  int_col 	= * (int *) C4SNetGetData(col);
	int  int_sample = * (int *) C4SNetGetData(sample);

	signed char *dataunit = C4SNetGetData(bitstream);

	void ** p_lastMatrix = (void **) C4SNetGetData(lastMatrix);
	unsigned char *result = (unsigned char*) *p_lastMatrix;

	void ** p_outfile  = (void **) C4SNetGetData(outfile);
	FILE *st_outfile = (FILE*) *p_outfile;
	void ** p_infile  = (void **) C4SNetGetData(infile);
	FILE *st_infile = (FILE*) *p_infile;

	int int_total_cols = * (int *) C4SNetGetData(width);// bmpheader->width>>4;
	int int_total_rows = * (int *) C4SNetGetData(height);

	int iter1;

	for (iter1 = 0; iter1 < 64; iter1++ )
	{
		result[ (6 * 64 * int_total_cols * int_row) + (6 * 64 * int_col)  +  (64 * int_sample)  + iter1 ] = get_array_char(0, 64, iter1, dataunit);
	}

	if (((int_count * 100) %  int_elements) == 0)
	{
		printf("Complete %d %%\n", (int_count * 100) /  int_elements);
	}

	int_count = int_count + 1;
	if ( int_count < int_elements)
	{
		C4SNetOut(hnd,1, C4SNetCreate(CTYPE_int, 1, &int_count),
				    total_ele,
					width,
					height,

					C4SNetCreate(CTYPE_uchar, sizeof (void *), &result),
					outfile,
					infile, firstTime
			);

	}else
	{
	    printf("\nProcessed more than %d %dx%d-blocks.",(int_total_rows-1)*int_total_cols,MATRIX_SIZE,MATRIX_SIZE);  // +col

		int irow, icol, isample, ielement;

		char dataunit[64];

		if(firstTime == 0)
		{
			vlc_init_start();
			firstTime = 1;
		}

		for (irow=0; irow < int_total_rows; irow++)
		{
			for (icol = 0; icol < int_total_cols; icol++)
			{
				for (isample = 0; isample < 6; isample++)
				{
					for (ielement = 0; ielement < 64; ielement++)
					{
						dataunit[ielement] = (unsigned char) (result + ((6 * 64 * int_total_cols * irow) + (6 * 64 * icol)  +  (64 * isample) ))[ielement];
					}

					if (isample < 4)
					{
						EncodeDataUnit(dataunit,  0, st_outfile);
					}
					else
					{
						if ( isample == 4)
						{
							EncodeDataUnit(dataunit,  1, st_outfile);
						}else
						{
							EncodeDataUnit(dataunit,  2, st_outfile);

						}
					}
				}

			}
		}

		//vlc_stop_done(int_total_rows, int_total_cols, 'F');

		/* Start close BMP JPG */

		/* Start write jpeg footer */
		unsigned char footer[2];
		footer[0] = 0xff;
		footer[1] = 0xd9;
		//        fseek(file,0,SEEK_END);
		fwrite(footer,sizeof(footer),1, st_outfile);
		/* End write jpeg footer*/

		fclose(st_outfile);
		fclose(st_infile);

		/*End close BMP JPG */
		C4SNetOut(hnd, 2, 0);

		C4SNetFree(count);
		C4SNetFree(total_ele);
		C4SNetFree(width);
		C4SNetFree(height);
		C4SNetFree(lastMatrix);
		C4SNetFree(outfile);
		C4SNetFree(infile);
		C4SNetFree(bitstream);
		C4SNetFree(color);
	    C4SNetFree(row);
		C4SNetFree(col);
		C4SNetFree(sample);
	}


	return hnd;
}
