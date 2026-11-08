#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Return:
 * 0 - Not found
 * 1 - Found */
static int find_envvar(const char *source, const char **start_pos, size_t *length) {
    const char *reader;
    size_t size = 1;
    int waiting_close = 0;
    
    *length = 0;
    *start_pos = NULL;

    if (source == NULL || *source == '\0') {
        return 0;
    }

    *start_pos = strchr(source, '$');

    if (*start_pos == NULL) {
        return 0;
    }
    /* Skip $, since all envvars start with this char */
    reader = *start_pos + 1;
    
    while (*reader != '\0') {
        if (*reader == '_' || isalnum(*reader)) {
            if (size <= 2 && isdigit(*reader)) {
                goto not_found;
            }
            size++;
        } 
        else if (*reader == '{') {
            if (size != 1) {
                goto not_found;
            }
            size++;
            waiting_close = 1;
        } 
        else if (*reader == '}') {
            if ((waiting_close && size == 2) || size == 1) {
                goto not_found;
            }
            
            if (waiting_close) {
                size++;
            }
            
            waiting_close = 0;
            break;
        } 
        else
            break;

        reader++;
    }

    if (waiting_close) {
        goto not_found;
    }

    *length = size;
    return 1;

not_found:
    *length = 0;
    *start_pos = NULL;
    return 0;
}

/* Expand env variables in 'source' arg and save to 'result'
 *  Return:
 * 1 - Success
 * 0 - Fail */
int expand_envvar(const char *source, char *result, size_t max_size) {
    const char *envvar_p;
    size_t envvar_name_size = 0;

    *result = '\0';

    while (find_envvar(source, &envvar_p, &envvar_name_size)) {
        char *envvar_name, *envvar_value;
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

/* Format a custom mail command. Only %s substitutes the validated MAILFROM
 * value; %% escapes a literal percent. Other percent sequences remain
 * unchanged, preserving existing -m command arguments. */
int format_mail_command(const char *template, const char *sender,
                        char *output, size_t max_size) {
    size_t used = 0;

    if (!template || !sender || !output || max_size == 0)
        return 0;

    while (*template) {
        const char *replacement = NULL;
        size_t length;

        if (*template == '%' && template[1] == 's') {
            replacement = sender;
            template += 2;
        } else if (*template == '%' && template[1] == '%') {
            replacement = "%";
            template += 2;
        }

        if (replacement) {
            length = strlen(replacement);
            if (length > max_size - used - 1)
                goto too_long;
            memcpy(output + used, replacement, length);
            used += length;
        } else {
            if (used == max_size - 1)
                goto too_long;
            output[used++] = *template++;
        }
    }

    output[used] = '\0';
    return 1;

too_long:
    output[0] = '\0';
    return 0;
}
