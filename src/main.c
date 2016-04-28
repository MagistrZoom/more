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

#include <limits.h> /* just for one constant */

#define NL "\n"
#define NLC '\n'
#define COMMAND_SET (1)
#define COMMAND_UNSET (0)

char *prompt = "Next";

int pipe_fd = 0;
int tab_spaces = 8;
sigset_t sig, osig;

int in_tty = 0;
struct termios term = { 0 };


enum CMD {
	CMD_EXIT = 0,	/* interrupt 											 */
	CMD_HELP, 		/* tiny help */
	CMD_SCREEN, 	/* i<SPACE> (screen usually) down 						 */
	CMD_SCREEN_DEF, /* i<SPACE> (screen usually) down and set up i as default*/
	CMD_LINE, 		/* i<CR> 	scroll i(1) lines and set up i as default 		 */
	CMD_HALF,		/* id 		by default scroll 11 lines, i as new default */
	CMD_SKIP,		/* is		skip i(1) lines of text 					 */
	CMD_SKIP_SCREEN,/* if		skip i(1) screenfuls of text 				 */
	CMD_BACK,		/* ib		backward i(1) screenfuls of text 			 */
	CMD_PRINTL,		/* =		print line 									 */
	CMD_REPEAT,		/* .		repeat last command 						 */
} CMD;

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

/* TODO: need to implement */
enum CMD wait_for_a_command(){
	command_mode(COMMAND_SET);	
	char key;
	int rerror = read(STDIN_FILENO, &key, 1);
	if(rerror == -1){ exit(perr(errno)); }

	command_mode(COMMAND_UNSET);	
	if(key == NLC){
		return CMD_LINE;
	}
	if(key == ' '){
		return CMD_SCREEN;
	}
	if(key == 'q'){
		return CMD_EXIT;
	}
	return CMD_SCREEN;
}

/* 
 * prompt in the bottom of screen 
 */
void print_prompt(size_t cur, size_t end){
	int percent = round((double)cur/end*100);
	
	/* set up foreground black and background white 
	 * print prompt, percents
	 */
	zprintf("[30;47m--%s--(%d%%) [0m", prompt, percent);
}
void clean_prompt(){
	/* cleans entire line and returns carriage */
	zprintf("[2K\r");
}


int main(int argc, char *argv[]) {
	/*TODO: argument and options parsing			*/
	/*TODO: check syscalls */


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
	int rows = 24;
	
	int read_fd;
	
	off_t end = 0, start = 0;

	enum CMD cmd;

	char *nlptr;

	char *buf = calloc(sizeof(char), page_size);
	char *ptr;

	size_t line_l = columns;
	size_t real_l = 0; /* with respect to tab alignment */
	size_t offset = 0;
	int read_e;
	int lines = 0;
	int nlflag = 0;
	int iflag = 0;


	/* default lengths */
	int current_rows_limit = rows;
	int screen_rows_limit = rows;
	int line_rows_limit = 1;

	/* flags 4 commands */
	int lnflag = 0; /* line mode */
	int scflag = 0; /* screen mode */

	size_t print_b = 0;			
	size_t buf_b;
	
	/* reopen control STDIN descriptor and save stdin from pipe */
	if(!isatty(STDIN_FILENO)){
		in_tty = 1;
		pipe_fd = dup(STDIN_FILENO);
		read_fd = pipe_fd;
		close(STDIN_FILENO);
		int fd = open("/dev/tty", O_RDONLY);
		if(errno != 0 && fd == -1){
			return perr(errno);	
		}
	}

	if(!in_tty){
		struct stat read_stat = { 0 };
		int err_stat = stat(argv[1], &read_stat);
		if(err_stat == -1){ return perr(errno); }
		
		read_fd = open(argv[1], O_RDONLY);
		if(read_fd == -1){ return perr(errno); }

		/* way to get filesize */
		/* end = lseek(read_fd, 0, SEEK_END);
		 *if(end == -1) { return perr(errno); }
		 *start = lseek(read_fd, 0, SEEK_SET);
		 *if(start == -1) { return perr(errno); }
		 */
		end = read_stat.st_size;
	} else {
		end = ULONG_MAX;
	}

	if(argc > 2){
		lines = 2 + 1 + strastr(argv[1], NL);
 		put_header(argv[1]);	
	}

	while(print_b < end){
		read_e = read(read_fd, buf + offset, page_size - offset - 1);

		if(read_e == 0) { return 0; }
		if(read_e == -1) { return perr(errno); }

		/* balance read(2) output */
		read_e += offset; 

		ptr = buf;
		offset = 0;
		while(ptr - buf < read_e){
			nlptr = memmem(ptr, read_e , NL, 1)+1;

			if(nlptr < buf){
				/* move last line */
				offset = read_e - (ptr - buf);
				memcpy(buf, ptr, offset+1);
				buf[offset+1] = 0;
				break;
			} else {
				line_l = (nlptr-ptr)<columns?(nlptr-ptr):columns;
			}

			/* get real string size on terminal 
			 * scrlen >= strlen (or nlptr - buf - buf_b)
			 */
			real_l = scrlen(ptr, line_l, tab_spaces);

			/* decrease line_l until real_l > columns */
			/* 
			 * TODO: small perfomance boost, get logic line 
			 * from start to columns
			 */
			while(real_l > columns){
				nlflag = 1;
				real_l = scrlen(ptr, --line_l, tab_spaces);
			}

			zputb(ptr, line_l);

			/* i dont know how does it work */
			if(nlflag && iflag){
				lines++;
			}
			if(iflag == 1){
				iflag = 0;
			} else {
				lines++;
			}
			if(nlflag){
				nlflag = 0;
				if(ptr[line_l] != NLC)
					zprintf(NL);
				else 
					lines--;
			}
			/* wut ta hell it was? */
			
			if(lines == current_rows_limit){
				lines = 0;

				if(lnflag){
					lnflag = 0;
					current_rows_limit = screen_rows_limit;
				}
				if(scflag){
					scflag = 0;
					current_rows_limit = screen_rows_limit;
				}

				/* nl for the command-line */
				if(ptr[line_l-1] != NLC && (ptr[line_l - 1] != TAB || real_l != columns)){ 
					zprintf(NL); 
				}

				print_prompt(print_b, end);

				cmd = wait_for_a_command();

				clean_prompt();
				switch(cmd) {
					case CMD_EXIT:
						return 0;
						break;
					case CMD_LINE:
						lnflag = 1;
						current_rows_limit = 1;
						break;
					case CMD_SCREEN:
						scflag = 1;
						current_rows_limit = screen_rows_limit;
						break;
				}
				
			}
			ptr     += line_l;
			print_b += line_l;
		}
	}
	
	close(read_fd);
	return 0;
}
