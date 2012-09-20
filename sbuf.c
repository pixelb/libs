/* Copyright: Pádraig Brady 2005
 * Summary: Variable length strings
 * License: LGPL
 * History:
 *     10 Nov 2005 : Initial version
 *     29 Nov 2005 : Made portable to early/non gcc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sbuf.h"

static void sbuf_init(sbuf_t *sb) {
    sb->buf = NULL;
    sb->NUL = 0;
    sb->buflen = 0;
}

sbuf_t *sbuf_new() {
    sbuf_t *sb = malloc(sizeof(*sb));
    if (!sb) abort(); //out of mem
    sbuf_init(sb);
    return sb;
}

void sbuf_delete(sbuf_t *sb) {
    free(sb->buf);
    free(sb);
}

void sbuf_reset(sbuf_t *sb) {
    sb->NUL = 0;
}

char *sbuf_ptr(sbuf_t *sb) {
    return sb->NUL ? sb->buf : "";
}

int sbuf_len(sbuf_t *sb) {
    return sb->NUL;
}

char *sbuf_detach(sbuf_t *sb) {
    char* buf = sb->buf;
    if (!buf) buf = strdup("");
    sbuf_init(sb);
    return buf;
}

void sbuf_move(sbuf_t *src, sbuf_t *dest) {
    free(dest->buf);
    dest->buf = src->buf;
    dest->NUL = src->NUL;
    dest->buflen = src->buflen;
    sbuf_init(src);
}

void sbuf_truncate(sbuf_t *sb, int len) {
    if (len < 0 || len > sb->NUL)
        return;
    sb->NUL = len;
    if (sb->buf)
        sb->buf[sb->NUL] = '\0';
}

/* Extend the buffer in sb by at least len bytes.
 * Note len should include the space required for the NUL terminator */
static void sbuf_extendby(sbuf_t *sb, int len) {
    len += sb->NUL;
    if (len <= sb->buflen) return;
    if (!sb->buflen) sb->buflen=64; //min size (to alleviate fragmentation)
    while (len > sb->buflen) sb->buflen *= 2;
    char* buf = realloc(sb->buf, sb->buflen);
    if (!buf) abort(); //out of mem
    sb->buf = buf;
}

void sbuf_appendbytes(sbuf_t *sb, const char *str, int len) {
    sbuf_extendby(sb, len + 1);
    memcpy(&sb->buf[sb->NUL], str, len);
    sb->NUL += len;
    sb->buf[sb->NUL] = '\0';
}

void sbuf_appendchar(sbuf_t *sb, char c) {
    sbuf_extendby(sb, 2);
    sb->buf[sb->NUL++] = c;
    sb->buf[sb->NUL] = '\0';
}

void sbuf_appendstr(sbuf_t *sb, const char *str) {
    sbuf_appendbytes(sb, str, strlen(str));
}

static void sbuf_vappendf(sbuf_t *sb, const char *fmt, const va_list ap) {
    int num_required;
    while ((num_required = vsnprintf(sb->buf+sb->NUL, sb->buflen-sb->NUL, fmt, ap)) >= sb->buflen-sb->NUL)
        sbuf_extendby(sb, num_required + 1);
    sb->NUL += num_required;
}

void sbuf_printf(sbuf_t *sb, const char *fmt, ...) {
    sbuf_reset(sb);

    va_list ap;
    va_start(ap, fmt);
    sbuf_vappendf(sb, fmt, ap);
    va_end(ap);
}

void sbuf_appendf(sbuf_t *sb, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    sbuf_vappendf(sb, fmt, ap);
    va_end(ap);
}
