#ifndef _FILTER_H_
#define _FILTER_H_

typedef struct filter_instr snet_filter_instr_t;


typedef enum {
  snet_tag,
  snet_btag,
  snet_field,
  create_record
} snet_filter_opcode_t;


snet_filter_instr_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...);
void SNetDestroyFilterInstruction( snet_filter_instr_t *instr);
#endif /* _FILTER_H_ */
