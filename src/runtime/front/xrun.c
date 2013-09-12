#include "node.h"

void *SNetNodeThreadStart(void *arg)
{
  worker_t      *worker = (worker_t *) arg;

  trace(__func__);

  SNetThreadSetSelf(worker);
  if (worker->role == InputManager) {
    SNetInputManagerRun(worker);
  } else {
    SNetWorkerRun(worker);
  }

  return arg;
}

/* Create workers and start them. */
void SNetNodeRun(snet_stream_t *input, snet_info_t *info, snet_stream_t *output)
{
  int                   id, mg, fildes[2], pipe_recv, stopped = 0;
  const int             num_workers = SNetThreadingWorkers();
  const int             num_managers = SNetThreadedManagers();
  const int             max_worker = SNetGetMaxProcs() + num_managers;
  int                   total_workers = num_workers + num_managers;
  worker_config_t      *config = SNetNewAlign(worker_config_t);

  trace(__func__);
  assert(input->dest);
  assert(output->from);
  assert(max_worker >= total_workers);

  if (pipe(fildes)) {
    SNetUtilDebugFatal("no pipe");
  } else {

    /* Allocate globally shared worker configuration. */
    config->worker_count = total_workers;
    config->workers = SNetNewAlignN(1 + max_worker, worker_t*);
    config->thief_limit = SNetThreadingThieves();
    if (config->thief_limit > 1) {
      LOCK_INIT2(config->idle_lock, config->thief_limit);
    } else {
      LOCK_INIT(config->idle_lock);
    }
    config->pipe_send = fildes[1];
    pipe_recv = fildes[0];

    /* Set all workers to NULL to prevent premature stealing. */
    for (id = 0; id <= max_worker; ++id) {
      config->workers[id] = NULL;
    }

    /* Create managers with new threads. */
    for (mg = 1; mg <= num_managers; ++mg) {
      id = mg + num_workers;
      config->workers[id] =
        SNetWorkerCreate(input->from, id, output->dest, InputManager);
      SNetThreadCreate(SNetNodeThreadStart, config->workers[id]);
    }
    /* Create data workers with new threads. */
    for (id = 1; id <= num_workers; ++id) {
      config->workers[id] =
        SNetWorkerCreate(input->from, id, output->dest, DataWorker);
      SNetThreadCreate(SNetNodeThreadStart, config->workers[id]);
    }
    /* Reuse main thread for worker with ID 1. */
    config->workers[1] =
      SNetWorkerCreate(input->from, 1, output->dest, DataWorker);
    SNetNodeThreadStart(config->workers[1]);
  }
}

void SNetNodeCleanup(void)
{
  int i;

  SNetNodeTableCleanup();
  SNetWorkerCleanup();
  for (i = snet_worker_count; i > 0; --i) {
    SNetWorkerDestroy(snet_workers[i]);
  }
  SNetDeleteN(snet_worker_count + 1, snet_workers);
  snet_worker_count = 0;
  snet_workers = NULL;
}

