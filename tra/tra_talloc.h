#ifndef TRA_TALLOC_H_
#define TRA_TALLOC_H_

#include "tra.h"

#ifdef __GNUC__
#define TRA_TYPEOF(t) __typeof(t)
#else
#define TRA_TYPEOF(t)
#endif

#define talloc(p, type) (type *)tra_xalloc_size(p, sizeof(type))
#define talloc_zero(p, type) (type *)tra_xcalloc_size(p, sizeof(type))

#define talloc_size(p, size) tra_xalloc_size(p, size)
#define talloc_zero_size(p, size) tra_xcalloc_size(p, size)

#define talloc_get_size(p) tra_get_size(p)
#define talloc_free_children(p) tra_free_children(p)
#define talloc_free(p) tra_free(p)

#define talloc_array(p, type, count) (type *)tra_xalloc_size(p, sizeof(type) * (count))
#define talloc_zero_array(p, type, count) (type *)tra_xcalloc_size(p, sizeof(type) * (count))
#define talloc_array_size(p, size, count) tra_xalloc_size(p, (size) * (count))
#define talloc_realloc(ctx, p, type, count) (type *)tra_xrealloc_size(ctx, p, sizeof(type) * (count))
#define talloc_realloc_size(ctx, p, size) tra_xrealloc_size(ctx, p, size)

#define talloc_ptrtype(p, ptr) talloc(p, TRA_TYPEOF(*ptr))
#define talloc_array_ptrtype(p, ptr, count) (TRA_TYPEOF(ptr))talloc_array_size(p, sizeof(*(ptr)), count)
#define talloc_steal(p, ptr) (TRA_TYPEOF(ptr))talloc_steal_(p, ptr)

#define talloc_set_destructor(p, d) tra_xset_destructor_talloc(p, d)
#define talloc_new(p) tra_xalloc_size(p, 0)
#define talloc_parent(p) tra_find_parent(p)
#define talloc_enable_leak_report tra_enable_leak_report

void *talloc_steal_(void *p, void *ptr);

void *talloc_memdup(void *t, const void *p, size_t n);

char *talloc_strdup(void *t, const char *p);
char *talloc_strdup_append(char *s, const char *a);
char *talloc_strdup_append_buffer(char *s, const char *a);

char *talloc_strndup(void *t, const char *p, size_t n);
char *talloc_strndup_append(char *s, const char *a, size_t n);
char *talloc_strndup_append_buffer(char *s, const char *a, size_t n);

#define talloc_asprintf tra_xasprintf
#define talloc_vasprintf tra_xvasprintf

char *talloc_vasprintf_append(char *s, const char *fmt, va_list ap) TRA_PRF(2, 0);
char *talloc_vasprintf_append_buffer(char *s, const char *fmt, va_list ap) TRA_PRF(2, 0);

char *talloc_asprintf_append(char *s, const char *fmt, ...) TRA_PRF(2, 3);
char *talloc_asprintf_append_buffer(char *s, const char *fmt, ...) TRA_PRF(2, 3);

#endif
