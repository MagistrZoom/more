#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "include/string.h"
#include "include/zprintf.h"

#include <malloc.h>

#include <math.h>

#include <termios.h>
#include <sys/ioctl.h>

#include <signal.h> 

#include <stdlib.h>

#define NL "\n"
#define NLC '\n'
#define COMMAND_SET (1)
#define COMMAND_UNSET (0)

int pipe_fd = 0;
int tab_spaces = 8;
sigset_t sig, osig;

int in_tty = 0;
struct termios term = { 0 };

int perr(int err){
	char *err_ptr = strerror(err);
	dzprintf(STDERR_FILENO, "%s\n", err_ptr);
	return err;
}
void put_header(char *filename){
	zprintf("[2J[H::::::::::::\n%s\n::::::::::::\n", filename);	
}
void command_mode(int action){
	if(action){
		term.c_lflag &= ICANON & !ECHO; 
		int tcs_set_err = tcsetattr(STDERR_FILENO, TCSANOW, &term);
		if(tcs_set_err == -1){ exit(perr(errno)); }
	    int sigproc_err = sigprocmask(SIG_BLOCK, &sig, &osig);
		if(sigproc_err == -1){ exit(perr(errno)); }
	} else {
		term.c_lflag &= !ICANON & ECHO; 
		int tcs_set_err = tcsetattr(STDERR_FILENO, TCSANOW, &term);
		if(tcs_set_err == -1){ exit(perr(errno)); }
	    int sigproc_err = sigprocmask(SIG_SETMASK, &osig, NULL);
		if(sigproc_err == -1){ exit(perr(errno)); }
	}
	
}
/* set up no-echo and canonical mode
 * block signals SIGINT and SIGTSTP
 * read one char
 * unblock signals SIGINT and SIGTSTP
 * set up echo and non-canonical mode
 */
void wait_for_a_command(char *key){
	command_mode(COMMAND_SET);	

	int rerror = read(STDIN_FILENO, key, 1);
	if(rerror == -1){ exit(perr(errno)); }

	command_mode(COMMAND_UNSET);	
}

int main(int argc, char *argv[]) {
	/*TODO: argument and options parsing			*/


	/* need to get optimal buf size */
	size_t page_size = sysconf(_SC_PAGESIZE);

	/* get terminal parameters and size */
	int term_err = tcgetattr(STDERR_FILENO, &term);
	if(term_err == -1){ return perr(errno); }
	struct winsize terminal_d = { 0 };
	term_err = ioctl(STDERR_FILENO, TIOCGWINSZ, &terminal_d);
	if(term_err == -1){ return perr(errno); }
//	int columns = terminal_d.ws_col;
//	int rows = terminal_d.ws_row;
	int columns = 80;
	int rows = 25;
	
	/* reopen control STDIN descriptor and save stdin from pipe */
	if(!isatty(STDIN_FILENO)){
		in_tty = 1;
		pipe_fd = dup(STDIN_FILENO);
		close(STDIN_FILENO);
		int fd = open("/dev/tty", O_RDONLY | O_LARGEFILE);
		if(errno != 0 && fd == -1){
			return perr(errno);	
		}
	}

	/*let's start to read regular files using mmap	*/
	if(!in_tty){
		struct stat read_stat = { 0 };
	
		int err_stat = stat(argv[1], &read_stat);
		if(err_stat == -1){ return perr(errno); }
		
		int read_fd = open(argv[1], O_RDONLY);
		if(read_fd == -1){ return perr(errno); }

		//way to get filesize
		off_t end = lseek(read_fd, 0, SEEK_END);
		if(end == -1) { return perr(errno); }
		off_t start = lseek(read_fd, 0, SEEK_SET);
		if(start == -1) { return perr(errno); }

		char *nlptr;

		char *buf = calloc(sizeof(char), page_size);
		char *ptr = buf;
		char key;

		size_t line_l = columns;
		int read_e;
		int lines = 0;
		lines = 2 + 1 + strastr(argv[1], NL);

	 	put_header(argv[1]);	

		size_t print_b = 0;			
		size_t buf_b;

		while(print_b < end){
			read_e = read(read_fd, buf, page_size - 1);
			if(read_e == 0) { return 0; }
			if(read_e == -1) { return perr(errno); }

			buf_b = 0;
			while(buf_b < read_e){
				nlptr = memmem(buf + buf_b, read_e , NL, 1) + 1;
				/*if(nlptr < buf){
					line_l = (read_e-buf_b)<columns?(read_e-buf_b):columns;
				} else {
					line_l = (nlptr-buf_b-buf)<columns?(nlptr-buf_b-buf):columns;
				}*/

				zputb(buf + buf_b, line_l);

				if(nlptr < buf){ zprintf(NL); }
				lines++;

				if(lines == rows){
					lines = 0;
					wait_for_a_command(&key);
					if(key == 'q'){
						return 0;
					}
				}
				buf_b   += line_l;
				print_b += line_l;
			}
		}
	}
	return 0;
}


