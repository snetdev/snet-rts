#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "resdefs.h"
#include "resstream.h"

typedef struct buffer buffer_t;
typedef struct stream stream_t;

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
  if ((n = res_socket_receive(fd, buf->data + buf->end, amount)) == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    } else {
      perror("read");
      return -1;
    }
  } else if (!n) {
    return -1;
  } else {
    buf->end += n;
    return n;
  }
}

int res_buffer_write(buffer_t* buf, int fd, int amount)
{
  char* data = buf->data + buf->start;
  int n;

  assert(buf->end > buf->start);
  assert(amount <= buf->end - buf->start);

  if ((n = res_socket_send(fd, data, amount)) == -1) {
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

void res_buffer_append(buffer_t* buf, const char* data, int size)
{
  res_buffer_reserve(buf, size);
  assert(buf->end + size <= buf->size);
  memcpy(buf->data + buf->end, data, size);
  res_buffer_appended(buf, size);
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
  if (stream->fd >= 0) {
    res_socket_close(stream->fd);
  }
}

stream_t* res_stream_create(int fd)
{
  stream_t* stream = xnew(stream_t);
  res_stream_init(stream, fd);
  return stream;
}

void res_stream_destroy(stream_t* stream)
{
  res_stream_done(stream);
  xfree(stream);
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
  return res_buffer_data(&stream->write)
       + res_buffer_stored(&stream->write);
}

stream_t* res_stream_from_string(const char* string)
{
  stream_t* stream = res_stream_create(-1);
  int len = strlen(string);
  res_buffer_append(&stream->read, string, len + 1);
  return stream;
}

void res_stream_reply(stream_t* stream, const char* fmt, va_list ap)
{
  const int max_reserve = 1024;
  const char *str = fmt, *arg;
  int size = 0;
  char* data = res_stream_outgoing(stream, &size);
  char* start = data;
  char* end = data + size;

  for (str = fmt; *str; ++str) {
    if (start + 50 > end) {
      res_stream_appended(stream, start - data);
      res_stream_reserve(stream, max_reserve);
      data = res_stream_outgoing(stream, &size);
      assert(size >= max_reserve);
      start = data;
      end = data + size;
    }
    if (*str != '%') {
      *start++ = *str;
      *start = '\0';
    } else if (str[1] == '\0') {
      assert(false);
    } else {
      switch (*++str) {
        case 's': {
          for (arg = va_arg(ap, char*); *arg; ++arg) {
            if (start + 10 > end) {
              res_stream_appended(stream, start - data);
              res_stream_reserve(stream, max_reserve);
              data = res_stream_outgoing(stream, &size);
              assert(size >= max_reserve);
              start = data;
              end = data + size;
            }
            *start++ = *arg;
            *start = '\0';
          }
        } break;
        case 'd': {
          char *first = start, *last;
          int n = va_arg(ap, int);
          res_assert(n >= 0, "Negative numbers not implemented.");
          /* First convert the number and store in reverse order. */
          do {
            *start++ = ((n % 10) + '0');
            *start = '\0';
            n /= 10;
          } while (n);
          /* Then reverse the order of the stored digits. */
          for (last = start - 1; last > first; --last, ++first) {
            char save = *first; *first = *last; *last = save;
          }
        } break;
        default: res_assert(false, "Unimplemented conversion.");
      }
    }
  }

  res_stream_appended(stream, start - data);
}
