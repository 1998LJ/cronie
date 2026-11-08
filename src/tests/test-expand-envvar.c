#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#include "cronie_common.h"

int main(void) {
    char out[255];
    char *envp[] = {"USER=alice", "HOME=/home/alice", "NAME=scheduled", "EMPTY=", NULL};
    assert(setenv("USER", "daemon", 1) == 0);
    assert(setenv("SOURCE_ONLY", "process", 1) == 0);

    assert(expand_envvar_in_env("cron-$USER@mydomain.com", out, sizeof out, envp));
    assert(strcmp(out, "cron-alice@mydomain.com") == 0);
    assert(expand_envvar_in_env("cron-${USER}@mydomain.com", out, sizeof out, envp));
    assert(strcmp(out, "cron-alice@mydomain.com") == 0);
    assert(expand_envvar_in_env("${NAME}-$USER@domain", out, sizeof out, envp));
    assert(strcmp(out, "scheduled-alice@domain") == 0);
    assert(expand_envvar_in_env("${EMPTY}-x", out, sizeof out, envp));
    assert(strcmp(out, "-x") == 0);
    assert(expand_envvar_in_env("$SOURCE_ONLY", out, sizeof out, envp));
    assert(strcmp(out, "process") == 0);
    assert(expand_envvar("cron-$USER@mydomain.com", out, sizeof out));
    assert(strcmp(out, "cron-daemon@mydomain.com") == 0);
    assert(expand_envvar_in_env("test", out, sizeof out, envp));
    assert(strcmp(out, "test") == 0);
    assert(expand_envvar_in_env("$USER", out, 6, envp));
    assert(strcmp(out, "alice") == 0);
    assert(!expand_envvar_in_env("$USER", out, 5, envp));
    assert(!expand_envvar_in_env("$USER", out, 0, envp));
    assert(!expand_envvar_in_env(NULL, out, sizeof out, envp));
    assert(!expand_envvar_in_env("$USER", NULL, sizeof out, envp));
    return 0;
}
