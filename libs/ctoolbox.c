#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

/**
 * Prints given msg along with the current errno and its
 * string description.
 *
 * More on errno: http://man7.org/linux/man-pages/man3/errno.3.html
 *
 * @param msg
 */
void printerrno(char *msg) {
    char *error_msg = strerror(errno);
    if (msg == NULL) {
        printf("%s (%d)\n", error_msg, errno);
    } else {
        printf("%s: %s (%d)\n", msg, error_msg, errno);
    }
    exit(errno);
}

/**
 * Checks whether two string are equal.
 * Shortcut for (strcmp(str1, str2) == 0)
 *
 * @param str1
 * @param str2
 * @return
 */
bool streq(char *str1, char *str2) {
   return strcmp(str1, str2) == 0;
}