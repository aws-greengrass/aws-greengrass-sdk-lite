#include "unity_handlers.h"
#include <gg/log.h>
#include <setjmp.h>
#include <unistd.h>
#include <unity.h>
#include <unity_internals.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static pid_t pid = -1;

__attribute__((weak)) int gg_test_protect(void) {
    pid = getpid();
    bool first_return = setjmp(Unity.AbortFrame) == 0;
    if (first_return) {
        GG_LOGT("pid=%d protecting.", (int) getpid());
    } else {
        GG_LOGT("pid=%d long jumped.", (int) getpid());
    }
    return setjmp(Unity.AbortFrame) == 0;
}

__attribute__((weak)) void gg_test_abort(void) {
    // Many tests use fork()
    // The child process must exit on test completion
    if (getpid() != pid) {
        fflush(stderr);
        fflush(stdout);
        GG_LOGT(
            "pid=%d exiting (%d).",
            (int) getpid(),
            (int) Unity.CurrentTestFailed
        );
        // allow TEST_PASS() to _Exit(0);
        _Exit((int) Unity.CurrentTestFailed);
    }
    GG_LOGT("pid=%d long jumping.", getpid());
    longjmp(Unity.AbortFrame, 1);
}
