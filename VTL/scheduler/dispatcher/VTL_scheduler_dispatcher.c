#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/scheduler/dispatcher/VTL_scheduler_dispatcher.h>
#include <VTL/scheduler/metadata/VTL_scheduler_metadata.h>
#include <VTL/utils/threading/VTL_thread_compat.h>

#include <VTL/content_platform/tg/VTL_content_platform_tg_net.h>
#include <VTL/content_platform/reddit/VTL_content_platform_reddit_net.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#  define vtl_sleep_sec(s) Sleep((DWORD)((s) * 1000))
#else
#  include <unistd.h>
#  define vtl_sleep_sec(s) sleep((unsigned)(s))
#endif

#ifdef _MSC_VER
#  define strdup _strdup
#endif

/* MAX_PATH — только в windows.h, поэтому общий буфер задаём своей
 * константой. 512 байт с запасом покрывает оба MAX_PATH (260) и
 * типичный PATH_MAX в Linux (4096 теоретически, но tmpdir сильно
 * короче). */
#define VTL_TMP_PATH_LEN 512


/*
 * Кладёт во временный файл проекта путь /<tmp>/vtl_sched_<id>.txt.
 * Под Windows /tmp/ не существует — берём результат GetTempPathA.
 */
static void make_tmp_path(char* buf, size_t sz, long long id)
{
#ifdef _WIN32
    char tmpdir[MAX_PATH];
    DWORD n = GetTempPathA(MAX_PATH, tmpdir);
    if (n == 0 || n > MAX_PATH) {
        snprintf(buf, sz, "vtl_sched_%lld.txt", id);
    } else {
        snprintf(buf, sz, "%svtl_sched_%lld.txt", tmpdir, id);
    }
#else
    snprintf(buf, sz, "/tmp/vtl_sched_%lld.txt", id);
#endif
}


/*
 * TG-сервис читает TG_CHAT_ID из env — менять переменную и звать
 * SendNow нужно атомарно, иначе параллельные потоки перепишут chat_id
 * друг другу. Mutex инициализируется лениво через vtl_once, чтобы
 * VTL_scheduler_dispatcher_Send можно было вызывать и напрямую,
 * минуя _Run.
 */
static vtl_mutex_t s_tg_env_mutex;
static vtl_once_t  s_tg_env_once = VTL_ONCE_INIT;

static void init_tg_env_mutex(void)
{
    vtl_mutex_init(&s_tg_env_mutex);
}

static VTL_AppResult send_tg(const VTL_scheduler_Post* post)
{
    VTL_scheduler_MetaTG meta;
    if (VTL_scheduler_meta_DeserializeTG(post->metadata, &meta) != VTL_res_kOk) {
        fprintf(stderr, "[dispatcher] TG post id=%lld: bad metadata\n", post->id);
        return VTL_res_kErr;
    }

    VTL_AppResult res;

    vtl_once_call(&s_tg_env_once, init_tg_env_mutex);
    vtl_mutex_lock(&s_tg_env_mutex);

#ifndef _WIN32
    setenv("TG_CHAT_ID", meta.chat_id, 1);
#else
    SetEnvironmentVariableA("TG_CHAT_ID", meta.chat_id);
#endif

    if (post->content_type == VTL_content_kTxt) {
        char tmp_path[VTL_TMP_PATH_LEN];
        make_tmp_path(tmp_path, sizeof(tmp_path), post->id);
        FILE* f = fopen(tmp_path, "wb");
        if (!f) {
            vtl_mutex_unlock(&s_tg_env_mutex);
            return VTL_res_kFileOpenErr;
        }
        fwrite(post->content, 1, strlen(post->content), f);
        fclose(f);

        if (meta.parse_mode[0] != '\0') {
            res = VTL_content_platform_tg_marked_text_SendNow(tmp_path);
        } else {
            res = VTL_content_platform_tg_text_SendNow(tmp_path);
        }
        remove(tmp_path);

    } else {
        const char* path = post->content;
        const char* ext  = strrchr(path, '.');

        if (ext && (strcmp(ext, ".mp3") == 0 ||
                    strcmp(ext, ".ogg") == 0 ||
                    strcmp(ext, ".flac") == 0)) {
            res = VTL_content_platform_tg_audio_SendNow(path);
        } else if (ext && (strcmp(ext, ".mp4") == 0 ||
                           strcmp(ext, ".mkv") == 0)) {
            res = VTL_content_platform_tg_video_SendNow(path);
        } else if (ext && (strcmp(ext, ".jpg") == 0 ||
                           strcmp(ext, ".jpeg") == 0 ||
                           strcmp(ext, ".png") == 0)) {
            res = VTL_content_platform_tg_photo_SendNow(path);
        } else {
            res = VTL_content_platform_tg_document_SendNow(path);
        }
    }

    vtl_mutex_unlock(&s_tg_env_mutex);
    return res;
}

static VTL_AppResult send_reddit(const VTL_scheduler_Post* post)
{
    VTL_scheduler_MetaReddit meta;
    if (VTL_scheduler_meta_DeserializeReddit(post->metadata, &meta) != VTL_res_kOk) {
        fprintf(stderr, "[dispatcher] Reddit post id=%lld: bad metadata\n", post->id);
        return VTL_res_kErr;
    }

    if (post->content_type == VTL_content_kFilePath) {
        /* Для фото/видео описание лежит в .md рядом с файлом — это
         * контракт сервиса reddit_upload_photo. */
        char md_path[VTL_TMP_PATH_LEN];
        snprintf(md_path, sizeof(md_path), "%s.md", post->content);
        return VTL_wrapped_reddit_upload_photo(meta.subreddit,
                                               post->content,
                                               md_path);
    } else {
        char tmp_path[VTL_TMP_PATH_LEN];
        make_tmp_path(tmp_path, sizeof(tmp_path), post->id);
        FILE* f = fopen(tmp_path, "wb");
        if (!f) return VTL_res_kFileOpenErr;
        fwrite(post->content, 1, strlen(post->content), f);
        fclose(f);
        VTL_AppResult res = VTL_wrapped_reddit_send_text(meta.subreddit, tmp_path);
        remove(tmp_path);
        return res;
    }
}

/*
 * VK-драйвер в проекте пока не реализован — заглушка, чтобы scheduler
 * мог хранить и отдавать VK-записи без падений до появления клиента.
 */
static VTL_AppResult send_vk(const VTL_scheduler_Post* post)
{
    VTL_scheduler_MetaVK meta;
    if (VTL_scheduler_meta_DeserializeVK(post->metadata, &meta) != VTL_res_kOk) {
        fprintf(stderr, "[dispatcher] VK post id=%lld: bad metadata\n", post->id);
        return VTL_res_kErr;
    }

    fprintf(stderr, "[dispatcher] VK send stub: peer_id=%lld content=%.60s\n",
            meta.peer_id,
            post->content ? post->content : "(file)");

    return VTL_res_kOk;
}


VTL_AppResult VTL_scheduler_dispatcher_Send(VTL_scheduler_Repo*        repo,
                                            const VTL_scheduler_Post*  post)
{
    if (!post) return VTL_res_kInvalidParamErr;

    fprintf(stdout, "[dispatcher] sending post id=%lld sn=%s\n",
            post->id, post->sn_type == VTL_sn_kTG ? "TG" :
                      post->sn_type == VTL_sn_kReddit ? "REDDIT" : "VK");

    VTL_AppResult res;
    switch (post->sn_type) {
        case VTL_sn_kTG:
            res = send_tg(post);
            break;
        case VTL_sn_kReddit:
            res = send_reddit(post);
            break;
        case VTL_sn_kVK:
            res = send_vk(post);
            break;
        default:
            fprintf(stderr, "[dispatcher] post id=%lld: unknown sn_type\n",
                    post->id);
            return VTL_res_kErr;
    }

    if (res == VTL_res_kOk) {
        if (repo) VTL_scheduler_repo_MarkExecuted(repo, post->id);
        fprintf(stdout, "[dispatcher] post id=%lld sent OK\n", post->id);
    } else {
        fprintf(stderr, "[dispatcher] post id=%lld send FAILED (code %d)\n",
                post->id, (int)res);
    }

    return res;
}


/* Контекст потока: каждый поток владеет своей копией строк записи. */
typedef struct _VTL_sched_WorkerCtx
{
    VTL_scheduler_Repo* repo;
    VTL_scheduler_Post  post;
} VTL_sched_WorkerCtx;

static void* worker_thread(void* arg)
{
    VTL_sched_WorkerCtx* ctx = (VTL_sched_WorkerCtx*)arg;
    VTL_scheduler_dispatcher_Send(ctx->repo, &ctx->post);
    VTL_scheduler_post_Free(&ctx->post);
    free(ctx);
    return NULL;
}

/* Глубокое копирование владельческих строк — иначе у потоков общая
 * память и double-free на VTL_scheduler_postlist_Free. */
static VTL_scheduler_Post post_deep_copy(const VTL_scheduler_Post* src)
{
    VTL_scheduler_Post dst = *src;
    dst.created_user = src->created_user ? strdup(src->created_user) : NULL;
    dst.content      = src->content      ? strdup(src->content)      : NULL;
    dst.metadata     = src->metadata     ? strdup(src->metadata)     : NULL;
    return dst;
}

static void dispatch_batch_parallel(VTL_scheduler_Repo*           repo,
                                    const VTL_scheduler_PostList* list)
{
    if (!list || list->length == 0) return;

    vtl_thread_t* threads = (vtl_thread_t*)malloc(list->length * sizeof(vtl_thread_t));
    if (!threads) return;

    size_t spawned = 0;
    for (size_t i = 0; i < list->length; ++i) {
        VTL_sched_WorkerCtx* ctx =
                (VTL_sched_WorkerCtx*)malloc(sizeof(VTL_sched_WorkerCtx));
        if (!ctx) break;

        ctx->repo = repo;
        ctx->post = post_deep_copy(&list->items[i]);

        if (vtl_thread_create(&threads[spawned], worker_thread, ctx) != 0) {
            /* Не смогли создать поток — fallback на синхронную отправку,
             * чтобы запись всё-таки ушла. */
            fprintf(stderr, "[dispatcher] vtl_thread_create failed for id=%lld,"
                            " falling back to sync\n", list->items[i].id);
            VTL_scheduler_dispatcher_Send(repo, &list->items[i]);
            VTL_scheduler_post_Free(&ctx->post);
            free(ctx);
        } else {
            ++spawned;
        }
    }

    for (size_t i = 0; i < spawned; ++i) {
        vtl_thread_join(threads[i]);
    }

    free(threads);
}


void VTL_scheduler_dispatcher_Run(VTL_scheduler_Repo* repo,
                                  int                 poll_interval_sec,
                                  int                 lookahead_sec,
                                  volatile int*       stop_flag)
{
    if (!repo || !stop_flag) return;

    fprintf(stdout, "[dispatcher] started: poll=%ds lookahead=%ds\n",
            poll_interval_sec, lookahead_sec);

    while (!(*stop_flag)) {
        VTL_scheduler_PostList list;
        memset(&list, 0, sizeof(list));

        VTL_AppResult res = VTL_scheduler_repo_GetPending(repo,
                                                          lookahead_sec,
                                                          &list);
        if (res == VTL_res_kOk && list.length > 0) {
            fprintf(stdout, "[dispatcher] found %zu pending post(s)\n",
                    list.length);
            dispatch_batch_parallel(repo, &list);
        }

        VTL_scheduler_postlist_Free(&list);

        /* Спим секундными шагами, чтобы быстро среагировать на stop_flag
         * и не висеть до конца poll_interval_sec. */
        for (int s = 0; s < poll_interval_sec && !(*stop_flag); ++s) {
            vtl_sleep_sec(1);
        }
    }

    fprintf(stdout, "[dispatcher] stopped\n");
}
