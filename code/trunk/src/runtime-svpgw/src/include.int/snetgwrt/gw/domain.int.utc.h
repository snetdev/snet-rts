/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : domain.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_DOMAIN_INT_H
#define __SVPSNETGWRT_GW_DOMAIN_INT_H

#include "gw/domain.utc.h"

/*---*/

#include "common.int.utc.h"

/*---*/

#include "core/buffer.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Types of domains.
 */

typedef enum {
    DOMAIN_TYPE_NET,
    DOMAIN_TYPE_ALIAS

} snet_domain_type_t;

/*----------------------------------------------------------------------------*/

/**
 * Datatype for the cons-list (the file "conslist.int.utc.h"
 * is not included to avoid cyclic references).
 */
typedef struct cons_list snet_conslst_t;

/**
 * Instances of the data type below represent
 * a running instance of the Graph Walker.
 */
typedef struct domain_gw_info snet_domain_gw_info_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetDomainSubSystemInit();

extern void
SNetDomainSubSystemDestroy();

/*----------------------------------------------------------------------------*/

extern void
SNetDomainInitNet(
    snet_domain_t *snetd,
    snet_graphctor_fptr_t gctor,
    snet_netout_tfptr_t netout_tfun);

extern void
SNetDomainInitAlias(
    snet_domain_t *snetd, 
    const snet_domain_t *snetd_orig);

/*---*/

extern snet_domain_t*
SNetDomainCreateAlias(const snet_domain_t *snetd_orig);

/*----------------------------------------------------------------------------*/

extern snet_domain_type_t
SNetDomainGetType(const snet_domain_t *snetd);

extern snet_place_t
SNetDomainGetHostPlace(const snet_domain_t *snetd);

/*---*/

extern snet_conslst_t*
SNetDomainGetConsList(const snet_domain_t *snetd);

extern snet_gnode_t*
SNetDomainGetGraphRoot(const snet_domain_t *snetd);

extern snet_buffer_t*
SNetDomainGetOutBuffer(const snet_domain_t *snetd);

extern snet_netout_tfptr_t
SNetDomainGetOutThreadFunction(const snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/

extern snet_base_t*
SNetDomainToBase(snet_domain_t *snetd);

extern const snet_base_t*
SNetDomainToBaseConst(const snet_domain_t *snetd);

#endif // __SVPSNETGWRT_GW_DOMAIN_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

