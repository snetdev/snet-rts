#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memfun.h>

#include <C4SNet.h>
#include "factorial.h"
#include "bool.h"
#include "distribution.h"
#include "fun.h"

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

void initialise() {

  SNetGlobalInitialise();
  C4SNetInit( 0); 
}

void *InputThread(void *ptr) 
{
  snet_record_t *rec1, *rec2;
  snet_tl_stream_t *start_stream;

  int F_x_1, F_x_3, F_r_1, F_r_3;

  c4snet_data_t *field1, *field2, *field3, *field4;

  int k;

  start_stream = DistributionWaitForInput(); 

  if(start_stream != NULL) {

    rec1 = SNetRecCreate( REC_data,
             SNetTencVariantEncode( 
	      SNetTencCreateVector( 2, F__factorial__x,  F__factorial__r),
	      SNetTencCreateVector( 0), 
              SNetTencCreateVector( 0)));

    rec2 = SNetRecCreate( REC_data, 
	     SNetTencVariantEncode( 
	      SNetTencCreateVector( 2, F__factorial__x,  F__factorial__r),
 	      SNetTencCreateVector( 0), 
              SNetTencCreateVector( 0)));
    
    F_x_1 = 4;
    F_r_1 = 1;

    F_x_3 = 11;
    F_r_3 = 1;
    
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
    

    printf("\nPut record to Buffer:");
    PRINT_RECORD( rec1);
    printf("\n");
    SNetTlWrite( start_stream, rec1);
    
    printf("\nPut record to Buffer:");
    PRINT_RECORD( rec2);
    printf("\n");
    SNetTlWrite( start_stream, rec2);
    
    printf("\n\n\n");
  
    printf("put terminate\n");
    SNetTlWrite(start_stream, SNetRecCreate( REC_terminate));

    SNetTlMarkObsolete(start_stream); 
  }

  return NULL;
}

void *OutputThread(void *ptr) 
{
  snet_record_t *resrec;
  snet_tl_stream_t *res_stream;
  bool terminate;

  int j,k;

  res_stream = DistributionWaitForOutput();

  if(res_stream != NULL) {
    for( j=0, terminate = false; !terminate; j++) {
      resrec = NULL;
      resrec = SNetTlRead( res_stream);
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
  }
 
  return NULL;
}

int main(int argc, char *argv[]) 
{

  snet_tl_stream_t *res_stream;
  pthread_t ithread;
  int node;

  initialise();

  SNetDistFunRegisterLibrary("factorial", 
			     SNetFun2ID_factorial,
			     SNetID2Fun_factorial);


  node = DistributionInit(argc, argv);

  if(node == 0) {
    res_stream = DistributionStart(SNet__factorial___factorial);
  }
   
  pthread_create(&ithread, NULL, InputThread, NULL);
  pthread_detach(ithread);

  OutputThread(NULL);

  if(node == 0) {
    DistributionStop();
  }

  DistributionDestroy();

  SNetGlobalDestroy();

  return( 0);
}
