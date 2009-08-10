/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : atomicops.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_ATOMICOPS_INT_H
#define __SVPSNETGWRT_ATOMICOPS_INT_H

#include "common.int.utc.h"

/*----------------------------------------------------------------------------*/

extern unsigned long 
SNetAtomicAddULong(volatile unsigned long *var, unsigned long val);

extern unsigned long 
SNetAtomicSubULong(volatile unsigned long *var, unsigned long val);

#endif // __SVPSNETGWRT_ATOMICOPS_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

