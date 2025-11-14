#include "unity_handlers.h"
#include <setjmp.h>
#include <unistd.h>
#include <unity.h>
#include <unity_internals.h>
#include <stdio.h>
#include <stdlib.h>

static pid_t pid;

int custom_test_protect(void) {
    pid = getpid();
    return setjmp(Unity.AbortFrame) == 0;
}

_Noreturn void custom_test_abort(void) {
    // Many tests use fork()
    // The child process must exit on test failure
    if (getpid() != pid) {
        fflush(stderr);
        fflush(stdout);
        // allow TEST_PASS() to _Exit(0);
        _Exit((int) Unity.CurrentTestFailed);
    }
    longjmp(Unity.AbortFrame, 1);
}
