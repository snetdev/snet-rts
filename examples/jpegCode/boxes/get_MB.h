/*
 * get_MB.h
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#ifndef GET_MB_H_
#define GET_MB_H_

#include "globalSettings.h"

#define RGB2Y(r, g, b)     (((66*r + 129*g + 25*b + 128)>>8)+128)
#define RGB2Cr(r, g, b)    (((-38*r - 74*g + 112*b + 128)>>8)+128)
#define RGB2Cb(r, g, b)   (((112*r - 94*g - 18*b + 128)>>8)+128)

void *get_MB (void *hnd, c4snet_data_t *rows, c4snet_data_t *cols, c4snet_data_t * infile, c4snet_data_t * outfile);
#endif /* PC_H_ */
