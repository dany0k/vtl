#ifndef _VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H
#define _VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H

/* Потоки: pthread на UNIX, WinAPI на Windows */
#ifdef _WIN32
#  include <windows.h>
   typedef HANDLE vtl_thread_t;
   static inline int  vtl_thread_create(vtl_thread_t* t, void*(*fn)(void*), void* a)
                      { *t = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)(void*)fn,a,0,NULL); return *t?0:-1; }
   static inline void vtl_thread_join(vtl_thread_t t) { WaitForSingleObject(t,INFINITE); CloseHandle(t); }
#else
#  include <pthread.h>
   typedef pthread_t vtl_thread_t;
   static inline int  vtl_thread_create(vtl_thread_t* t, void*(*fn)(void*), void* a) { return pthread_create(t,NULL,fn,a); }
   static inline void vtl_thread_join(vtl_thread_t t) { pthread_join(t,NULL); }
#endif

/* Монотонный таймер - для замеров времени */
#ifdef _WIN32
static inline double vtl_monotonic_seconds(void)
{
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}
#else
#  include <time.h>
static inline double vtl_monotonic_seconds(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
#endif

#endif /* _VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H */
