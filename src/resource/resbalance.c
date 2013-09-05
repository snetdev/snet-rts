#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "resdefs.h"
#include "restopo.h"
#include "resource.h"

/* A client returns all its resources to the server (when it exits). */
void res_release_client(client_t* client)
{
  int           p;
  host_t       *host = res_local_host();

  if (client->local_grantmap) {
    for (p = 0; p < host->nprocs; ++p) {
      if (HAS(client->local_grantmap, p)) {
        proc_t* proc = host->procs[p];
        assert(proc->clientbit == client->bit);
        assert(proc->state >= Grant);
        CLR(client->local_grantmap, p);
        client->local_granted -= 1;
        assert(client->local_granted >= 0);
        if (proc->state >= Accept) {
          client->local_accepted -= 1;
          assert(client->local_accepted >= 0);
        }
        if (proc->state == Revoke) {
          client->local_revoked -= 1;
          assert(client->local_revoked >= 0);
        }
        proc->state = Avail;
        proc->core->assigned -= 1;
        assert(proc->core->assigned >= 0);
        proc->core->cache->assigned -= 1;
        assert(proc->core->cache->assigned >= 0);
        proc->core->cache->numa->assigned -= 1;
        assert(proc->core->cache->numa->assigned >= 0);
        if (client->local_grantmap == 0) {
          break;
        }
      }
    }
  }
  assert(client->local_granted == 0);
  assert(client->local_accepted == 0);
  assert(client->local_revoked == 0);
}

/* A client confirms a previous processor grant. */
int res_accept_procs(client_t* client, intlist_t* ints)
{
  host_t* host = res_local_host();
  const int size = res_list_size(ints);
  int procs_accepted = 0;
  int i;
  for (i = 0; i < size; ++i) {
    int procnum = res_list_get(ints, i);
    if (NOT(client->local_grantmap, procnum)) {
      res_warn("Client accepts ungranted proc %d (%lu).",
                procnum, client->local_grantmap);
      return -1;
    } else {
      proc_t* proc = host->procs[procnum];
      if (proc->state != Grant) {
        res_warn("Client accepts proc %d in state %d.", procnum, proc->state);
        return -1;
      } else {
        proc->state = Accept;
        client->local_accepted += 1;

        ++procs_accepted;
      }
    }
  }
  return procs_accepted;
}

/* A client returns previously granted processors. */
int res_return_procs(client_t* client, intlist_t* ints)
{
  host_t* host = res_local_host();
  const int size = res_list_size(ints);
  int procs_returned = 0;
  int i;
  for (i = 0; i < size; ++i) {
    int procnum = res_list_get(ints, i);
    if (NOT(client->local_grantmap, procnum)) {
      res_warn("Client returns ungranted proc %d (%lu).",
                procnum, client->local_grantmap);
      return -1;
    } else {
      proc_t* proc = host->procs[procnum];
      assert(proc->clientbit == client->bit);
      if (proc->state >= Grant && proc->state <= Revoke) {
        proc->state = Avail;
        client->local_granted -= 1;
        assert(client->local_granted >= 0);
        if (proc->state >= Accept) {
          client->local_accepted -= 1;
          assert(client->local_accepted >= 0);
          if (proc->state == Revoke) {
            client->local_revoked -= 1;
            assert(client->local_revoked >= 0);
          }
        }
        CLR(client->local_grantmap, procnum);

        proc->state = Avail;
        proc->core->assigned -= 1;
        assert(proc->core->assigned >= 0);
        proc->core->cache->assigned -= 1;
        assert(proc->core->cache->assigned >= 0);
        proc->core->cache->numa->assigned -= 1;
        assert(proc->core->cache->numa->assigned >= 0);

        ++procs_returned;
      } else {
        res_warn("Client returns proc %d in state %d.", procnum, proc->state);
        return -1;
      }
    }
  }
  return procs_returned;
}

/* Compare the load of two clients, descending. */
int res_client_compare_local_workload_desc(const void *p, const void *q)
{
  const client_t* P = (const client_t *) p;
  const client_t* Q = (const client_t *) q;
  int A = P->local_workload;
  int B = Q->local_workload;
  int comparison = ((A == B) ? (Q->bit - P->bit) : (A - B));
  return comparison;
}

static client_t** get_sorted_clients(intmap_t* map)
{
  int           i = 0, iter = -1;
  const int     num_clients = res_map_count(map);
  client_t    **all = xmalloc(num_clients * sizeof(client_t *));
  client_t     *client;

  res_map_iter_init(map, &iter);
  while ((client = res_map_iter_next(map, &iter)) != NULL) {
    assert(i < num_clients);
    all[i++] = client;
  }
  qsort(all, num_clients, sizeof(client_t *),
        res_client_compare_local_workload_desc);

  return all;
}

/* Each task can be run on a dedicated core. */
void res_rebalance_cores(intmap_t* map, int ncores, int nprocs)
{
  int           i = 0, p, o;
  host_t       *host = res_local_host();
  const int     num_clients = res_map_count(map);
  client_t     *client, **all = get_sorted_clients(map);
  bitmap_t      revoke = BITMAP_ZERO;
  int           nrevokes = 0;
  bitmap_t      assign = BITMAP_ZERO;
  int           nassigns = 0;
  
  /* Restrict use of hyperthreaded cores to one proc per core. */
  for (i = 0; i < host->nprocs; ++i) {
    core_t *core = host->cores[i];
    if (core->assigned >= 2) {
      int found = 0;
      for (p = 0; i < core->nprocs; ++p) {
        proc_t* proc = core->procs[p];
        if (proc->state == Grant || proc->state == Accept) {
          if (++found >= 2) {
            SET(revoke, proc->clientbit);
            ++nrevokes;
            client = res_map_get(map, proc->clientbit);
            SET(client->local_revoking, p);
            client->local_revoked += 1;
            proc->state = Revoke;
          }
        }
      }
    }
  }

  /* Check all clients for a need for more or less cores. */
  for (i = 0; i < num_clients; ++i) {
    client_t *client = all[i];
    int used = client->local_granted - client->local_revoked;
    int need = client->local_workload - used;
    if (need > 0) {
      /* Find allocatable cores to satisfy the client need. */
      for (o = 0; o < host->ncores && need > 0; ++o) {
        if (NOT(host->coreassign, o)) {
          core_t* core = host->cores[o];
          proc_t* proc = core->procs[0];
          assert(core->assigned == 0);
          assert(proc->state == Avail);
          SET(assign, client->bit);
          ++nassigns;
          SET(client->local_assigning, proc->res->logical);
          client->local_granted += 1;
          proc->state = Grant;
          core->assigned += 1;
          core->cache->assigned += 1;
          core->cache->numa->assigned += 1;
          SET(host->coreassign, o);
        }
      }
    }
  }

  /* Issue revoke commands to the clients. */
  for (i = 0; i < num_clients && nrevokes > 0; ++i) {
    client = all[i];
    if (HAS(revoke, client->bit)) {
      res_client_reply(client, "{ revoke 0 ");
      for (p = 0; p <= MAX_BIT && client->local_revoking; ++p) {
        if (HAS(client->local_revoking, p)) {
          res_client_reply(client, "%d ", p);
          CLR(client->local_revoking, p);
          --nrevokes;
          client->local_revoked += 1;
        }
      }
      res_client_reply(client, "} \n");
    }
    assert(!client->local_revoking);
  }
  assert(!nrevokes);

  /* Communicate newly granted processors. */
  for (i = 0; i < num_clients && nassigns > 0; ++i) {
    client = all[i];
    if (HAS(assign, client->bit)) {
      res_client_reply(client, "{ grant 0 ");
      for (p = 0; p <= MAX_BIT && client->local_assigning; ++p) {
        if (HAS(client->local_assigning, p)) {
          res_client_reply(client, "%d ", p);
          CLR(client->local_assigning, p);
          SET(client->local_grantmap, p);
          --nassigns;
        }
      }
      res_client_reply(client, "} \n");
    }
    assert(!client->local_assigning);
  }
  assert(!nassigns);

  xfree(all);
}

void res_rebalance_procs(intmap_t* map)
{
  int           i = 0, p, o;
  host_t       *host = res_local_host();
  const int     num_clients = res_map_count(map);
  client_t     *client, **all = get_sorted_clients(map);
  bitmap_t      revoke = BITMAP_ZERO;
  int           nrevokes = 0;
  bitmap_t      assign = BITMAP_ZERO;
  int           nassigns = 0;
  
  /* Check all clients for a need for more or less processors. */
  for (i = 0; i < num_clients; ++i) {
    client_t *client = all[i];
    int used = client->local_granted - client->local_revoked;
    int need = client->local_workload - used;
    if (need > 0) {
      /* Find allocatable cores to satisfy the client need. */
      for (o = 0; o < host->ncores && need > 0; ++o) {
        if (NOT(host->coreassign, o)) {
          core_t* core = host->cores[o];
          proc_t* proc = core->procs[0];
          assert(core->assigned == 0);
          assert(proc->state == Avail);
          SET(assign, client->bit);
          ++nassigns;
          SET(client->local_assigning, proc->res->logical);
          client->local_granted += 1;
          proc->state = Grant;
          core->assigned += 1;
          core->cache->assigned += 1;
          core->cache->numa->assigned += 1;
          SET(host->coreassign, o);
        }
      }
    }
  }

  /* Issue revoke commands to the clients. */
  for (i = 0; i < num_clients && nrevokes > 0; ++i) {
    client = all[i];
    if (HAS(revoke, client->bit)) {
      res_client_reply(client, "{ revoke 0 ");
      for (p = 0; p <= MAX_BIT && client->local_revoking; ++p) {
        if (HAS(client->local_revoking, p)) {
          res_client_reply(client, "%d ", p);
          CLR(client->local_revoking, p);
          --nrevokes;
          client->local_revoked += 1;
        }
      }
      res_client_reply(client, "} \n");
    }
    assert(!client->local_revoking);
  }
  assert(!nrevokes);

  /* Communicate newly granted processors. */
  for (i = 0; i < num_clients && nassigns > 0; ++i) {
    client = all[i];
    if (HAS(assign, client->bit)) {
      res_client_reply(client, "{ grant 0 ");
      for (p = 0; p <= MAX_BIT && client->local_assigning; ++p) {
        if (HAS(client->local_assigning, p)) {
          res_client_reply(client, "%d ", p);
          CLR(client->local_assigning, p);
          SET(client->local_grantmap, p);
          --nassigns;
        }
      }
      res_client_reply(client, "} \n");
    }
    assert(!client->local_assigning);
  }
  assert(!nassigns);

  xfree(all);
}

void res_rebalance_proportional(intmap_t* map)
{
}

void res_rebalance_minimal(intmap_t* map)
{
}

void res_rebalance(intmap_t* map)
{
  client_t* client;
  int num_clients = 0;
  int total_load = 0;
  int nprocs = res_local_procs();
  int ncores = res_local_cores();
  intmap_iter_t iter;

  res_map_iter_init(map, &iter);
  while ((client = res_map_iter_next(map, &iter)) != NULL) {
    if (client->local_workload >= 1) {
      ++num_clients;
      total_load += client->local_workload;
    }
  }

  printf("%s: tl %d, nc %d, np %d\n", __func__,
         total_load, ncores, nprocs);

  if (total_load <= ncores) {
    res_rebalance_cores(map, ncores, nprocs);
  }
  else if (total_load <= nprocs) {
    res_rebalance_procs(map);
  }
  else if (num_clients < nprocs) {
    res_rebalance_proportional(map);
  }
  else {
    res_rebalance_minimal(map);
  }
}

