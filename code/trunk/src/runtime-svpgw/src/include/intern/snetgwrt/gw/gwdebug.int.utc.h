/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : gwdebug.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_GWDEBUG_INT_H
#define __SVPSNETGWRT_GW_GWDEBUG_INT_H

#include "common.int.utc.h"
#include "conslist.int.utc.h"

/*---*/


#define SVPSNETGWRT_GWDPRINT

/*----------------------------------------------------------------------------*/
/* Debug print format strings and args macros */

#define DPRINT_MAC_SYNCCELL_STATE_SEARCH()        ""
#define DPRINT_MAC_SYNC_CELL_GW_RESTART()         ""
#define DPRINT_MAC_SYNC_CELL_GW_RESTART_WALKING() ": already walking"

#define DPRINT_MAC_\
SYNC_CELL_GW_RESTART_NWALKING(state) ": NOT walking (state=%d)", state

/*---*/

#define DPRINT_MAC_\
SYNCCELL_SWAP_HND_BEGIN(gnode, sc_state, swpd_cnode, swpd_hnd) \
    ": "                                                       \
    "gnode=%p, "                                               \
    "sc_state=%p, "                                            \
    "swapped(cnode=%p, hnd=%p)", gnode, sc_state, swpd_cnode, swpd_hnd

/*---*/

#define DPRINT_MAC_\
SYNCCELL_WALK_HND_BEGIN()               ""

#define DPRINT_MAC_\
SYNCCELL_WALK_HND_CELL_STATE_NULL()     ": dead synccell"

#define DPRINT_MAC_\
SYNCCELL_WALK_HND_CELL_STATE_NOT_NULL() ""

#define DPRINT_MAC_\
SYNCCELL_WALK_HND_MATCHED(mpcnt)        ": matched_patterns_cnt=%u", mpcnt

#define DPRINT_MAC_\
SYNCCELL_WALK_HND_END()                 ""

/*---*/

#define DPRINT_MAC_\
SYNCCELL_OUT(gnode, sc_state) \
    ": gnode=%p, sc_state=%p", gnode, sc_state

/*---*/

#define DPRINT_MAC_SYNCCELL_HND()                ""
#define DPRINT_MAC_SYNCCELL_HND_CHECK_DEFINITE() ""

#define DPRINT_MAC_\
SYNCCELL_DEFINITE_CHECK_PASSED(pcnt, mpcnt, dmpcnt, gwstate_status) \
    ": "                                                            \
    "patterns_cnt=%u, "                                             \
    "matched_patterns_cnt=%u, "                                     \
    "defmatched_patterns_cnt=%u, "                                  \
    "gwstate_status=%u",                                            \
    pcnt, mpcnt, dmpcnt, gwstate_status

#define DPRINT_MAC_\
SYNCCELL_TEST_DEFINITE_MATCH_SCSTATE_COMPLETE(dmpcnt) \
    ": defmatched_patterns_cnt=%u", dmpcnt

#define DPRINT_MAC_\
SYNCCELL_HND_DEFINITE_CHECKED(result, gwstate_status) \
    ": result=%d, gwstate_status=%d", result, gwstate_status

#define DPRINT_MAC_SYNCCELL_HND_TRY_SET_STATE()    ""
#define DPRINT_MAC_SYNCCELL_HND_WAKEUP_FLAG_SET()  ""

#define DPRINT_MAC_\
SYNCCELL_HND_TRY_SET_STATE_DONE(gwstate_status) \
    ": gwstate_status=%d", gwstate_status

#define DPRINT_MAC_SYNCCELL_HND_INFPROP_START()    ""
#define DPRINT_MAC_SYNCCELL_HND_INFPROP_END()      ""

#define DPRINT_MAC_SYNCCELL_HND_END(continue_walk) \
    ": continue_walk=%d", continue_walk

/*----------------------------------------------------------------------------*/

#define DPRINT_MAC_STAR_WALK_IN_BEGIN() ""
#define DPRINT_MAC_STAR_WALK_IN_CONDT() ": exit star"
#define DPRINT_MAC_STAR_WALK_IN_CONDF() ": entering star"
#define DPRINT_MAC_STAR_WALK_IN_END()   ""

#define DPRINT_MAC_STAR_MERGE_BEGIN()   ""
#define DPRINT_MAC_STAR_MERGE_CONDT()   ": exit star"
#define DPRINT_MAC_STAR_MERGE_CONDF()   ": entering star again"
#define DPRINT_MAC_STAR_MERGE_END()     ""


/*---*/

#define DPRINT_MAC_ISPLIT_WALK_IN_BEGIN() "" 
#define DPRINT_MAC_ISPLIT_WALK_IN_END()   ""

#define DPRINT_MAC_\
ISPLIT_WALK_IN_TAGVALS(tagval, rtagval) \
    ": tag_val=%d, rtag_val=%llu", tagval, rtagval

/*----------------------------------------------------------------------------*/

#define DPRINT_MAC_MOVING()          "..."
#define DPRINT_MAC_MOVED_TONULL()    ": to <NULL>"
#define DPRINT_MAC_MOVED()           ""
#define DPRINT_MAC_POPPED()          ""
#define DPRINT_MAC_POPPED_ELST()     ": empty cons-list"
#define DPRINT_MAC_DESTROYED(ncnode) ": cons-node->next=%p", ncnode

/*----------------------------------------------------------------------------*/

#define DPRINT_MAC_ON_INFIMUM_CHANGE_SYNCCELL_NTAM() \
    ": not there any more"

#define DPRINT_MAC_ON_INFIMUM_CHANGE_SYNCCELL_FWGW() \
    ": found walking GW"

#define DPRINT_MAC_ON_INFIMUM_CHANGE_SYNCCELL_STDF() \
    ": still there ---> is definite match"

#define DPRINT_MAC_\
ON_INFIMUM_CHANGE_SYNCCELL_ST(gnode, sc_state) \
    ": still there ---> gnode=%p, sc_state=%p", gnode, sc_state

#define DPRINT_MAC_ON_INFIMUM_CHANGE_SYNCCELL_NWSS() \
    ": not in WAIT_SYNCCELL state"

#define DPRINT_MAC_ON_INFIMUM_CHANGE_SYNCCELL_SGW() \
    ": starting GW..."

#define DPRINT_MAC_ON_INFIMUM_CHANGE_SYNCCELL_GWSTRD() \
    ": GW started!"

/*----------------------------------------------------------------------------*/

#define DPRINT_MAC_\
ON_INFIMUM_CHANGE_SYNCCELL_START() ""

#define DPRINT_MAC_\
ON_INFIMUM_CHANGE_SYNCCELL_TEST_DEFINITE_MATCH() ""

#define DPRINT_MAC_\
ON_INFIMUM_CHANGE_SYNCCELL_TEST_DEFINITE_MATCH_DONE(result) \
    ": result=%d", result

#define DPRINT_MAC_\
ON_INFIMUM_CHANGE_SYNCCELL_START_GW_CHKSTART()   ""

#define DPRINT_MAC_\
ON_INFIMUM_CHANGE_SYNCCELL_START_GW_CHKSTART_DONE() ""

#define DPRINT_MAC_ON_INFIMUM_CHANGE_SYNCCELL_END(result) \
    ": result=%d", result

/*----------------------------------------------------------------------------*/

#define DPRINT_MAC_INFIMUM_UPDATE()         ": from prev"
#define DPRINT_MAC_INFPROPAGATE()           ""
#define DPRINT_MAC_STOPPING_INFPROP()       ""
#define DPRINT_MAC_GW_TERMINATING()         ": reached end"
#define DPRINT_MAC_GW_TERMINATING_SUSPEND() ": suspend"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * This is the "debug print" macro that is
 * used within the code.
 */
#ifdef SVPSNETGWRT_GWDPRINT
#define SNetGwDPrint(            \
    cnode,                       \
    dprint_mac, dprint_mac_args) \
    SNetGwDebugPrint(            \
        cnode,                   \
        #dprint_mac              \
        DPRINT_MAC_##dprint_mac dprint_mac_args)

#define SNetGwDPrintWithVariant(                     \
    cnode,                                           \
    dprint_mac_variant, dprint_mac, dprint_mac_args) \
    SNetGwDebugPrint(                                \
        cnode,                                       \
        #dprint_mac                                  \
        DPRINT_MAC_##dprint_mac##_##dprint_mac_variant dprint_mac_args)

#else  // SVPSNETGWRT_GWDPRINT not defined
#define SNetGwDPrint( \
    cnode,            \
    dprint_mac, dprint_mac_args)

#define SNetGwDPrintWithVariant(  \
    cnode,                        \
    dprint_mac_variant, dprint_mac, dprint_mac_args)

#endif // SVPSNETGWRT_GWDPRINT

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Function used by the macros above to do the actual
 * debug printing.
 */
extern void
SNetGwDebugPrint(snet_conslst_node_t *cnode, const char *fmt, ...);

#endif // __SVPSNETGWRT_GW_GWDEBUG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

