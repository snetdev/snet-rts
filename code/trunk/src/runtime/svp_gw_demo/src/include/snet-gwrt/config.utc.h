/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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
#define SVPSNETGWRT_PLATFORM_PTL
// #define SVPSNETGWRT_PLATFORM_PTL_PLUS
// #define SVPSNETGWRT_PLATFORM_MGRID
// #define SVPSNETGWRT_PLATFORM_UNKNOWN

/**
 * The target platform can be a mix of all the above
 * in which case more than one of the above symbols should
 * be defined. In that case the following should also be defined.
 */
// #define SVPSNETGWRT_PLATFORM_MIXED

/*----------------------------------------------------------------------------*/
/**
 * Library type
 */
// #define SVPSNETGWRT_LIB_TYPE_HARD
#define SVPSNETGWRT_LIB_TYPE_SOFT_STATIC
// #define SVPSNETGWRT_LIB_TYPE_SOFT_DYNAMIC

/*----------------------------------------------------------------------------*/
/**
 * If defined some harmless warnings are being
 * disabled at the appropriate places (if the compiler
 * supports it).
 */
#define SVPSNETGWRT_DISABLE_SELECTED_WARNINGS

/*----------------------------------------------------------------------------*/
/**
 * If defined sets the level of debugging code the 
 * library will have.
 */
// #define SVPSNETGWRT_DEBUG 5

#endif // __SVPSNETGWRT_CONFIG_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

