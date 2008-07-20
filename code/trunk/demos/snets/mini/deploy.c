#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "buffer.h"
#include "sacinterface.h"
#include "snetentities.h"
#include "memfun.h"
#include "SAC4SNet.h"

#include "mini.h"
#include "cwrapper.h"


#define MAX_PT_LEN 65536 
#define VSIZE 4





void *Feeder( void *buf)
{ 
  int i;
  snet_buffer_t *inbuf = (snet_buffer_t*)buf;
  snet_record_t *rec1;
  snet_variantencoding_t *type1;
  int *int_array;

  int_array = malloc( VSIZE * sizeof(int));
  for( i=0; i<VSIZE; i++) {
    int_array[i] = i;
  }

  type1 = SNetTencVariantEncode( 
           SNetTencCreateVector( 1, F__mini__A),
           SNetTencCreateVector( 0),
           SNetTencCreateVector( 0));

   rec1 = SNetRecCreate( REC_data, type1);
   SNetRecSetField( rec1, F__mini__A, SACARGconvertFromIntPointer( int_array, 1, VSIZE));
   SNetRecSetInterfaceId( rec1, 0);
  
    SNetBufPut( inbuf, rec1);
    fprintf(stderr, "\n");
  
    
    SNetBufPut( inbuf, SNetRecCreate( REC_terminate));
 return( NULL);
}


void *Reader( void *buf)
{
  bool terminate;
  int i, shape, *cbit_array;
  SACarg *output;
  snet_buffer_t *outbuf = (snet_buffer_t*)buf;
  snet_record_t *resrec;


  terminate = false;
  while( !terminate) {
    resrec = SNetBufGet( outbuf);
    if( SNetRecGetDescriptor( resrec) == REC_terminate) {
      terminate = true;
    }
    else {
      output = SNetRecTakeField( resrec,  F__mini__A);
  
      shape = SACARGgetShape( output, 0);
      cbit_array = SACARGconvertToIntArray( SACARGnewReference( output));
      fprintf(stderr, "Output:\n");
      for( i=0; i<shape; i++) {
        fprintf(stderr, "%d ", cbit_array[i]);
      }
      fprintf(stderr, "\n");
    }
    SNetRecDestroy( resrec);
  }
  fprintf(stdout, "\n");
  return( NULL);
}


int main(int argc, char **argv) 
{
  snet_buffer_t *inbuf, *outbuf;
  pthread_t feeder, reader;

  /* Prepare buffer */
  inbuf = SNetBufCreate( 10);

  /* Invoke network */
  SNetGlobalInitialise();
  SAC4SNetInit( 0);
  outbuf =  SNet__mini___mini( inbuf);

  
  /* Build record(s)  and feed to network */
  pthread_create( &feeder, NULL, Feeder, inbuf);
  pthread_create( &reader, NULL, Reader, outbuf);


  pthread_join( feeder, NULL);
  pthread_join( reader, NULL);

  
  return( 0);
}
