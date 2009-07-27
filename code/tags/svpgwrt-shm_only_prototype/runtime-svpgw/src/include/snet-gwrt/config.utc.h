/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : config.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header file that contains symbols for enabling / disabling
                     features (thus allowing some customization).

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_CONFIG_H
#define __SVPSNETGWRT_CONFIG_H

/**
 * Define this to specify that this
 * header is the configuration header
 */
#define SVPSNETGWRT_CFG_HEADER

/**
 * Platform
 */
#define SVPSNETGWRT_SVP_PLATFORM_UTCPTL
// #define SVPSNETGWRT_SVP_PLATFORM_DUTCPTL
// #define SVPSNETGWRT_SVP_PLATFORM_MGRID
// #define SVPSNETGWRT_SVP_PLATFORM_UNKNOWN

/*----------------------------------------------------------------------------*/
/**
 * If defined the runtime assumes that it runs on a system with distributed
 * memory. If not then a shared memory system is assumed (which can speedup
 * things since unecessary checks / operations do not take place).
 *
 * NOTE!: By default this symbol is always defined when 
 * SVPSNETGWRT_SVP_PLATFORM_DUTCPTL is defined.
 */
// #define SVPSNETGWRT_ASSUME_DISTRIBUTED_MEMORY

/*----------------------------------------------------------------------------*/
/**
 * Library type (default = SVPSNETGWRT_LIB_TYPE_SOFT_STATIC)
 */
// #define SVPSNETGWRT_LIB_TYPE_HARD
// #define SVPSNETGWRT_LIB_TYPE_SOFT_STATIC
// #define SVPSNETGWRT_LIB_TYPE_SOFT_DYNAMIC

/*----------------------------------------------------------------------------*/
/**
 * If defined some "harmless" warnings are being
 * disabled at the appropriate places (if the compiler
 * supports it).
 */
#define SVPSNETGWRT_DISABLE_SELECTED_WARNINGS

/*----------------------------------------------------------------------------*/
/**
 * If defined sets the level of debugging code the 
 * library will have (if not defined a default value is 
 * used in "debug" builds).
 */
#define SVPSNETGWRT_DEBUG_LEVEL 5

#endif // __SVPSNETGWRT_CONFIG_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

