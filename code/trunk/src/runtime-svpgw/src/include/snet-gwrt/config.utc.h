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

/*----------------------------------------------------------------------------*/
/**
 * If defined sets the level of debugging code the 
 * library will have.
 */
// #define SVPSNETGWRT_DEBUG 5

#endif // __SVPSNETGWRT_CONFIG_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

