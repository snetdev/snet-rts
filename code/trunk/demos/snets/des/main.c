#include <stdio.h>
#include <stdlib.h>

#include <sacinterface.h>
#include <snetentities.h>
#include <SAC2SNet.h>

#include <cwrapper.h>
#include <des.h>



int main() 
{
  snet_buffer_t *inbuf, *outbuf;
  snet_record_t *rec1, *resrec;
  snet_variantencoding_t *type1;
  SACarg *in_text, *in_key, *output;
  int *key, *resarray1;
  int *atoi_text;
  int i, shape, dim;

  /* Build SACarg */

  // Key
  key = malloc( 16 * sizeof( int));
  for( i=0; i<16; i++) {
    key[i] = i;
  }
  desboxes__genKey1( &in_key,SACARGconvertFromIntPointer( key, 1, 16)); // SAC function
  
  // Text
  atoi_text = malloc( 8 * sizeof( int));
  for( i=0; i<8; i++) {
//    atoi_text[i] = getc( stdin);
    atoi_text[i] = i+128;
  }
  desboxes__ints2bitBlocks1( &in_text, SACARGconvertFromIntPointer( atoi_text, 1,8));
  printf("\nInput:\n");

  
  /* Build record */
  type1 = SNetTencVariantEncode( 
            SNetTencCreateVector(2 , F__des__Pt, F__des__Key),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0));

  rec1 = SNetRecCreate( REC_data, type1);
  SNetRecSetField( rec1, F__des__Pt, in_text);
  SNetRecSetField( rec1, F__des__Key, in_key);
  SNetRecSetInterfaceId( rec1, 0);

  /* Prepare buffer */
  inbuf = SNetBufCreate( 10);
  
  SNetBufPut( inbuf, rec1);

  /* Invoke network */
  SNetGlobalInitialise();
  SAC2SNet_init( 0);
  outbuf =  SNet__des___des( inbuf);

  /* Inspect results */
  printf("\n\n >>> Wait for resulting record <<< \n\n\n");
  resrec = SNetBufGet( outbuf);
  
  output = SNetRecGetField( resrec,  F__des__Ct);
  dim = SACARGgetDim( output);
  printf("\nDim of result: %d\n", dim);
  shape = SACARGgetShape( output, 0);
  printf("Shape[0] of result: %d\n\n", shape);

  resarray1 = SACARGconvertToIntArray( output);
  
  for( i=0; i<shape; i++) {
    printf("%d ", resarray1[i]);
  }
  printf("\n\n");
  return( 0);
}
