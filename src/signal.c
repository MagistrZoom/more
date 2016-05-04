#include "include/signal.h"

void lock_signals(){
	int sigproc_err = sigprocmask(SIG_SETMASK, &osig, NULL);
	zassert(sigproc_err < 0)
}
void unlock_signals(){
	int sigproc_err = sigprocmask(SIG_BLOCK, &sig, &osig);
	zassert(sigproc_err < 0)
}
