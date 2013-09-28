// some lame tests
#include <assert.h>
#include <string.h>

#include "tra.h"

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
    unsigned char *x = tra_alloc_size(NULL, 123);
    memset(x, 56, 123);
    unsigned char *y = tra_alloc_size(x, 456);
    memset(y, 12, 456);
    unsigned char *z = tra_alloc_size(x, 456);
    memset(z, 13, 456);
    tra_set_destructor(z, test_dtor);
    assert(tra_find_parent(y) == x);
    x = tra_realloc_size(NULL, x, 23423);
    assert(x[45] == 56);
    assert(!dtor_called);
    tra_free(x);
    assert(dtor_called);
}
