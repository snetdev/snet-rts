#ifndef FILTER_HEADER
#define FILTER_HEADER

#include "snettypes.h"
#include "handle.h"
#include "stream.h"

extern snet_filter_instruction_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...);
extern void SNetDestroyFilterInstruction( snet_filter_instruction_t *instr);
extern snet_filter_instruction_set_t *SNetCreateFilterInstructionSet( int num, ...);
extern int SNetFilterGetNumInstructions( snet_filter_instruction_set_t *set);
extern snet_filter_instruction_set_list_t *SNetCreateFilterInstructionSetList( int num, ...);

extern void SNetDestroyFilterInstructionSetList( snet_filter_instruction_set_list_t *list);

extern stream_t *SNetFilter( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guards, ... );


extern stream_t *SNetTranslate( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guards, ... );

extern stream_t *SNetNameShift( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */
    int offset,
    snet_variantencoding_t *untouched);

#endif
