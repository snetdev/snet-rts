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

#ifndef __SVPSNETGWRT_GW_CONFIG_INT_H
#define __SVPSNETGWRT_GW_CONFIG_INT_H

/**
 * Define this to specify that this
 * header is the configuration header
 */
#define SVPSNETGWRT_INTERN_GW_CFG_HEADER

/*----------------------------------------------------------------------------*/
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
#define SVPSNETGWRT_IDXVEC_ITEM_MAX_DEC_DIGITS  128 

/*----------------------------------------------------------------------------*/
/**
 * Bucket size for the list used as a "cache" for
 * places the GW is using for executing boxes.
 */
#define SVPSNETGWRT_BOX_EXECPLC_CACHE_BUCKET_SZ 4

/*----------------------------------------------------------------------------*/
/**
 * Bucket size for the lists used for keeping
 * track of synchro-cell "states".
 */
#define SVPSNETGWRT_SYNCCELL_STATELST_BUCKET_SZ 32

/*----------------------------------------------------------------------------*/
/**
 * When the SVP platform is the non-distributed uTC-PTL it does not
 * really make sense for the resource manager to allocate a new place
 * for every box but do the create locally. This is due to the way 
 * that SVP platform implements places. However this might change in'
 * the future.
 * 
 * Thus if the following symbol is defined then even for that SVP
 * platform the resource manager will allocate "proper" places for
 * boxes when requested.
 */
// #define SVPSNETGWRT_RESMNG_ALLOC_PLACE_ON_NORMAL_UTCPTL

/*----------------------------------------------------------------------------*/
/**
 * The extention that is being added to the given name
 * of a box in order to generate its extended name.
 */
#define SVPSNETGWRT_BOX_XNAME_EXT "__snet"

/*----------------------------------------------------------------------------*/
/**
 * Threshold value that controls the maximum number of
 * records for each invokation of the user-defined 
 * "network output handler" thread function.
 *
 * If this is 0 then there is no fixed limit.
 */
#define SVPSNETGWRT_NETOUT_TFUN_INVOKE_THRESHOLD 64

#endif // __SVPSNETGWRT_GW_CONFIG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

