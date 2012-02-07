#ifndef _SNET_BITMAP_H_
#define _SNET_BITMAP_H_

#define SNET_BITMAP_ERR -1

typedef struct snet_util_bitmap snet_util_bitmap_t;

snet_util_bitmap_t *SNetUtilBitmapCreate(int size);

int SNetUtilBitmapCopy(snet_util_bitmap_t *from, snet_util_bitmap_t *to);

void SNetUtilBitmapDestroy(snet_util_bitmap_t *map);

int SNetUtilBitmapSize(snet_util_bitmap_t *map);

int SNetUtilBitmapSet(snet_util_bitmap_t *map, unsigned int bit);

int SNetUtilBitmapClear(snet_util_bitmap_t *map, unsigned int bit);

int SNetUtilBitmapFindNSet(snet_util_bitmap_t *map);

int SNetUtilBitmapGet(snet_util_bitmap_t *map, unsigned int bit);

#endif /* _SNET_BITMAP_H_ */
