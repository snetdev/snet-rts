#ifndef RESSTREAM_H_INCLUDED
#define RESSTREAM_H_INCLUDED

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
