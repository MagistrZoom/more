#include "zassert.h"
#include <errno.h>
#include <signal.h> 

sigset_t sig, osig;

void lock_signals();
void unlock_signals();
