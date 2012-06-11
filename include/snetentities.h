#ifndef _SNETENTITIES_H_
#define _SNETENTITIES_H_

#include <stdio.h>

#include "handle.h"
#include "constants.h"
#include "expression.h"
#include "interface_functions.h"
#include "snettypes.h"

int SNetNetToId(snet_startup_fun_t fun);
snet_startup_fun_t SNetIdToNet(int id);


/****************************************************************************/
/* Alias                                                                    */
/****************************************************************************/
snet_stream_t *SNetAlias( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_startup_fun_t net);



/****************************************************************************/
/* Box                                                                      */
/****************************************************************************/
snet_stream_t *SNetBox(snet_stream_t *instream,
    snet_info_t *info,
    int location,
    const char *boxname,
    snet_box_fun_t fun,
    snet_exerealm_create_fun_t exerealm_create,
    snet_exerealm_update_fun_t exerealm_update,
    snet_exerealm_destroy_fun_t exerealm_destroy,
    snet_int_list_list_t *out_variants);


/****************************************************************************/
/* Synchrocell                                                              */
/****************************************************************************/
snet_stream_t *SNetSync( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *patterns,
    snet_expr_list_t *guards);


/****************************************************************************/
/* Filter                                                                   */
/****************************************************************************/
snet_stream_t *SNetFilter( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_t *in_variant,
    snet_expr_list_t *guards, ... );

snet_stream_t *SNetTranslate( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_t *in_variant,
    snet_expr_list_t *guards, ... );

snet_stream_t *SNetNameShift( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    int offset,
    snet_variant_t *untouched);


/****************************************************************************/
/* Serial                                                                   */
/****************************************************************************/
snet_stream_t *SNetSerial(snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);


/****************************************************************************/
/* Parallel                                                                 */
/****************************************************************************/
snet_stream_t *SNetParallel( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *types,
    ...);

snet_stream_t *SNetParallelDet( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *types,
    ...);


/****************************************************************************/
/* Star                                                                     */
/****************************************************************************/
snet_stream_t *SNetStar( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_list_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

snet_stream_t *SNetStarIncarnate( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_list_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

snet_stream_t *SNetStarDet( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_list_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

snet_stream_t *SNetStarDetIncarnate( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_list_t *type,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);


/****************************************************************************/
/* Split                                                                    */
/****************************************************************************/
snet_stream_t *SNetSplit( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag,
    int utag);

snet_stream_t *SNetSplitDet( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag,
    int utag);

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


/****************************************************************************/
/* Feedback                                                                 */
/****************************************************************************/
snet_stream_t *SNetFeedback( snet_stream_t *inbuf,
    snet_info_t *info,
    int location,
    snet_variant_list_t *back_pattern,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a);


#endif /* _SNETENTITIES_H_ */
