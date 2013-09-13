#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include "node.h"

int SNetGetWorkerId(void)
{
  worker_t* worker = SNetThreadGetSelf();
  return worker ? worker->id : 0;
}

typedef enum pipe_mesg_type {
  MesgDone = 1,
} pipe_mesg_type_t;

typedef struct pipe_mesg {
  int   type;
  int   id;
} pipe_mesg_t;

static void SNetPipeSend(int fd, const pipe_mesg_t* mesg)
{
  int out = write(fd, mesg, sizeof(pipe_mesg_t));
  if (out != sizeof(pipe_mesg_t)) {
    SNetUtilDebugFatal("[%s.%d]: Write pipe returns %d instead of %d bytes: %s\n",
                       __func__, SNetDistribGetNodeId(), out,
                       (int) sizeof(mesg), strerror(errno));
  }
}

static void SNetPipeReceive(int fd, pipe_mesg_t* mesg)
{
  int in = read(fd, mesg, sizeof(pipe_mesg_t));
  if (in != sizeof(pipe_mesg_t)) {
    SNetUtilDebugFatal("[%s.%d]: Read pipe returns %d instead of %d bytes: %s\n",
                       __func__, SNetDistribGetNodeId(), in,
                       (int) sizeof(mesg), strerror(errno));
  }
}

void *SNetNodeThreadStart(void *arg)
{
  worker_t     *worker = (worker_t *) arg;
  pipe_mesg_t   mesg;

  SNetThreadSetSelf(worker);
  if (worker->role == InputManager) {
    SNetInputManagerRun(worker);
  } else {
    SNetWorkerRun(worker);
  }

  mesg.type = MesgDone;
  mesg.id = worker->id;
  SNetPipeSend(worker->config->pipe_send, &mesg);

  return arg;
}

/* Create workers and start them. */
void SNetNodeRun(snet_stream_t *input, snet_info_t *info, snet_stream_t *output)
{
  int                   id, mg, fildes[2], pipe_recv;
  int                   created = 0, destroyed = 0;
  const int             num_workers = SNetThreadingWorkers();
  const int             num_managers = SNetThreadedManagers();
  int                   total_workers = num_workers + num_managers;
  const int             worker_limit = SNetGetMaxProcs() + num_managers;
  const int             max_worker = MAX(worker_limit, num_workers + num_managers);

  total_workers = MAX(total_workers, max_worker);
  trace(__func__);
  assert(input->dest);
  assert(output->from);

  if (pipe(fildes)) {
    SNetUtilDebugFatal("no pipe: %s", strerror(errno));
  } else {
    worker_config_t *config = SNetNewAlign(worker_config_t);

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
        SNetWorkerCreate(id, input->from, output->dest, InputManager, config);
      SNetThreadCreate(SNetNodeThreadStart, config->workers[id]);
      ++created;
    }
    /* Create data workers with new threads. */
    for (id = 1; id <= num_workers; ++id) {
      config->workers[id] =
        SNetWorkerCreate(id, input->from, output->dest, DataWorker, config);
      SNetThreadCreate(SNetNodeThreadStart, config->workers[id]);
      ++created;
    }

    while (destroyed < created) {
      pipe_mesg_t mesg;
      SNetPipeReceive(pipe_recv, &mesg);
      switch (mesg.type) {
        case MesgDone: {
          ++destroyed;
          if (SNetDebugTL()) {
            printf("[%s:%d]: worker %d done.\n", __func__, SNetDistribGetNodeId(), mesg.id);
          }
        } break;
        default: SNetUtilDebugFatal("[%s.%s]: Invalid message type %d", __func__,
                                    SNetDistribGetNodeId(), mesg.type);
      }
    }
  }
}

