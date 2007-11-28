#include <stdio.h>
#include <stdlib.h>

#include <sacinterface.h>
#include <snetentities.h>
#include <SAC2SNet.h>
#include <pthread.h>
#include <sudokusolve.h>
#include <memfun.h>
#include <constants.h>


extern void FibreScanIntArray( int** destarray, int **no_use,
                        FILE *stream, int dim, int *shp);


#define SIZE 9
#define ITERATIONS 500 


typedef struct {
 snet_record_t **in_recs;
 snet_buffer_t *inbuf;
} thread_arg_t;

void *Feed( void *ptr) {
  thread_arg_t *inf = (thread_arg_t*)ptr;
  int i;

  for( i=0; i<ITERATIONS; i++) {
    SNetBufPut( inf->inbuf, inf->in_recs[i]);
  }

  return( NULL);
}

int main()
{
  thread_arg_t *inf;
  pthread_t t;

  snet_buffer_t *inbuf, *outbuf;
  snet_record_t *rec1, *resrec, **in_records;
  snet_variantencoding_t *type1;
  SACarg *input, *output;
  int *dim, savedim, *resarray1;
  int *board, *shape, *useless; 
  int i,j,k;


  inf = SNetMemAlloc( sizeof( thread_arg_t));
  in_records = SNetMemAlloc( ITERATIONS * sizeof( snet_record_t*));

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
  inbuf = SNetBufCreate( BUFFER_SIZE);
  

  /* Invoke network */
  SNetGlobalInitialise();
  SAC2SNet_init( 0, 0);
  outbuf =  SNet__sudokusolve___sudokusolve( inbuf);

  printf("\n\nCreating input records...");
  for( i=0; i<ITERATIONS-1; i++) {
    in_records[i] = SNetRecCopy( rec1);
  }
  in_records[ITERATIONS-1] = rec1;

  inf->inbuf = inbuf;
  inf->in_recs = in_records;

  pthread_create( &t, NULL, Feed, inf);
  pthread_detach( t);

  /* Inspect results */
  printf("\n\n >>> Wait for resulting record <<< \n\n\n");

  for( k=0; k<ITERATIONS; k++) {
    resrec = SNetBufGet( outbuf);
    output = SNetRecTakeField( resrec,  F__sudokusolve__board);
    resarray1 = SACARGconvertToIntArray( output);
    printf("\nResult %d/%d\n", k, ITERATIONS);
    for( i=0; i<SIZE; i++) {
      for( j=0; j<SIZE; j++) {
        printf(" %d ", resarray1[(i*SIZE)+j]);
      }
      printf("\n");
    }
    free( resarray1);
    SNetRecDestroy( resrec);
  }

  return( 0);
}
