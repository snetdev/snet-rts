/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : snetcgenhdr.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Because of differences in a few things between the 
                     current version of the SNet compiler and this runtime
                     system the files "snetentities.h" and "networkinterface.h"
                     contain several "#defines" and type definitions so that 
                     compiler generated code can be used unchanged.

                     However some of these need to be "undone"!! The purpose
                     of this file is to do exactly that. It should be included
                     after EVERY time a SNet compiler generated header (or
                     multiple headers) is included!!

                     Of course this only applies for the cases where additional
                     modules (other than the one the compiler generates) need
                     to include those headers (e.g. a custom network interface).


    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/
/**
 * Currently the only thing that needs to be "undone" is the 
 * "#define" of "char" to "const char" workaround!!
 */
#ifdef char
#undef char
#endif

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

