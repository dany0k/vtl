#ifndef _VTL_THREAD_COMPAT_H
#define _VTL_THREAD_COMPAT_H

/*
 * Тонкий header-only слой совместимости для потоков и mutex'ов.
 * Linux/macOS — pthread, Windows — нативное WinAPI
 * (CreateThread / CRITICAL_SECTION).
 *
 * Параллельный по сути со слоем в publication/text/asciidoc/compat.h,
 * вынесен сюда чтобы любой модуль мог использовать без cross-module
 * зависимостей.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <windows.h>
#include <stdint.h>

typedef HANDLE           vtl_thread_t;
typedef CRITICAL_SECTION vtl_mutex_t;
typedef INIT_ONCE        vtl_once_t;
#define VTL_ONCE_INIT INIT_ONCE_STATIC_INIT

static inline int vtl_thread_create(vtl_thread_t* t, void* (*fn)(void*), void* arg)
{
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(void*)fn, arg, 0, NULL);
    return (*t == NULL) ? -1 : 0;
}

static inline void* vtl_thread_join(vtl_thread_t t)
{
    WaitForSingleObject(t, INFINITE);
    DWORD code = 0;
    GetExitCodeThread(t, &code);
    CloseHandle(t);
    return (void*)(uintptr_t)code;
}

static inline int vtl_mutex_init(vtl_mutex_t* m)
{
    InitializeCriticalSection(m);
    return 0;
}

static inline int vtl_mutex_destroy(vtl_mutex_t* m)
{
    DeleteCriticalSection(m);
    return 0;
}

static inline int vtl_mutex_lock(vtl_mutex_t* m)
{
    EnterCriticalSection(m);
    return 0;
}

static inline int vtl_mutex_unlock(vtl_mutex_t* m)
{
    LeaveCriticalSection(m);
    return 0;
}

/*
 * InitOnceExecuteOnce ждёт сигнатуру BOOL(*)(PINIT_ONCE, PVOID, PVOID*).
 * Прячем простой void(*)(void) callback в Parameter и зовём из батута.
 * Cast function-pointer ↔ data-pointer формально UB по C, но на Windows
 * платформенно гарантирован.
 */
static BOOL CALLBACK vtl_once_trampoline_(PINIT_ONCE once, PVOID param, PVOID* ctx)
{
    void (*fn)(void) = (void (*)(void))(uintptr_t)param;
    (void)once;
    (void)ctx;
    fn();
    return TRUE;
}

static inline void vtl_once_call(vtl_once_t* o, void (*fn)(void))
{
    InitOnceExecuteOnce(o, vtl_once_trampoline_,
                        (PVOID)(uintptr_t)fn, NULL);
}

#else

#include <pthread.h>

typedef pthread_t       vtl_thread_t;
typedef pthread_mutex_t vtl_mutex_t;
typedef pthread_once_t  vtl_once_t;
#define VTL_ONCE_INIT PTHREAD_ONCE_INIT

static inline int vtl_thread_create(vtl_thread_t* t, void* (*fn)(void*), void* arg)
{
    return pthread_create(t, NULL, fn, arg);
}

static inline void* vtl_thread_join(vtl_thread_t t)
{
    void* res = NULL;
    pthread_join(t, &res);
    return res;
}

static inline int vtl_mutex_init(vtl_mutex_t* m)
{
    return pthread_mutex_init(m, NULL);
}

static inline int vtl_mutex_destroy(vtl_mutex_t* m)
{
    return pthread_mutex_destroy(m);
}

static inline int vtl_mutex_lock(vtl_mutex_t* m)
{
    return pthread_mutex_lock(m);
}

static inline int vtl_mutex_unlock(vtl_mutex_t* m)
{
    return pthread_mutex_unlock(m);
}

static inline void vtl_once_call(vtl_once_t* o, void (*fn)(void))
{
    pthread_once(o, fn);
}

#endif

#ifdef __cplusplus
}
#endif

#endif
