#include "include/tty.h"

int handle_stdin(int *in_tty, int *pipe_fd, int *read_fd){
		*in_tty = 1;
		*pipe_fd = dup(STDIN_FILENO);
		zassert(*pipe_fd < 0)
		*read_fd = *pipe_fd;
		close(STDIN_FILENO);
		int fd = open("/dev/tty", O_RDONLY);
		zassert(fd < 0)
}

void reset_tty(){
       int tcs_set_err = tcsetattr(STDERR_FILENO, TCSANOW, &def_term);
       zassert(tcs_set_err < 0)
}

void term_mode(int action, struct termios *term){
	if(action == COMMAND_SET){
		term->c_cc[VMIN] = 1;
		term->c_cc[VTIME] = 0;
		term->c_lflag &= ICANON & !ECHO; 
		int tcs_set_err = tcsetattr(STDERR_FILENO, TCSANOW, term);
		zassert(tcs_set_err < 0)
		lock_signals();
	} else {
		term->c_lflag &= !ICANON & ECHO; 
		int tcs_set_err = tcsetattr(STDERR_FILENO, TCSANOW, term);
		zassert(tcs_set_err < 0)
		unlock_signals();
	}
}
