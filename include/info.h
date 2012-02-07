#ifndef _SNET_INFO_H_
#define _SNET_INFO_H_

#include <stdint.h>
#include "bool.h"

typedef int snet_info_tag_t;
typedef struct snet_info snet_info_t;

snet_info_tag_t SNetInfoCreateTag();
snet_info_t *SNetInfoInit();
void SNetInfoSetTag(snet_info_t *info, snet_info_tag_t tag, uintptr_t data,
                   void *(*copyFun)(void *));
uintptr_t SNetInfoGetTag(snet_info_t *info, snet_info_tag_t tag);
uintptr_t SNetInfoDelTag(snet_info_t *info, snet_info_tag_t tag);
snet_info_t *SNetInfoCopy(snet_info_t *info);
void SNetInfoDestroy(snet_info_t *info);
#endif /* _SNET_INFO_H_H */
