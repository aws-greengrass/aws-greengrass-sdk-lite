#ifndef GG_TEST_PROCESS_WAIT_H
#define GG_TEST_PROCESS_WAIT_H

#ifdef __cplusplus
#include <gg/error.hpp>
#else
#include <gg/error.h>
#endif

#include <sys/types.h>
#include <stdbool.h>

GgError gg_process_wait(pid_t pid, bool *exit_status);

#endif
