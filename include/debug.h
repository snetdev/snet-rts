#ifndef _DEBUG_H_
#define _DEBUG_H_

/*
 * reports an error and terminates the application after that.
 * parameters are equal to printf.
 */
extern void SNetUtilDebugFatal(char* message, ...);

/*
 * reports an error.
 * parameters are equal to printf.
 */
extern void SNetUtilDebugNotice(char* message, ...);



extern void SNetUtilDebugFatalTask(char* msg, ...);

extern void SNetUtilDebugNoticeTask(char* msg, ...);


#endif /* _DEBUG_H_ */
