#ifndef _FILTER_H_
#define _FILTER_H_

#include "snettypes.h"

typedef enum filter_opcode {
	snet_tag,
	snet_btag,
	snet_field,
	create_record
} snet_filter_opcode_t;

typedef struct filter_instruction {
  snet_filter_opcode_t opcode;
  int *data;
  snet_expr_t *expr;
} snet_filter_instruction_t;

typedef struct filter_instruction_set {
  int num;
  snet_filter_instruction_t **instructions;
} snet_filter_instruction_set_t;

typedef struct filter_instruction_set_list {
  int num;
  snet_filter_instruction_set_t **lst;
} snet_filter_instruction_set_list_t;


extern struct filter_instruction *SNetCreateFilterInstruction( enum filter_opcode opcode, ...);
extern void SNetDestroyFilterInstruction( struct filter_instruction *instr);
extern struct filter_instruction_set *SNetCreateFilterInstructionSet( int num, ...);
extern int SNetFilterGetNumInstructions( struct filter_instruction_set *set);
extern struct filter_instruction_set_list *SNetCreateFilterInstructionSetList( int num, ...);

extern void SNetDestroyFilterInstructionSetList( struct filter_instruction_set_list *list);


extern snet_filter_instruction_set_list_t *SNetCreateFilterInstructionList( int num, ...);
extern int SNetFilterInstructionsGetNumSets( snet_filter_instruction_set_list_t *lst);
extern snet_filter_instruction_set_t **SNetFilterInstructionsGetSets( snet_filter_instruction_set_list_t *lst);
#endif /* _FILTER_H_ */
