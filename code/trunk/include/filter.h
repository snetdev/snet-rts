#ifndef _FILTER_H_
#define _FILTER_H_

typedef enum filter_opcode snet_filter_opcode_t;
typedef struct filter_instr snet_filter_instr_t;

#include "expression.h"

enum filter_opcode {
  snet_tag,
  snet_btag,
  snet_field,
  create_record
};

struct filter_instr {
  snet_filter_opcode_t opcode;
  int name, newName;
  snet_expr_t *expr;
};

snet_filter_instr_t *SNetCreateFilterInstruction( enum filter_opcode opcode, ...);
void SNetDestroyFilterInstruction( snet_filter_instr_t *instr);
#endif /* _FILTER_H_ */
