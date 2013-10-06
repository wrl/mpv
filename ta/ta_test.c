// some lame tests
#include <assert.h>
#include <string.h>

#include "ta.h"

static bool dtor_called;

static void test_dtor(void *ptr)
{
    unsigned char *z = ptr;
    assert(z[455] == 13);
    assert(!dtor_called);
    dtor_called = true;
}

int main(int argc, char **argv)
{
    unsigned char *x = ta_alloc_size(NULL, 123);
    memset(x, 56, 123);
    unsigned char *y = ta_alloc_size(x, 456);
    memset(y, 12, 456);
    unsigned char *z = ta_alloc_size(x, 456);
    memset(z, 13, 456);
    ta_set_destructor(z, test_dtor);
    assert(ta_find_parent(y) == x);
    x = ta_realloc_size(NULL, x, 23423);
    assert(x[45] == 56);
    assert(!dtor_called);
    ta_free(x);
    assert(dtor_called);
}
