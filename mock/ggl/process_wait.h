#ifndef GGL_TEST_PROCESS_WAIT_H
#define GGL_TEST_PROCESS_WAIT_H

#ifdef __cplusplus
#include <ggl/error.hpp>
#else
#include <ggl/error.h>
#endif

#include <sys/types.h>
#include <stdbool.h>

GglError gg_process_wait(pid_t pid, bool *exit_status);

#endif
