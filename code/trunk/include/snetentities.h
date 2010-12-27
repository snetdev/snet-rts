#ifndef _SNETENTITIES_H_
#define _SNETENTITIES_H_

#include <stdio.h>

#include "snettypes.h"
#include "handle.h"
#include "typeencode.h"
#include "constants.h"
#include "expression.h"
#include "interface_functions.h"




/****************************************************************************/
/* Alias                                                                    */
/****************************************************************************/
snet_stream_t *SNetAlias( snet_stream_t *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_startup_fun_t net);



/****************************************************************************/
/* Box                                                                      */
/****************************************************************************/
snet_stream_t *SNetBox(snet_stream_t *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    const char *boxname,
    snet_box_fun_t fun,
    snet_box_sign_t *sign);


/****************************************************************************/
/* Synchrocell                                                              */
/****************************************************************************/
snet_stream_t *SNetSync( snet_stream_t *input, 
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *outtype,
    snet_typeencoding_t *patterns,
    snet_expr_list_t *guards);


/****************************************************************************/
/* Filter                                                                   */
/****************************************************************************/
snet_stream_t *SNetFilter( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guards, ... );

snet_stream_t *SNetTranslate( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guards, ... );

snet_stream_t *SNetNameShift( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    int offset,
    snet_variantencoding_t *untouched);


/****************************************************************************/
/* Serial                                                                   */
/****************************************************************************/
snet_stream_t *SNetSerial(snet_stream_t *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);


/****************************************************************************/
/* Parallel                                                                 */
/****************************************************************************/
snet_stream_t *SNetParallel( snet_stream_t *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_list_t *types,
    ...);

snet_stream_t *SNetParallelDet( snet_stream_t *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_list_t *types,
    ...);


/****************************************************************************/
/* Star                                                                     */
/****************************************************************************/
snet_stream_t *SNetStar( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

snet_stream_t *SNetStarIncarnate( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

snet_stream_t *SNetStarDet( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

snet_stream_t *SNetStarDetIncarnate( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);


/****************************************************************************/
/* Split                                                                    */
/****************************************************************************/
snet_stream_t *SNetSplit( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, 
    int utag);

snet_stream_t *SNetSplitDet( snet_stream_t *inbuf, 
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag,
    int utag);

#ifdef DISTRIBUTED_SNET
snet_stream_t *SNetLocSplit(snet_stream_t *instream,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag);

snet_stream_t *SNetLocSplitDet(snet_stream_t *instream,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag,
    int utag);
#endif




#endif /* _SNETENTITIES_H_ */
