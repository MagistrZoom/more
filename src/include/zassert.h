#pragma once
#define zassert(eq) if(eq){ _exit(perr(errno)); }
