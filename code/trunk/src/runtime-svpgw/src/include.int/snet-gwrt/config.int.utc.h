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
 * Sets the size (in terms of index items) an index vector increases
 * with each re-allocation (For long networks bigger values will improve
 * performance).
 */
#define SVPSNETGWRT_IDXVEC_INCR_STEP 16

/**
 * Size in decimal digits that an index item can be. Used by
 * the SNetIdxVecToString() function as a thresshold for the size
 * of the buffer required to store a string representation of an
 * index vector.
 */
#define SVPSNETGWRT_IDXVEC_ITEM_MAX_DEC_DIGITS 128 

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

/*----------------------------------------------------------------------------*/
/**
 * Since there is no direct support in SVP for suspend/signal pattern
 * of communication (like condition variables in POSIX) between families
 * of threads the SNetPopOut() does not suspend when the global output buffer
 * is empty but returns "NULL". Thus the process that reads the output of
 * a network needs to continuusly "poll" the buffer by calling SNetPopOut()
 * continuusly.
 *
 * When the following is defined a "hack" that involves the "kill" SVP action
 * is used to emulate the suspend/signal pattern thus allowing a process/thread
 * to suspend on an empty buffer. This solution alsos makes the assumption
 * that mutual exclusive places are not recursive in SVP which so far it
 * appears to hold for the uTC-PTL.
 */
// #define SVPSNETGWRT_USE_KILL_HACK_FOR_OUT_BUFFER_SUSPEND

#endif // __SVPSNETGWRT_CONFIG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

