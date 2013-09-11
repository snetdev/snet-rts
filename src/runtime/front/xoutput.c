#include "node.h"
#include "interface_functions.h"


/* This function prints records to stdout */
static void printRec(snet_record_t *rec, output_arg_t *hnd)
{
  snet_ref_t *field;
  int name, val;
  char *label = NULL;
  char *interface = NULL;
  snet_record_mode_t mode;

  if (++hnd->num_outputs == 1) {
    fprintf(hnd->file, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n\n");
  }

  switch (REC_DESCR(rec)) {
    case REC_data:
      mode = SNetRecGetDataMode(rec);
      if (mode == MODE_textual) {
        fprintf(hnd->file, "<record xmlns=\"snet-home.org\" type=\"data\" mode=\"textual\" >\n");
      } else {
        fprintf(hnd->file, "<record xmlns=\"snet-home.org\" type=\"data\" mode=\"binary\" >\n");
      }

      /* Fields */
      RECORD_FOR_EACH_FIELD(rec, name, field) {
        int id = SNetRecGetInterfaceId(rec);

        if ((label = SNetInIdToLabel(hnd->labels, name)) != NULL){
          if ((interface = SNetInIdToInterface(hnd->interfaces, id)) != NULL) {
            fprintf(hnd->file, "<field label=\"%s\" interface=\"%s\">", label,
                    interface);

            if (mode == MODE_textual) {
              SNetInterfaceGet(id)->serialisefun(hnd->file,
                                                 SNetRefGetData(field));
            } else {
              SNetInterfaceGet(id)->encodefun(hnd->file,
                                              SNetRefGetData(field));
            }

            fprintf(hnd->file, "</field>\n");
            SNetMemFree(interface);
          }

          SNetMemFree(label);
        } else{
          SNetUtilDebugFatal("Unknown field %d at output!", name);
        }
      }

       /* Tags */
      int unknown_tag = 0;
      RECORD_FOR_EACH_TAG(rec, name, val) {
        if ((label = SNetInIdToLabel(hnd->labels, name)) != NULL) {
          fprintf(hnd->file, "<tag label=\"%s\">%d</tag>\n", label, val);
        } else{
          unknown_tag = name;
        }

        SNetMemFree(label);
      }
      if (unknown_tag) {
          SNetUtilDebugFatal("Unknown tag %d at output!", unknown_tag);
      }

      /* BTags */
      RECORD_FOR_EACH_BTAG(rec, name, val) {
        if ((label = SNetInIdToLabel(hnd->labels, name)) != NULL){
          fprintf(hnd->file, "<btag label=\"%s\">%d</btag>\n", label, val);
        } else{
          SNetUtilDebugFatal("Unknown binding tag %d at output!", name);
        }

        SNetMemFree(label);
      }

      fprintf(hnd->file, "</record>\n");
      break;

    case REC_terminate:
      fprintf(hnd->file, "<record type=\"terminate\" />\n");
      if (SNetDistribIsRootNode()) SNetDistribGlobalStop();
      break;

    default:
      SNetRecUnknown(__func__, rec);
  }

  fprintf(hnd->file, "\n");
  fflush(hnd->file);
}

/* Output a record to stdout */
void SNetNodeOutput(snet_stream_desc_t *desc, snet_record_t *rec)
{
  output_arg_t  *out = DESC_NODE_SPEC(desc, output);

  trace(__func__);
  switch (REC_DESCR(rec)) {
    case REC_data:
      printRec(rec, out);
      SNetRecDestroy(rec);
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknown(__func__, rec);
  }
}

/* Terminate an output landing. */
void SNetTermOutput(landing_t *land, fifo_t *fifo)
{
  snet_record_t *rec = SNetRecCreate(REC_terminate);

  trace(__func__);
  printRec(rec, NODE_SPEC(land->node, output));
  NODE_SPEC(land->node, output)->terminated = true;
  SNetRecDestroy(rec);
  SNetFreeLanding(land);
}

/* Destroy an output node. */
void SNetStopOutput(node_t *node, fifo_t *fifo)
{
  trace(__func__);
  SNetDelete(node);
}

/* Create the node which prints records to stdout */
void SNetInOutputInit(FILE *file,
                      snetin_label_t *labels,
                      snetin_interface_t *interfaces,
                      snet_stream_t *input)
{
  node_t   *node;
  output_arg_t *out;

  trace(__func__);
  node = SNetNodeNew(NODE_output, ROOT_LOCATION, &input, 1, NULL, 0,
                     SNetNodeOutput, SNetStopOutput, SNetTermOutput);
  out = NODE_SPEC(node, output);
  out->file = file;
  out->labels = labels;
  out->interfaces = interfaces;
  out->num_outputs = 0;
  out->terminated = false;
}

