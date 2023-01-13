#ifndef __FREE_DELETE__
#define __FREE_DELETE__

#include <cstdlib>

struct free_delete
{
    void operator()(void* x) { free(x); }
};

#endif
