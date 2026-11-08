#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Find $NAME or ${NAME}; leave a literal or malformed dollar expression
 * untouched and keep searching for a later, valid reference. */
static int find_envvar(const char *source, const char **start_pos, size_t *length) {
    const char *candidate;

    *length = 0;
    *start_pos = NULL;
    if (source == NULL)
        return 0;

    for (candidate = strchr(source, '$'); candidate != NULL;
         candidate = strchr(candidate + 1, '$')) {
        const char *reader = candidate + 1;
        int braces = (*reader == '{');

        if (braces)
            reader++;
        if (*reader != '_' && !isalpha((unsigned char)*reader))
            continue;
        reader++;
        while (*reader == '_' || isalnum((unsigned char)*reader))
            reader++;
        if (braces) {
            if (*reader != '}')
                continue;
            reader++;
        }
        *start_pos = candidate;
        *length = (size_t)(reader - candidate);
        return 1;
    }
    return 0;
}

/* Look up variables in the cron job's environment before falling back
 * to the daemon process environment for backward compatibility. */
static const char *env_value(const char *name, char *const *envp) {
    size_t name_len;
    char *const *item;

    if (envp == NULL)
        return NULL;

    name_len = strlen(name);
    for (item = envp; *item != NULL; item++) {
        if (strncmp(*item, name, name_len) == 0 && (*item)[name_len] == '=')
            return *item + name_len + 1;
    }
    return NULL;
}

/* Expand env variables in 'source' arg and save to 'result'
 *  Return:
 * 1 - Success
 * 0 - Fail */
static int expand_envvar_impl(const char *source, char *result, size_t max_size,
                              char *const *envp, const char *home,
                              const char *user, int allow_process_env) {
    const char *envvar_p;
    size_t envvar_name_size = 0;

    if (source == NULL || result == NULL || max_size == 0)
        return 0;

    *result = '\0';

    while (find_envvar(source, &envvar_p, &envvar_name_size)) {
        char *envvar_name;
        const char *envvar_value;
        size_t prefix_size;
        
        /* Copy content before env var name */
        prefix_size = envvar_p - source;
        
        if (prefix_size > 0) {
            if ((strlen(result) + prefix_size + 1) > max_size) {
                goto too_big;
            }
            strncat(result, source, prefix_size);
        }
        
        /* skip envvar name */
        source = envvar_p + envvar_name_size;
        
        /* copy envvar name, ignoring $, { and } chars*/
        envvar_p++; 
        envvar_name_size--;
      
        if (*envvar_p == '{') {
            envvar_p++;
            envvar_name_size = envvar_name_size - 2;
        }

        envvar_name = malloc(envvar_name_size + 1);
        if (envvar_name == NULL)
            goto too_big;

        strncpy(envvar_name, envvar_p, envvar_name_size);
        envvar_name[envvar_name_size] = '\0';

        /* Copy envvar value to result */
        /* USER and LOGNAME are fixed by the crontab owner's passwd entry;
         * HOME can be changed by an earlier crontab assignment. */
        if (user && (strcmp(envvar_name, "USER") == 0 ||
                     strcmp(envvar_name, "LOGNAME") == 0))
            envvar_value = user;
        else
            envvar_value = env_value(envvar_name, envp);
        if (envvar_value == NULL && home && strcmp(envvar_name, "HOME") == 0)
            envvar_value = home;
        /* Job definitions must not gain access to arbitrary variables from
         * crond's privileged process environment. Keep that legacy fallback
         * only for the existing mail-expansion entry points. */
        if (envvar_value == NULL && allow_process_env)
            envvar_value = getenv(envvar_name);
        free(envvar_name);
        
        if (envvar_value != NULL) {
            if ((strlen(result) + strlen(envvar_value) + 1) > max_size) {
                goto too_big;
            }
            strcat(result, envvar_value);
        }
    }

    /* Copy any character left in the source string */
    if (*source != '\0') {
        if ((strlen(result) + strlen(source) + 1) > max_size) {
            goto too_big;
        }
        strcat(result, source);
    }
    
    return 1;
    
too_big:
    return 0;
}

/* Legacy process-environment expansion, retained for existing callers. */
int expand_envvar(const char *source, char *result, size_t max_size) {
    return expand_envvar_impl(source, result, max_size, NULL, NULL, NULL, 1);
}

/* Expand against the job environment, including its USER/HOME values. */
int expand_envvar_in_env(const char *source, char *result, size_t max_size,
                         char *const *envp) {
    return expand_envvar_impl(source, result, max_size, envp, NULL, NULL, 1);
}

/* Expand a full NAME=value crontab assignment using preceding assignments,
 * the crontab owner's HOME fallback and fixed USER/LOGNAME account values.
 * Leave NAME= unchanged, so the result can be passed to env_set(). */
int expand_crontab_assignment(const char *source, char *result, size_t max_size,
                              char *const *envp, const char *home,
                              const char *user) {
    const char *equals;
    size_t prefix_size;

    if (source == NULL || result == NULL || max_size == 0)
        return 0;

    equals = strchr(source, '=');
    if (equals == NULL)
        return 0;

    prefix_size = (size_t)(equals - source) + 1;
    if (prefix_size >= max_size)
        return 0;

    memcpy(result, source, prefix_size);
    return expand_envvar_impl(equals + 1, result + prefix_size,
                              max_size - prefix_size, envp, home, user, 0);
}
