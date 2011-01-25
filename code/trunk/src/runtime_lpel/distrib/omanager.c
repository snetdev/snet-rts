#ifdef DISTRIBUTED_SNET
/** <!--********************************************************************-->
 * $Id: omanager.c 2892 2010-10-28 10:20:51Z dlp $
 *
 * @file omanager.c
 *
 * @brief    Manages outgoing connection of a node of distributed S-Net
 *
 *           One output thread per connection is created.
 *
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 13.1.2009
 *
 *****************************************************************************/

#include <pthread.h>
#include <mpi.h>
#include <stdio.h>
#include "unistd.h"

#include "omanager.h"
#include "record_p.h"
#include "memfun.h"
#include "message.h"
#include "debug.h"

#include "lpelif.h"

/** <!--********************************************************************-->
 *
 * @name OManager
 *
 * <!--
 * static void OManagerTask( lpel_task_t *self, void *ptr) : Main loop of an output thread.
 * void SNetOManagerUpdateRoutingTable(snet_stream_t *stream, int node, int index) : Add a new output thread
 * -->
 *
 *****************************************************************************/
/*@{*/


#define ORIGINAL_MAX_MSG_SIZE 256

/** @struct omanager_data_t
 *
 *   @brief Output thread's private data
 *
 */

typedef struct {
  lpel_stream_t *stream;  /**< Input stream. */
  int node;               /**< Node to send the records. */
  int index;              /**< Stream index. */
} omanager_data_t;

/** <!--********************************************************************-->
 *
 * @fn  static void OManagerOutputTask( lpel_task_t *self, void *ptr)
 *
 *   @brief  Main loop of an output thread
 *
 *           Fetches, serializes and sends records to a node.
 *
 *   @param ptr  Pointer to output thread's private data.
 *
 *   @return NULL.
 *
 ******************************************************************************/

static void OManagerOutputTask( lpel_task_t *self, void *ptr)
{
  bool terminate = false;

  omanager_data_t *data = (omanager_data_t *)ptr;

  snet_record_t *record;

  int position;
  int *buf;
  int buf_size;

  lpel_stream_desc_t *instream = LpelStreamOpen( self, data->stream, 'r');

  buf_size = ORIGINAL_MAX_MSG_SIZE;
  buf = SNetMemAlloc(sizeof(char) * buf_size);

  

  while(!terminate) {

    record = LpelStreamRead( instream);

    switch(SNetRecGetDescriptor(record)) {
    case REC_sync:

      data->stream = (lpel_stream_t*) SNetRecGetStream(record);

      SNetRecDestroy(record);
      break;
    case REC_data:
    case REC_collect:
    case REC_sort_end:
      position = 0;
     
      while(SNetRecPack(record, MPI_COMM_WORLD, &position, buf, buf_size) != MPI_SUCCESS) {
	
	SNetMemFree(buf);
	
	buf_size += ORIGINAL_MAX_MSG_SIZE;
	
	buf = SNetMemAlloc(sizeof(char) * buf_size);
	
	position = 0;

      }
      
      MPI_Send(buf, position, MPI_PACKED, data->node, data->index, MPI_COMM_WORLD);
     
      SNetRecDestroy(record);
      break;
    case REC_terminate:
      terminate = true;

      /* No need to check for message size, as buf_size will always be greater! */

      position = 0;

      SNetRecPack(record, MPI_COMM_WORLD, &position, buf, buf_size);

      MPI_Send(buf, position, MPI_PACKED, data->node, data->index, MPI_COMM_WORLD);

      SNetRecDestroy(record);
      break;
    default:
      SNetUtilDebugFatal("OManager: Unknown record received!");
      break;
    }


  }

  LpelStreamClose( instream, true);

  SNetMemFree(buf);
  SNetMemFree(data);
}

/** <!--********************************************************************-->
 *
 * @fn  void SNetOManagerUpdateRoutingTable(snet_stream_t *stream, int node, int index)
 *
 *   @brief  Creates a new output thread to serialize and send records to other node.
 *
 *
 *   @param stream  Stream from which to get records 
 *   @param node    Node where to send the records
 *   @param index   Stream index to use
 *
 ******************************************************************************/
void SNetOManagerUpdateRoutingTable(snet_stream_t *stream, int node, int index)
{
  omanager_data_t *data;
  char name[20];

#ifdef DISTRIBUTED_DEBUG
  int my_rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  SNetUtilDebugNotice("Output to %d:%d added", node , index);
#endif /* DISTRIBUTED_DEBUG */

  data = SNetMemAlloc(sizeof(omanager_data_t));

  data->stream = (lpel_stream_t*) stream;
  data->node = node;
  data->index = index;
  
  
  /* create a detached wrapper thread */
  (void) snprintf(name, 20, "output_n%02d_i%02d", node, index);
  SNetLpelIfSpawnWrapper( OManagerOutputTask, (void*)data, name);
  
  return;
}

/*@}*/

#endif /* DISTRIBUTED_SNET */
