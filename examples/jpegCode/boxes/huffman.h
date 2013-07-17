/*
 * huffman.h
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#ifndef HUFFMAN_H_
#define HUFFMAN_H_


#include "globalSettings.h"
#include <string.h>

static unsigned int vlc_remaining;
static unsigned char vlc_amount_remaining;
static unsigned char dcvalue[4];   // 3 is enough

#define vlc_output_byte(c, outfile, row, col, nameMatrix)          fwrite(&c, 1, 1, outfile)//;   put_char(c,row, col, nameMatrix)

int vlc_init_start();

void ConvertDCMagnitudeC(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght);
void ConvertACMagnitudeC(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght);
void ConvertDCMagnitudeY(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght);
void ConvertACMagnitudeY(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght);
char Extend (char additional, unsigned char magnitude);
void ReverseExtend (char value, unsigned char *magnitude, unsigned char *bits);


void WriteRawBits16(unsigned char amount_bits, unsigned int bits, FILE *outfile);
void HuffmanEncodeFinishSend(FILE *outfile);
void HuffmanEncodeUsingDCTable(unsigned char magnitude, FILE *outfile);
void HuffmanEncodeUsingACTable(unsigned char mag, FILE *outfile);

char EncodeDataUnit(char dataunit[64], unsigned int color, FILE *outfile);

#endif /* HUFFMAN_H_ */
