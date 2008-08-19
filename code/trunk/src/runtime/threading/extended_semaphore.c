/**
 * $Id$
 */

/*
 * @file
 * 
 * This implements a semaphore with min- and max-value, functions to wait 
 * and functions to check if the semaphore's value is maximal or minimal.
 *
 * This semaphore was implemented, because we want to use a custom
 * scheduling mechanism, and the pthread-provided semaphore is a 
 * black box, that is, we cannot modify the event mechanism used in it.
 * Furthermore, the pothread-semaphore does not offer all operations we
 * need. For example, we have a blocking, non-removing read, 
 * which waits while the semaphore has value 0, but does not 
 * decrement the value of the semaphore.
 * Furhtermore, we cannot use the pthread-semaphore on the buffer, because
 * in the buffer, we have a maximum value and a minimum value.
 *
 * Thus, I implemented this extended semaphore.
 * At first, this semaphore has either a minimum value, a maximum value
 * or both. If you supply neither boundary, an error will be signalled, 
 * because in that case, the semaphore would never block and thus it would
 * be a useless bunch of memory.
 * The minimum and maximum values work like a semaphore works: If the current
 * value of the semaphore is the minimum value, then decrement and 'wait while
 * min value' will block until the value is not the minimum value anymore, 
 * after that, decrement will decrease the value of the semaphore by 1.
 * If the value of the semaphore is the specified maximum value, Increment
 * and 'wait while max value' will block, once the value has been decremented
 * at least once, increment will increase the value of the semaphore by one.
 * Furthermore, the semaphore provides two functions 'is minimal' and 'is
 * maximal', which return true if a minimum (maximum) value has been defined
 * and the current value is equal to that. If no minimum (maximum) value has
 * been set, the functions just return false. This makes sense, because a
 * question 'semaphore, is your value maximal?' basically corresponds to 
 * 'buffer, is your message space full?'. A semaphore with no maximum value
 * corresponds to an unbounded buffer, and thus, the buffer would always answer
 * 'no.'.
 *
 * An important implementation detail is that the mutex that is passed in the
 * Create-function must be a fast mutex, because only the fast-mutex returns
 * EBUSY if the same thread tries to relock an already locked mutex. 
 * The mutex is passed in, because the semaphore is supposed to unlock a 
 * data structure it is used in on waiting and I have not found a better way
 * to check if a given mutex is locked.
 */
#include "extended_semaphore.h"
#include "bool.h"
#include "pthread.h"
#include "debug.h"
#include "memfun.h"
#include "errno.h"

struct extended_semaphore {
  pthread_mutex_t *access; /* must be fast mutex */


  /* has_min_value => min_value and value_increased welldefined */
  bool has_min_value;
  int min_value;
  pthread_cond_t *value_increased;

  /* has_max_value => max_value and value_decreased welldefined */
  bool has_max_value;
  int max_value;
  pthread_cond_t *value_decreased;

  /* has_min_value => min_value <= value,
   *  has_max_value => value <= max_value 
   */
  int value;
};

/* @fn static void CheckLock(snet_ex_sem_t *subject, char* function_name)
 * This functions checks if the semaphore is locked and signals an error if
 * it is not.
 * 
 * This checks if the semaphore is locked by calling trylock and checking
 * the return value (as said above, this assumes fast mutexes). The parameter
 * function_name is inserted into the error string and must be nullterminated.
 * Furthermore, this function assumes the function to have exactly one argument
 * called target.
 *
 * @param subject the semaphore to check
 * @param function_name the name of the function
 */
static void CheckLock(snet_ex_sem_t *subject, char *function_name) {
  int access_status;
  access_status = pthread_mutex_trylock(subject->access);
  if(access_status != EBUSY) {
    SNetUtilDebugFatal("(ERROR (SEMAPHORE %x) "
                       "(%s (target %x))"
                       "(Semaphore not locked))",
                       (unsigned int) subject, 
                       function_name,
                       (unsigned int) subject);
  }
}

/* @fn snet_ex_sem_t SnetExSemCreate(pthread_mutex_t *access,
 *   bool has_min_value, int min_value,
 *   bool has_max_value, int max_value,
 *   int initial_value)
 * @brief Creates a semaphore with the potentially given boundaries and the 
 *        given initial value
 * 
 * This creates a new extended semaphore. The mutex passed in must be the
 * same as the mutex used by the surrounding data structure, as the semaphore
 * will unlock that mutex during blocking. If has_min_value is false, the
 * parameter min_value is ignored, otherwise, it must be a valid value, 
 * that is, it must be >= initial_value. The same goes for has_max_value,
 * max_value and initial_value <= max_value. If both has_min_value and
 * has_max_value are false, an error is signalled.
 * 
 * @param access the mutex to unlock during waiting
 * @param has_min_value true if a min_value exists
 * @param min_value the min value if it exists, false otherwise
 * @param has_max_value true if a max_value exists
 * @param max_value the maximum value if it exists
 * @param initial_value the initial value of the semaphore
 * @return a new semaphore
 */
snet_ex_sem_t *SNetExSemCreate(pthread_mutex_t *access,
                                     bool has_min_value, int min_value,
                                     bool has_max_value, int max_value,
                                     int initial_value) {
  snet_ex_sem_t *result;

  if(!has_min_value && !has_max_value) {
    SNetUtilDebugFatal("(ERROR (Semaphore Creation) "
                       "(Useless Semaphore without boundaries))");
  }
  result = SNetMemAlloc(sizeof(snet_ex_sem_t));

  SNetUtilDebugNotice("(CREATION (SEMAPHORE %x) (access %x))",
                      (unsigned int) result, (unsigned int) access);
  result->access = access;
  if(has_min_value) {
    if(initial_value < min_value) {
      SNetUtilDebugFatal("(ERROR (Semaphore Creation)"
                         "(%d = initial_value < min_value = %d))",
                         initial_value, min_value);
    }
    result->has_min_value = true;
    result->min_value = min_value;
    result->value_increased = SNetMemAlloc(sizeof(pthread_cond_t));
    pthread_cond_init(result->value_increased, NULL);
  } else {
    result->has_min_value = false;
    /* min_value and value increased will not be used anywhere */
  }

  if(has_max_value) {
    if(max_value < initial_value) {
      SNetUtilDebugFatal("(ERROR (Semaphore Creation)"
                         "(%d = max_value < initial_value = %d))",
                         max_value, initial_value);
    }
    result->has_max_value = true;
    result->max_value = max_value;
    result->value_decreased = SNetMemAlloc(sizeof(pthread_cond_t));
    pthread_cond_init(result->value_decreased, NULL);
  } else {
    result->has_max_value = false;
    /* max_value and value_decreased will not be used anywhere */
  }
  
  result->value = initial_value;
  return result;
}

/* @fn void SnetExSemDestroy(snet_ex_sem_t *victim)
 * @brief deallocates the semaphore
 */
void SNetExSemDestroy(snet_ex_sem_t *victim) {
  if(victim->has_min_value) {
    pthread_cond_destroy(victim->value_increased);
    SNetMemFree(victim->value_increased);
  }

  if(victim->has_max_value) {
    pthread_cond_destroy(victim->value_decreased);
    SNetMemFree(victim->value_decreased);
  }

  SNetMemFree(victim);
}

snet_ex_sem_t *SNetExSemIncrement(snet_ex_sem_t *target) {
  CheckLock(target, "SNetExSemIncrement");

  if(target->has_max_value) {
    if(target->value == target->max_value) {
      pthread_cond_wait(target->value_decreased, target->access);
    }
  }

  target->value += 1;

  if(target->has_min_value) {
    pthread_cond_signal(target->value_increased);
  }
  return target;
}

snet_ex_sem_t *SNetExSemDecrement(snet_ex_sem_t *target) {
  CheckLock(target, "SNetExSemDecrement");

  if(target->has_min_value) {
    if(target->value == target->min_value) {
      pthread_cond_wait(target->value_increased, target->access);
    }
  }

  target->value -= 1;

  if(target->has_max_value) {
    pthread_cond_signal(target->value_decreased);
  }
  return target;
}

void SNetExSemWaitWhileMinValue(snet_ex_sem_t *target) {
  CheckLock(target, "SNetExSemWaitWhileMinValue");

  if(!target->has_min_value) {
    SNetUtilDebugFatal("(ERROR (SEMAPHORE %x)"
                       "(SNetExSemWaitWhileMinValue (target  %x))",
                       "(Semaphore has no min value!))",
                       (unsigned int) target, (unsigned int) target);
  }
  if(target->value == target->min_value) {
    SNetUtilDebugNotice("waiting, releasing mutex %x", (unsigned int) target->access);
    pthread_cond_wait(target->value_increased, target->access);
    SNetUtilDebugNotice("semaphore done with waiting");
  }
}

void SNetExSemWaitWhileMaxValue(snet_ex_sem_t *target) {
  CheckLock(target, "SNetExSemWaitWhileMaxValue");
  
  if(!target->has_max_value) {
    SNetUtilDebugFatal("(ERROR (SEMAPHORE %x)"
                       "(SNetExSemWaitWhileMaxValue (target %x))"
                       "(Semaphore has no max value))",
                       (unsigned int) target, (unsigned int) target);
  }
  if(target->value == target->max_value) {
    pthread_cond_wait(target->value_decreased, target->access);
  }
}

int SNetExSemGetValue(snet_ex_sem_t *target) {
  CheckLock(target, "SNetExSemGetValue");
  return target->value;
}

bool SNetExSemIsMaximal(snet_ex_sem_t *target) {
  CheckLock(target, "SNetExSemIsMaximal");
  if(target->has_max_value) {
    return target->value == target->max_value;
  } else {
    return false;
  }
}

bool SNetExSemIsMinimal(snet_ex_sem_t *target) {
  SNetUtilDebugNotice("Semaphore access: %x\n", (unsigned int) target->access);
  CheckLock(target, "SNetExSemIsMinimal");
  if(target->has_min_value) {
    return target->value == target->min_value;
  } else {
    return false;
  }
}
