/*
 * ColorConversion.h
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#ifndef COLORCONVERSION_H_
#define COLORCONVERSION_H_

#include "ejpgl.h"

#define RGB2Y(r, g, b)     (((66*r + 129*g + 25*b + 128)>>8)+128)
#define RGB2Cr(r, g, b)    (((-38*r - 74*g + 112*b + 128)>>8)+128)
#define RGB2Cb(r, g, b)   (((112*r - 94*g - 18*b + 128)>>8)+128)

void RGB2YCrCb(void *hnd, signed char pixelmatrix[MACRO_BLOCK_SIZE][MACRO_BLOCK_SIZE*3], unsigned int sample, int int_row, int int_col);

#endif /* COLORCONVERSION_H_ */
