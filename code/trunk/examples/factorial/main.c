#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memfun.h>

#include <C4SNet.h>
#include "factorial.h"
#include "bool.h"

#define PRINT_RECORD( NAME) __PRINT_RECORD( NAME, false)


#define __PRINT_RECORD( NAME, FREE)\
     printf("\n - %2d fields: ", SNetRecGetNumFields( NAME));\
    for( k=0; k<SNetRecGetNumFields( NAME); k++) {\
      printf(" %s=%d ", snet_factorial_labels[ SNetRecGetFieldNames( NAME)[k] ],\
                        *((int*)C4SNetDataGetData( SNetRecGetField( NAME, SNetRecGetFieldNames( NAME)[k]))));\
    }\
    printf("\n - %2d tags  : ", SNetRecGetNumTags( NAME));\
    for( k=0; k<SNetRecGetNumTags( NAME); k++) {\
      printf(" %s=%d ", snet_factorial_labels[ SNetRecGetTagNames( NAME)[k] ],\
                               SNetRecGetTag( NAME, SNetRecGetTagNames( NAME)[k]));\
    }\
    printf("\n - %2d btags : ", SNetRecGetNumBTags( NAME));\
    for( k=0; k<SNetRecGetNumBTags( NAME); k++) {\
      printf(" %s=%d ", snet_factorial_labels[ SNetRecGetBTagNames( NAME)[k] ],\
                        (int)SNetRecGetBTag( NAME, SNetRecGetBTagNames( NAME)[k]));\
    }
    
char **names;

void initialise() 
{
  SNetGlobalInitialise();
  C4SNetInit( 0); 
}

int main() 
{
  snet_record_t *rec1, *rec2, /* *rec3, *rec4, *rec5, *rec6,*/ *resrec;
  snet_tl_stream_t *start_stream, *res_stream;

  int j,k;

  int F_x_1, F_x_2, F_x_3, F_x_4, F_x_5, F_x_6, F_r_1, F_r_2, F_r_3, F_r_4, F_r_5, F_r_6, F_z_1;

  c4snet_data_t *field1, *field2, *field3, *field4;

  bool terminate;

  initialise();

  rec1 = SNetRecCreate( REC_data,  
                        SNetTencVariantEncode( 
			  SNetTencCreateVector( 2, F__factorial__x, F__factorial__r), 
			  SNetTencCreateVector( 0), 
			  SNetTencCreateVector( 0)));

  rec2 = SNetRecCreate( REC_data,
                        SNetTencVariantEncode( 
                          SNetTencCreateVector( 2, F__factorial__x, F__factorial__r), 
			  SNetTencCreateVector( 0), 
			  SNetTencCreateVector( 0)));

  /*
  rec3 = SNetRecCreate( REC_data, 
			SNetTencVariantEncode( 
                          SNetTencCreateVector( 2, F__factorial__x, F__factorial__r), 
			  SNetTencCreateVector( 0), 
			  SNetTencCreateVector( 0)));

  rec4 = SNetRecCreate( REC_data, 
                        SNetTencVariantEncode( 
                          SNetTencCreateVector( 2, F__factorial__x, F__factorial__r), 
			  SNetTencCreateVector( 0), 
			  SNetTencCreateVector( 0)));

  rec5 = SNetRecCreate( REC_data,
                        SNetTencVariantEncode( 
			  SNetTencCreateVector( 2, F__factorial__x, F__factorial__r), 
			  SNetTencCreateVector( 0), 
			  SNetTencCreateVector( 0)));

  rec6 = SNetRecCreate( REC_data,
                        SNetTencVariantEncode( 
                          SNetTencCreateVector( 2, F__factorial__x, F__factorial__r), 
			  SNetTencCreateVector( 0), 
			  SNetTencCreateVector( 0)));
  */

  F_x_1 = 4; 
  F_r_1 = 1;

  F_x_2 = 2;
  F_r_2 = 1;
  
  F_x_3 = 11;
  F_r_3 = 1;

  F_x_4 = 7;
  F_r_4 = 1;
  
  F_x_5 = 4;
  F_r_5 = 1;
  
  F_x_6 = 10;
  F_r_6 = 1;

  F_z_1 = 42;

  field1 = C4SNetDataCreate( CTYPE_int, &F_x_1);
  field2 = C4SNetDataCreate( CTYPE_int, &F_r_1);
  field3 = C4SNetDataCreate( CTYPE_int, &F_x_3);
  field4 = C4SNetDataCreate( CTYPE_int, &F_r_3);

  SNetRecSetInterfaceId( rec1, 0);

  SNetRecSetField( rec1, F__factorial__x, (void*)field1);
  SNetRecSetField( rec1, F__factorial__r, (void*)field2);

  SNetRecSetInterfaceId( rec2, 0);

  SNetRecSetField( rec2, F__factorial__x, (void*)field3);
  SNetRecSetField( rec2, F__factorial__r, (void*)field4);


  /*  SNetRecSetField( rec2, F_x, (void*)F_x_2);
  SNetRecSetField( rec3, F_x, (void*)F_x_3);
  SNetRecSetField( rec4, F_x, (void*)F_x_4);
  SNetRecSetField( rec5, F_x, (void*)F_x_5);
  SNetRecSetField( rec6, F_x, (void*)F_x_6);

  SNetRecSetField( rec1, F_r, (void*)F_r_1);
  SNetRecSetField( rec2, F_r, (void*)F_r_2);
  SNetRecSetField( rec3, F_r, (void*)F_r_3);
  SNetRecSetField( rec4, F_r, (void*)F_r_4);
  SNetRecSetField( rec5, F_r, (void*)F_r_5);
  SNetRecSetField( rec6, F_r, (void*)F_r_6);


  SNetRecAddTag( rec1, T_s);
  SNetRecSetTag( rec1, T_s, 43);
*/
  start_stream = SNetTlCreateStream( 10);
  //REGISTER_BUFFER( start_buf);
  //SET_BUFFER_NAME( start_buf, "initialBuffer");


  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec1);
  printf("\n");
  SNetTlWrite( start_stream, rec1);

  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec2);
  printf("\n");
  SNetTlWrite( start_stream, rec2);

/*
  SNetTlWrite( start_stream, rec2);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec2);
  printf("\n"); 

  SNetTlWrite(  start_stream, rec3);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec3);
  printf("\n");

  SNetTlWrite(  start_stream, rec4);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec4);
  printf("\n");
  
  SNetTlWrite(  start_stream, rec5);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec5);
  printf("\n");
  
  SNetTlWrite(  start_stream, rec6);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec6);
  printf("\n");
*/
  printf("\n\n\n");

  res_stream = SNet__factorial___factorial( start_stream);


  printf("\n  >>> Wait,  then press a key to send a termination record. <<<\n");
  getc( stdin);
  SNetTlWrite( start_stream, SNetRecCreate( REC_terminate));


  printf("\nPress any key to see resulting record\n");
  getc( stdin);

  for( j=0, terminate = false; !terminate; j++) {
    resrec = NULL;
    resrec = SNetTlRead( res_stream);
    printf("\nRecord %d:", j);
    if( SNetRecGetDescriptor( resrec) == REC_data) {
     PRINT_RECORD( resrec);  
     printf("\n");
    }
    else {
      printf("\n - Control Record, Type: %d\n", SNetRecGetDescriptor( resrec));
      if(SNetRecGetDescriptor( resrec) == REC_terminate) {
	terminate = true;
      }
    }
   SNetRecDestroy( resrec);
  }
  
  SNetTlMarkObsolete( start_stream);
  printf("\n\nPress any key to exit.\n");
  getc( stdin);
  printf("\nEnd.\n"); 

  SNetGlobalDestroy();

  return( 0);
}
