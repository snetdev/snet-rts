/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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

#ifndef __SVPSNETGWRT_CONFIG_INT_H
#define __SVPSNETGWRT_CONFIG_INT_H

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
// #define SVPSNETGWRT_NO_PONTER_MASKING

/**
 * Sets whether memory allocations should
 * be monitored (for debug builds this is always
 * defined).
 */
// #define SVPSNETGWRT_MONITOR_MALLOC

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Sets the size (in terms of index items) a graph index increases
 * with each re-allocation (For long networks bigger values will improve
 * performance).
 */
#define SVPSNETGWRT_GRAPH_INDEX_INCR_STEP 16

/**
 * Size in decimal digits that an index item can be. Used by
 * the SNetGInxToString() function as a thresshold for the size
 * of the buffer required to store a string representation of an index.
 */
#define SVPSNETGWRT_GRAPH_INDEX_ITEM_MAX_DEC_DIGITS 128 

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Defines the default bucket size for a buffer
 */
#define SVPSNETGWRT_BUFFER_DEFAULT_BUCKET_SZ  512

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

#endif // __SVPSNETGWRT_CONFIG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

