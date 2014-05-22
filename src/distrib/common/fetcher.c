/*
 * fetcher.c
 *
 *  Created on: May 22, 2014
 *      Author: thiennga
 */
#include <assert.h>
#include "threading.h"
#include "memfun.h"
#include "fetcher.h"
#include "bool.h"
#include "record.h"
#include "stream.h"
#include "distribcommon.h"
#include "reference.h"

void SNetFetcherTask(snet_entity_t *ent, void *args)
{
  (void) ent; /* NOT USED */
  fetch_arg_t *farg = (fetch_arg_t *)args;
  snet_stream_desc_t *instream = SNetStreamOpen(farg->input, 'r');
  bool terminate = false;
  snet_record_t *rec;
  snet_ref_t *ref;
  void *dest;

  while(!terminate) {
  	rec = SNetStreamRead(instream);
    switch(SNetRecGetDescriptor(rec)) {
    	case REC_fetch:
    		ref = FETCH_REC( rec, ref);
    		dest = FETCH_REC(rec, dest);
    		SNetDistribSendData(ref, SNetRefGetData(ref), dest);
    		SNetRefUpdate(ref, -1);
    		SNetMemFree(ref);
    		SNetRecDestroy(rec);
    		break;
    	case REC_terminate:
    		terminate = true;
    		SNetRecDestroy(rec);
    		break;
    	default:
    		assert(0);
    }
  }
  SNetStreamClose(instream, 1);
  SNetMemFree(farg);
}

void SNetFetcher(snet_stream_t *input) {
	fetch_arg_t *farg = (fetch_arg_t *) SNetMemAlloc(sizeof(fetch_arg_t));
	farg->input = input;
	SNetThreadingSpawn(
	    SNetEntityCreate( ENTITY_other, -1, NULL,
	    "fetcher", &SNetFetcherTask, (void *) farg));
}

