#ifndef _C4SNET_H_
#define _C4SNET_H_

/** <!--********************************************************************-->
 *
 * @file C4SNet.h
 *
 * C4SNet is a simple C language interface for S-Net. 
 *
 * The language interface contains all the functionality required 
 * by S-Net runtime environment and supports all the C primary data
 * types and arrays of promary types. C4SNet also offers a set of 
 * functions for the programmers to use in their box codes.
 *
 * Currently structured data types and pointers are nott supported.
 *
 *****************************************************************************/

/** <!--********************************************************************-->
 *
 * @typedef c4snet_data_t
 *
 *   @brief  C4SNet data element. Contains the actual C data.
 *
 *****************************************************************************/

typedef struct cdata c4snet_data_t;

/** <!--********************************************************************-->
 *
 * @typedef c4snet_container_t;
 *
 *   @brief  C4SNet container element. Can be used to return values from the box code.
 *
 *****************************************************************************/

typedef struct container c4snet_container_t;

/** <!--********************************************************************-->
 *
 * @enum c4snet_vtype_t
 *
 *   @brief  C4SNet mapping for variable types.    
 *
 *****************************************************************************/

typedef enum{
    VTYPE_unknown, /*!< unkown data type. Not a valid type! */
    VTYPE_simple,  /*!< simple type */
    VTYPE_array    /*!< array  type */
}c4snet_vtype_t;

/** <!--********************************************************************-->
 *
 * @enum c4snet_type_t
 *
 *   @brief  C4SNet mapping for C primary types.    
 *
 *****************************************************************************/

typedef enum{
    CTYPE_unknown, /*!< unkown data type. Not a valid type! */
    CTYPE_uchar,   /*!< unsigned char */
    CTYPE_char,    /*!< signed char */
    CTYPE_ushort,  /*!< unsigned short int */
    CTYPE_short,   /*!< signed short int */
    CTYPE_uint,    /*!< unsigned int */
    CTYPE_int,     /*!< signed int */
    CTYPE_ulong,   /*!< unsigned long int */
    CTYPE_long,    /*!< signed long int */
    CTYPE_float,   /*!< float */
    CTYPE_double,  /*!< double */
    CTYPE_ldouble, /*!< long double */
}c4snet_type_t;

/** <!--********************************************************************-->
 *
 * @name Common functions
 *
 *****************************************************************************/
/*@{*/

/** <!--********************************************************************-->
 *
 * @fn void C4SNetInit(int id)
 *
 *   @brief  Language interface initialization function.
 *
 *           Registers C4SNet language interface to S-Net runtime environment.
 *
 *   @note   This function must be called before any calls to any other C4SNet
 *           language interface functions.
 *
 *   @note   This function is automatically called by S-Net compiler generated 
 *           default main function and must not be called in applications using
 *           the default main function.
 *
 *   @param id    identifier of the language interface. The identifier must 
 *                have value of 0 or more.
 *
 *****************************************************************************/

void C4SNetInit( int id);

/** <!--********************************************************************-->
 *
 * @fn void C4SNetOut(void *hnd, int variant, ...)
 *
 *   @brief  Communicates back the results of the box.
 *
 *           The arguments following the variant number are the produced 
 *           fields followed by produced tags followed by produced binding 
 *           tags. Fields, tags and binding tags are in the same order as they
 *           appear in the box output type (defned in the S-Net source code). 
 *           Fields are pointers to c4snet_data_t structures, tags and binding 
 *           tags are of type int.
 *
 *   @param hnd     Pointer to opaque handle object (passed as parameter to the box function)
 *   @param variant Variant number ofthe resulted record.
 *   @param ...     Produced results
 *
 *****************************************************************************/

void C4SNetOut( void *hnd, int variant, ...);

/*@}*/

/** <!--********************************************************************-->
 *
 * @name Data functions
 *
 *****************************************************************************/
/*@{*/

/** <!--********************************************************************-->
 *
 * @fn int C4SNetSizeof(c4snet_data_t *data)
 *
 *   @brief Returns size of the data inside c4snet_data_t struct in bytes.
 *          In case of an data array, size of one array element is returned.
 *
 *   @param data Pointer to the data.
 *
 *   @return size of the data inside c4snet_data_t struct in bytes.   
 *
 *****************************************************************************/

int C4SNetSizeof(c4snet_data_t *data);

/** <!--********************************************************************-->
 *
 * @fn c4snet_data_t *C4SNetDataCreate( c4snet_type_t type, void *data)
 *
 *   @brief  Creates a new c4snet_data_t struct.
 *
 *   @param type Type of the data.
 *   @param data Pointer to the data.
 * 
 *   @notice In case of primary types the data will be copied!
 *
 *   @return Pointer to the created struct, or NULL in case of error.        
 *
 *****************************************************************************/

c4snet_data_t *C4SNetDataCreate( c4snet_type_t type, const void *data);

/** <!--********************************************************************-->
 *
 * @fn c4snet_data_t *C4SNetDataCreateArray( c4snet_type_t type, void *data)
 *
 *   @brief  Creates a new c4snet_data_t struct.
 *
 *   @param type Type of the data.
 *   @param size Number of elements in the array.
 *   @param data Pointer to the data.
 *
 *   @notice Unlike in case of primary types the data will NOT be copied!
 *
 *   @return Pointer to the created struct, or NULL in case of error.        
 *
 *****************************************************************************/

c4snet_data_t *C4SNetDataCreateArray( c4snet_type_t type, int size, void *data);


/** <!--********************************************************************-->
 *
 * @fn void *C4SNetDataCopy( void *ptr)
 *
 *   @brief  Copies c4snet_data_t struct.
 *
 *   @param ptr Pointer to type c4snet_data_t struct to be copied.
 *
 *   @return Pointer to the created struct, or NULL in case of error.   
 *
 *****************************************************************************/

void *C4SNetDataCopy( void *ptr);

/** <!--********************************************************************-->
 *
 * @fn void C4SNetDataFree( void *ptr)
 *
 *   @brief  Frees the memory allocated for c4snet_data_t struct.
 *
 *   @param ptr Pointer to type c4snet_data_t struct to be freed.  
 *
 *****************************************************************************/

void C4SNetDataFree( void *ptr);

/** <!--********************************************************************-->
 *
 * @fn void *C4SNetDataGetData( c4snet_data_t *c)
 *
 *   @brief Returns the actual data.
 *
 *   @param ptr Pointer to type c4snet_data_t struct. 
 * 
 *   @return Pointer to the actual data. 
 *
 *****************************************************************************/

void *C4SNetDataGetData( c4snet_data_t *ptr);

/** <!--********************************************************************-->
 *
 * @fn c4snet_type_t C4SNetDataGetType( c4snet_data_t *c)
 *
 *   @brief Returns the type of the data.
 *
 *   @param ptr Pointer to type c4snet_data_t struct. 
 *
 *   @return Type of the data or CTYPE_unknown in case of an error. 
 *
 *****************************************************************************/

c4snet_type_t C4SNetDataGetType( c4snet_data_t *c);

/** <!--********************************************************************-->
 *
 * @fn c4snet_type_t C4SNetDataGetVType( c4snet_data_t *c)
 *
 *   @brief Returns the vtype of the data.
 *
 *   @param ptr Pointer to type c4snet_data_t struct. 
 *
 *   @return VType of the data or VTYPE_unknown in case of an error. 
 *
 *****************************************************************************/

c4snet_vtype_t C4SNetDataGetVType( c4snet_data_t *c);

/** <!--********************************************************************-->
 *
 * @fn int C4SNetDataGetArraySize( c4snet_data_t *c);
 *
 *   @brief Returns number of elements in the data array
 *
 *   @param ptr Pointer to type c4snet_data_t struct. 
 *
 *   @return Number of elements in the data array or -1 in case the data is not an array
 *
 *****************************************************************************/

int C4SNetDataGetArraySize( c4snet_data_t *c);


/*@}*/






/** <!--********************************************************************-->
 *
 * @name Container functions
 *
 *****************************************************************************/
/*@{*/

/** <!--********************************************************************-->
 *
 * @fn c4snet_container_t *C4SNetContainerCreate( void *hnd, int variant)
 *
 *   @brief  Creates a container that can be used to return values.
 *
 *   @param hnd     Pointer to opaque handle object (passed as parameter to the box function)
 *   @param variant Variant number of the resulted record.
 *
 *   @return Pointer to the created container, or NULL in case of an error.
 *
 *****************************************************************************/

c4snet_container_t *C4SNetContainerCreate( void *hnd, int variant);

/** <!--********************************************************************-->
 *
 * @fn c4snet_container_t *C4SNetContainerSetField( c4snet_container_t *c, void *ptr)
 *
 *   @brief  Adds field into a container.
 *
 *   @note   The fields must be passed in the same order as they appear 
 *           in the box output type defined in the S-Net source code.
 *
 *   @param c   Pointer to the container.     
 *   @param ptr Pointer to the c4snet_data_t struct of the field.
 *
 *   @return Pointer to the container, or NULL in case of an error.
 *
 *****************************************************************************/

c4snet_container_t *C4SNetContainerSetField( c4snet_container_t *c, void *ptr);

/** <!--********************************************************************-->
 *
 * @fn c4snet_container_t *C4SNetContainerSetTag( c4snet_container_t *c, int value)
 *
 *   @brief  Adds tag into a container.
 *
 *   @note   The tags must be passed in the same order as they appear 
 *           in the box output type defined in the S-Net source code.
 *
 *   @param c     Pointer to the container.     
 *   @param value Value of the tag.
 *
 *   @return Pointer to the container, or NULL in case of an error.
 *
 *****************************************************************************/

c4snet_container_t *C4SNetContainerSetTag( c4snet_container_t *c, int value);

/** <!--********************************************************************-->
 *
 * @fn c4snet_container_t *C4SNetContainerSetBTag( c4snet_container_t *c, int value)
 *
 *   @brief  Adds binding tag into a container.
 *
 *   @note   The binding tags must be passed in the same order as they appear 
 *           in the box output type defined in the S-Net source code.
 *
 *   @param c     Pointer to the container.     
 *   @param value Value of the binding tag.
 *
 *   @return Pointer to the container, or NULL in case of an error.
 *
 *****************************************************************************/

c4snet_container_t *C4SNetContainerSetBTag( c4snet_container_t *c, int value);

/** <!--********************************************************************-->
 *
 * @fn void C4SNetContainerOut( c4snet_container_t *c)
 *
 *   @brief  Communicates back the results of the box stored in a container.
 *
 *   @param c Pointer to the container whose content is to be returned.
 *
 *****************************************************************************/

void C4SNetContainerOut( c4snet_container_t *c);

/*@}*/

#endif /* _C4SNET_H_ */
