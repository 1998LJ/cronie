#include <assert.h>
#include <string.h>

#include "cronie_common.h"

static void test_format(void) {
    char output[128];

    assert(format_mail_command("/usr/sbin/sendmail -t -f %s",
                               "user@example.com", output, sizeof output));
    assert(strcmp(output, "/usr/sbin/sendmail -t -f user@example.com") == 0);

    assert(format_mail_command("/usr/bin/mail -t", "sender", output, sizeof output));
    assert(strcmp(output, "/usr/bin/mail -t") == 0);

    assert(format_mail_command("app --literal=%%s --pattern=%x --sender=%s",
                               "jane", output, sizeof output));
    assert(strcmp(output, "app --literal=%s --pattern=%x --sender=jane") == 0);

    assert(format_mail_command("command --sender=%s", "user%tag@example.com",
                               output, sizeof output));
    assert(strcmp(output, "command --sender=user%tag@example.com") == 0);

    assert(format_mail_command("command%", "user", output, sizeof output));
    assert(strcmp(output, "command%") == 0);

    assert(format_mail_command("a%s%s", "b", output, sizeof output));
    assert(strcmp(output, "abb") == 0);

    assert(format_mail_command("%s", "abcd", output, 5));
    assert(strcmp(output, "abcd") == 0);

    assert(!format_mail_command("%s", "abcd", output, 4));
    assert(output[0] == '\0');

    assert(!format_mail_command("--long-flag", "any", output, 5));
    assert(output[0] == '\0');

    assert(!format_mail_command("x", "y", output, 0));
}

int main(void) {
    test_format();
    return 0;
}
