#ifndef _VTL_SCHEDULER_REPO_H
#define _VTL_SCHEDULER_REPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/scheduler/VTL_scheduler_data.h>
#include <VTL/utils/db/VTL_utils_db_credentials.h>
#include <VTL/VTL_app_result.h>
#include <libpq-fe.h>
#include <time.h>

typedef struct _VTL_scheduler_Repo
{
    PGconn* conn;
} VTL_scheduler_Repo;

VTL_AppResult VTL_scheduler_repo_Open(VTL_scheduler_Repo*  repo,
                                       VTL_db_Credentals*  creds);

void          VTL_scheduler_repo_Close(VTL_scheduler_Repo* repo);

VTL_AppResult VTL_scheduler_repo_EnsureTable(VTL_scheduler_Repo* repo);

VTL_AppResult VTL_scheduler_repo_Insert(VTL_scheduler_Repo* repo,
                                         VTL_scheduler_Post* post);

VTL_AppResult VTL_scheduler_repo_GetById(VTL_scheduler_Repo* repo,
                                          long long           id,
                                          VTL_scheduler_Post* out);

/* Pending = executed=false И send_date_time <= NOW()+lookahead_sec. */
VTL_AppResult VTL_scheduler_repo_GetPending(VTL_scheduler_Repo*     repo,
                                              int                     lookahead_sec,
                                              VTL_scheduler_PostList* list);

VTL_AppResult VTL_scheduler_repo_Update(VTL_scheduler_Repo*       repo,
                                         const VTL_scheduler_Post* post);

VTL_AppResult VTL_scheduler_repo_MarkExecuted(VTL_scheduler_Repo* repo,
                                               long long           id);

VTL_AppResult VTL_scheduler_repo_Delete(VTL_scheduler_Repo* repo,
                                         long long           id);

/* Освобождает строки внутри Post/PostList — сами структуры остаются
 * (Post в стеке вызывающего, items внутри PostList). */
void VTL_scheduler_post_Free(VTL_scheduler_Post* post);
void VTL_scheduler_postlist_Free(VTL_scheduler_PostList* list);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_REPO_H */
