#ifndef TRA_H_
#define TRA_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __GNUC__
#define TRA_PRF(a1, a2) __attribute__ ((format(printf, a1, a2)))
#else
#define TRA_PRF(a1, a2)
#endif

void *tra_alloc_size(void *tra_parent, size_t size);
void *tra_calloc_size(void *tra_parent, size_t size);
void *tra_realloc_size(void *tra_parent, void *ptr, size_t size);
size_t tra_get_size(void *ptr);
void tra_free(void *ptr);
void tra_free_children(void *ptr);
bool tra_set_destructor(void *ptr, void (*destructor)(void *));
bool tra_set_destructor_talloc(void *ptr, int (*destructor)(void *));
bool tra_set_parent(void *child, void *parent);
void *tra_find_parent(void *ptr);

char *tra_asprintf(void *tra_parent, const char *fmt, ...) TRA_PRF(2, 3);
char *tra_vasprintf(void *tra_parent, const char *fmt, va_list ap) TRA_PRF(2, 0);

bool tra_asprintf_append(char **str, const char *fmt, ...) TRA_PRF(2, 3);
bool tra_vasprintf_append(char **str, const char *fmt, va_list ap) TRA_PRF(2, 0);

bool tra_asprintf_append_buffer(char **str, const char *fmt, ...) TRA_PRF(2, 3);
bool tra_vasprintf_append_buffer(char **str, const char *fmt, va_list ap) TRA_PRF(2, 0);

void tra_enable_leak_report(void);

// Ugly macros that crash on OOM.
// All of these mirror real functions (with a 'x' added after the 'tra_'
// prefix), and the only difference is that out of memory conditions will lead
// to a call to abort(), instead of returning an error code.
#define tra_xalloc_size(...) tra_oom_p(tra_alloc_size(__VA_ARGS__))
#define tra_xcalloc_size(...) tra_oom_p(tra_calloc_size(__VA_ARGS__))
#define tra_xrealloc_size(...) tra_oom_p(tra_realloc_size(__VA_ARGS__))
#define tra_xset_destructor(...) tra_oom_b(tra_set_destructor(__VA_ARGS__))
#define tra_xset_destructor_talloc(...) tra_oom_b(tra_set_destructor_talloc(__VA_ARGS__))
#define tra_xset_parent(...) tra_oom_b(tra_set_parent(__VA_ARGS__))
#define tra_xasprintf(...) tra_oom_s(tra_asprintf(__VA_ARGS__))
#define tra_xvasprintf(...) tra_oom_s(tra_vasprintf(__VA_ARGS__))
#define tra_xasprintf_append(...) tra_oom_b(tra_asprintf_append(__VA_ARGS__))
#define tra_xvasprintf_append(...) tra_oom_b(tra_vasprintf_append(__VA_ARGS__))
#define tra_xasprintf_append_buffer(...) tra_oom_b(tra_asprintf_append_buffer(__VA_ARGS__))
#define tra_xvasprintf_append_buffer(...) tra_oom_b(tra_vasprintf_append_buffer(__VA_ARGS__))

void *tra_oom_p(void *p);
void tra_oom_b(bool b);
char *tra_oom_s(char *s);

#endif
