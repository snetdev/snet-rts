/*******************************************************************************
 *
 * $Id: input.c 2864 2010-09-17 11:28:30Z dlp $
 *
 * Author: Daniel Prokesch, Vienna University of Technology
 * -------
 *
 * Date:   27.10.2010
 * -----
 *
 * Description:
 * ------------
 *
 * Input thread for S-NET network interface.
 *
 *******************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "memfun.h"
#include "input.h"
#include "snetentities.h"
#include "label.h"
#include "interface.h"
#include "bool.h"
#include "debug.h"
#include "parser.h"

#include "spawn.h"
#include "stream.h"
#include "task.h"
#include "scheduler.h"

#ifdef DISTRIBUTED_SNET
#include "distribution.h"
#endif /* DISTRIBUTED_SNET */


typedef struct { 
  FILE *file;
  snetin_label_t *labels;
  snetin_interface_t *interfaces;
  stream_t *buffer;
} handle_t;


/**
 * This is the task doing the global input
 */
static void GlobInputTask( task_t *self, void* data)
{
  handle_t *hnd = (handle_t *)data;

#ifdef DISTRIBUTED_SNET
  hnd->buffer = DistributionWaitForInput();
#endif /* DISTRIBUTED_SNET */

  if(hnd->buffer != NULL) {
    int i;
    stream_desc_t *outstream = StreamOpen( self, hnd->buffer, 'w');

    SNetInParserInit( hnd->file, hnd->labels, hnd->interfaces, outstream);
    i = SNET_PARSE_CONTINUE;
    while(i != SNET_PARSE_TERMINATE){
      i = SNetInParserParse();
    }
    SNetInParserDestroy();

    StreamClose( outstream, false);
  }

  SNetMemFree(hnd);
}


void SNetInInputInit(FILE *file,
		     snetin_label_t *labels, 
#ifdef DISTRIBUTED_SNET
		     snetin_interface_t *interfaces
#else /* DISTRIBUTED_SNET */
		     snetin_interface_t *interfaces,
		     stream_t *out_buf
#endif /* DISTRIBUTED_SNET */
  )
{
  handle_t *hnd = SNetMemAlloc(sizeof(handle_t));

  hnd->file = file;
  hnd->labels = labels;
  hnd->interfaces = interfaces;
#ifdef DISTRIBUTED_SNET
  hnd->buffer = NULL;
#else /* DISTRIBUTED_SNET */
  hnd->buffer = out_buf;
#endif /* DISTRIBUTED_SNET */

  /* create a joinable wrapper thread */
  SNetSpawnWrapper( GlobInputTask, (void*)hnd, "glob_input");
}
