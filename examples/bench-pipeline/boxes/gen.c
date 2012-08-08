#include <C4SNet.h>

/* 
 * gen: ((n,w,c) -> (n,w,c))
 * outputs n records, each with an output value of c between 0 and the input value of c.
 */
void *gen( void *hnd , c4snet_data_t *n, c4snet_data_t *w, c4snet_data_t *c)
{
    int int_n = *(int *) C4SNetGetData(n);
    int int_w = *(int *) C4SNetGetData(w);
    int int_c = *(int *) C4SNetGetData(c);
    
    C4SNetFree(n);
    C4SNetFree(w);
    C4SNetFree(c);
    
    int r = 0;
    
    for (int i = 0; i < int_n; ++i, ++r)
    {
        r = r % int_c;
        C4SNetOut(hnd, 1, 
                  C4SNetCreate(CTYPE_int, 1, &int_n), 
                  C4SNetCreate(CTYPE_int, 1, &int_w), 
                  C4SNetCreate(CTYPE_int, 1, &r));
    }
    
    return hnd;
}

/* 
 * gent: ((<n>,<w>,<c>) -> (<n>,<w>,<c>))
 * idem gen, with tags
 */
void *gent( void *hnd , int n, int w, int c)
{
    int r;
    for (int i = 0; i < n; ++i, ++r)
    {
        r = r % c;
        C4SNetOut(hnd, 1, n, w, r);
    }

    return hnd;
}

/* 
 * genxt: ((<n>,<w>,<c>,<p>) -> (<n>,<w>,<c>,<xN>) | ....)
 * generate n * p records
 */
void *genxt( void *hnd , int n, int w, int c, int p)
{
    int r = 0, v = 0;
    for (int i = 0; i < n; ++i, ++r)
    {
        r = r % c;

        for (int v = 0; v < p; ++v)
            C4SNetOut(hnd, 1 + v, n, w, r, 1);
    }

    return hnd;
}
