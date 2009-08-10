
static int SNet__ts00__A(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_a = NULL;

  rec = SNetHndGetRecord(hnd);

  field_a = SNetRecTakeField(rec, F__ts00__a);

  return SNetCall__A(hnd, field_a);
}


#define SNET_BOX_SERVICE(name)                            \
    DISTRIBUTABLE_THREAD(name)(                           \
            dutc_db db, dutc_global(snet_handle_t *) hnd) \
    {                                                     \
        GINOUT(hnd);                                      \
    }                                                     \
    ASSOCIATE_THREAD(name)                                \
                                                          \
    thread void name(snet_handle_t *__snet__hnd__) { }    \
    
    
/*---*/

#define SNET_BOX_IMPL(name)                               \
    SNET_SANE_SERVICE(OCR);                               \
                                                          \
    thread void name(snet_handle_t *__snet__hnd__)        \
    {                                                     \
        snet_record_t *__snet__rec__ =                    \
            SNetHndGetRecord(__snet__hnd__)               \
                                                          \
        if (1) 

/*---*/

#define SNET_recget_tag(name) \
    int name = SNetRecGetTag(__snet__rec__);

#define SNET_recget_btag(name, id) \
    int name = SNetRecGetBTag(__snet__rec__);

#define SNET_recget_field(name, id) \
    void *name = SNetRecGetField(__snet__rec__);
       
/*----------------------------------------------------------------------------*/

#define SNET_BOX(name) int SNetCall__##name

/*---*/

#define SNET_recsettag(name

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* .h */

SNET_BOX_DECL(OCR)(
    snet_handle_t *hnd,
    int ta, int tb, int tc, 
    void *fa, void *fb, void *fc);

/*----------------------------------------------------------------------------*/
/* .imp.utc */

SNET_BOX_CALL(OCR)(
    snet_handle_t *hnd,
    int ta, int tb, int tc, 
    void *fa, void *fb, void *fc)
{
    SNET_recset_tag(hnd, ta);
    SNET_recset_tag(hnd, tb);
    SNET_recset_tag(hnd, tc);

    SNET_recset_field(hnd, fa);
    SNET_recset_field(hnd, fb);
    SNET_recset_field(hnd, fc);

    SNET_create_box(hnd);
}

/* .imp.utc */
SNET_BOX_EXPORT(OCR);

/*----------------------------------------------------------------------------*/
/* .utc */

SNET_BOX_IMPL(OCR)
{
    SNET_rec_tag(ta);
    SNET_rec_tag(tb);
    SNET_rec_tag(tc);

    SNET_rec_field(fa);
    SNET_rec_field(fb);
    SNET_rec_field(fc);

    /**
     * Do whatever with ta, tb, tc,
     * fa, fb and fc!!
     */
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

