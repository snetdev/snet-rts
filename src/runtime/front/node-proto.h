/* node.c */

worker_t **SNetNodeGetWorkers(void);
int SNetNodeGetWorkerCount(void);

/* Assign a new index to a stream and add it to the stream table. */
void SNetNodeTableAdd(snet_stream_t *stream);

/* Retrieve the node table index from a node structure. */
snet_stream_t* SNetNodeTableIndex(int table_index);

/* Cleanup the node table */
void SNetNodeTableCleanup(void);

/* Create a new node and connect its in/output streams. */
node_t *SNetNodeNew(
  node_type_t type,
  int location,
  snet_stream_t **ins,
  int num_ins,
  snet_stream_t **outs,
  int num_outs,
  node_work_fun_t work,
  node_stop_fun_t stop,
  node_term_fun_t term);
void SNetNodeStop(worker_t *worker);
void *SNetNodeThreadStart(void *arg);

/* Create workers and start them. */
void SNetNodeRun(snet_stream_t *input, snet_info_t *info, snet_stream_t *output);
void SNetNodeCleanup(void);

/* Create a new stream emmanating from this node. */
snet_stream_t *SNetNodeStreamCreate(node_t *node);
const char* SNetNodeTypeName(node_type_t type);
const char* SNetNodeName(node_t *node);

/* xbox.c */


/* Process one record. */
void SNetNodeBox(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a box landing. */
void SNetTermBox(landing_t *land, fifo_t *fifo);

/* Destroy a box node. */
void SNetStopBox(node_t *node, fifo_t *fifo);

/* Create a box node. */
snet_stream_t *SNetBox( snet_stream_t *input,
                        snet_info_t *info,
                        int location,
                        const char *boxname,
                        snet_box_fun_t boxfun,
                        snet_exerealm_create_fun_t exerealm_create,
                        snet_exerealm_update_fun_t exerealm_update,
                        snet_exerealm_destroy_fun_t exerealm_destroy,
                        snet_int_list_list_t *output_variants);

/* xcoll.c */


/* Collect output records and forward */
void SNetNodeCollector(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a collector landing. */
void SNetTermCollector(landing_t *land, fifo_t *fifo);

/* Destroy a collector node. */
void SNetStopCollector(node_t *node, fifo_t *fifo);

/* Create a dynamic collector */
snet_stream_t *SNetCollectorDynamic(
    snet_stream_t *instream,
    int location,
    snet_info_t *info,
    bool is_det,
    node_t *peer);

/* Register a new stream with the dynamic collector */
void SNetCollectorAddStream(
    node_t *node,
    snet_stream_t *stream);
snet_stream_t *SNetCollectorStatic(
    int num,
    snet_stream_t **instreams,
    int location,
    snet_info_t *info,
    bool is_det,
    node_t *peer);

/* xdet.c */

int SNetDetGetLevel(void);
void SNetDetIncrLevel(void);
void SNetDetDecrLevel(void);

/* Set determinism level and return the previous value. */
int SNetDetSwapLevel(int level);

/* Copy the stack of detref references from one record to another. */
void SNetRecDetrefCopy(snet_record_t *new_rec, snet_record_t *old_rec);

/* Destroy the stack of detrefs for a record. */
void SNetRecDetrefDestroy(snet_record_t *rec, snet_stream_desc_t **desc_ptr);

/* Create and initialize a new detref structure. */
detref_t* SNetRecDetrefCreate(snet_record_t *rec, long seqnr, landing_t *leave);

/* Add a sequence number to a record to support determinism. */
void SNetRecDetrefAdd(
    snet_record_t *rec,
    long seqnr,
    landing_t *leave,
    fifo_t *fifo);

/* Record needs support for determinism when entering a network. */
void SNetDetEnter(
    snet_record_t *rec,
    landing_detenter_t *land,
    bool is_det,
    snet_entity_t *ent);

/* Examine a REC_detref for remaining de-references from a remote location. */
void SNetDetLeaveCheckDetref(snet_record_t *rec, fifo_t *fifo);

/* Output queued records if allowed, while preserving determinism. */
void SNetDetLeaveDequeue(landing_t *landing);

/* Record leaves a deterministic network */
void SNetDetLeaveRec(snet_record_t *rec, landing_t *landing);

/* xdistrib.c */

void hello(const char *s);

/* Initialize distributed computing. Called by SNetInRun in networkinterface.c. */
void SNetDistribInit(int argc, char **argv, snet_info_t *info);

/* Start input+output managers. Called by SNetInRun in networkinterface.c. */
void SNetDistribStart(void);

/* Cause dist layer to terminate. */
void SNetDistribStop(void);

/* Dummy function needed for networkinterface.c */
snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *in, int loc);
void SNetRecDetrefStackSerialise(snet_record_t *rec, void *buf);
void SNetRecDetrefRecSerialise(snet_record_t *rec, void *buf);
void SNetRecDetrefStackDeserialise(snet_record_t *rec, void *buf);
snet_record_t* SNetRecDetrefRecDeserialise(void *buf);

/* xdripback.c */


/* DripBack process a record. */
void SNetNodeDripBack(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate any kind of dripback landing. */
void SNetTermDripBack(landing_t *land, fifo_t *fifo);

/* Destroy a dripback node. */
void SNetStopDripBack(node_t *node, fifo_t *fifo);

/* DripBack creation function */
snet_stream_t *SNetDripBack(
    snet_stream_t       *input,
    snet_info_t         *info,
    int                  location,
    snet_variant_list_t *back_patterns,
    snet_expr_list_t    *guards,
    snet_startup_fun_t   box_a);

/* xfeedback.c */


/* Test if a record matches at least one of the loopback conditions.
 * This is executed for records from the backloop and from outside. */
bool SNetFeedbackMatch(
    snet_record_t       *rec,
    snet_variant_list_t *back_patterns,
    snet_expr_list_t    *guards);

/* A record arriving via the loopback now leaves the feedback network.
 * Remove the detref structure and check for termination conditions. */
void SNetFeedbackLeave(snet_record_t *rec, landing_t *landing, fifo_t *detfifo);

/* Feedback forward a record. */
void SNetNodeFeedback(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate any kind of feedback landing. */
void SNetTermFeedback(landing_t *land, fifo_t *fifo);

/* Destroy a feedback node. */
void SNetStopFeedback(node_t *node, fifo_t *fifo);

/* Feedback creation function */
snet_stream_t *SNetFeedback(
    snet_stream_t       *input,
    snet_info_t         *info,
    int                  location,
    snet_variant_list_t *back_patterns,
    snet_expr_list_t    *guards,
    snet_startup_fun_t   box_a);

/* xfifo.c */


/* Initialize FIFO with a node with an empty item pointer. */
void SNetFifoInit(fifo_t *fifo);

/* Create a new FIFO */
fifo_t *SNetFifoCreate(void);

/* Cleanup a FIFO. */
void SNetFifoDone(fifo_t *fifo);

/* Delete a FIFO. */
void SNetFifoDestroy(fifo_t *fifo);

/* Append a new item at the tail of a FIFO. */
void SNetFifoPut(fifo_t *fifo, void *item);

/* Retrieve an item from the head of a FIFO, or NULL if empty. */
void *SNetFifoGet(fifo_t *fifo);

/* A fused send/recv if caller owns both sides. */
void *SNetFifoPutGet(fifo_t *fifo, void *item);

/* Peek non-destructively at the first FIFO item. */
void *SNetFifoPeekFirst(fifo_t *fifo);

/* Peek non-destructively at the last FIFO item. */
void *SNetFifoPeekLast(fifo_t *fifo);

/* Return true iff FIFO is empty. */
bool SNetFifoTestEmpty(fifo_t *fifo);

/* Return true iff FIFO is non-empty. */
bool SNetFifoNonEmpty(fifo_t *fifo);

/* Detach the tail part of a FIFO queue from a fifo structure. */
fifo_node_t *SNetFifoGetTail(fifo_t *fifo, fifo_node_t **tail_ptr);

/* Attach a tail of a FIFO queue to an existing fifo structure. */
void SNetFifoPutTail(fifo_t *fifo, fifo_node_t *node, fifo_node_t *tail);

/* xfilter.c */

snet_filter_instr_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...);
void SNetDestroyFilterInstruction( snet_filter_instr_t *instr);

/**
 * Filter task
 */
void SNetNodeFilter(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a filter landing. */
void SNetTermFilter(landing_t *land, fifo_t *fifo);

/* Destroy a filter node. */
void SNetStopFilter(node_t *node, fifo_t *fifo);

/**
 * Filter creation function
 */
snet_stream_t* SNetFilter( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs, ...);

/**
 * Translate creation function
 */
snet_stream_t* SNetTranslate( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs, ...);

/* xgarbage.c */


/* Terminate a garbage landing: decrease references to subsequent resources. */
void SNetTermGarbage(landing_t *land, fifo_t *fifo);

/* Change an identity landing into a garbage landing. */
void SNetBecomeGarbage(landing_t *land);

/* xhashptr.c */


/* Create a new hash table for pointer hashing of 2^power size. */
struct hash_ptab* SNetHashPtrTabCreate(size_t power, bool auto_resize);

/* Destroy a hash table for pointer lookup. */
void SNetHashPtrTabDestroy(struct hash_ptab *tab);

/* Resize a pointer hash table to a new power size. */
void SNetHashPtrTabResize(struct hash_ptab *tab, size_t new_power);

/* Store a pointer + value pair into a pointer hashing table. */
void SNetHashPtrStore(struct hash_ptab *tab, void *key, void *val);

/* Lookup a value in the pointer hashing table. */
void *SNetHashPtrLookup(struct hash_ptab *tab, void *key);

/* Remove a value from the pointer hashing table. */
void *SNetHashPtrRemove(struct hash_ptab *tab, void *key);

/* Verify whether the table has zero elements. */
bool SNetHashPtrTabEmpty(struct hash_ptab *tab);

/* Return the first key in the hash table. */
void *SNetHashPtrFirst(struct hash_ptab *tab);

/* Return the next key in the hash table after 'key'. */
void *SNetHashPtrNext(struct hash_ptab *tab, void *key);

/* xident.c */


/* Terminate an identity landing. */
void SNetTermIdentity(landing_t *land, fifo_t *fifo);

/* Change a landing into an identity landing. */
void SNetBecomeIdentity(landing_t *land, snet_stream_desc_t *out);

/* xinput.c */


/* Read one record from the parser. */
input_state_t SNetGetNextInputRecord(landing_t *land);
void SNetCloseInput(node_t* node);

/* A dummy terminate function which is never called. */
void SNetTermInput(landing_t *land, fifo_t *fifo);

/* Deallocate input node */
void SNetStopInput(node_t *node, fifo_t *fifo);

/* A dummy input function for input node which is never called. */
void SNetNodeInput(snet_stream_desc_t *desc, snet_record_t *rec);

/* Create the node which reads records from an input file via the parser.
 * This is called from networkinterface.c to initialize the input module.
 */
void SNetInInputInit(
    FILE                *file,
    snetin_label_t      *labels,
    snetin_interface_t  *interfaces,
    snet_stream_t       *output);

/* xlanding.c */


/* Create a new landing with reference count 1. */
landing_t *SNetNewLanding(
    node_t *node,
    snet_stream_desc_t *prev,
    landing_type_t type);

/* Add a landing to the stack of future destination landings. */
void SNetPushLanding(snet_stream_desc_t *desc, landing_t *land);

/* Take the first landing from the stack of future landings. */
landing_t *SNetPopLanding(snet_stream_desc_t *desc);

/* Peek non-destructively at the top of the stack of future landings. */
landing_t *SNetTopLanding(const snet_stream_desc_t *desc);

/* Free the type specific dynamic memory resources of a landing. */
void SNetCleanupLanding(landing_t *land);

/* Destroy a landing. */
void SNetFreeLanding(landing_t *land);

/* Decrease the reference count of a landing and collect garbage. */
void SNetLandingDone(landing_t *land);

/* Create a new landing for a box. */
void SNetNewBoxLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev);

/* Create a new landing for a dispatcher. */
void SNetNewParallelLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev);

/* Create a new landing for a star. */
void SNetNewStarLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev);

/* Create a new landing for a split. */
void SNetNewSplitLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev);

/* Create a new landing for a dripback. */
void SNetNewDripBackLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev);

/* Create a new landing for a feedback. */
void SNetNewFeedbackLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev);

/* Give a string representation of the landing type */
const char *SNetLandingTypeName(landing_type_t ltype);

/* Give a string representation of the landing type */
const char *SNetLandingName(landing_t *land);

/* xmanager.c */


/* Setup data structures for input manager. */
void SNetInputManagerInit(void);

/* Start input manager thread. */
void SNetInputManagerStart(void);

/* Cleanup input manager. */
void SNetInputManagerStop(void);

/* Process one input message. */
bool SNetInputManagerDoTask(worker_t *worker);

/* Loop forever processing network input. */
void SNetInputManagerRun(worker_t *worker);

/* Convert a distributed communication protocol message type to a string. */
const char* SNetCommName(int i);

/* xnameshift.c */


/* Nameshift node */
void SNetNodeNameShift(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a nameshift landing. */
void SNetTermNameShift(landing_t *land, fifo_t *fifo);

/* Destroy a nameshift node. */
void SNetStopNameShift(node_t *node, fifo_t *fifo);
snet_stream_t *SNetNameShift(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    int offset,
    snet_variant_t *untouched);

/* xnodist.c */

void hello(const char *s);

/* Initialize distributed computing. Called by SNetInRun in networkinterface.c. */
void SNetDistribInit(int argc, char **argv, snet_info_t *info);

/* Start input+output managers. Called by SNetInRun in networkinterface.c. */
void SNetDistribStart(void);
void SNetDistribGlobalStop(void);
void SNetDistribWaitExit(snet_info_t *info);

/* Needed frequently for record IDs and field references. */
int SNetDistribGetNodeId(void);

/* Are we identified by location? Needed frequently for field references. */
bool SNetDistribIsNodeLocation(int location);

/* Test for rootness. First call by SNetInRun in networkinterface.c. */
bool SNetDistribIsRootNode(void);

/* Is this application supporting distributed computations? */
bool SNetDistribIsDistributed(void);

/* Use by distribution layer in C4SNet.c. */
void SNetDistribPack(void *src, ...);

/* Use by distribution layer in C4SNet.c. */
void SNetDistribUnpack(void *dst, ...);

/* Request a field from transfer. Called when distributed. */
void SNetDistribFetchRef(snet_ref_t *ref);

/* Update field reference counter. Called when distributed. */
void SNetDistribUpdateRef(snet_ref_t *ref, int count);

/* Dummy function needed for networkinterface.c */
snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *in, int loc);

/* Dummy function which is irrelevant for non-Distributed S-Net. */
void SNetInputManagerRun(worker_t *worker);

/* Dummy function which is irrelevant for non-Distributed S-Net. */
snet_stream_desc_t *SNetTransferOpen(
    int loc,
    snet_stream_desc_t *desc,
    snet_stream_desc_t *prev);
void SNetRecDetrefStackSerialise(snet_record_t *rec, void *buf);
void SNetRecDetrefRecSerialise(snet_record_t *rec, void *buf);
void SNetRecDetrefStackDeserialise(snet_record_t *rec, void *buf);
snet_record_t* SNetRecDetrefRecDeserialise(void *buf);

/* xobserve.c */

snetin_label_t* SNetObserverGetLabels(void);
snetin_interface_t* SNetObserverGetInterfaces(void);

/* Create a new socket observer; this is called by the compiler output. */
snet_stream_t *SNetObserverSocketBox(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    const char *address,
    int port,
    bool interactive,
    const char *position,
    char type,
    char level,
    const char *code);

/* Create a new file observer; this is called by the compiler output. */
snet_stream_t *SNetObserverFileBox(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    const char *filename,
    const char *position,
    char type,
    char level,
    const char *code);

/* Store data needed for observing. */
int SNetObserverInit(snetin_label_t *labs, snetin_interface_t *ifs);

/* Destroy allocated data for observing. */
void SNetObserverDestroy(void);

/* xoutput.c */


/* Output a record to stdout */
void SNetNodeOutput(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate an output landing. */
void SNetTermOutput(landing_t *land, fifo_t *fifo);

/* Destroy an output node. */
void SNetStopOutput(node_t *node, fifo_t *fifo);

/* Create the node which prints records to stdout */
void SNetInOutputInit(FILE *file,
                      snetin_label_t *labels,
                      snetin_interface_t *interfaces,
                      snet_stream_t *input);

/* xpar.c */


/* Process a record for a parallel node. */
void SNetNodeParallel(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a parallel landing. */
void SNetTermParallel(landing_t *land, fifo_t *fifo);

/* Destroy a parallel node. */
void SNetStopParallel(node_t *node, fifo_t *fifo);
snet_stream_t *SNetParallel(
    snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *variant_lists,
    ...);
snet_stream_t *SNetParallelDet(
    snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_list_list_t *variant_lists,
    ...);

/* xserial.c */

snet_stream_t *SNetSerial(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

/* xsplit.c */


/* Split node process a record. */
void SNetNodeSplit(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a split landing. */
void SNetTermSplit(landing_t *land, fifo_t *fifo);

/* Destroy a split node. */
void SNetStopSplit(node_t *node, fifo_t *fifo);

/* Return the current indexed placement stack level. */
int SNetLocSplitGetLevel(void);

/* Increment the indexed placement stack level by one. */
void SNetLocSplitIncrLevel(void);

/* Decrement the indexed placement stack level by one. */
void SNetLocSplitDecrLevel(void);

/* Return the current indexed placement stack level. */
int SNetSubnetGetLevel(void);

/* Increment the indexed placement stack level by one. */
void SNetSubnetIncrLevel(void);

/* Decrement the indexed placement stack level by one. */
void SNetSubnetDecrLevel(void);

/**
 * Convenience function for creating a Split, DetSplit, LocSplit or LocSplitDet.
 * This is determined by the parameters 'is_byloc' and 'is_det'.
 * Parameter 'is_byloc' says whether to create an indexed placement combinator.
 * Parameter 'is_det' specifies if the combinator is deterministic.
 */
snet_stream_t *CreateSplit(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag,
    int utag,
    bool is_byloc,
    bool is_det);

/* Non-det Split creation function */
snet_stream_t *SNetSplit( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag);

/* Det Split creation function */
snet_stream_t *SNetSplitDet( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag);

/* Non-det Location Split creation function */
snet_stream_t *SNetLocSplit( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag);

/* Det Location Split creation function */
snet_stream_t *SNetLocSplitDet( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    int ltag, int utag);

/* xstack.c */

void SNetStackInit(snet_stack_t *stack);
snet_stack_t *SNetStackCreate(void);
void SNetStackDone(snet_stack_t *stack);
void SNetStackDestroy(snet_stack_t *stack);
bool SNetStackIsEmpty(snet_stack_t *stack);
bool SNetStackNonEmpty(snet_stack_t *stack);
void SNetStackPush(snet_stack_t *stack, void *item);
void SNetStackAppend(snet_stack_t *stack, void *item);
void *SNetStackTop(const snet_stack_t *stack);
void *SNetStackPop(snet_stack_t *stack);
void SNetStackPopAll(snet_stack_t *stack);

/* Shallow duplicate a stack. Stack 'dest' must exist as empty. */
void SNetStackCopy(snet_stack_t *dest, const snet_stack_t *source);

/* Construct a literal copy of an existing stack. */
snet_stack_t* SNetStackClone(const snet_stack_t *source);

/* Swap two stacks. */
void SNetStackSwap(snet_stack_t *one, snet_stack_t *two);

/* Compute number of elements in the stack. */
int SNetStackElementCount(snet_stack_t *stack);

/* xstar.c */


/* Star component: forward record to instance or to collector. */
void SNetNodeStar(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a star landing. */
void SNetTermStar(landing_t *land, fifo_t *fifo);

/* Destroy a star node. */
void SNetStopStar(node_t *node, fifo_t *fifo);

/* Star creation function */
snet_stream_t *SNetStar( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

/* Star incarnate creation function */
snet_stream_t *SNetStarIncarnate( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

/* Det Star creation function */
snet_stream_t *SNetStarDet(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

/* Det star incarnate creation function */
snet_stream_t *SNetStarDetIncarnate(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b);

/* xstream.c */


/* Allocate a new stream */
snet_stream_t *SNetStreamCreate(int capacity);

/* Free a stream */
void SNetStreamDestroy(snet_stream_t *stream);

/* Dummy function needed for linking with other libraries. */
void *SNetStreamRead(snet_stream_desc_t *sd);

/* Enqueue a record to a stream and add a note to the todo list. */
void SNetStreamWrite(snet_stream_desc_t *desc, snet_record_t *rec);

/* Enqueue a record to a stream and add a note to the todo list. */
void SNetWrite(snet_stream_desc_t **desc_ptr, snet_record_t *rec, bool last);

/* Dequeue a record and process it. */
void SNetStreamWork(snet_stream_desc_t *desc, worker_t *worker);

/* Destroy a stream descriptor. */
void SNetStreamClose(snet_stream_desc_t *desc);

/* Deallocate a stream, but remember it's destination node. */
void SNetStopStream(snet_stream_t *stream, fifo_t *fifo);

/* Decrease the reference count of a stream descriptor by one. */
void SNetDescDone(snet_stream_desc_t *desc);

/* Let go of multiple references to a stream descriptor. */
void SNetDescRelease(snet_stream_desc_t *desc, int count);

/* Open a descriptor to an output stream.
 *
 * The source landing which opens the stream is determined by parameter 'prev'.
 * The new descriptor holds a pointer to a landing which instantiates
 * the destination node as determined by the stream parameter 'stream'.
 *
 * Special cases:
 * (1) Dispatchers should create an additional landing for a future collector
 * and push this landing onto a stack of future landings.
 * (2) Collectors should retrieve their designated landing from this stack
 * and verify that it is theirs.
 * (3) Each star instance should create the subsequent star incarnation
 * and push this incarnation onto the stack of landings.
 * (4) When a destination landing is part of subnetwork of an
 * indexed placement combinator then the node location must be
 * retrieved from the 'dyn_locs' attribute of the previous landing.
 * (5) When the location is on a different machine then a 'transfer'
 * landing must be created instead which connects to a remote location.
 */
snet_stream_desc_t *SNetStreamOpen(
    snet_stream_t *stream,
    snet_stream_desc_t *prev);

/* xsync.c */


/*****************************************************************************/

void SNetSyncInitDesc(snet_stream_desc_t *desc, snet_stream_t *stream);

/* Sync box task */
void SNetNodeSync(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a sync landing. */
void SNetTermSync(landing_t *land, fifo_t *fifo);

/* Destroy a sync node. */
void SNetStopSync(node_t *node, fifo_t *fifo);

/* Create a new Synchro-Cell */
snet_stream_t *SNetSync(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *patterns,
    snet_expr_list_t *guard_exprs );

/* xthread.c */

int SNetGetNumProcs(void);

/* How many workers? */
int SNetThreadingWorkers(void);

/* How many thieves? */
int SNetThreadingThieves(void);

/* How many distributed computing threads? */
int SNetThreadedManagers(void);

/* What kind of garbage collection to use? */
bool SNetGarbageCollection(void);

/* Whether to be verbose */
bool SNetVerbose(void);

/* Whether to enable debugging information */
bool SNetDebug(void);

/* Whether debugging of distributed front is enabled */
bool SNetDebugDF(void);

/* Whether debugging of garbage collection is enabled */
bool SNetDebugGC(void);

/* Whether debugging of the streams layer is enabled */
bool SNetDebugSL(void);

/* Whether debugging of the threading layer is enabled */
bool SNetDebugTL(void);

/* Whether debugging of work stealing is enabled */
bool SNetDebugWS(void);

/* Whether to use a deterministic feedback */
bool SNetFeedbackDeterministic(void);

/* Whether to use optimized sync-star */
bool SNetZipperEnabled(void);

/* The stack size for worker threads in bytes */
size_t SNetThreadStackSize(void);

/* Whether to apply an input throttle. */
bool SNetInputThrottle(void);

/* The number of unconditional input records. */
double SNetInputOffset(void);

/* The rate at which input can increase depending on output. */
double SNetInputFactor(void);

/* Extract the box concurrency specification for a given box name.
 * The default concurrency specification can be given as a number,
 * which is optionally followed by a capital 'D' for determinism.
 *      16D
 * Or as a number followed by a list of box names to which it applies:
 *      3:foo,bar
 * Or as a combination of these separated by spaces:
 *      16D 3:foo,bar 4D:mit 1:out
 * A default concurrency applies only if no box specific one exists.
 */
int SNetGetBoxConcurrency(const char *box, bool *is_det);

/* Process command line options. */
int SNetThreadingInit(int argc, char**argv);

/* Create a thread to instantiate a worker */
void SNetThreadCreate(void *(*func)(void *), worker_t *worker);
void SNetThreadSetSelf(worker_t *self);
worker_t *SNetThreadGetSelf(void);
int SNetThreadingStop(void);
int SNetThreadingCleanup(void);

/* xtostring.c */


/* Make a short description of a record type. */
void SNetRecordTypeString(snet_record_t *rec, char *buf, size_t size);

/* Convert a record type to a dynamically allocated string. */
char *SNetGetRecordTypeString(snet_record_t *rec);

/* Make a short description of a variant type. */
void SNetVariantString(snet_variant_t *var, char *buf, size_t size);

/* Convert a variant type to a dynamically allocated string. */
char *SNetGetVariantString(snet_variant_t *var);

/* Make a short description of a variant list type. */
void SNetVariantListString(snet_variant_list_t *vl, char *buf, size_t size);

/* Convert a variant list type to a dynamically allocated string. */
char *SNetGetVariantListString(snet_variant_list_t *vl);

/* xtransfer.c */


/* Combine two signed 32-bits integers into a 64-bit unsigned hash key. */
uint64_t snet_ints_to_key(int i1, int i2);

/* Initialize storage for continuations. */
void SNetTransferInit(void);

/* Delete storage for continuations. */
void SNetTransferStop(void);

/* Restore landing state from a continuation. */
void SNetTransferRestore(landing_t *land, snet_stream_desc_t *desc);

/* Forward a record to a different node. */
void SNetNodeTransfer(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a landing of type 'transfer'. */
void SNetTermTransfer(landing_t *land, fifo_t *fifo);

/* Initialize a transfer landing, which connects to a remote location. */
snet_stream_desc_t *SNetTransferOpen(
    int destination_location,
    snet_stream_desc_t *desc,
    snet_stream_desc_t *prev);

/* Setup a transfer landing to connect back to a remote continuation. */
void SNetTransferReturn(
    landing_t *next,
    snet_stream_desc_t *desc,
    snet_stream_desc_t *prev);

/* xworker.c */


/* Init worker data */
void SNetWorkerInit(void);

/* Cleanup worker data */
void SNetWorkerCleanup(void);

/* Create a new worker. */
worker_t *SNetWorkerCreate(
    node_t *input_node,
    int worker_id,
    node_t *output_node,
    worker_role_t role);

/* Destroy a worker. */
void SNetWorkerDestroy(worker_t *worker);

/* Add a new unit of work to the worker's todo list.
 * At return iterator should point to the new item.
 */
void SNetWorkerTodo(worker_t *worker, snet_stream_desc_t *desc);

/* Steal a work item from another worker */
void SNetWorkerStealVictim(worker_t *victim, worker_t *thief);

/* Return the worker which is the input manager for Distributed S-Net. */
worker_t* SNetWorkerGetInputManager(void);

/* Test if other workers have work to do. */
bool SNetWorkerOthersBusy(worker_t *worker);

/* Cleanup unused memory. */
void SNetWorkerMaintenaince(worker_t *worker);

/* Wait for other workers to finish. */
void SNetWorkerWait(worker_t *worker);

/* Process work forever and read input until EOF. */
void SNetWorkerRun(worker_t *worker);

/* xzipper.c */


/* Merge a data record */
void SNetNodeZipperData(
    snet_record_t *rec,
    const zipper_arg_t *zarg,
    landing_zipper_t *land);

/* Destroy the landing state of a Zipper node */
void SNetNodeZipperTerminate(landing_zipper_t *land, zipper_arg_t *zarg);

/* Process records for fused sync-star. */
void SNetNodeZipper(snet_stream_desc_t *desc, snet_record_t *rec);

/* Terminate a zipper landing. */
void SNetTermZipper(landing_t *land, fifo_t *fifo);

/* Free a merge node */
void SNetStopZipper(node_t *node, fifo_t *fifo);

/* Create a new Zipper */
snet_stream_t *SNetZipper(
    snet_stream_t       *input,
    snet_info_t         *info,
    int                  location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t    *exit_guards,
    snet_variant_list_t *sync_patterns,
    snet_expr_list_t    *sync_guards );

