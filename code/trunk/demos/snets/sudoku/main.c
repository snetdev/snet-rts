#include <stdio.h>
#include <stdlib.h>

#include <sacinterface.h>
#include <snetentities.h>
#include <SAC2SNet.h>

#include <sudokusolve.h>

extern void FibreScanIntArray( int** destarray, int **no_use,
                        FILE *stream, int dim, int *shp);



#define SIZE 9

int main()
{
  snet_buffer_t *inbuf, *outbuf;
  snet_record_t *rec1, *resrec;
  snet_variantencoding_t *type1;
  SACarg *input, *output;
  int *dim, savedim, *resarray1;
  int *board, *shape, *useless; 
  int i,j;

  /* Build SACarg */
  dim = malloc( sizeof( int));
  *dim = 9;
  savedim = *dim;
  
  board = malloc( (SIZE*SIZE) * sizeof( int));
  useless = malloc( (SIZE*SIZE) * sizeof( int));
  shape = malloc( 2*sizeof( int));

  shape[0] = SIZE;
  shape[1] = SIZE;

  for( i=0; i<SIZE; i++) {
    for( j=0; j<SIZE; j++) {
      board[(i*SIZE)+j] = 0;
    }
  }
  
  FibreScanIntArray( &board, &useless, stdin, 2, shape);
  
  printf("\nInitial Board:\n\n");
  for( i=0; i<SIZE; i++) {
    for( j=0; j<SIZE; j++) {
      printf(" %d ", board[(i*SIZE)+j]);
    }
    printf("\n");
  }
  printf("\n");

  input = SACARGconvertFromIntPointer( board, 2, SIZE, SIZE);

  /* Build record */
  type1 = SNetTencVariantEncode( 
            SNetTencCreateVector(1 , F__sudokusolve__board),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0));

  rec1 = SNetRecCreate( REC_data, type1);
  SNetRecSetField( rec1, F__sudokusolve__board, input);
  SNetRecSetInterfaceId( rec1, 0);

  /* Prepare buffer */
  inbuf = SNetBufCreate( 10);
  
  SNetBufPut( inbuf, rec1);

  /* Invoke network */
  SNetGlobalInitialise();
  SAC2SNet_init( 0, 0);
  outbuf =  SNet__sudokusolve___sudokusolve( inbuf);

  /* Inspect results */
  printf("\n\n >>> Wait for resulting record <<< \n\n\n");

  resrec = SNetBufGet( outbuf);

  output = SNetRecGetField( resrec,  F__sudokusolve__board);
  resarray1 = SACARGconvertToIntArray( output);


  printf("\n");
  for( i=0; i<SIZE; i++) {
    for( j=0; j<SIZE; j++) {
      printf(" %d ", resarray1[(i*SIZE)+j]);
    }
    printf("\n");
  }


  return( 0);
}
