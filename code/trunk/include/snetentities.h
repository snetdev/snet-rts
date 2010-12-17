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
struct stream *SNetAlias( struct stream *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_startup_fun_t net);



/****************************************************************************/
/* Box                                                                      */
/****************************************************************************/
struct stream *SNetBox(struct stream *instream,
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
struct stream *SNetSync( struct stream *input, 
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
struct stream *SNetFilter( struct stream *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guards, ... );

struct stream *SNetTranslate( struct stream *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guards, ... );

struct stream *SNetNameShift( struct stream *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    int offset,
    snet_variantencoding_t *untouched);


/****************************************************************************/
/* Serial                                                                   */
/****************************************************************************/
struct stream *SNetSerial(struct stream *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);


/****************************************************************************/
/* Parallel                                                                 */
/****************************************************************************/
struct stream *SNetParallel( struct stream *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_list_t *types,
    ...);

struct stream *SNetParallelDet( struct stream *instream,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_list_t *types,
    ...);


/****************************************************************************/
/* Star                                                                     */
/****************************************************************************/
struct stream *SNetStar( struct stream *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

struct stream *SNetStarIncarnate( struct stream *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

struct stream *SNetStarDet( struct stream *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */ 
    snet_typeencoding_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

struct stream *SNetStarDetIncarnate( struct stream *inbuf,
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
struct stream *SNetSplit( struct stream *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag, 
    int utag);

struct stream *SNetSplitDet( struct stream *inbuf, 
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    int ltag,
    int utag);

#ifdef DISTRIBUTED_SNET
struct stream *SNetLocSplit(struct stream *instream,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag);

struct stream *SNetLocSplitDet(struct stream *instream,
    snet_info_t *info, 
    int location,
    snet_startup_fun_t box_a,
    int ltag,
    int utag);
#endif




#endif /* _SNETENTITIES_H_ */
