#ifndef FILTER_HEADER
#define FILTER_HEADER

#include "handle.h"

extern snet_filter_instruction_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...);
extern void SNetDestroyFilterInstruction( snet_filter_instruction_t *instr);
extern snet_filter_instruction_set_t *SNetCreateFilterInstructionSet( int num, ...);
extern int SNetFilterGetNumInstructions( snet_filter_instruction_set_t *set);
extern snet_filter_instruction_set_list_t *SNetCreateFilterInstructionSetList( int num, ...);

extern void SNetDestroyFilterInstructionSetList( snet_filter_instruction_set_list_t *list);

#ifdef FILTER_VERSION_2
extern snet_buffer_t
*SNetFilter( snet_buffer_t *inbuf,
             snet_typeencoding_t *in_type,
             snet_expr_list_t *guards, ... );


extern snet_buffer_t
*SNetTranslate( snet_buffer_t *inbuf,
                snet_typeencoding_t *in_type,
                snet_expr_list_t *guards, ... );
#else 

extern snet_buffer_t *SNetFilter( snet_buffer_t *inbuf,
                                  snet_typeencoding_t *in_type,
                                  snet_typeencoding_list_t *out_types,
                                  snet_expr_list_t *guards,  ...);

extern snet_buffer_t *SNetTranslate( snet_buffer_t *inbuf,
                                     snet_typeencoding_t *in_type,
                                     snet_typeencoding_t *out_type, ...);
#endif

extern snet_buffer_t 
*SNetNameShift( snet_buffer_t *inbuf,
                int offset,
                snet_variantencoding_t *untouched);

#endif
