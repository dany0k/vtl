#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H

/*
 * Тонкий слой совместимости — потоки.
 * Под Linux/macOS: pthread
 * Под Windows: WinAPI (CreateThread / WaitForSingleObject)
 *
 * Идентичен VTL_publication_text_op_asciidoc_compat.h — вынесен отдельно
 * чтобы модуль discord не зависел от модуля asciidoc.
 */

#ifdef _WIN32

#include <windows.h>
#include <stdint.h>

typedef HANDLE vtl_thread_t;

static inline int vtl_thread_create(vtl_thread_t* t, void* (*fn)(void*), void* arg)
{
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(void*)fn, arg, 0, NULL);
    return (*t == NULL) ? -1 : 0;
}

static inline void vtl_thread_join(vtl_thread_t t)
{
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
}

#else

#include <pthread.h>

typedef pthread_t vtl_thread_t;

static inline int vtl_thread_create(vtl_thread_t* t, void* (*fn)(void*), void* arg)
{
    return pthread_create(t, NULL, fn, arg);
}

static inline void vtl_thread_join(vtl_thread_t t)
{
    pthread_join(t, NULL);
}

#endif

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_COMPAT_H */
