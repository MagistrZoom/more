#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "include/string.h"
#include "include/zprintf.h"

#include <malloc.h>

#include <math.h>

#include <termios.h>

#include <signal.h> 

#include <stdlib.h>

#define NL "\n"
#define COLUMNS (80)
#define LINES (24)

#define COMMAND_SET (1)
#define COMMAND_UNSET (0)

int in_tty = 0;
int piped_fd = 0;
struct termios term = { 0 };
sigset_t sig, osig;

/* TODO: make header */
int perr(int err){
	char *err_ptr = strerror(err);
	dzprintf(STDERR_FILENO, "%s\n", err_ptr);
	return err;
}

/* TODO: make header */
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


	/* get terminal parameters */
	int term_err = tcgetattr(STDERR_FILENO, &term);
	if(term_err == -1){ return perr(errno); }
	
	/* set up signal handlers */
	sigemptyset(&sig);
	sigaddset(&sig, SIGINT);        /* block SIGINT */
    sigaddset(&sig, SIGTSTP);       /* block SIGTSTP */

	size_t page_size = sysconf(_SC_PAGESIZE);
	/* reopen control STDIN descriptor and save stdin from pipe */
	if(!isatty(STDIN_FILENO)){
		in_tty = 1;
		piped_fd = dup(STDIN_FILENO);
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
		if(err_stat == -1){
			return perr(errno);
		}
		
		int read_fd = open(argv[1], O_RDONLY | O_LARGEFILE);
		if(read_fd == -1){
			return perr(errno);
		}


		char *buf = malloc(sizeof(char)*page_size);
		char *ptr;
		char *token;
		char *newline = "\n";
		char bc;
		char key;

		int r_code;
		int line_len;
		size_t lines; 
		size_t line;
		size_t read_bytes = 0;
		size_t offset = 0;
		size_t nls;


		//way to get filesize
		off_t end = lseek(read_fd, 0, SEEK_END);
		if(end == -1) { return perr(errno); }
		off_t start = lseek(read_fd, 0, SEEK_SET);
		if(start == -1) { return perr(errno); }

		lines = 2 + 1 + strastr(argv[1], NL);
		put_header(argv[1]);	

		while(1){
			r_code = read(read_fd, buf + offset, page_size - offset - 1);
			if(!r_code) { return 0; /* end of file */ }
			if(r_code == -1){ return perr(errno); }

			nls = strastr(buf, NL);
			offset = 0;

			read_bytes += r_code;
			
			ptr = buf;
			//now need to print lines and stop for waiting
			while((token = strsep(&ptr, NL)) != NULL){
				//count lines
				line_len = strlen(token);
				
				line = ceil((double)line_len/COLUMNS);
				if(!line){
					line++;
				}
				if(lines + line < LINES){
					lines += line;
				} else {
					/* end of current screen 
					 * need to print last piece of line
					 */
					bc = token[(LINES - lines + 1)*COLUMNS];
					token[(LINES - lines + 1)*COLUMNS] = 0;
					zprintf("%s\n", token);
					token[(LINES - lines + 1)*COLUMNS] = bc;
					
					nls -= lines;

					lines = 0;
					if(nls < LINES){
						//not enough data to display full screen
						//need to append data in the buffer
						offset = strlen(ptr+strlen(token));
						strcpy(buf, ptr + strlen(token));
						break; 
					}

					wait_for_a_command(&key);
					if(key == 'q'){
						return 0;
					}

					continue;
				}
				zprintf("%s\n", token);
			}
		}
		
	}
	return 0;
}


