#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "buffer.h"
#include <sacinterface.h>
#include <snetentities.h>
#include <memfun.h>
#include <SAC4SNet.h>

#include <simpleDesboxes.h>
#include <simpleDes.h>

#define MAX_PT_LEN 65536 


bool DECIPHER;


void ToString( int *bits) 
{
  int i,j, byte;
  int res;

  byte = 0;
  for( j=0; j<8; j++) {
    res = 0;
    for( i=0; i<8; i++) {
      res += (bits[(byte*8)+i] * pow( 2, 7-i));
    }
    byte += 1;
    printf("%c", res);
  }
}


void *Feeder( void *buf)
{ 
  snet_tl_stream_t *inbuf = (snet_tl_stream_t*)buf;
  snet_record_t *rec1;
  snet_variantencoding_t *type1;
  SACarg *in_key;
  int *key, str_len;
  int i, j;

  key = malloc( 16 * sizeof( int));
  for( i=0; i<16; i++) {
    key[i] = i;
  }
  simpleDesboxes__genKey1( &in_key,SACARGconvertFromIntPointer( key, 1, 16)); 


  // Text
  {
  SACarg *in_bits, *in_string;
  int *current_block, *bit_array;
  char *plaintext;

  plaintext = malloc( MAX_PT_LEN * sizeof( char));
  fprintf(stderr, "\n\nText to be enciphered: ");
  fgets( plaintext, MAX_PT_LEN, stdin);
  str_len = strlen( plaintext);

  simpleDesboxes__to_string1( &in_string, 
                        SACARGconvertFromCharPointer( plaintext, 1, str_len-1));
  
  simpleDesboxes__string2bitBlocks1( &in_bits, in_string);

  bit_array = SACARGconvertToIntArray( SACARGnewReference( in_bits));
  fprintf(stderr, "\nInput:\n");
  for( i=0; i<SACARGgetShape( in_bits, 0); i++) {
    current_block = SNetMemAlloc( 64 * sizeof( int));
    fprintf(stderr, "Block %d:\n", i);
    for( j=0; j<64; j++) {
      current_block[j]= bit_array[(i*64)+j];
      fprintf(stderr, "%d ", current_block[j]);
    }
    if( DECIPHER) {
      type1 = SNetTencVariantEncode( 
              SNetTencCreateVector( 2 , F__simpleDes__Pt, F__simpleDes__Key),
              SNetTencCreateVector( 1, T__simpleDes__Decipher),
              SNetTencCreateVector( 0));
    }
    else {
      type1 = SNetTencVariantEncode( 
              SNetTencCreateVector( 2 , F__simpleDes__Pt, F__simpleDes__Key),
              SNetTencCreateVector( 0),
              SNetTencCreateVector( 0));
    }

   rec1 = SNetRecCreate( REC_data, type1);
   SNetRecSetField( rec1, F__simpleDes__Pt, 
       SACARGconvertFromIntPointer( current_block, 1, 64));
   SNetRecSetField( rec1, F__simpleDes__Key, SACARGnewReference( in_key));
   SNetRecSetInterfaceId( rec1, 0);
  
    SNetTlWrite( inbuf, rec1);
    fprintf(stderr, "\n");
  }
  }
  SNetTlWrite( inbuf, SNetRecCreate( REC_terminate));
 return( NULL);
}


void *Reader( void *buf)
{
  bool terminate;
  int i, j, shape, *cbit_array;
  SACarg *output;
  snet_tl_stream_t *outbuf = (snet_tl_stream_t*)buf;
  snet_record_t *resrec;


  terminate = false; j = 0;
  while( !terminate) {
    resrec = SNetTlRead( outbuf);
    if( SNetRecGetDescriptor( resrec) == REC_terminate) {
      terminate = true;
    }
    else {
      output = SNetRecTakeField( resrec,  F__simpleDes__Ct);
  
      shape = SACARGgetShape( output, 0);
      cbit_array = SACARGconvertToIntArray( SACARGnewReference( output));
      fprintf(stderr, "Enciphered Bits (Block %d):\n", j++);
      for( i=0; i<shape; i++) {
        fprintf(stderr, "%d ", cbit_array[i]);
      }
      fprintf(stderr, "\n");
      ToString( cbit_array);
    }
    SNetRecDestroy( resrec);
  }
  fprintf(stdout, "\n");
  return( NULL);
}


int main(int argc, char **argv) 
{
  snet_tl_stream_t *inbuf, *outbuf;
  pthread_t feeder, reader;

  /* if cmdl argument given, we're in decipher mode */
  DECIPHER = (argc > 1);


  /* Prepare buffer */
  inbuf = SNetTlCreateStream( 10);

  /* Invoke network */
  SNetGlobalInitialise();
  SAC4SNetInit( 0);
  outbuf =  SNet__simpleDes___simpleDes( inbuf);
  
  /* Build record(s)  and feed to network */
  pthread_create( &feeder, NULL, Feeder, inbuf);
  pthread_create( &reader, NULL, Reader, outbuf);

  pthread_join( reader, NULL);
  pthread_join( feeder, NULL);

  SNetTlMarkObsolete( inbuf);
  
  return( 0);
}
