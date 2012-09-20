/* Copyright: Pádraig Brady 2005
 * Summary: Variable length strings
 * License: LGPL
 * History:
 *     10 Nov 2005 : Initial version
 *     29 Nov 2005 : Made portable to early/non gcc
 */

/*
  Notice that no status is returned from these routines.
  If any operation fails (runs out of mem), abort() is called.
*/

#ifndef SBUF_H
#define SBUF_H

#include <stdarg.h>
#include <stdio.h>

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
    #define GNUC_PRINTF_CHECK(fmt_idx, arg_idx) __attribute__((format (printf, fmt_idx, arg_idx)))
#else   /* !__GNUC__ */
    #define GNUC_PRINTF_CHECK(fmt_idx, arg_idx)
#endif  /* !__GNUC__ */

typedef struct _sbuf {
    char *buf;
    int NUL;
    int buflen;
} sbuf_t;

sbuf_t *sbuf_new(void);         // create an sbuf
void sbuf_delete(sbuf_t *sb);   // free an sbuf
void sbuf_reset(sbuf_t *sb);    // clear sbuf contents (doesn't free mem)
int sbuf_len(sbuf_t *sb);       // return contents length (excluding NUL)
char *sbuf_ptr(sbuf_t *sb);     // return pointer to sbuf contents
char *sbuf_detach(sbuf_t *sb);  // Detach and return sbuf contents (you must free)
void sbuf_truncate(sbuf_t *sb, int len);
void sbuf_move(sbuf_t *src, sbuf_t *dest);
void sbuf_appendstr(sbuf_t *sb, const char *string);
void sbuf_appendbytes(sbuf_t *sb, const char *str, int len);
void sbuf_appendchar(sbuf_t *sb, char c);
void sbuf_printf(sbuf_t *sb, const char *fmt, ...) GNUC_PRINTF_CHECK(2,3);
void sbuf_appendf(sbuf_t *sb, const char *fmt, ...) GNUC_PRINTF_CHECK(2,3);

#endif //SBUF_H
