#ifndef _RECORD_P_H_
#define _RECORD_P_H_

#include "record.h"
#include "stack.h"

enum record_descr {
  REC_data,
  REC_sync,
  REC_collect,
  REC_sort_begin,
  REC_sort_end,
  REC_terminate,
  REC_probe,
  REC_trigger_initialiser
};

enum record_mode {
  MODE_textual,
  MODE_binary,
};

extern snet_util_stack_t *SNetRecGetIterationStack(snet_record_t *rec);
extern void SNetRecCopyIterations(snet_record_t *source, snet_record_t *target);


#endif /* _RECORD_P_H_ */

