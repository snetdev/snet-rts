#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include "resdefs.h"

void res_buffer_init(buffer_t* buf)
{
  buf->data = NULL;
  buf->size = 0;
  buf->start = 0;
  buf->end = 0;
}

void res_buffer_done(buffer_t* buf)
{
  xdel(buf->data);
}

void res_buffer_appended(buffer_t* buf, int amount)
{
  assert(buf->end + amount <= buf->size);
  buf->end += amount;
}

void res_buffer_reserve(buffer_t* buf, int amount)
{
  if (buf->size - buf->end < amount) {
    if (buf->start == 0) {
      int newsize = buf->end + amount;
      buf->data = xrealloc(buf->data, newsize);
      buf->size = newsize;
    } else {
      int newsize = buf->end - buf->start + amount;
      if (newsize <= buf->size) {
        memmove(buf->data, buf->data + buf->start, buf->end - buf->start);
        buf->end -= buf->start;
        buf->start = 0;
      } else {
        char *newdata = xmalloc(newsize);
        memcpy(newdata, buf->data + buf->start, buf->end - buf->start);
        xfree(buf->data);
        buf->data = newdata;
        buf->end -= buf->start;
        buf->start = 0;
        buf->size = newsize;
      }
    }
  }
}

int res_buffer_stored(buffer_t* buf)
{
  return buf->end - buf->start;
}

int res_buffer_avail(buffer_t* buf)
{
  return buf->size - buf->end;
}

char* res_buffer_data(buffer_t* buf)
{
  return buf->data + buf->start;
}

void res_buffer_take(buffer_t* buf, int amount)
{
  assert(amount >= 0);
  assert(amount <= buf->end - buf->start);
  buf->start += amount;
}

int res_buffer_read(buffer_t* buf, int fd, int amount)
{
  int n;
  res_buffer_reserve(buf, amount);
  if ((n = read(fd, buf->data + buf->end, amount)) == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    } else {
      perror("read");
      return -1;
    }
  } else {
    buf->end += n;
    return n;
  }
}

int res_buffer_write(buffer_t* buf, int fd, int amount)
{
  int n;
  assert(buf->end > buf->start);
  assert(amount <= buf->end - buf->start);
  if ((n = write(fd, buf->data + buf->start, buf->end - buf->start)) == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    } else {
      perror("write");
      return -1;
    }
  } else {
    buf->start += n;
    return n;
  }
}

void res_stream_init(stream_t* stream, int fd)
{
  stream->fd = fd;
  res_buffer_init(&stream->read);
  res_buffer_init(&stream->write);
}

void res_stream_done(stream_t* stream)
{
  res_buffer_done(&stream->read);
  res_buffer_done(&stream->write);
  close(stream->fd);
}

int res_stream_read(stream_t* stream)
{
  int amount = 1024;
  return res_buffer_read(&stream->read, stream->fd, amount);
}

int res_stream_write(stream_t* stream)
{
  int amount = res_buffer_stored(&stream->write);
  return res_buffer_write(&stream->write, stream->fd, amount);
}

bool res_stream_writing(stream_t* stream)
{
  return res_buffer_stored(&stream->write) > 0;
}

char* res_stream_incoming(stream_t* stream, int* amount)
{
  *amount = res_buffer_stored(&stream->read);
  return res_buffer_data(&stream->read);
}

void res_stream_take(stream_t* stream, int amount)
{
  res_buffer_take(&stream->read, amount);
}

void res_stream_reserve(stream_t* stream, int amount)
{
  res_buffer_reserve(&stream->write, amount);
}

void res_stream_appended(stream_t* stream, int amount)
{
  res_buffer_appended(&stream->write, amount);
}

char* res_stream_outgoing(stream_t* stream, int* amount)
{
  *amount = res_buffer_avail(&stream->write);
  return res_buffer_data(&stream->write);
}

