#ifndef GG_TEST_PROCESS_WAIT_H
#define GG_TEST_PROCESS_WAIT_H

#include <gg/error.h>
#include <sys/types.h>

GgError gg_process_wait(pid_t pid);

#endif
