#ifndef _RECORD_P_H_
#define _RECORD_P_H_

#include "record.h"


enum record_descr {
  REC_data,
  REC_sync,
  REC_collect,
  REC_sort_end,
  REC_terminate,
  REC_trigger_initialiser
};

enum record_mode {
  MODE_textual,
  MODE_binary,
};


#endif /* _RECORD_P_H_ */

