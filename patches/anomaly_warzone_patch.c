#define _GNU_SOURCE 
#include <dlfcn.h> 
#include <semaphore.h> 
#include <stdio.h> 
#include <time.h> 
#include <unistd.h> 
static int (*_realSemTimedWait)(sem_t *, const struct timespec *) = NULL; 

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{ 
    if (abs_timeout->tv_nsec >= 1000000000)
    { 
        //fprintf(stderr, "to: %lu:%lu\n", abs_timeout->tv_sec, abs_timeout->tv_nsec); 
        ((struct timespec *)abs_timeout)->tv_nsec -= 1000000000; 
        ((struct timespec *)abs_timeout)->tv_sec++; 
    } 
    return _realSemTimedWait(sem, abs_timeout); 
} 

__attribute__((constructor)) void init(void) 
{
    _realSemTimedWait = dlsym(RTLD_NEXT, "sem_timedwait");
}
