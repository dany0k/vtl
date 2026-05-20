#ifndef _VTL_SCHEDULER_DISPATCHER_H
#define _VTL_SCHEDULER_DISPATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/scheduler/VTL_scheduler_data.h>
#include <VTL/scheduler/db/VTL_scheduler_db_repo.h>
#include <VTL/VTL_app_result.h>

VTL_AppResult VTL_scheduler_dispatcher_Send(VTL_scheduler_Repo*       repo,
                                             const VTL_scheduler_Post* post);

/* Блокирующий цикл; выход — stop_flag != 0 из другого потока. */
void VTL_scheduler_dispatcher_Run(VTL_scheduler_Repo* repo,
                                   int                 poll_interval_sec,
                                   int                 lookahead_sec,
                                   volatile int*       stop_flag);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_DISPATCHER_H */
