#pragma once
#include <unistd.h>
#define zassert(eq) if(eq){ _exit(perr(errno)); }
