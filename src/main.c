#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "include/string.h"
#include "include/zprintf.h"

#include <malloc.h>

#include <math.h>

#include <termios.h>
#include <sys/ioctl.h>

#include <signal.h> 

#include <limits.h> /* just for one constant */

#define NL "\n"
#define NLC '\n'
#define COMMAND_SET (1)
#define COMMAND_UNSET (0)
#define INIT_LINES (256)

#define reset_cmd(flag, v) do{ if(flag){ flag = 0; current_rows_limit = v;}} while(0)
#define cmd_case(N,M) case N: return CMD_##M; break

char *prompt = "More";
char *help = "Most commands optionally preceded by integer argument k.  \n\
Defaults in brackets.  Star (*) indicates argument becomes new default.\n\
---------------------------------------------------------------------------\n\
<space>     Display next k lines of text [current screen size]\n\
z           Display next k lines of text [current screen size]*\n\
<return>    Display next k lines of text [1]*\n\
d			Scroll k lines [current scroll size, initially 11]*\n\
q 			Exit from more\n\
b 			Skip backwards k screenfuls of text [1]\n\
=           Display current line number\n\
:f          Display current file name and line number\n\
.           Repeat previous command\n\
---------------------------------------------------------------------------\n";

int pipe_fd = 0;
int tab_spaces = 8;
sigset_t sig, osig;

struct termios term = { 0 };

enum CMD {
	CMD_EXIT = 0,	/*!q		interrupt 									 */
	CMD_HELP, 		/* h		tiny help  								     */
	CMD_SCREEN, 	/*!i<SP>	(screen usually) down 						 */
	CMD_SCREEN_DEF, /*!iz 		(screen usually) down and set up i as default*/
	CMD_LINE, 		/*!i<CR> 	scroll i(1) lines and set up i as default 	 */
	CMD_HALF,		/*!id 		by default scroll 11 lines, i as new default */
	CMD_SKIP,		/* is		skip i(1) lines of text 					 */
	CMD_SKIP_SCREEN,/* if		skip i(1) screenfuls of text 				 */
	CMD_BACK,		/* ib		backward i(1) screenfuls of text, files only */
	CMD_PRINTL,		/*!=		print line 									 */
	CMD_PRINTFNL,	/*!:f 		print filename and line						 */
	CMD_REPEAT,		/* .		repeat last command 						 */
	CMD_INV,		/*!			any other command							 */
	CMD_NOP
} CMD;

void command_mode(int action){
	if(action){
		term.c_lflag &= ICANON & !ECHO; 
		int tcs_set_err = tcsetattr(STDERR_FILENO, TCSANOW, &term);
		zassert(tcs_set_err < 0)
	    int sigproc_err = sigprocmask(SIG_BLOCK, &sig, &osig);
		zassert(sigproc_err < 0)
	} else {
		term.c_lflag &= !ICANON & ECHO; 
		int tcs_set_err = tcsetattr(STDERR_FILENO, TCSANOW, &term);
		zassert(tcs_set_err < 0)
	    int sigproc_err = sigprocmask(SIG_SETMASK, &osig, NULL);
		zassert(sigproc_err < 0)
	}
}

/* set up no-echo and canonical mode
 * block signals SIGINT and SIGTSTP
 * read one char
 * unblock signals SIGINT and SIGTSTP
 * set up echo and non-canonical mode
 */
enum CMD wait_for_a_command(size_t *k){
	command_mode(COMMAND_SET);	

	size_t rerror = 1;
	size_t len = 16;
	size_t i = 0;
	char key;
	char *ptr = calloc(1, 16);
	while(rerror){
		rerror = read(STDIN_FILENO, &key, 1);
		zassert(rerror < 0);
		if(key < '0' || key > '9' || !rerror){
			break;
		}
		if(i >= len){
			char *tmp = realloc(ptr, len*=2);
			zassert(!tmp);
			ptr = tmp;
		}
		ptr[i] = key;
		i++;
	}
	char *endptr;
	size_t k_tmp = strtoll(ptr, &endptr, 10);
	if(endptr)
		*k = k_tmp;

	switch(key){
		cmd_case(' ', SCREEN);
		cmd_case('z', SCREEN_DEF);
		cmd_case(NLC, LINE);
		cmd_case('d', HALF);
		cmd_case('s', SKIP);
		cmd_case('f', SKIP_SCREEN);
		cmd_case('b', BACK);

		cmd_case('q', EXIT);
		cmd_case('h', HELP);
		cmd_case('=', PRINTL);
		cmd_case('.', REPEAT);
		case ':':
			rerror = read(STDIN_FILENO, &key, 1);
			zassert(rerror <= 0); /*omg*/
			switch(key){
				case 'f':
					return CMD_PRINTFNL;
					break;
			}
			break;
	}

	free(ptr);
	command_mode(COMMAND_UNSET);	

	return CMD_INV;
}

/* 
 * prompt in the bottom of screen 
 */
void print_prompt(size_t cur, size_t end, int in_tty){
	int percent = round((double)cur/end*100);
	
	/* set up foreground black and background white 
	 * print prompt, percents
	 */
	if(!in_tty){
		zprintf("[30;47m--%s--(%d%%)[0m", prompt, percent);
	} else {
		zprintf("[30;47m--%s--[0m", prompt);
	}
}
void clean_prompt(){
	/* cleans entire line and returns carriage */
	zprintf("[2K\r");
}

int main(int argc, char *argv[]) {
	/*TODO: argument and options parsing			*/


	/* need to get optimal buf size */
	size_t page_size = sysconf(_SC_PAGESIZE);

	int in_tty = 0;
	/* get terminal parameters and size */
	int term_err = tcgetattr(STDERR_FILENO, &term);
	zassert(term_err < 0)
	struct winsize terminal_d = { 0 };
	term_err = ioctl(STDERR_FILENO, TIOCGWINSZ, &terminal_d);
	zassert(term_err < 0)
	int columns = terminal_d.ws_col;
	int rows = terminal_d.ws_row - 1; /*1 line reserved for a prompt*/
//	int columns = 80;
//	int rows = 24;
	
	int read_fd;
	
	off_t end = 0, start = 0;

	char *buf = calloc(sizeof(char), page_size);
	/* assoc array "logic line" -> "file offset" */
	size_t *line_offset = calloc(INIT_LINES, sizeof(size_t)); 
	size_t line_offset_limit = INIT_LINES;
	char *ptr, *nlptr;

	int total_lines = 0; 
	int line_counter = 0; 

	size_t print_b = 0;			
	size_t line_l = columns;
	size_t real_l = 0; /* length with respect to tab alignment */
	size_t offset = 0;
	size_t i = 0;
	size_t read_e;

	unsigned int lines = 0;
	int nlflag = 0;
	int iflag = 0;
	int skflag = 0;

	/* default lengths */
	int current_rows_limit = rows;
	int screen_rows_limit = current_rows_limit;
	int screen_def_rows_limit = current_rows_limit;
	int line_rows_limit   = 1;
	int half_rows_limit   = 11;

	/* flags 4 commands */
	int lnflag = 0; /* line mode */
	int scflag = 0; /* screen mode */
	int hscflag = 0; /* half-screen mode */

	enum CMD cmd;
	/* repeat cache */
	enum CMD previous_command = CMD_NOP;

	/* reopen control STDIN descriptor and save stdin from pipe */
	if(!isatty(STDIN_FILENO)){
		in_tty = 1;
		pipe_fd = dup(STDIN_FILENO);
		zassert(pipe_fd < 0)
		read_fd = pipe_fd;
		close(STDIN_FILENO);
		int fd = open("/dev/tty", O_RDONLY);
		zassert(fd < 0)
	}

	if(!in_tty){
		struct stat read_stat = { 0 };
		int err_stat = stat(argv[1], &read_stat);
		zassert(err_stat < 0)
		
		read_fd = open(argv[1], O_RDONLY);
		zassert(read_fd < 0)

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
		zassert(read_e < 0)

		/* balance read(2) output */
		read_e += offset; 

		ptr = buf;
		offset = 0;
		while(ptr - buf < read_e){
			nlptr = memmem(ptr, read_e - (ptr-buf), NL, 1)+1;

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
			 * scrlen >= strlen (or nlptr - ptr)
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
/*				if(total_lines >= line_offset_limit){
					size_t *tmp = realloc(line_offset, line_offset_limit*=2);
					zassert(!tmp)
					line_offset = tmp;
				}
				line_offset[total_lines++] = print_b + (ptr - buf);*/
				lines++;
			}
			if(iflag){
				iflag = 0;
			} else {
/*				if(total_lines >= line_offset_limit){
					size_t *tmp = realloc(line_offset, line_offset_limit*=2);
					zassert(!tmp)
					line_offset = tmp;
				}
				line_offset[total_lines++] = print_b + (ptr - buf);*/
				lines++; 
			}
			if(nlflag){
				nlflag = 0;
				if(ptr[line_l] != NLC){
					zprintf(NL);
				}
				else {
					lines--;
/*					line_offset[total_lines--] = print_b + (ptr - buf);*/
				}
			}
			/* wut ta hell it was? */
			
			if(lines == current_rows_limit){
/*				total_lines += lines;*/
				lines = 0;

				reset_cmd(lnflag, 1);
				reset_cmd(scflag, screen_rows_limit);
				reset_cmd(hscflag, half_rows_limit);

				/* nl for the command-line */
				if(ptr[line_l-1] != NLC && (ptr[line_l-1] != TAB || real_l != columns)){ 
					zprintf(NL); 
				}
				
				print_prompt(print_b, end, in_tty);
REQ_CMD:
				if(print_b +line_l < end/* || -w flag */)
					cmd = wait_for_a_command(&i);

				clean_prompt();

				if(cmd != CMD_REPEAT){ previous_command = cmd; }
EXEC_CMD:		
				switch(cmd) {
					case CMD_EXIT:
						return 0;
						break;
					case CMD_LINE:
						lnflag = 1;
						screen_def_rows_limit = i?i:screen_def_rows_limit;
						screen_rows_limit = i?i:screen_rows_limit;
						current_rows_limit = i?i:1;
						break;
					case CMD_SCREEN_DEF:
						screen_def_rows_limit = i?i:screen_def_rows_limit;
					case CMD_SCREEN:
						scflag = 1;
						screen_rows_limit = i?i:screen_rows_limit;
						current_rows_limit = i?i:screen_rows_limit;
						break;
					case CMD_HALF:
						hscflag = 1;
						half_rows_limit = i?i:half_rows_limit;
						current_rows_limit = half_rows_limit;
						break;
					case CMD_PRINTL:
						clean_prompt();
						zprintf("%d", total_lines);
						goto REQ_CMD;
						break;
					case CMD_PRINTFNL:
						clean_prompt();
						zprintf("\"%s\" %d", argv[1], total_lines);
						goto REQ_CMD;
						break;
					case CMD_HELP:
						clean_prompt();
						zprintf("%s", help);
						print_prompt(print_b, end, in_tty);
						break;
					case CMD_REPEAT:
						cmd = previous_command;
						goto EXEC_CMD;
						break; 
					case CMD_BACK:
/*						line_counter = total_lines - (i?i:1+1)*rows;
						if(!in_tty && line_counter >= 0){
							skflag = 1;
							total_lines = line_counter;
							print_b = line_offset[total_lines];
							off_t seek_e = lseek(read_fd, print_b, SEEK_SET);
							zassert(seek_e < 0)
							break;
						}*/
					case CMD_INV:
						clean_prompt();
						zprintf("Invalid command");
					default:
						goto REQ_CMD;
						break;
				}
			}
			if(skflag){
				skflag = 0;
				break;
			}
			ptr     += line_l;
			print_b += line_l;
		}
	}
	
	free(buf);
	close(read_fd);
	return 0;
}
