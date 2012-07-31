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
#include "parserutils.h"

#include "threading.h"

#include "distribution.h"

typedef struct {
  FILE *file;
  snetin_label_t *labels;
  snetin_interface_t *interfaces;
  snet_stream_t *buffer;
} handle_t;


/**
 * This is the task doing the global input
 */
static void GlobInputTask(void* data)
{
  handle_t *hnd = (handle_t *)data;

  if(hnd->buffer != NULL) {
    int i;
    snet_stream_desc_t *outstream = SNetStreamOpen(hnd->buffer, 'w');

    SNetInParserInit( hnd->file, hnd->labels, hnd->interfaces, outstream);
    i = SNET_PARSE_CONTINUE;
    while(i != SNET_PARSE_TERMINATE){
      i = SNetInParserParse();
    }
    SNetInParserDestroy();

    SNetStreamClose( outstream, false);
  }

  SNetMemFree(hnd);
}


void SNetInInputInit(FILE *file,
                     snetin_label_t *labels,
                     snetin_interface_t *interfaces,
                     snet_stream_t *in_buf
  )
{
  handle_t *hnd = SNetMemAlloc(sizeof(handle_t));

  hnd->file = file;
  hnd->labels = labels;
  hnd->interfaces = interfaces;
  hnd->buffer = in_buf;

  SNetThreadingSpawn( ENTITY_other, -1, NULL,
        "glob_input", GlobInputTask, hnd);
}
