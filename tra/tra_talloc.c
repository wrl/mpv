/* Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tra_talloc.h"

void *talloc_steal_(void *p, void *ptr)
{
    if (ptr)
        tra_xset_parent(ptr, p);
    return ptr;
}

void *talloc_memdup(void *t, const void *p, size_t n)
{
    if (!p)
        return NULL;
    void *res = talloc_size(t, n);
    if (!res)
        return NULL;
    memcpy(res, p, n);
    return res;
}

static char *strndup_append_at(void *t, char *s, size_t at,
                              const char *append, size_t n)
{
    size_t append_len = append ? strnlen(append, n) : 0;
    if (n < append_len)
        append_len = n;

    s = talloc_realloc_size(t, s, at + append_len + 1);
    if (!s)
        return NULL;
    memcpy(s + at, append, append_len);
    s[at + append_len] = '\0';
    return s;
}

char *talloc_strdup(void *t, const char *p)
{
    return talloc_memdup(t, p, p ? strlen(p) + 1 : 0);
}

char *talloc_strdup_append(char *s, const char *a)
{
    return strndup_append_at(NULL, s, s ? strlen(s) : 0, a, (size_t)-1);
}

char *talloc_strdup_append_buffer(char *s, const char *a)
{
    size_t size = talloc_get_size(s);
    if (size > 0)
        size -= 1;
    return strndup_append_at(NULL, s, size, a, (size_t)-1);
}

char *talloc_strndup(void *t, const char *p, size_t n)
{
    return strndup_append_at(t, NULL, 0, p, n);
}

char *talloc_strndup_append(char *s, const char *a, size_t n)
{
    return strndup_append_at(NULL, s, s ? strlen(s) : 0, a, n);
}

char *talloc_strndup_append_buffer(char *s, const char *a, size_t n)
{
    size_t size = talloc_get_size(s);
    if (size > 0)
        size -= 1;
    return strndup_append_at(NULL, s, size, a, n);
}

char *talloc_vasprintf_append(char *s, const char *fmt, va_list ap)
{
    tra_xvasprintf_append(&s, fmt, ap);
    return s;
}

char *talloc_vasprintf_append_buffer(char *s, const char *fmt, va_list ap)
{
    tra_xvasprintf_append_buffer(&s, fmt, ap);
    return s;
}

char *talloc_asprintf_append(char *s, const char *fmt, ...)
{
    char *res;
    va_list ap;
    va_start(ap, fmt);
    res = talloc_vasprintf_append(s, fmt, ap);
    va_end(ap);
    return res;
}

char *talloc_asprintf_append_buffer(char *s, const char *fmt, ...)
{
    char *res;
    va_list ap;
    va_start(ap, fmt);
    res = talloc_vasprintf_append_buffer(s, fmt, ap);
    va_end(ap);
    return res;
}
