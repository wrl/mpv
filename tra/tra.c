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

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "tra.h"

#define DEBUG_CHECK

// Note: the header must have a size that guarantees natural alignment.
struct tra_header {
    size_t size;                // size of the user allocation (after header)
    struct tra_header *prev;    // ring list containing siblings
    struct tra_header *next;
    struct tra_ext_header *ext;
#ifdef DEBUG_CHECK
    unsigned int canary;
#endif
};

#define CANARY 0xD3ADB3EF

// Needed for non-leaf allocations, or extended features such as destructors.
struct tra_ext_header {
    struct tra_header *header;  // points back to normal header
    struct tra_header children; // list of children, with this as sentinel
    void (*destructor)(void *);
    int (*destructor_talloc)(void *);
};

#define MAX_ALLOC (((size_t)-1) - sizeof(struct tra_ext_header))

// tra_ext_header.children.size is set to this
#define CHILDREN_SENTINEL ((size_t)-1)

static struct tra_header *get_header(void *ptr)
{
    struct tra_header *h = ptr ? (struct tra_header *)ptr - 1 : NULL;
#ifdef DEBUG_CHECK
    if (h)
        assert(h->canary == CANARY);
#endif
    return h;
}

static struct tra_ext_header *get_or_alloc_ext_header(void *ptr)
{
    struct tra_header *h = get_header(ptr);
    if (!h)
        return NULL;
    if (!h->ext) {
        h->ext = malloc(sizeof(struct tra_ext_header));
        if (!h->ext)
            return NULL;
        *h->ext = (struct tra_ext_header) {
            .header = h,
            .children = {
                .next = &h->ext->children,
                .prev = &h->ext->children,
                // Needed by find_parent_node():
                .size = CHILDREN_SENTINEL,
                .ext = h->ext,
            },
        };
    }
    return h->ext;
}

/* Set the parent allocation of child. If parent==NULL, remove the parent.
 * Returns true on success, false on OOM or if child==NULL.
 * Setting parent==NULL always succeeds.
 */
bool tra_set_parent(void *child, void *parent)
{
    struct tra_header *ch = get_header(child);
    if (!ch)
        return false;
    struct tra_ext_header *parent_eh = get_or_alloc_ext_header(parent);
    if (parent && !parent_eh) // do nothing on OOM
        return false;
    // Unlink from previous parent
    if (ch->next) {
        ch->next->prev = ch->prev;
        ch->prev->next = ch->next;
        ch->next = ch->prev = NULL;
    }
    // Link to new parent - insert at end of list (possibly orders destructors)
    if (parent_eh) {
        struct tra_header *children = &parent_eh->children;
        ch->next = children;
        ch->prev = children->prev;
        children->prev->next = ch;
        children->prev = ch;
    }
    return true;
}

static void *init_alloc(void *tra_parent, struct tra_header *h, size_t size)
{
    if (!h)
        return NULL;
    *h = (struct tra_header) {
        .size = size,
#ifdef DEBUG_CHECK
        .canary = CANARY,
#endif
    };
    if (!tra_set_parent(h + 1, tra_parent)) {
        tra_free(h);
        return NULL;
    }
    return h + 1;
}

/* Allocate size bytes of memory. If tra_parent is not NULL, this is used as
 * parent allocation (if tra_parent is freed, this allocation is automatically
 * freed as well). size==0 allocates a block of size 0 (i.e. returns non-NULL).
 * Returns NULL on OOM.
 */
void *tra_alloc_size(void *tra_parent, size_t size)
{
    if (size >= MAX_ALLOC)
        return NULL;
    return init_alloc(tra_parent, malloc(sizeof(struct tra_header) + size), size);
}

/* Exactly the same as tra_alloc_size(), but the returned memory block is
 * initialized to 0.
 */
void *tra_calloc_size(void *tra_parent, size_t size)
{
    if (size >= MAX_ALLOC)
        return NULL;
    return init_alloc(tra_parent, calloc(1, sizeof(struct tra_header) + size), size);
}

/* Reallocate the allocation given by ptr and return a new pointer. Much like
 * realloc(), the returned pointer can be different, and on OOM, NULL is
 * returned.
 *
 * size==0 is equivalent to tra_free(ptr).
 * ptr==NULL is equivalent to tra_alloc_size(tra_parent, size).
 *
 * tra_parent is used only in the ptr==NULL case.
 */
void *tra_realloc_size(void *tra_parent, void *ptr, size_t size)
{
    if (size >= MAX_ALLOC)
        return NULL;
    if (!size) {
        tra_free(ptr);
        return NULL;
    }
    if (!ptr)
        return tra_alloc_size(tra_parent, size);
    struct tra_header *h = get_header(ptr);
    if (h->size == size)
        return ptr;
    h = realloc(h, sizeof(struct tra_header) + size);
    if (!h)
        return NULL;
    h->size = size;
    if (h->next) {
        // Relink siblings
        h->next->prev = h;
        h->prev->next = h;
    }
    if (h->ext) {
        // Relink children
        h->ext->header = h;
        h->ext->children.next->prev = &h->ext->children;
        h->ext->children.prev->next = &h->ext->children;
    }
    return h + 1;
}

/* Return the allocated size of ptr. This returns the size parameter of the
 * most recent tra_alloc.../tra_realloc... call.
 * If ptr==NULL, return 0.
 */
size_t tra_get_size(void *ptr)
{
    struct tra_header *h = get_header(ptr);
    return h ? h->size : 0;
}

/* Free all allocations that (recursively) have ptr as parent allocation, but
 * do not free ptr itself.
 */
void tra_free_children(void *ptr)
{
    struct tra_header *h = get_header(ptr);
    struct tra_ext_header *eh = h ? h->ext : NULL;
    if (!eh)
        return;
    // xxx: should children unlinked before calling free to avoid cycles?
    while (eh->children.next != &eh->children)
        tra_free(eh->children.next + 1);
}

/* Free the given allocation, and all of its direct and indirect children.
 */
void tra_free(void *ptr)
{
    struct tra_header *h = get_header(ptr);
    if (!h)
        return;
    if (h->ext && h->ext->destructor)
        h->ext->destructor(ptr);
    if (h->ext && h->ext->destructor_talloc)
        h->ext->destructor_talloc(ptr);
    tra_free_children(ptr);
    if (h->next) {
        // Unlink from sibling list
        h->next->prev = h->prev;
        h->prev->next = h->next;
    }
#ifdef DEBUG_CHECK
    h->canary = 0;
#endif
    free(h->ext);
    free(h);
}

/* Set a destructor that is to be called when the given allocation is freed.
 * (Whether the allocation is directly freed with tra_free() or indirectly by
 * freeing its parent does not matter.) There is only one destructor. If an
 * destructor was already set, it's overwritten.
 *
 * The destructor will be called with ptr as argument. The destructor can do
 * anything, but it must not attempt to free or realloc ptr. The destructor
 * is run before the allocation's children are freed (also, before their
 * destructors are run).
 *
 * Returns false if ptr==NULL, or on OOM.
 */
bool tra_set_destructor(void *ptr, void (*destructor)(void *))
{
    struct tra_ext_header *eh = get_or_alloc_ext_header(ptr);
    if (!eh)
        return false;
    eh->destructor = destructor;
    return true;
}

/* Provided for API compatibility. */
bool tra_set_destructor_talloc(void *ptr, int (*destructor)(void *))
{
    struct tra_ext_header *eh = get_or_alloc_ext_header(ptr);
    if (!eh)
        return false;
    eh->destructor_talloc = destructor;
    return true;
}

static struct tra_header *find_parent_node(void *ptr)
{
    struct tra_header *h = get_header(ptr);
    if (!h || !h->next)
        return NULL;
    for (struct tra_header *cur = h->next; cur != h; cur = cur->next) {
        if (cur->size == CHILDREN_SENTINEL)
            return cur->ext->header;
    }
    return NULL;
}

/* Return the ptr's parent allocation, or NULL if there isn't any.
 *
 * Warning: this has O(N) runtime complexity with N sibling allocations!
 */
void *tra_find_parent(void *ptr)
{
    struct tra_header *ph = find_parent_node(ptr);
    return ph ? ph + 1 : NULL;
}

// -----------------------------------------------------------------------------

void tra_enable_leak_report(void)
{
    // well...
}

// -----------------------------------------------------------------------------

static bool tra_vasprintf_append_at(char **str, size_t at, const char *fmt,
                                    va_list ap)
{
    size_t size;

    va_list copy;
    va_copy(copy, ap);
    char c;
    size = vsnprintf(&c, 1, fmt, copy);
    va_end(copy);

    if (tra_get_size(*str) < at + size + 1) {
        char *t = tra_realloc_size(NULL, *str, at + size + 1);
        if (!t)
            return false;
        *str = t;
    }
    vsnprintf(*str + at, size + 1, fmt, ap);
    return true;
}

/* Like snprintf(); returns the formatted string as allocation (NULL on OOM).
 */
char *tra_asprintf(void *tra_parent, const char *fmt, ...)
{
    char *res;
    va_list ap;
    va_start(ap, fmt);
    res = tra_vasprintf(tra_parent, fmt, ap);
    va_end(ap);
    return res;
}

char *tra_vasprintf(void *tra_parent, const char *fmt, va_list ap)
{
    char *res = NULL;
    tra_vasprintf_append_at(&res, 0, fmt, ap);
    if (!res || !tra_set_parent(res, tra_parent)) {
        tra_free(res);
        return NULL;
    }
    return res;
}

/* Append the formatted string to *str (after strlen(*str), the allocation is
 * tra_realloced if needed).
 * Returns false on OOM, with *str left untouched.
 */
bool tra_asprintf_append(char **str, const char *fmt, ...)
{
    bool res;
    va_list ap;
    va_start(ap, fmt);
    res = tra_vasprintf_append(str, fmt, ap);
    va_end(ap);
    return res;
}

bool tra_vasprintf_append(char **str, const char *fmt, va_list ap)
{
    return tra_vasprintf_append_at(str, str && *str ? strlen(*str) : 0, fmt, ap);
}

/* Append the formatted string one byte before the end of the allocation of
 * str. Compared to tra_asprintf_append(), this is useful if you know that the
 * string ends with the allocation, and the last byte of the allocation is the
 * terminating \0.
 * Returns false on OOM, with *str left untouched.
 */
bool tra_asprintf_append_buffer(char **str, const char *fmt, ...)
{
    bool res;
    va_list ap;
    va_start(ap, fmt);
    res = tra_vasprintf_append_buffer(str, fmt, ap);
    va_end(ap);
    return res;
}

bool tra_vasprintf_append_buffer(char **str, const char *fmt, va_list ap)
{
    size_t size = tra_get_size(*str);
    if (size > 0)
        size -= 1;
    return tra_vasprintf_append_at(str, size, fmt, ap);
}

// -----------------------------------------------------------------------------

void *tra_oom_p(void *p)
{
    if (!p)
        abort();
    return p;
}

void tra_oom_b(bool b)
{
    if (!b)
        abort();
}

char *tra_oom_s(char *s)
{
    if (!s)
        abort();
    return s;
}
