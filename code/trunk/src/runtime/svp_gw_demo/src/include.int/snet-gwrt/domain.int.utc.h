/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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

#ifndef __SVPSNETGWRT_DOMAIN_INT_H
#define __SVPSNETGWRT_DOMAIN_INT_H

#include "domain.utc.h"

/*---*/

#include "common.int.utc.h"
#include "conslist.int.utc.h"
#include "buffer.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Types of domains.
 */

typedef enum {
    DOMAIN_TYPE_NET,
    DOMAIN_TYPE_ALIAS,
    DOMAIN_TYPE_WORKER

} snet_domain_type_t;

/*----------------------------------------------------------------------------*/
/**
 * Instances of the data type below represent
 * a running instance of the Graph Walker.
 */

typedef struct domain_gw_info snet_domain_gw_info_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void SNetDomainSubSystemInit();
extern void SNetDomainSubSystemDestroy();

/*----------------------------------------------------------------------------*/

extern snet_domain_t*
SNetDomainCreateAlias(const snet_domain_t *snetd_orig);

extern snet_domain_t*
SNetDomainCreateWorker(const snet_domain_t *snetd_master);

/*----------------------------------------------------------------------------*/

extern bool 
SNetDomainIsSame(const snet_domain_t *snetd1, const snet_domain_t *snetd2);

/*----------------------------------------------------------------------------*/

extern snet_domain_type_t
SNetDomainGetType(const snet_domain_t *snetd);

extern snet_conslst_t*
SNetDomainGetConsList(const snet_domain_t *snetd);

extern const snet_gnode_t*
SNetDomainGetGraphRoot(const snet_domain_t *snetd);

extern snet_buffer_t*
SNetDomainGetOutBuffer(const snet_domain_t *snetd);

/*---*/

extern place SNetDomainGetGWPlace(const snet_domain_t *snetd);
extern place SNetDomainGetOutBufferCondPlace(const snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/

extern void SNetDomainRegisterGW(const snet_domain_gw_info_t *gw);

#endif // __SVPSNETGWRT_DOMAIN_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/
