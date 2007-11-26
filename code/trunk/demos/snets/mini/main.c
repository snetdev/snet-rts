#include <stdio.h>

#include <snetentities.h>

#include <sacinterface.h>
#include <mini.h>
#include <SAC2SNet.h>

#include <cwrapper.h>

int main()
{

  int i;
  int input[] = {1,2,3,4,5,6};
  int *output;

  SACarg *in, *out;
  snet_record_t *rec;
  snet_buffer_t *inbuf, *outbuf;

  printf("\n\nInput: ");
  for( i=0; i<5; i++) {
    printf("%d ", input[i]);
  }
  printf("\n\n");

  in = SACARGconvertFromIntPointer( input, 1, 6);

  rec = SNetRecCreate( REC_data, 
                       SNetTencVariantEncode(
                         SNetTencCreateVector( 1, 1),
                         SNetTencCreateVector( 0),
                         SNetTencCreateVector( 0)));
  SNetRecSetField( rec, 1, in);
  SNetRecSetInterfaceId( rec, 0);

  
  SNetGlobalInitialise();
  SAC2SNet_init( 0, SACTYPE_SNet_SNet); 
  
  
  inbuf = SNetBufCreate( 10);

  SNetBufPut( inbuf, rec);

  outbuf = SNet__mini( inbuf);



  while( true) {
  
    rec = SNetBufGet( outbuf);
    out = SNetRecTakeField( rec, 2);
    output = SACARGconvertToIntArray( out);

    printf("\n\nOutput: ");
    for( i=0; i<5; i++) {
      printf("%d ", output[i]);
    }
    printf("\n\n");
  }

  return( 0);
}
