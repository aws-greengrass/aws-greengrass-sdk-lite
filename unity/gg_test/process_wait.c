#include "gg/process_wait.h"
#include <errno.h>
#include <gg/error.h>
#include <gg/log.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

GgError gg_process_wait(pid_t pid) {
    while (true) {
        siginfo_t info = { 0 };
        int ret = waitid(P_PID, (id_t) pid, &info, WEXITED);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            GG_LOGE(
                "Err %d when calling waitid on process %d.", errno, (int) pid
            );
            return GG_ERR_FAILURE;
        }

        switch (info.si_code) {
        case CLD_EXITED:
            if (info.si_status != 0) {
                GG_LOGE(
                    "Process %d exited with status %d.",
                    (int) pid,
                    (int) info.si_status
                );
                return GG_ERR_FAILURE;
            }
            return GG_ERR_OK;
        case CLD_KILLED:
        case CLD_DUMPED:
            GG_LOGE("Process %d exited abnormally", (int) pid);
            return GG_ERR_FAILURE;
        default:;
        }
    }
}
