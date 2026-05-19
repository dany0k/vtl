#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H

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

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H */
