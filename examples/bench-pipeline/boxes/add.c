#include <C4SNet.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int init = 0;
int use_rand = 0;
int randfact = 0;

static int max_bound(int);

void *add( void *hnd, c4snet_data_t *n, c4snet_data_t *w)
{
  int int_n = *(int *) C4SNetGetData(n);
  int int_w = *(int *) C4SNetGetData(w);

  C4SNetFree(n);
  C4SNetFree(w);

  int workload = max_bound(int_w);
  
  int i, s;
  for (s = int_n, i = 0; i <= workload; ++i)
  {
         s += int_n;
        __asm__ __volatile__ ("");
  }

  C4SNetOut(hnd, 1, C4SNetCreate(CTYPE_int, 1, &int_n), C4SNetCreate(CTYPE_int, 1, &int_w));

  return hnd;
}

void *addt( void *hnd, int n, int w)
{
  int i, s;

  int workload = max_bound(w);

  for (s = n, i = 0; i <= workload; ++i)
  {
         s += n;
        __asm__ __volatile__ ("");
  }

  C4SNetOut(hnd, 1, n, w);

  return hnd;
}



static int max_bound(int n)
{
    if (init == 0)
    {
        char *p = getenv("WORKLOAD_FUZZINESS");
        if (p != NULL)
        {
            use_rand = 1;
            randfact = atoi(p);
            randfact = (randfact < 0) ? 0 : randfact;
            randfact = (randfact > 100) ? 100 : randfact;

            srand(time(NULL));
        }
        init = 1;
    }
    if (use_rand)
    {
        float fuzzyness = n * randfact / 100.;
        
        int r = rand();
        int m = (float)r * fuzzyness / (float)RAND_MAX;

        return n - m;
    }
    else
        return n;
}

