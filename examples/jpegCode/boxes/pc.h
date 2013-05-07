/*
 * pc.h
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#ifndef PC_H_
#define PC_H_

#include "jpeg.h"

#define __I386
#define __LITTLEENDIAN

#if PC_C_
FILE *infile, *outfile;
double st, en;
static unsigned char buffer[MACRO_BLOCK_SIZE*3];  // move array on main memory

INFOHEADER _bmpheader;
INFOHEADER *bmpheader;
JPEGHEADER _jpegheader;
#else
extern INFOHEADER *bmpheader;
#endif


void *openBMPJPG(void *hnd, c4snet_data_t * bmpfilename, c4snet_data_t * jpgfilename);
int closeBMPJPG();
void *get_MB (void *hnd, c4snet_data_t *mb_row, c4snet_data_t *mb_col);
void put_char(unsigned char c, int row, int col, char nameMatrix);
int getbmpheader(INFOHEADER *header);
void writejpegfooter();
void readbmpfile(signed char pixelmatrix[MACRO_BLOCK_SIZE][MACRO_BLOCK_SIZE*3], unsigned int mrow, unsigned int mcol, INFOHEADER * header);

#endif /* PC_H_ */
