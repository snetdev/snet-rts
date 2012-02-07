/**
 * $Id
 *
 * @file bitmap.c
 *
 * Implements a simple bitmap.
 *
 */

#include <string.h>

#include "bitmap.h"
#include "memfun.h"


#define BITS_PER_BYTE 8

#define BIT_SET 0x1
#define BIT_CLEAR 0x0
#define BYTE_FULL_SET 0xf

#define BYTE_OFFSET(bit) (bit / BITS_PER_BYTE)
#define BIT_OFFSET(bit) (bit % BITS_PER_BYTE)

/**
 *
 * @name Bitmap
 *
 * <!--
 * snet_util_bitmap_t *SNetUtilBitmapCreate(int size) : Creates a new bitmap
 * int SNetUtilBitmapCopy(snet_util_bitmap_t *from, snet_util_bitmap_t *to) : Copies bitmap
 * void SNetUtilBitmapDestroy(snet_util_bitmap_t *map) : Destroyes bitmap
 * int SNetUtilBitmapSize(snet_util_bitmap_t *map) : Returns size of the bitmap
 * int SNetUtilBitmapSet(snet_util_bitmap_t *map, int bit) : Sets bit in the bitmap
 * int SNetUtilBitmapClear(snet_util_bitmap_t *map, int bit) : Clears bit in a bitmap
 * int SNetUtilBitmapFindNSet(snet_util_bitmap_t *map) : Sets first 0-bit in the bitmap
 * int SNetUtilBitmapGet(snet_util_bitmap_t *map, int bit) : Returns bit from a bitmap
 * -->
 *
 *****************************************************************************/
/*@{*/


/** @struct snet_util_bitmap
 *
 *   @brief Bitmap type.
 *
 */

struct snet_util_bitmap {
  unsigned int size;       /**< Size of the bitmap in bytes */
  unsigned int search_pos; /**< Start position for next search */
  unsigned char *bitmap;   /**< The actual bitmap */
};


/** <!--********************************************************************-->
 *
 * @fn snet_util_bitmap_t *SNetUtilBitmapCreate(int size)
 *
 *   @brief   Creates bitmap of given size. All the bits are cleared.
 *
 *   @param size  Size of the bitmap in bits. The size is rounded up to nearest full byte
 *
 *   @return      Created bitmap, or NULL in case of an error.
 *
 ******************************************************************************/

snet_util_bitmap_t *SNetUtilBitmapCreate(int size)
{
  snet_util_bitmap_t *map;

  if(size <= 0) {
    return NULL;
  }

  if((map = SNetMemAlloc(sizeof(snet_util_bitmap_t))) == NULL) {
    return NULL;
  }

  /* Size in full bytes */
  map->size = (size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE;

  map->search_pos = 0;

  map->bitmap = SNetMemAlloc(sizeof(unsigned char) * map->size);

  memset(map->bitmap, BIT_CLEAR,  map->size);
  
  return map;
}


/** <!--********************************************************************-->
 *
 * @fn int SNetUtilBitmapCopy(snet_util_bitmap_t *from, snet_util_bitmap_t *to)
 *
 *   @brief   Copies bitmap 'from' to bitmap 'to'.
 *
 *            Size of 'to' must be at least that of 'from'. All extra bits are 
 *            cleared.
 *  
 *   @param from  Bitmap to be copied. 
 *   @param to    Bitmap to which bits are copied. 
 *
 *   @return      0 in case of success, SNET_BITMAP_ERR in case of an error.
 *
 ******************************************************************************/

int SNetUtilBitmapCopy(snet_util_bitmap_t *from, snet_util_bitmap_t *to)
{
  if(to->size < from->size) {
    return SNET_BITMAP_ERR;
  }

  memcpy(to->bitmap, from->bitmap, from->size);

  memset(to->bitmap + from->size, BIT_CLEAR, to->size - from->size);

  to->search_pos = from->search_pos;
  
  return 0; 
}


/** <!--********************************************************************-->
 *
 * @fn void SNetUtilBitmapDestroy(snet_util_bitmap_t *map)
 *
 *   @brief   Destroyes a bitmap and frees the memory.
 *  
 *   @param map   Bitmap to be destroyed. 
 *
 ******************************************************************************/

void SNetUtilBitmapDestroy(snet_util_bitmap_t *map)
{
  SNetMemFree(map->bitmap);
  SNetMemFree(map);
}


/** <!--********************************************************************-->
 *
 * @fn int SNetUtilBitmapSize(snet_util_bitmap_t *map)
 *
 *   @brief   Returns the size of the bitmap in bits.
 *  
 *   @param map   The bitmap. 
 *
 *   @return      Size of the bitmap in bits.
 *
 ******************************************************************************/

int SNetUtilBitmapSize(snet_util_bitmap_t *map)
{
  return map->size * BITS_PER_BYTE;
}


/** <!--********************************************************************-->
 *
 * @fn int SNetUtilBitmapSet(snet_util_bitmap_t *map, int bit)
 *
 *   @brief   Sets bit 'bit'.
 *
 *            The value of the bit is 1 after the operation.
 *  
 *   @param map   The bitmap.
 *   @param bit   Index of the bit to be set.
 *
 *   @return      value of the bit before operation in case of success, 
 *                SNET_BITMAP_ERR in case of an error.
 *
 ******************************************************************************/

int SNetUtilBitmapSet(snet_util_bitmap_t *map, unsigned int bit)
{
  int old;

  if(bit >= map->size) {
    return SNET_BITMAP_ERR;
  }
  
  old = (map->bitmap[BYTE_OFFSET(bit)] >> BIT_OFFSET(bit)) & BIT_SET;

  map->bitmap[BYTE_OFFSET(bit)] |= (BIT_SET << BIT_OFFSET(bit));

  return old;
}


/** <!--********************************************************************-->
 *
 * @fn int SNetUtilBitmapClear(snet_util_bitmap_t *map, int bit)
 *
 *   @brief   Clears bit 'bit'.
 *
 *            The value of the bit is 0 after the operation.
 *  
 *   @param map   The bitmap.
 *   @param bit   Index of the bit to be set.
 *
 *   @return      value of the bit before operation in case of success, 
 *                SNET_BITMAP_ERR in case of an error.
 *
 ******************************************************************************/

int SNetUtilBitmapClear(snet_util_bitmap_t *map, unsigned int bit)
{
  int old;

  if(bit >= map->size) {
    return SNET_BITMAP_ERR;
  }

  old = (map->bitmap[BYTE_OFFSET(bit)] >> BIT_OFFSET(bit)) & BIT_SET;

  map->bitmap[BYTE_OFFSET(bit)] &= ~(BIT_SET << BIT_OFFSET(bit));
  
  /* Lower the search position */
  if(map->search_pos > BYTE_OFFSET(bit)) {
    map->search_pos = BYTE_OFFSET(bit);
  }
  
  return old;
}


/** <!--********************************************************************-->
 *
 * @fn int SNetUtilBitmapFindNSet(snet_util_bitmap_t *map)
 *
 *   @brief   Find a clear bit in the bitmap and sets it.
 *
 *            The first 0-bit from the start of the bitmap is selected.
 *            The value of the bit is 1 after the operation.
 *  
 *   @param map   The bitmap.
 *
 *   @return      index of the bit in case of success, 
 *                SNET_BITMAP_ERR in case of an error.
 *
 ******************************************************************************/

int SNetUtilBitmapFindNSet(snet_util_bitmap_t *map)
{
  int j;

  for(; map->search_pos < map->size; map->search_pos++) { 
    if(map->bitmap[map->search_pos] != BYTE_FULL_SET) {
 
      for(j = 0; j < BITS_PER_BYTE; j++) {	
	if(((map->bitmap[map->search_pos] >> j) & BIT_SET) == BIT_CLEAR) {

	  map->bitmap[map->search_pos] |= (BIT_SET << j);	  
	  return map->search_pos * BITS_PER_BYTE + j;
	}
      }
    }
  }

  return SNET_BITMAP_ERR;
}


/** <!--********************************************************************-->
 *
 * @fn int SNetUtilBitmapGet(snet_util_bitmap_t *map, int bit)
 *
 *   @brief   Returns value of the bit 'bit'.
 * *  
 *   @param map   The bitmap.
 *   @param bit   Index of the bit to be returned.
 *
 *   @return      value of the bit in case of success, 
 *                SNET_BITMAP_ERR in case of an error.
 *
 ******************************************************************************/

int SNetUtilBitmapGet(snet_util_bitmap_t *map, unsigned int bit)
{
  if(bit >= map->size) {
    return SNET_BITMAP_ERR;
  }
  
  return (map->bitmap[BYTE_OFFSET(bit)] >> BIT_OFFSET(bit)) & BIT_SET; 
}

/*@}*/
