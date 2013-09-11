#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex.h>

#include "node.h"
#include "observers.h"
#include "interface_functions.h"

#define NODE_OBS(node)     ((observer_t *) &((node)->u))

/* Shorter names for observer data levels. */
typedef enum data_level {
  Labels = SNET_OBSERVERS_DATA_LEVEL_LABELS,
  TagVal = SNET_OBSERVERS_DATA_LEVEL_TAGVALUES,
  AllVal = SNET_OBSERVERS_DATA_LEVEL_ALLVALUES,
} data_level_t;

/* Shorter names for observer types. */
typedef enum obs_place {
  Before = SNET_OBSERVERS_TYPE_BEFORE,
  After  = SNET_OBSERVERS_TYPE_AFTER,
} obs_place_t;

/* A file/socket observer goes through three states. */
typedef enum obs_state {
  Initial,
  Active,
  Closing,
} obs_state_t;

typedef struct observer {
  struct observer       *next;
  node_t                *node;
  landing_t             *land;
  FILE                  *file;
  obs_state_t            state;

  /* state for file observers: */
  const char            *filename;

  /* state for socket observers: */
  struct sockaddr_in     addr;
  int                    sock;
} observer_t;

/* Pattern of reply message for regex. */
static const char pattern[] =
  "(<\\?xml[ \n\t]+version=\"1\\.0\"[ \n\t]*\\?>)?"
  "<observer[ \n\t]+(xmlns=\"snet-home\\.org\"[ \n\t]+)?"
  "oid=\"([0-9]+)\"([ \n\t]+xmlns=\"snet-home\\.org\"[ \n\t]+)?"
  "[ \n\t]*((/>)|(>[ \n\t]*</observer[ \n\t]*>))";

/* Compiled regex */
static regex_t                  *preg;

/* These are used to provide instance specific IDs for observers */
static int                       oid_pool;

static snetin_label_t           *labels;
static snetin_interface_t       *interfaces;
static observer_t               *observers;

snetin_label_t* SNetObserverGetLabels(void) { return labels; }
snetin_interface_t* SNetObserverGetInterfaces(void) { return interfaces; }

/* Convert a record to string representation and write to 'file'. */
static int FileRecord(
    FILE *file,
    snet_record_t *rec,
    int type,
    int oid,
    int level,
    const char *position,
    const char *code)
{
  int name, val, id;
  snet_ref_t *field;
  char *label = NULL;
  char *interface = NULL;
  snet_record_mode_t mode;

  fprintf(file,
          "<?xml version=\"1.0\"?>\n"
          "<observer xmlns=\"snet-home.org\" oid=\"%d\" position=\"%s\" "
          "type=\"%s\" ",
          oid, position,
          type == Before ? "before" : "after");

  if (code != NULL) {
    fprintf(file, "code=\"%s\" ", code);
  }

  fprintf(file, ">\n");

  switch (REC_DESCR(rec)) {

    case REC_data:
      mode = SNetRecGetDataMode(rec);

      fprintf(file, "<record type=\"data\" mode=\"%s\" >\n",
              mode == MODE_textual ? "textual" : "binary");

      id = SNetRecGetInterfaceId(rec);
      /* fields */
      RECORD_FOR_EACH_FIELD(rec, name, field) {
        if ((label = SNetInIdToLabel(labels, name)) != NULL) {
          if (level == AllVal
             && (interface = SNetInIdToInterface(interfaces, id)) != NULL) {
            fprintf(file, "<field label=\"%s\" interface=\"%s\" >",
                    label, interface);

            if (mode == MODE_textual) {
              SNetInterfaceGet(id)->serialisefun(file, SNetRefGetData(field));
            } else {
              SNetInterfaceGet(id)->encodefun(file, SNetRefGetData(field));
            }

            fprintf(file, "</field>\n");
          } else {
            fprintf(file, "<field label=\"%s\" />\n", label);
          }
        }
      }

      /* tags */
      RECORD_FOR_EACH_TAG(rec, name, val) {
        if ((label = SNetInIdToLabel(labels, name)) != NULL) {
          if (level == Labels) {
            fprintf(file, "<tag label=\"%s\" />\n", label);
          }else {
            fprintf(file, "<tag label=\"%s\" >%d</tag>\n", label, val);
          }
        }
        SNetMemFree(label);
      }

      /* btags */
      RECORD_FOR_EACH_BTAG(rec, name, val) {
        if ((label = SNetInIdToLabel(labels, name)) != NULL) {
          if (level == Labels) {
            fprintf(file, "<btag label=\"%s\" />\n", label);
          }else {
            fprintf(file, "<btag label=\"%s\" >%d</btag>\n", label, val);
          }
        }
        SNetMemFree(label);
      }

      fprintf(file, "</record>\n");
      break;
    case REC_sync:
      fprintf(file, "<record type=\"sync\" />\n");
      break;
    case REC_trigger_initialiser:
      fprintf(file, "<record type=\"trigger_initialiser\" />\n");
      break;
    case REC_detref:
      fprintf(file, "<record type=\"detref\" />\n");
      break;
    default:
      break;
  }
  fprintf(file, "</observer>\n");
  fflush(file);

  return 0;
}

/* Wait for interactive user to give confirmation on a socket connection. */
static bool GetConfirmation(observer_t *obs, int obs_oid)
{
  int           ret;
  const size_t  nmatch = 4;
  regmatch_t    pmatch[nmatch];
  int           oid;
  size_t        size = 1*1024;
  size_t        off = 0;
  ssize_t       got;
  char         *buf = SNetNewN(size + 1, char);
  bool          success = false;

  assert(preg);
  while (success == false) {
    got = recv(obs->sock, buf + off, size - off, 0);
    if (got == -1) {
      SNetUtilDebugNotice("[%s]: socket read: %s", __func__, strerror(errno));
      obs->state = Closing;
      break;
    }
    else if (got == 0) {
      SNetUtilDebugNotice("[%s]: socket closed while waiting for confirmation",
                          __func__);
      obs->state = Closing;
      break;
    }
    else {
      off += got;
      buf[off] = '\0';
      if ((ret = regexec(preg, buf, nmatch, pmatch, REG_EXTENDED)) == 0) {
        buf[pmatch[3].rm_eo] = '\0';
        if (sscanf(&buf[pmatch[3].rm_so], "%d", &oid) == 1) {
          if (oid == obs_oid) {
            success = true;
            break;
          }
        }
      }
    }
    if (size < off + 1024) {
      SNetMemResize(buf, size += size);
    }
  }
  SNetDelete(buf);
  return success;
}

/* A file/socket observer receives a REC_observ message. */
static void SNetNodeObserver2(snet_stream_desc_t *desc, snet_record_t *in_rec)
{
  node_t                *node2 = DESC_NODE(desc);
  observer_t            *obs = NODE_OBS(node2);
  int                    oid = OBSERV_REC(in_rec, oid);
  snet_record_t         *out_rec = OBSERV_REC(in_rec, rec);
  node_t                *node1 = DESC_FROM(desc);
  observer_arg_t        *arg = NODE_SPEC(node1, observer);
  snet_stream_desc_t    **outdesc_ptr = OBSERV_REC(in_rec, desc);

  trace(__func__);
  assert(REC_DESCR(in_rec) == REC_observ);

  if (obs->filename) {
    if (obs->state == Initial) {
      obs->file = fopen(obs->filename, "a");
      if (obs->file == NULL) {
        SNetUtilDebugNotice("[%s]: cannot open %s: %s", __func__,
                            obs->filename, strerror(errno));
        obs->state = Closing;
      } else {
        obs->state = Active;
      }
    }
  } else {
    if (obs->state == Initial) {
      obs->state = Closing;
      obs->sock = socket(PF_INET, SOCK_STREAM, 0);
      if (obs->sock == -1) {
        SNetUtilDebugNotice("[%s]: no socket: %s", __func__, strerror(errno));
      }
      else if (connect(obs->sock, (struct sockaddr*)&obs->addr,
                       (socklen_t)sizeof(obs->addr))) {
        SNetUtilDebugNotice("[%s]: no connect to %s:%d: %s",
                            __func__, arg->address, arg->port, strerror(errno));
        close(obs->sock);
        obs->sock = -1;
      }
      else if ((obs->file = fdopen(obs->sock, "w")) == NULL) {
        SNetUtilDebugNotice("[%s]: no fdopen: %s", __func__, strerror(errno));
        close(obs->sock);
        obs->sock = -1;
      }
      else {
        obs->state = Active;
      }
    }
  }

  if (obs->state == Active) {
    FileRecord(obs->file, out_rec, arg->type,
               oid, arg->level, arg->position, arg->code);
    if (ferror(obs->file)) {
      SNetUtilDebugNotice("[%s]: error writing to %s:%d: %s", __func__,
                          arg->address, arg->port, strerror(errno));
      obs->state = Closing;
    }
    else if (arg->interactive) {
      if (GetConfirmation(obs, oid) == false) {
        obs->state = Closing;
      }
    }
  }

  SNetRecDestroy(in_rec);
  SNetWrite(outdesc_ptr, out_rec, true);
  SNetDescDone(*outdesc_ptr);
}

/* Deallocate node. */
static void SNetStopObserver2(node_t *node, fifo_t *fifo)
{
  /* impossible */
  assert(false);
}

/* Terminate landing. */
static void SNetTermObserver2(landing_t *landing, fifo_t *fifo)
{
  /* impossible */
  assert(false);
}

/* See if a file/socket has already been opened; if not then allocate it. */
static node_t *FindObserver(const char *filename, struct sockaddr_in *addr)
{
  observer_t       *obs;

  trace(__func__);
  for (obs = observers; obs; obs = obs->next) {
    if (filename) {
      if (obs->filename && !strcmp(filename, obs->filename)) {
        break;
      }
    }
    else if (obs->filename == NULL) {
      if (!memcmp(addr, &obs->addr, sizeof(*addr))) {
        break;
      }
    }
  }
  if (!obs) {
    node_t *node = SNetNew(node_t);
    node->type = NODE_observer2;
    node->work = SNetNodeObserver2;
    node->stop = SNetStopObserver2;
    node->term = SNetTermObserver2;

    /* Make sure observer has enough room within the node union: */
    switch (true) {
      case false: break;
      case (sizeof(node->u) >= sizeof(observer_t)):
        obs = NODE_OBS(node);
        obs->node = node;
        obs->state = Initial;
        obs->land = SNetNewLanding(node, NULL, LAND_empty);
        obs->file = NULL;
        obs->filename = filename;
        if (addr) {
          obs->addr = *addr;
        } else {
          memset(&obs->addr, 0, sizeof(obs->addr));
        }
        obs->sock = -1;
        obs->next = observers;
        observers = obs;
        break;
    }
  }
  return obs->node;
}

/* An observer receives a record. */
static void SNetNodeObserver(snet_stream_desc_t *desc, snet_record_t *in_rec)
{
  const observer_arg_t  *arg = DESC_NODE_SPEC(desc, observer);
  landing_observer_t    *land = DESC_LAND_SPEC(desc, observer);
  snet_record_t         *out_rec;

  trace(__func__);
  if (land->oid == 0) {
    land->oid = AAF(&oid_pool, 1);
    SNetPushLanding(desc, NODE_OBS(arg->output1->dest)->land);
    land->outdesc = SNetStreamOpen(arg->output1, desc);
    /* Also allocate the outgoing descriptor for the sock observer 2. */
    land->outdesc2 = SNetStreamOpen(arg->output2, desc);
    land->outdesc2->source = land->outdesc->landing;
  }
  out_rec = SNetRecCreate(REC_observ, land->oid, in_rec, &land->outdesc2);
  DESC_INCR(land->outdesc2);
  SNetWrite(&land->outdesc, out_rec, true);
}

/* Deallocte observer node. */
static void SNetStopObserver(node_t *node, fifo_t *fifo)
{
  observer_arg_t        *arg = NODE_SPEC(node, observer);

  trace(__func__);
  SNetStopStream(arg->output2, fifo);
  SNetStreamDestroy(arg->output1);
  SNetDelete(node);
}

/* Deallocte observer landing. */
static void SNetTermObserver(landing_t *land, fifo_t *fifo)
{
  landing_observer_t    *obs = LAND_SPEC(land, observer);

  trace(__func__);
  if (obs->outdesc) {
    SNetFifoPut(fifo, obs->outdesc);
    SNetFifoPut(fifo, obs->outdesc2);
  }
  SNetFreeLanding(land);
}

/* Create a new observer node. */
static snet_stream_t *NewObserver(
    node_t *node2,
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    const char *filename,
    const char *address,
    int port,
    bool interactive,
    const char *position,
    char type,
    char level,
    const char *code)
{
  snet_stream_t         *output;
  node_t                *node;
  observer_arg_t        *arg;

  trace(__func__);

  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_observer, location, &input, 1, &output, 1,
                     SNetNodeObserver, SNetStopObserver, SNetTermObserver);
  arg = NODE_SPEC(node, observer);
  arg->output1 = output;
  arg->output2 = SNetStreamCreate(0);
  arg->filename = filename;
  arg->address = address;
  arg->port = port;
  arg->interactive = interactive;
  arg->position = position;
  arg->type = type;
  arg->level = level;
  arg->code = code;

  STREAM_DEST(arg->output1) = node2;
  STREAM_FROM(arg->output2) = node2;

  if (interactive && preg == NULL) {
    preg = SNetNew(regex_t);

    /* Compile regex pattern to parse reply messages. */
    if (regcomp(preg, pattern, REG_EXTENDED) != 0) {
      SNetUtilDebugNotice("[%s]: regex compile error", __func__);
      SNetDelete(preg);
      preg = NULL;
      interactive = false;
    }

    /* Also ignore signals for writes to a closing socket. */
    signal(SIGPIPE, SIG_IGN);
  }

  return arg->output2;
}

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
    const char *code)
{
  struct hostent        *host;
  struct sockaddr_in    inet;
  node_t                *node2;

  trace(__func__);

  memset(&inet, 0, sizeof(inet));
  if ((inet.sin_addr.s_addr = inet_addr(address)) == INADDR_NONE) {
    host = gethostbyname(address);
    if (!host) {
      #define NAME(x) #x
      #define HERR(l) (h_errno == (l)) ? NAME(l) :
      SNetUtilDebugNotice("[%s]: Could not lookup %s: %s\n", __func__, address,
        HERR(HOST_NOT_FOUND)
        HERR(NO_ADDRESS)
        HERR(NO_DATA)
        HERR(NO_RECOVERY)
        HERR(TRY_AGAIN)
        "For unknown reasons.");
      return input;
    }
    if (host->h_addrtype != AF_INET) {
      SNetUtilDebugNotice("[%s]: Only IPv4 handled", __func__);
      return input;
    }
    memcpy(&inet.sin_addr, host->h_addr, host->h_length);
  }
  inet.sin_family = AF_INET;
  inet.sin_port = htons(port);

  node2 = FindObserver(NULL, &inet);
  if (!node2) {
    return input;
  }

  return NewObserver(node2, input, info, location,
                     NULL, address, port, interactive,
                     position, type, level, code);
}

/* Create a new file observer; this is called by the compiler output. */
snet_stream_t *SNetObserverFileBox(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    const char *filename,
    const char *position,
    char type,
    char level,
    const char *code)
{
  node_t                *node2;

  trace(__func__);

  node2 = FindObserver(filename, NULL);
  if (!node2) {
    return input;
  }

  return NewObserver(node2, input, info, location,
                     filename, NULL, 0, false,
                     position, type, level, code);
}

/* Store data needed for observing. */
int SNetObserverInit(snetin_label_t *labs, snetin_interface_t *ifs)
{
  trace(__func__);

  labels = labs;
  interfaces = ifs;

  return 0;
}

/* Destroy allocated data for observing. */
void SNetObserverDestroy(void)
{
  observer_t       *obs, *next;

  trace(__func__);

  for (obs = observers; obs; obs = next) {
    node_t *node = obs->node;
    next = obs->next;
    if (obs->file) {
      fclose(obs->file);
    }
    if (obs->sock >= 0) {
      close(obs->sock);
    }
    obs->land->refs = 0;
    SNetFreeLanding(obs->land);
    SNetDelete(node);
  }
  if (preg) {
    regfree(preg);
    SNetDelete(preg);
  }
}

