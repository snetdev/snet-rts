/*
 * openBMP.h
 *
 *  Created on: Feb 11, 2013
 *      Author: vicentsanzmarco
 */

#include "openBMP.h"
#include <math.h>

void *openBMP(void *hnd, c4snet_data_t * bmpfilename, c4snet_data_t * jpgfilename)
{
	FILE *infile, *outfile;
	INFOHEADER _bmpheader;
	INFOHEADER *bmpheader;
	JPEGHEADER _jpegheader;

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

    /* Start get bmp header */

	unsigned char buffer[4];

	fseek(infile,14,SEEK_SET);
	fread(bmpheader, sizeof(INFOHEADER), 1, infile);
	fseek(infile, SEEK_CUR, 1024);


	bmpheader->width  = abs(bmpheader->width);
	bmpheader->height = abs(bmpheader->height);

	bmpheader->xresolution = abs(bmpheader->xresolution);
	bmpheader->yresolution = abs(bmpheader->yresolution);

	printf("\nImage width: %d pixels", bmpheader->width);
	printf("\nImage height: %d pixels\n", bmpheader->height);


	//if (getbmpheader(bmpheader) == 0)
	//{
	  //  printf("\n%s is not a valid BMP-file",char_bmpfilename);
		//exit(0);
	//}
    /*End get bmp header */

	/* Start writejpegheader */

	unsigned int  huffmantablesize, previoussize;
	unsigned char QTcount, i, j, components, id, huffmantablecount;
	unsigned short length, headerlength;

	//Number of Quatization Tables
	QTcount = 2;
	headerlength = 12; //12 bytes are needed for the markers
	huffmantablecount = 4;  //2 AC and 2 DC tables
	huffmantablesize = 0;

	_jpegheader.SOIMarker[0] = 0xff;
	_jpegheader.SOIMarker[1] = 0xd8;

	//APP0 segment
	_jpegheader.app0.APP0Marker[0] = 0xff;
	_jpegheader.app0.APP0Marker[1] = 0xe0;

	headerlength += 16; //APP0 marker is always 16 bytes long
	_jpegheader.app0.Length[0] = 0x00;
	_jpegheader.app0.Length[1] = 0x10;
	_jpegheader.app0.Identifier[0] = 0x4a;
	_jpegheader.app0.Identifier[1] = 0x46;
	_jpegheader.app0.Identifier[2] = 0x49;
	_jpegheader.app0.Identifier[3] = 0x46;
	_jpegheader.app0.Identifier[4] = 0x00;
	_jpegheader.app0.Version[0] = 0x01;
	_jpegheader.app0.Version[1] = 0x00;
	_jpegheader.app0.Units = 0x00;
	_jpegheader.app0.XDensity[0] = 0x00;
	_jpegheader.app0.XDensity[1] = 0x01;
	_jpegheader.app0.YDensity[0] = 0x00;
	_jpegheader.app0.YDensity[1] = 0x01;
	_jpegheader.app0.ThumbWidth = 0x00;
	_jpegheader.app0.ThumbHeight = 0x00;

	//Quantization Table Segment
	_jpegheader.qt.QTMarker[0] = 0xff;
	_jpegheader.qt.QTMarker[1] = 0xdb;
	length = (QTcount<<6) + QTcount + 2;
	headerlength += length;
	_jpegheader.qt.Length[0] = (length & 0xff00)>>8;
	_jpegheader.qt.Length[1] = length & 0xff;
	// jpegheader->qt.QTInfo = 0x00; // index = 0, precision = 0
	//write Quantization table to header
	i = 0;

	for (id=0; id<QTcount; id++)
	{
		_jpegheader.qt.QTInfo[(id<<6)+id] = id;

		for(i=0;i<64;i++)
		{
			_jpegheader.qt.QTInfo[i+1+id+(id<<6)] = qtable[i];
		}
	}

	//Start of Frame segment
	_jpegheader.sof0.SOF0Marker[0] = 0xff;
	_jpegheader.sof0.SOF0Marker[1] = 0xc0;

	if(bmpheader->bits == 8)
	{
		components = 0x01;
	}
	else {
		components = 0x03;
	}

	length = 8 + 3*components;
	headerlength += length;
	_jpegheader.sof0.Length[0] = (length & 0xff00) >> 8;
	_jpegheader.sof0.Length[1] = length & 0xff;
	_jpegheader.sof0.DataPrecision = 0x08;
	_jpegheader.sof0.ImageHeight[0] = (bmpheader->height & 0xff00) >> 8;
	_jpegheader.sof0.ImageHeight[1] = bmpheader->height & 0xff;
	_jpegheader.sof0.ImageWidth[0] = (bmpheader->width & 0xff00) >> 8;
	_jpegheader.sof0.ImageWidth[1] = bmpheader->width & 0xff;
	_jpegheader.sof0.Components  = components;

	for (i=0; i < components; i++)
	{
		_jpegheader.sof0.ComponentInfo[i][0] = i+1; //color component

		if(i==0)
		{
			_jpegheader.sof0.ComponentInfo[i][1] = 0x22; //4:2:0 subsampling
		} else {
			_jpegheader.sof0.ComponentInfo[i][1] = 0x11; //4:2:0 subsampling
		}

		_jpegheader.sof0.ComponentInfo[i][2] = (i==0)? 0x00 : 0x01; //quantization table ID
	}
	//Start of Huffman Table Segment
	_jpegheader.ht.HTMarker[0] = 0xff;
	_jpegheader.ht.HTMarker[1] = 0xc4;

	//Set dummy HT segment length
	length = 0;//tablecount*17;
	_jpegheader.ht.Length[0] = (length & 0xff00) >> 8;
	_jpegheader.ht.Length[1] = length & 0xff;
	previoussize = 0;

	for (id=0; id < huffmantablecount; id++)
	{
		huffmantablesize = 0;
		switch (id)
		{
			case 0 : _jpegheader.ht.HuffmanInfo[previoussize] = 0x00;
				 break;
			case 1 : _jpegheader.ht.HuffmanInfo[previoussize] = 0x10;
				 break;
			case 2 : _jpegheader.ht.HuffmanInfo[previoussize] = 0x01;
				 break;
			case 3 : _jpegheader.ht.HuffmanInfo[previoussize] = 0x11;
				 break;
		}

		for (i=1; i <= 16; i++)
		{
			_jpegheader.ht.HuffmanInfo[i+previoussize] =  huffmancount[id][i-1];
			huffmantablesize += huffmancount[id][i-1];
		}

		for (i=0; i < huffmantablesize; i++)
		{
			_jpegheader.ht.HuffmanInfo[i+previoussize+17] = (id%2 == 1)? huffACvalues[i] : huffDCvalues[i];
		}

		previoussize += huffmantablesize + 17;
	}
	//Set real HT segment length
	length = 2+previoussize;
	headerlength += length;
	_jpegheader.ht.Length[0] = (length & 0xff00) >> 8;
	_jpegheader.ht.Length[1] = length & 0xff;
	//Reset marker segment

	//Start of Scan Header Segment
	_jpegheader.sos.SOSMarker[0] = 0xff;
	_jpegheader.sos.SOSMarker[1] = 0xda;
	length = 6 + (components<<1);
	headerlength += length;
	_jpegheader.sos.Length[0] = (length & 0xff00) >> 8;
	_jpegheader.sos.Length[1] =  length & 0xff;
	_jpegheader.sos.ComponentCount = components; //number of color components in the image
	_jpegheader.sos.Component[0][0] = 0x01; //Y component
	_jpegheader.sos.Component[0][1] = 0x00; //indexes of huffman tables for Y-component

	if (components == 0x03)
	{
		_jpegheader.sos.Component[1][0] = 0x02; //the CB component
		_jpegheader.sos.Component[1][1] = 0x11; //indexes of huffman tables for CB-component
		_jpegheader.sos.Component[2][0] = 0x03; //The CR component
		_jpegheader.sos.Component[2][1] = 0x11; //indexes of huffman tables for CR-component
	}
	//following bytes are ignored since progressive scan is not to be implemented
	_jpegheader.sos.Ignore[0] = 0x00;
	_jpegheader.sos.Ignore[1] = 0x3f;
	_jpegheader.sos.Ignore[2] = 0x00;

	jpegheadersize = headerlength;

	//jpegheadersize = writejpegheader(bmpheader, &_jpegheader);

	if (jpegheadersize == 0)
	{
		printf("\nerror in writing jpg header");
		exit(0);
	}

	/*End writejpegheader */

	fwrite(&_jpegheader,jpegheadersize,1,outfile);

    rows = bmpheader->height>>4;
    cols = bmpheader->width>>4;

    printf("COLS: %d - ROWS: %d\n" , cols ,rows);

	C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &rows), C4SNetCreate(CTYPE_int, 1, &cols),
			C4SNetCreate(CTYPE_uchar, sizeof(void*), &infile),
			C4SNetCreate(CTYPE_uchar, sizeof(void*), &outfile)
	);

	C4SNetFree(bmpfilename);
	C4SNetFree(jpgfilename);

	return hnd;
}

