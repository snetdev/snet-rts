#ifndef FILTER_HEADER
#define FILTER_HEADER

#include "snettypes.h"
#include "handle.h"
#include "stream_layer.h"

extern snet_filter_instruction_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...);
extern void SNetDestroyFilterInstruction( snet_filter_instruction_t *instr);
extern snet_filter_instruction_set_t *SNetCreateFilterInstructionSet( int num, ...);
extern int SNetFilterGetNumInstructions( snet_filter_instruction_set_t *set);
extern snet_filter_instruction_set_list_t *SNetCreateFilterInstructionSetList( int num, ...);

extern void SNetDestroyFilterInstructionSetList( snet_filter_instruction_set_list_t *list);

extern snet_tl_stream_t *SNetFilter( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
				     snet_info_t *info, 
				     int location,
#endif /* DISTRIBUTED_SNET */
				     snet_typeencoding_t *in_type,
				     snet_expr_list_t *guards, ... );


extern snet_tl_stream_t *SNetTranslate( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
					snet_info_t *info, 
					int location,
#endif /* DISTRIBUTED_SNET */
					snet_typeencoding_t *in_type,
					snet_expr_list_t *guards, ... );

extern snet_tl_stream_t *SNetNameShift( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
					snet_info_t *info, 
					int location,
#endif /* DISTRIBUTED_SNET */
					int offset,
					snet_variantencoding_t *untouched);

#endif
