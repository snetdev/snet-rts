#ifndef _BUFFER_H_
#define _BUFFER_H_

#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 16
#endif


typedef struct buffer buffer_t;





extern buffer_t *BufferCreate( void);
extern void BufferDestroy( buffer_t *buf);
extern void *BufferTop( buffer_t *buf);
extern void BufferPop( buffer_t *buf);
extern int BufferIsSpace( buffer_t *buf);
extern void BufferPut( buffer_t *buf, void *item);
extern void **BufferGetWptr( buffer_t *buf);
extern void **BufferGetRptr( buffer_t *buf);
extern void BufferRegisterFlag( buffer_t *buf, volatile void **flag_ptr);

#endif /* _BUFFER_H_ */
