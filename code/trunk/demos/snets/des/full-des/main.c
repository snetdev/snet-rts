#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sacinterface.h>
#include <snetentities.h>
#include <SAC2SNet.h>

#include <cwrapper.h>
#include <des.h>

#define MAX_PT_LEN 1024

int main() 
{
  snet_buffer_t *inbuf, *outbuf;
  snet_record_t *rec1, *resrec;
  snet_variantencoding_t *type1;
  SACarg *in_bits, *in_key, *in_string, *output, *ct;
  int *key, str_len, *bit_array, *cbit_array;
  char *plaintext, *ciphertext;
  int i, shape, dim;

  /* Build SACarg */

  // Key
  key = malloc( 16 * sizeof( int));
  for( i=0; i<16; i++) {
    key[i] = i;
  }
  desboxes__genKey1( &in_key,SACARGconvertFromIntPointer( key, 1, 16)); 


  // Text
  plaintext = malloc( MAX_PT_LEN * sizeof( char));
  printf("\n\nText to be enciphered: ");
  fgets( plaintext, MAX_PT_LEN, stdin);
  str_len = strlen( plaintext);

  desboxes__to_string1( &in_string, 
                        SACARGconvertFromCharPointer( plaintext, 1, str_len));
  
  desboxes__string2bitBlocks1( &in_bits, in_string);

  bit_array = SACARGconvertToIntArray( SACARGnewReference( in_bits));
  printf("\nInput:\n");
  for( i=0; i<SACARGgetShape( in_bits, 0); i++) {
    printf("%d ", bit_array[i]);
  }



  /* Build record */
  type1 = SNetTencVariantEncode( 
            SNetTencCreateVector(2 , F__des__Pt, F__des__Key),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0));

  rec1 = SNetRecCreate( REC_data, type1);
  SNetRecSetField( rec1, F__des__Pt, in_bits);
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
  printf("\n\nProcessing Data... \n\n");
  resrec = SNetBufGet( outbuf);
  
  output = SNetRecGetField( resrec,  F__des__Ct);
  
  shape = SACARGgetShape( output, 0);
  cbit_array = SACARGconvertToIntArray( SACARGnewReference( output));
  printf("Enciphered Bits:\n");
  for( i=0; i<shape; i++) {
    printf("%d ", cbit_array[i]);
  }
  printf("\n\n");
 

  
  /*
  desboxes__bitBlocks2string1( &ct, output);
  SACARGconvertFromCharPointer( ciphertext, 1, 8);
  printf("%s\n\n", ciphertext);
  */
  return( 0);
}
