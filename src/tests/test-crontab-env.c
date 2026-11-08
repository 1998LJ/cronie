#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cronie_common.h"

static void test_assignment(const char *source, const char *expected,
                            char *const *envp) {
    char output[255];
    assert(expand_crontab_assignment(source, output, sizeof output, envp,
                                     "/home/alice", "alice"));
    assert(strcmp(output, expected) == 0);
}

int main(void) {
    char output[255];
    char *empty[] = {NULL};
    char *prior[] = {"NAME=private", "HOME=/alt/home", "EMPTY=", NULL};

    assert(setenv("HOME", "/root", 1) == 0);
    assert(setenv("USER", "root", 1) == 0);
    assert(setenv("LOGNAME", "root", 1) == 0);
    assert(setenv("ONLY_PROCESS", "fallback", 1) == 0);

    test_assignment("PATH=$HOME/.local/bin:/usr/bin",
                    "PATH=/home/alice/.local/bin:/usr/bin", empty);
    test_assignment("MAILTO=${USER}@example.com", "MAILTO=alice@example.com", empty);
    test_assignment("OWNER=$LOGNAME", "OWNER=alice", empty);
    test_assignment("PATH=$HOME/$NAME", "PATH=/alt/home/private", prior);
    test_assignment("PATH=$EMPTY/x", "PATH=/x", prior);
    test_assignment("PATH=$ONLY_PROCESS/x", "PATH=fallback/x", empty);
    test_assignment("NAME=no-variables", "NAME=no-variables", empty);

    assert(expand_crontab_assignment("NAME=$USER", output, 11, empty,
                                     "/home/alice", "alice"));
    assert(strcmp(output, "NAME=alice") == 0);
    assert(!expand_crontab_assignment("NAME=$USER", output, 10, empty,
                                      "/home/alice", "alice"));
    assert(!expand_crontab_assignment("NAME=$USER", output, 0, empty,
                                      "/home/alice", "alice"));
    assert(!expand_crontab_assignment("INVALID", output, sizeof output, empty,
                                      "/home/alice", "alice"));

    /* Existing process-environment API must remain compatible. */
    assert(expand_envvar("$USER", output, sizeof output));
    assert(strcmp(output, "root") == 0);
    return 0;
}
