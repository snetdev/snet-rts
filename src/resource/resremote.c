#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "resdefs.h"
#include "restopo.h"
#include "resource.h"
#include "resclient.h"

/* Return all remote client resources to the server (on client exit). */
void res_release_client_remote(client_t* client)
{
  int           p, r;

  for (r = 1; r < client->nremotes; ++r) {
    remote_t* remote = &client->remotes[r];
    if (remote->grantmap) {
      host_t* host = res_topo_get_host(r);
      for (p = 0; p < host->nprocs; ++p) {
        if (HAS(remote->grantmap, p)) {
          proc_t* proc = host->procs[p];
          assert(proc->clientbit == client->bit);
          assert(proc->state >= ProcGrant);
          CLR(remote->grantmap, p);
          proc->state = ProcAvail;
          proc->core->assigned -= 1;
          assert(proc->core->assigned >= 0);
          proc->core->cache->assigned -= 1;
          assert(proc->core->cache->assigned >= 0);
          proc->core->cache->numa->assigned -= 1;
          assert(proc->core->cache->numa->assigned >= 0);
          CLR(host->procassign, p);
          if (proc->core->assigned == 0) {
            CLR(host->coreassign, proc->core->logical);
          }
          if (remote->grantmap == 0) {
            break;
          }
        }
      }
    }
  }
  client->remote_granted = 0;
  client->remote_accepted = 0;
  client->remote_revoked = 0;
}

/* A client confirms a previous remote processor grant. */
int res_accept_procs_remote(client_t* client, intlist_t* ints)
{
  const int size = res_list_size(ints);
  if (size < 2) {
    res_warn("Client accept list invalid length %d.\n", size);
    return -1;
  } else {
    const int sysid = res_list_get(ints, 0);
    if (sysid <= 0) {
      res_warn("Client remote accept list for invalid system %d.\n", sysid);
      return -1;
    } else {
      remote_t* remote = &client->remotes[sysid];
      host_t* host = res_topo_get_host(sysid);
      int i, procs_accepted = 0;

      for (i = 1; i < size; ++i) {
        int procnum = res_list_get(ints, i);
        if (NOT(remote->grantmap, procnum)) {
          res_warn("Client accepts ungranted proc %d (%lu).\n",
                    procnum, remote->grantmap);
          return -1;
        } else {
          proc_t* proc = host->procs[procnum];
          if (proc->state != ProcGrant && proc->state != ProcRevoke) {
            res_warn("Client accepts proc %d in state %d.\n", procnum, proc->state);
            return -1;
          } else {
            assert(proc->clientbit == client->bit);
            if (proc->state == ProcGrant) {
              proc->state = ProcAccept;
            }
            client->remote_accepted += 1;
            ++procs_accepted;
          }
        }
      }
      return procs_accepted;
    }
  }
}

/* A client returns previously granted processors. */
int res_return_procs_remote(client_t* client, intlist_t* ints)
{
  const int size = res_list_size(ints);
  if (size < 2) {
    res_warn("Client return list invalid length %d.\n", size);
    return -1;
  } else {
    const int sysid = res_list_get(ints, 0);
    if (sysid != 0) {
      res_warn("Client return list for invalid system %d.\n", sysid);
      return -1;
    } else {
      remote_t* remote = &client->remotes[sysid];
      host_t* host = res_topo_get_host(sysid);
      int i, procs_returned = 0;

      for (i = 1; i < size; ++i) {
        int procnum = res_list_get(ints, i);
        if (NOT(remote->grantmap, procnum)) {
          res_warn("Client returns ungranted proc %d (%lu).\n",
                    procnum, remote->grantmap);
          return -1;
        } else {
          proc_t* proc = host->procs[procnum];
          assert(proc->clientbit == client->bit);
          if (proc->state < ProcGrant && proc->state > ProcRevoke) {
            res_warn("Client returns proc %d in state %d.\n", procnum, proc->state);
            return -1;
          } else {
            client->remote_granted -= 1;
            assert(client->remote_granted >= 0);
            if (proc->state >= ProcAccept) {
              client->remote_accepted -= 1;
              assert(client->remote_accepted >= 0);
              if (proc->state == ProcRevoke) {
                client->remote_revoked -= 1;
                assert(client->remote_revoked >= 0);
              }
            }
            CLR(remote->grantmap, procnum);

            proc->state = ProcAvail;
            proc->core->assigned -= 1;
            assert(proc->core->assigned >= 0);
            proc->core->cache->assigned -= 1;
            assert(proc->core->cache->assigned >= 0);
            proc->core->cache->numa->assigned -= 1;
            assert(proc->core->cache->numa->assigned >= 0);

            CLR(host->procassign, procnum);
            if (proc->core->assigned == 0) {
              CLR(host->coreassign, proc->core->logical);
            }

            ++procs_returned;
          }
        }
      }
      if (procs_returned) {
        client->rebalance_remote = true;
      }
      return procs_returned;
    }
  }
}

static double inline res_prio(const client_t* p)
{
  return (p && p->remote_workload > 0)
    ? ((p->remote_workload - p->remote_granted) / p->remote_workload)
    : -1e9;
}

/* Compare the unfulfilled load of two clients, descending. */
int res_client_compare_remote(const void *p, const void *q)
{
  const client_t* P = * (client_t * const *) p;
  const client_t* Q = * (client_t * const *) q;
  double delta = res_prio(P) - res_prio(Q);
  return (delta < 0) ? -1 : (delta > 0) ? 1 : 0;
}

static client_t** get_sorted_remote_clients(intmap_t* map, int* count)
{
  int           n = 0, k;
  const int     num_clients = res_map_count(map);
  client_t    **all = xcalloc(num_clients, sizeof(client_t *));
  client_t     *client;
  intmap_iter_t iter;

  res_map_iter_init(map, &iter);
  while ((client = (client_t *) res_map_iter_next(map, &iter)) != NULL) {
    if (client->remote_workload >= 1) {
      if (client->remote_workload > client->remote_granted) {
        assert(n < num_clients);
        all[n++] = client;
      }
    }
  }
  for (k = n; k < num_clients; ++k) {
    all[k] = NULL;
  }
  if (n >= 2) {
    qsort(all, n, sizeof(client_t *), res_client_compare_remote);
  }

  *count = n;
  return all;
}

typedef struct alloc alloc_t;
struct alloc {
  client_t     *client;
  int           sysid;
  int           proc;
};

static int res_compare_alloc(const void *p, const void *q)
{
  const alloc_t* P = * (alloc_t * const *) p;
  const alloc_t* Q = * (alloc_t * const *) q;

  return (P < Q) ? -1 :
         (Q > P) ? 1 :
         (P->sysid != Q->sysid) ? (P->sysid - Q->sysid) :
         (P->proc - Q->proc);
}

void res_rebalance_remote(intmap_t* map)
{
  const int     nhosts = res_remote_hosts();
  int           c, i, r, nall = 0;
  client_t    **all = get_sorted_remote_clients(map, &nall);
  alloc_t      *allocs = NULL;
  int           num_allocs = 0, max_allocs = 0, old_allocs;
  bitmap_t      assign = BITMAP_ZERO;
  int           nassigns = 0;

  while (nall > 0 && nhosts > 0) {
    for (i = 0; i < nall; ++i) {
      client_t* client = all[i];
      if (res_prio(client) > 0) {
        for (r = 1; r < nhosts; r++) {
          if (HAS(client->access, r)) {
            host_t* host = res_topo_get_host(r);
            if (host) {
              int uc = res_host_unassigned_cores(host);
              if (uc > 0) {
                for (c = 0; c < host->ncores; ++c) {
                  if (NOT(host->coreassign, c)) {
                    core_t* core = host->cores[c];
                    proc_t* proc = core->procs[0];
                    assert(core->assigned == 0);
                    assert(proc->state == ProcAvail);
                    SET(assign, client->bit);
                    ++nassigns;
                    proc->state = ProcGrant;
                    core->assigned += 1;
                    core->cache->assigned += 1;
                    core->cache->numa->assigned += 1;
                    proc->clientbit = client->bit;
                    SET(host->coreassign, c);
                    SET(host->procassign, proc->logical);
                    if (num_allocs >= max_allocs) {
                      max_allocs = (max_allocs > 2) ? (3*max_allocs/2) : 4;
                      allocs = xrealloc(allocs, max_allocs * sizeof(*allocs));
                    }
                    allocs[num_allocs].client = client;
                    allocs[num_allocs].sysid = r;
                    allocs[num_allocs].proc = proc->logical;
                    num_allocs += 1;
                    goto redo;
                  }
                }
              }
            }
          }
        }
      }
    }
redo:
    if (old_allocs < num_allocs) {
      old_allocs = num_allocs;
      nall = 0;
      xfree(all);
      all = get_sorted_remote_clients(map, &nall);
    } else {
      break;
    }
  }

  xfree(all);

  qsort(allocs, num_allocs, sizeof(*allocs), res_compare_alloc);

  for (i = 0; i < num_allocs; ++i) {
    if (i == 0 || allocs[i].client != allocs[i - 1].client ||
        allocs[i].sysid != allocs[i - 1].sysid)
    {
      res_client_reply(allocs[i].client, "{ grant %d ", allocs[i].sysid);
    }
    res_client_reply(allocs[i].client, "%d ", allocs[i].proc);
    if (i + 1 >= num_allocs || allocs[i].client != allocs[i + 1].client ||
        allocs[i].sysid != allocs[i + 1].sysid)
    {
      res_client_reply(allocs[i].client, "} \n");
    }
  }
}

