#include "gg/process_wait.h"
#include <errno.h>
#include <gg/error.h>
#include <gg/log.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

GgError gg_process_wait(pid_t pid, bool *exit_status) {
    while (true) {
        siginfo_t info = { 0 };
        int ret = waitid(P_PID, (id_t) pid, &info, WEXITED);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            GG_LOGE("Err %d when calling waitid.", errno);
            return GG_ERR_FAILURE;
        }

        switch (info.si_code) {
        case CLD_EXITED:
            if (exit_status != NULL) {
                *exit_status = info.si_status == 0;
            }
            return GG_ERR_OK;
        case CLD_KILLED:
        case CLD_DUMPED:
            if (exit_status != NULL) {
                *exit_status = false;
            }
            return GG_ERR_OK;
        default:;
        }
    }
}
