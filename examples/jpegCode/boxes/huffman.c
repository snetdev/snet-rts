/*
Only encoder
This version works correctly, it is tested with testcase.jpg
The translation into real huffman codes works.
Changed: If huffman wants to send 0xFFxx (FF in one byte) than there must be 0x00 inserted between FF and xx
possible fault in finish send:
-must it be filled up with zeros?          YES
-must it be filled up to one bye? or 2 byte? --> in this code there is filled up to 2 bytes, but I (joris) thinks this must be filled up to 1 byte.
 still dont know
- 24-11-05 code clean up
- 24-11-05 tables added for color



Block numbers:
Y = 0
cb =1
cr= 2
*/
//---------------------------------------------------------------------------

#define HUFFMAN_C_      1
#include "huffman.h"


int vlc_init_start()
{
	vlc_remaining=0x00;
	vlc_amount_remaining=0x00;
    memset(dcvalue, 0, 4);
    return 0;

}


int vlc_stop_done(int row, int col, char nameMatrix) {

       HuffmanEncodeFinishSend(row, col, nameMatrix);
       return 0;
}

void ConvertDCMagnitudeC(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght)
{
    unsigned char len;

/*  if ((magnitude>16) || ((len=convertDCMagnitudeCLengthTable[magnitude])==0)) {
        printf("WAARDE STAAT NIET IN TABEL!!!!!!!!!!!!!!!!!!!!\n");
        } */

    len=convertDCMagnitudeCLengthTable[magnitude];
    *lenght = len;
    *out = convertDCMagnitudeCOutTable[magnitude];

}

//===========================================================================
void ConvertACMagnitudeC(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght)
{
    unsigned char len;

    len = convertACMagnitudeCLengthTable[magnitude];
/*  if (!len) {
        printf("WAARDE STAAT NIET IN TABEL!!!!!!!!!!!!!!!!!!!!\n");
        } */
    *lenght = len;
    *out = convertACMagnitudeCOutTable[magnitude];

}


//===========================================================================
void ConvertDCMagnitudeY(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght)
{
    unsigned char len;

/*  if ((magnitude>16) || ((len=convertDCMagnitudeYLengthTable[magnitude])==0)) {
        printf("WAARDE STAAT NIET IN TABEL!!!!!!!!!!!!!!!!!!!!\n");
        } */
    len=convertDCMagnitudeYLengthTable[magnitude];
    *lenght = len;
    *out = convertDCMagnitudeYOutTable[magnitude];
}

//===========================================================================
void ConvertACMagnitudeY(unsigned char magnitude,unsigned short int *out, unsigned short int *lenght)
{
    unsigned char len;

    len = convertACMagnitudeYLength[magnitude];
/*  if (!len) {
#ifndef __MICROBLAZE
    printf("WAARDE STAAT NIET IN TABEL!!!!!!!!!!!!!!!!!!!!\n");
#endif
        } */
    *lenght = len;
    *out = convertACMagnitudeYOut[magnitude];

}

char Extend (char additional, unsigned char magnitude)
{
        int vt= 1 << (magnitude-1);
        if ( additional < vt ) return (additional + (-1 << magnitude) + 1);
        else return additional;
}

void ReverseExtend (char value, unsigned char *magnitude, unsigned char *bits)
{
 // printf("reverseextend value= %d\n",*magnitude);
    if (value >=0)
    {
        *bits=value;
    }
    else
    {
        value=-value;
        *bits=~value;
    }
    *magnitude=0;
    while (value !=0)
    {
        value>>=1;
        ++*magnitude;
    }
 // printf("reverseextend magnitude= %d bits= %d",magnitude,bits);
    return;
}

void WriteRawBits16(unsigned char amount_bits, unsigned int bits, int row, int col, char nameMatrix)
//*remaining needs bo be more than 8 bits because 8 bits could be added and ther ecould already be up ot 7 bits in *remaining
// this function collects bits to send
// if there less than 16 bits collected, nothing is send and these bits are stored in *remaining. In *amount_remaining there is stated how much bits are stored in *remaining
// if more than 16 bits are collected, 16 bits are send and the remaining bits are stored again
{


        unsigned short int send;
        unsigned int mask;
        unsigned char send2;
        int count;
        mask=0x00;                                                              //init mask
        vlc_remaining=(vlc_remaining<<amount_bits);                                   //shift to make place for the new bits
        for (count=amount_bits; count>0; count--) mask=(mask<<1)|0x01;          //create mask for adding bit
        vlc_remaining=vlc_remaining | (bits&mask);                                    //add bits
        vlc_amount_remaining=vlc_amount_remaining + amount_bits;                      //change *amount_remaining to the correct new value
        if (vlc_amount_remaining >= 16)                                            //are there more than 16 bits in buffer, send 16 bits
        {
/* #ifndef __MICROBLAZE
if (vlc_amount_remaining >= 32 ) printf("ERROR, more bits to send %d",vlc_amount_remaining);
#endif */
                send=vlc_remaining>>(vlc_amount_remaining-16);                        //this value can be send/stored (in art this can be dony by selecting bits)
                send2=(send & 0xFF00) >>8;
		  vlc_output_byte(send2, row, col, nameMatrix);
                if (send2==0xFF)
                {
                        send2=0x00;
			  vlc_output_byte(send2, row, col, nameMatrix);
                }
                send2=send & 0xFF;
		  vlc_output_byte(send2, row, col, nameMatrix);
                if (send2==0xFF)
                {
                        send2=0x00;
		  	   vlc_output_byte(send2, row, col, nameMatrix);
                }
                vlc_amount_remaining=vlc_amount_remaining-16;                         //descrease by 16 because these are send
        }
        return;
}

void HuffmanEncodeFinishSend(int row, int col, char nameMatrix)
// There are still some bits left to send at the end of the 8x8 matrix (or maybe the file),
// the remaining bits are filled up with ones and send
// possible fault: -must it be filled up with ones?
{
        unsigned short int send;
        unsigned int mask;
        int  count;
        mask=0x00;                                                              //init mask
        if (vlc_amount_remaining >= 8)                                             //2 bytes to send, send first byte
        {
                send=vlc_remaining>>(vlc_amount_remaining-8);                         //shift so that first byte is ready to send
		  vlc_output_byte(send&0xff, row, col, nameMatrix);
                if (send==0xFF)                                                 //is this still needed????
                {
                        send=0x00;
			   vlc_output_byte(send&0xff, row, col, nameMatrix);
                }
                vlc_amount_remaining=vlc_amount_remaining -8;                         // lower the value to the amount of bits that still needs to be send
        }
        if (vlc_amount_remaining >= 0)                                             //there is a last byte to send
        {
                send=vlc_remaining<<(8-vlc_amount_remaining);                         //shift the last bits to send to the front of the byte
                mask=0x00;                                                      //init mask
                for (count=(8-vlc_amount_remaining); count>0; count--) mask=(mask<<1)|0x01; //create mask to fill byte up with ones
                send=send | mask;                                               //add the ones to the byte
                vlc_output_byte(send&0xff, row, col, nameMatrix);
                vlc_amount_remaining=0x00;                                         //is this needed?
        }
        return;
}

void HuffmanEncodeUsingDCTable(unsigned char magnitude, int row, int col, char nameMatrix)
// Translate magnitude into needed data (from table) and send it
{
        unsigned short int huffmancode, huffmanlengt;
        ConvertDCMagnitudeY(magnitude, &huffmancode, &huffmanlengt);
        WriteRawBits16(huffmanlengt,huffmancode, row, col, nameMatrix);
   	//printf("Write DC magnitude= %2x \n",magnitude);
        //WriteRawBits16(0x08,magnitude,remaining,amount_remaining, file);
        return;
}

void HuffmanEncodeUsingACTable(unsigned char mag, int row, int col, char nameMatrix)
// Translate magnitude into needed data (from table) and send it
{
        unsigned short int huffmancode, huffmanlengt;
        ConvertACMagnitudeY(mag, &huffmancode, &huffmanlengt);
        WriteRawBits16(huffmanlengt,huffmancode, row, col, nameMatrix);
        return;
}


char EncodeDataUnit(char dataunit[64], unsigned int color, int row, int col, char nameMatrix, int sample)
{
	char difference;
	char last_dc_value;

	unsigned char magnitude,zerorun,ii,ert;
	unsigned int bits;
	unsigned char bit_char;

	 //init
  //    PrintMatrix(dataunit) ;
  	last_dc_value = dcvalue[color];
	difference = dataunit[0] - last_dc_value;
	last_dc_value=dataunit[0];
	ReverseExtend(difference, &magnitude,&bit_char);
	bits = bit_char;
	HuffmanEncodeUsingDCTable(magnitude, row, col, nameMatrix);


	WriteRawBits16(magnitude,bits, row, col, nameMatrix);
	zerorun=0;
	ii=1;

	while ( ii < 64 )
	{
		if (dataunit[ii] != 0 )
		{
			while ( zerorun >= 16 )
			{
				HuffmanEncodeUsingACTable(0xF0, row, col, nameMatrix);
				zerorun=zerorun-16;
			}

			ReverseExtend(dataunit[ii],&magnitude,&bit_char);

			bits=bit_char;
			ert= ((int)zerorun *16);                                     //ERROR !!!!!!!!!!!
			ert=ert + magnitude;

            HuffmanEncodeUsingACTable(ert, row, col, nameMatrix);
			WriteRawBits16(magnitude,bits, row, col, nameMatrix);
                        zerorun=0;
		}
		else zerorun=zerorun+1;
                ii++;
	}
	if ( zerorun != 0 )
        {
                HuffmanEncodeUsingACTable(0x00, row, col, nameMatrix);
        }
 	dcvalue[color] = last_dc_value;
        return 0;
}

