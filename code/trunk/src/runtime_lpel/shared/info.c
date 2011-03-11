#include <pthread.h>
#include "info.h"
#include "bool.h"
#include "memfun.h"

static snet_info_tag_t new_tag = 0;
static bool mutex_initialized = false;
static pthread_mutex_t info_mutex;

struct snet_info {
    snet_info_tag_t tag;
    void *data;
    struct snet_info *next;
};

snet_info_tag_t SNetInfoCreateTag()
{
    int result;
    pthread_mutex_lock(&info_mutex);
    result = new_tag++;
    pthread_mutex_unlock(&info_mutex);
    return result;
}

snet_info_t *SNetInfoInit()
{
  if (!mutex_initialized) {
  //FIXME: error handling?
    pthread_mutex_init(&info_mutex, NULL);
    mutex_initialized = true;
  }

  snet_info_t *new = SNetMemAlloc(sizeof(*new));
  new->tag = -1;
  new->data = NULL;
  new->next = NULL;
  return new;
}

void SNetInfoSetTag(snet_info_t *info, snet_info_tag_t tag, void *data)
{
  pthread_mutex_lock(&info_mutex);
  while (info->tag != tag && info->tag != -1) {
      info = info->next;
  }

  if (info->tag == tag) {
      info->data = data;
  } else if (info->tag == -1) {
      info->tag = tag;
      info->data = data;
      info->next = SNetInfoInit();
  }
  pthread_mutex_unlock(&info_mutex);
}

void *SNetInfoDelTag(snet_info_t *info, snet_info_tag_t tag)
{
  void *result;
  snet_info_t *tmp;

  pthread_mutex_lock(&info_mutex);

  while (info->next != NULL && info->next->tag != tag) {
      info = info->next;
  }

  if (info->next == NULL) return NULL;

  tmp = info->next;
  info->next = tmp->next;
  result = tmp->next;
  SNetMemFree(tmp);
  pthread_mutex_unlock(&info_mutex);
  return result;
}

void *SNetInfoGetTag(snet_info_t *info, snet_info_tag_t tag)
{
  void *result = NULL;
  pthread_mutex_lock(&info_mutex);
  while (info->tag != tag && info->tag != -1) {
      info = info->next;
  }

  if (info->tag == tag) result = info->data;
  pthread_mutex_unlock(&info_mutex);
  return result;
}

snet_info_t *SNetInfoCopy(snet_info_t *info) {
    snet_info_t *tmp, *result;
    pthread_mutex_lock(&info_mutex);
    tmp = result = SNetMemAlloc(sizeof(snet_info_t));
    do {
        tmp->tag = info->tag;
        tmp->data = info->data;
        tmp->next = info->next ? SNetMemAlloc(sizeof(snet_info_t)) : NULL;
        tmp = tmp->next;
        info = info->next;
    } while (info != NULL);
    pthread_mutex_unlock(&info_mutex);
    return result;
}

void SNetInfoDestroy(snet_info_t *info)
{
  pthread_mutex_lock(&info_mutex);
  snet_info_t *tmp;
  while (info != NULL) {
    tmp = info->next;
    SNetMemFree(info);
    info = tmp;
  }
  pthread_mutex_unlock(&info_mutex);
}
