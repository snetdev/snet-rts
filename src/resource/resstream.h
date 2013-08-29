#ifndef RESSTREAM_H_INCLUDED
#define RESSTREAM_H_INCLUDED

typedef struct buffer buffer_t;
typedef struct stream stream_t;

struct buffer {
  char*         data;
  int           size;
  int           start;
  int           end;
};

struct stream {
  int           fd;
  buffer_t      read;
  buffer_t      write;
};

#endif
