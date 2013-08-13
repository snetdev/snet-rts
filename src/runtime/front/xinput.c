#include <stdio.h>
#include <string.h>
#include "node.h"
#define _THREADING_H_
#include "parserutils.h"

/* Read one record from the parser. */
input_state_t SNetGetNextInputRecord(landing_t *land)
{
  landing_input_t       *linp = LAND_SPEC(land, input);
  input_arg_t           *iarg = NODE_SPEC(land->node, input);
  snet_record_t         *record = NULL;

  trace(__func__);
  assert(NODE_TYPE(land->node) == NODE_input);

  if (iarg->state == INPUT_reading) {
    // Ask parser for a new input record.
    while (SNetInParserGetNextRecord(&record) == SNET_PARSE_CONTINUE) {
      if (record) {
        break;
      }
    }
    if (record) {
      if (REC_DESCR(record) == REC_terminate) {
        SNetRecDestroy(record);
        iarg->state = INPUT_terminating;
      } else {
        linp->num_inputs += 1;
        SNetWrite(&linp->outdesc, record, false);
        return INPUT_reading;
      }
    } else {
      iarg->state = INPUT_terminating;
    }
    return iarg->state;
  } else {
    return INPUT_terminated;
  }
}

void SNetCloseInput(node_t* node)
{
  input_arg_t           *iarg = NODE_SPEC(node, input);
  landing_input_t       *linp = DESC_LAND_SPEC(iarg->indesc, input);

  trace(__func__);
  assert(iarg->state == INPUT_terminating);
  iarg->state = INPUT_terminated;
  SNetDescDone(linp->outdesc);
  SNetInParserDestroy();
}

/* A dummy terminate function which is never called. */
void SNetTermInput(landing_t *land, fifo_t *fifo)
{
  trace(__func__);
  assert(0);
}

/* Deallocate input node */
void SNetStopInput(node_t *node, fifo_t *fifo)
{
  input_arg_t   *iarg = NODE_SPEC(node, input);

  trace(__func__);
  iarg->indesc->landing->refs = 0;
  SNetFreeLanding(iarg->indesc->landing);
  iarg->indesc->landing = NULL;
  SNetStreamDestroy(iarg->indesc->stream);
  iarg->indesc->stream = NULL;
  SNetDelete(iarg->indesc);
  SNetStopStream(iarg->output, fifo);
  SNetDelete(node);
}

/* A dummy input function for input node which is never called. */
void SNetNodeInput(snet_stream_desc_t *desc, snet_record_t *rec)
{
  trace(__func__);
  assert(0);
}

/* Create the node which reads records from an input file via the parser.
 * This is called from networkinterface.c to initialize the input module.
 */
void SNetInInputInit(
    FILE                *file,
    snetin_label_t      *labels,
    snetin_interface_t  *interfaces,
    snet_stream_t       *output)
{
  input_arg_t           *iarg;
  node_t                *node;
  landing_input_t       *linp;
  const int              first_worker_id = 1;
  
  trace(__func__);

  /* Create input node in the fixed network. */
  node = SNetNodeNew(NODE_input, ROOT_LOCATION, NULL, 0, &output, 1,
                     SNetNodeInput, SNetStopInput, SNetTermInput);
  iarg = NODE_SPEC(node, input);
  iarg->output = output;
  iarg->state = INPUT_reading;

  /* Create an empty input descriptor: needed for SNetStreamOpen. */
  iarg->indesc = SNetNewAlign(snet_stream_desc_t);
  memset(iarg->indesc, 0, sizeof(snet_stream_desc_t));
  iarg->indesc->stream = SNetStreamCreate(0);
  iarg->indesc->stream->dest = node;

  /* Create landing: needed for locking the input node before use. */
  iarg->indesc->landing = SNetNewLanding(node, NULL, LAND_input);
  linp = DESC_LAND_SPEC(iarg->indesc, input);
  linp->num_inputs = 0;

  /* Create output descriptor: needed by parser for writing. */
  iarg->indesc->landing->id = first_worker_id;
  linp->outdesc = SNetStreamOpen(output, iarg->indesc);
  iarg->indesc->landing->id = 0;

  /* Initialize the parser */
  SNetInParserInit(file, labels, interfaces, NULL, NULL);
}

