#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memfun.h>
#include <crypt.h>

#include <C4SNet.h>
#include "crypto.h"
#include "bool.h"
#include "distribution.h"
#include "fun.h"

typedef struct {
  char *entries;
  int entry_count;
  char *dictionary;
  int dictionary_size;
  int dictionary_word_count;
  int nodes;
#ifndef DISTRIBUTED_SNET
  snet_tl_stream_t *stream;
#endif /* DISTRIBUTED_SNET */
} info_t;

static info_t *init(info_t *info, int argc, char *argv[]) 
{
  FILE *fp;
  int size;
  int i;
  char c;

  if(argc != 4) {
    return NULL;
  }

  /* 1: Dictionary file: */
  if((fp = fopen(argv[1],"r")) == NULL) {
    return NULL;
  }
  
  fseek(fp, 0, SEEK_END);
  info->dictionary_size = ftell(fp) + 2;
  fseek(fp, 0, SEEK_SET);
  
  info->dictionary = SNetMemAlloc(sizeof(char) * info->dictionary_size);
  
  i = 0;
  
  info->dictionary_word_count = 0;

  for(i = 0;(c = fgetc(fp)) != EOF; i++) {
    if(c == '\n') {
      info->dictionary[i] = '\0';
      info->dictionary_word_count++;
    } else {
      info->dictionary[i] = c;
    }
  }

  info->dictionary[i] = '\0';

  fclose(fp);

  /* 2: Entry file: */
  if((fp = fopen(argv[2],"r")) == NULL) {
    return NULL;
  }
  
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  
  info->entries = SNetMemAlloc(sizeof(char) * (size + 1));
  
  i = 0;
  
  info->entry_count = 0;

  for(i = 0;(c = fgetc(fp)) != EOF; i++) {
    if(c == '\n') {
      info->entry_count++;
    }

    info->entries[i] = c;   
  }

  info->entries[i] = '\0';
  
  fclose(fp);
 
  /* 5 Number of nodes: */
  info->nodes = atoi(argv[3]);

  return info; 
}


static void *InputThread(void *ptr) 
{
  info_t *info;

  snet_record_t *rec;
  snet_tl_stream_t *start_stream;

  c4snet_data_t *dictionary;
  c4snet_data_t *entries;

  info = (info_t *)ptr;

#ifdef DISTRIBUTED_SNET
  start_stream = DistributionWaitForInput();
#else
  start_stream = info->stream;
#endif /* DISTRIBUTED_SNET */

  if(start_stream != NULL) {
    rec = SNetRecCreate( REC_data,
	     SNetTencVariantEncode( 
	      SNetTencCreateVector( 2, F__crypto__dictionary, F__crypto__entries),
	      SNetTencCreateVector( 3, T__crypto__dictionary_size, T__crypto__num_entries, T__crypto__num_nodes), 
              SNetTencCreateVector( 0)));
    


    dictionary = C4SNetDataCreateArray( CTYPE_char, info->dictionary_size, info->dictionary);
    entries = C4SNetDataCreateArray( CTYPE_char, strlen(info->entries) + 1, info->entries);

    SNetRecSetInterfaceId( rec, 0);
    SNetRecSetField( rec, F__crypto__dictionary, dictionary);
    SNetRecSetField( rec, F__crypto__entries, entries);
    SNetRecSetTag( rec, T__crypto__dictionary_size, info->dictionary_word_count);
    SNetRecSetTag( rec, T__crypto__num_entries, info->entry_count);
    SNetRecSetTag( rec, T__crypto__num_nodes, info->nodes);

    SNetTlWrite(start_stream, rec);

    SNetTlWrite(start_stream, SNetRecCreate( REC_terminate));

    SNetTlMarkObsolete(start_stream); 
  }

  return NULL;
}

static void *OutputThread(void *ptr) 
{
  snet_record_t *resrec;
  snet_tl_stream_t *res_stream;
  bool terminate;
  char *password;
  int entry;

#ifdef DISTRIBUTED_SNET
  res_stream = DistributionWaitForOutput();
#else
  res_stream = (snet_tl_stream_t *)ptr;
#endif /* DISTRIBUTED_SNET */

  if(res_stream != NULL) {
    terminate = false;
    while(!terminate) {
      resrec = NULL;
      resrec = SNetTlRead(res_stream);
      if( SNetRecGetDescriptor(resrec) == REC_data) {
	entry = SNetRecGetTag(resrec, T__crypto__entry);
	if(SNetRecHasTag(resrec, T__crypto__false)) {
	  printf("Entry %d could not be cracked\n", entry);
	} else {
	  password = C4SNetDataGetData(SNetRecGetField(resrec, F__crypto__word));
	  printf("Entry %d was cracked. Matching word is \"%s\"\n", entry, password);
	}
      }
      else {
	if(SNetRecGetDescriptor( resrec) == REC_terminate) {
	  printf("terminated\n\n");
	  terminate = true;
	}
      }
      SNetRecDestroy( resrec);
    }
  }

  return NULL;
}

static void printUsage() 
{
  printf("USAGE: crypto <path to dictionary> <path to password file> <number of nodes>\n\n");  
}

int main(int argc, char *argv[]) 
{
#ifdef DISTRIBUTED_SNET
  int node;
#else
  snet_tl_stream_t **start_stream;
#endif /* DISTRIBUTED_SNET */

  snet_tl_stream_t *res_stream;
  pthread_t ithread;
  info_t info;

  SNetGlobalInitialise();

  C4SNetInit( 0); 

#ifdef DISTRIBUTED_SNET
  SNetDistFunRegisterLibrary("crypto", 
			     SNetFun2ID_crypto,
			     SNetID2Fun_crypto);
 
  node = DistributionInit(argc, argv);

  if(node == 0) {    
    if(init(&info, argc, argv) == NULL) {
      printUsage();

      DistributionStop();

      DistributionDestroy();

      SNetGlobalDestroy();
      
      return 1;
    }
    
    DistributionStart(SNet__crypto___crypto);
    
    pthread_create(&ithread, NULL, InputThread, &info);
    pthread_detach(ithread);
    
  }
  
  OutputThread(res_stream);

  if(node == 0) {
    DistributionStop();
  }

  DistributionDestroy();

#else
 
  if(init(&info, argc, argv) != NULL) {
    start_stream = SNetTlCreateStream(10);
    
    res_stream = SNet__crypto___crypto(start_stream);
    
    pthread_create(&ithread, NULL, InputThread, start_stream);
    pthread_detach(ithread);
    
    OutputThread(res_stream);
  } else {
    printUsage();

    SNetGlobalDestroy();

    return 1;
  }
#endif /* DISTRIBUTED_SNET */

  SNetGlobalDestroy();

  return 0;
}

