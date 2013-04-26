/*
 * pc.c
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#include "huffman.h"

void *openBMPJPG(void *hnd, c4snet_data_t * bmpfilename, c4snet_data_t * jpgfilename)
{
    int jpegheadersize;
    unsigned int cols, rows;

    char *char_bmpfilename =  C4SNetGetData(bmpfilename);
    char *char_jpgfilename =  C4SNetGetData(jpgfilename);


    bmpheader=&_bmpheader;

    infile = fopen(char_bmpfilename,"rb");

    if (infile == NULL)
    {
		printf("Input file %s does not exist!\n\nUSAGE: jpegcodec source_file destination_file [/E] [compression_rate]\n", char_bmpfilename);
        exit(0);
	}

    outfile = fopen(char_jpgfilename,"wb");

    if (outfile == NULL)
    {
		printf("Output file %s does not work!\n\nUSAGE: jpegcodec source_file destination_file [/E] [compression_rate]\n", char_jpgfilename);
        exit(0);
	}

    if (getbmpheader(bmpheader) == 0)
    {
        printf("\n%s is not a valid BMP-file",char_bmpfilename);
        exit(0);
    }

    jpegheadersize = writejpegheader(bmpheader, &_jpegheader);

    if (jpegheadersize == 0)
    {
        printf("\nerror in writing jpg header");
        exit(0);
	}

	fwrite(&_jpegheader,jpegheadersize,1,outfile);


    rows = bmpheader->height>>4;
    cols = bmpheader->width>>4;

    printf("COLS: %d - ROWS: %d\n" , cols ,rows);


    lastMatrix = malloc((rows * cols * 6 * 64) * sizeof(unsigned char));

    vlc_init_start();

	C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &rows), C4SNetCreate(CTYPE_int, 1, &cols));

	C4SNetFree(bmpfilename);
	C4SNetFree(jpgfilename);

	return hnd;
}

int closeBMPJPG()
{
    unsigned int col, cols, row, rows;

    rows = bmpheader->height>>4;
    cols = bmpheader->width>>4;

    printf("\nProcessed more than %d %dx%d-blocks.",(rows-1)*cols,MATRIX_SIZE,MATRIX_SIZE);  // +col

	writejpegfooter();

	fclose(outfile);
	fclose(infile);

     return 0;
}



void *get_MB (void *hnd, c4snet_data_t *rows, c4snet_data_t *cols)
{
	int size_matrix = MACRO_BLOCK_SIZE * (MACRO_BLOCK_SIZE*3);

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
		   readbmpfile(pixelmatrix, mb_row, mb_col, bmpheader);

		   for(sample=0;sample<5;sample++)
		   {
			  RGB2YCrCb(hnd, pixelmatrix, sample, mb_row, mb_col);
		   }
	   }
    }


    int total_ele = 0;
    int count = 0;
	total_ele = int_rows * int_cols *6;

	C4SNetOut(hnd,1,C4SNetCreate(CTYPE_int, 1, &total_ele), C4SNetCreate(CTYPE_int, 1, &count));

	return hnd;
}

void put_char(unsigned char c, int row, int col, char nameMatrix)
{
	//printf("[%d,%d] %c -> %04x -+-\n",row, col, nameMatrix, (unsigned char)c);
	fwrite(&c, 1, 1, outfile);

}
int getbmpheader(INFOHEADER *header)
{
	int retval;
	unsigned char buffer[4];

	fseek(infile,14,SEEK_SET);
	fread(header, sizeof(INFOHEADER), 1, infile);
	fseek(infile, SEEK_CUR, 1024);

	retval = 1;

    printf("\nImage width: %d pixels", bmpheader->width);
    printf("\nImage height: %d pixels\n", bmpheader->height);

    return retval;
}


void writejpegfooter()
{
   unsigned char footer[2];
   footer[0] = 0xff;
   footer[1] = 0xd9;
//        fseek(file,0,SEEK_END);
   fwrite(footer,sizeof(footer),1,outfile);
   return;

}


void readbmpfile(signed char pixelmatrix[MACRO_BLOCK_SIZE][MACRO_BLOCK_SIZE*3], unsigned int mrow, unsigned int mcol, INFOHEADER * header)
{
	unsigned int row, col;
	for(row = 0;row < MACRO_BLOCK_SIZE; row++)
	{
             	//Find first point of row in the matrix to be read.
		int position = -(3*header->width*(row + 1 + mrow*MACRO_BLOCK_SIZE)-(MACRO_BLOCK_SIZE*3)*mcol);
		long lposition = position;

		fseek(infile,lposition,SEEK_END);
             	//Read row from matrix
		fread(buffer, 1, MACRO_BLOCK_SIZE*3, infile);
		for(col = 0; col < MACRO_BLOCK_SIZE*3; col++)
		{
			pixelmatrix[row][col] = buffer[col]- 128;
		}
	}
	return;
 }




