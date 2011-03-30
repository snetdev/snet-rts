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

#ifndef __SVPSNETGWRT_NETIF_CONFIG_H
#define __SVPSNETGWRT_NETIF_CONFIG_H

/**
 * Define this to specify that this
 * header is the configuration header
 */
#define SVPSNETGWRT_NETIF_CFG_HEADER

/*----------------------------------------------------------------------------*/
/**
 * Specify the default format the network interface
 * component should use for input and output respectively.
 *
 * Valid values include:
 *      SNET_NETIF_IOFMT_TXT = Text
 *      SNET_NETIF_IOFMT_BIN = Binary
 *      SNET_NETIF_IOFMT_XML = XML
 */
#define SVPSNETGWRT_NETIF_DEFAULT_INPUT_FORMAT  SNET_NETIF_IOFMT_TXT
#define SVPSNETGWRT_NETIF_DEFAULT_OUTPUT_FORMAT SNET_NETIF_IOFMT_TXT

#endif // __SVPSNETGWRT_NETIF_CONFIG_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

