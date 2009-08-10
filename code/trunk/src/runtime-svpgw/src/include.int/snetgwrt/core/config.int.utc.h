/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : config.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header file that contains symbols for enabling / disabling
                     features (thus allowing some customization).

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_CORE_CONFIG_INT_H
#define __SVPSNETGWRT_CORE_CONFIG_INT_H

/**
 * Define this to specify that this
 * header is the configuration header
 */
#define SVPSNETGWRT_INTERN_CFG_HEADER

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * When defined pointers for objects created by the
 * the runtime to "user" code are not being masked
 * as it is done by default.
 */
// #define SVPSNETGWRT_NO_POINTER_MASKING

/**
 * Sets whether memory allocations should
 * be monitored (for debug builds this is always
 * defined).
 */
// #define SVPSNETGWRT_MONITOR_MALLOC

/**
 * Sets the "grow" rate (in number of entries) for the
 * reference lookup table used in the memory manager.
 *
 * NOTE!! If the runtime is built with the assumption that
 * it will only run on a shared memory system this option has
 * no effect because in that case no table is used (at least up to now).
 */
#define SVPSNETGWRT_MEMMNG_REF_LOOKUP_TBL_GROW_STEP 256

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Defines the default bucket size for a buffer
 */
#define SVPSNETGWRT_BUFFER_DEFAULT_BUCKET_SZ  256

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Defines the default bucket size for a list
 */
#define SVPSNETGWRT_LIST_DEFAULT_BUCKET_SZ    128

/**
 * Defined the size by which the vector of buckets
 * will increase in a list.
 */
#define SVPSNETGWRT_LIST_BUCKET_VEC_INCR_STEP 8

#endif // __SVPSNETGWRT_CORE_CONFIG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

