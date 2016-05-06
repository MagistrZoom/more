#include "include/command.h"
#include "include/zassert.h"
#include "include/string.h"
#include "include/zprintf.h"
#include "include/tty.h"
#include "include/signal.h"

#include <malloc.h>

#include <math.h>

#include <limits.h> /* just for one constant */

#define INIT_LINES (256)

char *prompt = "More";
char *help = "\n\
***************************************************************************\n\
Most commands optionally preceded by integer argument k.  \n\
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

struct termios term = { 0 };
struct termios def_term = { 0 };


/* 
 * prompt in the bottom of screen 
 */
void print_prompt(char *pr, size_t cur, size_t end, int in_tty){
	int percent = round((double)cur/end*100);
	
	/* set up foreground black and background white 
	 * print prompt, percents
	 */
	if(!in_tty){
		zprintf("[30;47m--%s--(%d%%)[0m", pr, percent);
	} else {
		zprintf("[30;47m--%s--[0m", pr);
	}
}
void clean_prompt(){
	/* cleans entire line and returns carriage */
	zprintf("[2K\r");
}


int main(int argc, char *argv[]) {
	if(argc < 2){
		zprintf("Usage: more -cw -[4mlines[0m filename\n");
		return EINVAL;
	}

	int flags = 0;

	char **files = parse_flags(&flags, argv);

	/* need to get optimal buf size */
	size_t page_size = sysconf(_SC_PAGESIZE);

	int in_tty = 0;
	/* get terminal parameters and size */
	int term_err = tcgetattr(STDERR_FILENO, &term);
	zassert(term_err < 0)
	def_term = term;
	struct winsize terminal_d = { 0 };
	term_err = ioctl(STDERR_FILENO, TIOCGWINSZ, &terminal_d);
	zassert(term_err < 0)
	int columns = terminal_d.ws_col;
	int rows = terminal_d.ws_row - 1; /*1 line reserved for a prompt*/
	if(flags & L_FLAG){
		rows = option_rows;
	}
	
	int read_fd;
	
	off_t end = 0;

	char *buf = calloc(sizeof(char), page_size);
	char *ptr;

	size_t print_b = 0;			
	size_t i = 0;			
	size_t offset = 0;
	size_t read_e;

	unsigned int lines = 0;
	unsigned int total_lines = 0;

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

	/* repeat cache */
	enum CMD previous_command = CMD_NOP;

	/* reopen control STDIN descriptor and save stdin from pipe */
	if(!isatty(STDIN_FILENO)){
		handle_stdin(&in_tty, &pipe_fd, &read_fd);
	}

	if(!in_tty){
		struct stat read_stat = { 0 };
		int err_stat = stat(files[0], &read_stat);
		zassert(err_stat < 0)
		
		read_fd = open(files[0], O_RDONLY);
		zassert(read_fd < 0)

		end = read_stat.st_size;
	} else {
		end = ULONG_MAX;
	}


	/* -c flag impl */
	if(flags & C_FLAG){
		clear_screen();
	}

	if(argc - (files - argv) > 1){
		if(!(flags & L_FLAG))
			lines = 2 + 1 + strastr(files[0], NL);
 		put_header(files[0]);	
	}

	while(print_b < end){
		read_e = read(read_fd, buf + offset, page_size - offset - 1);
		zassert(read_e < 0)
		
		if(!read_e && !offset) { 
			reset_tty(); 
			return 0; 
		}
		/* balance read output */
		read_e += offset; 
		
		ptr = buf;
		offset = 0;

		while(ptr - buf < read_e){
			size_t len = scrlen(ptr, read_e - (ptr-buf), columns, tab_spaces);
			//end of buffer reached
			if(len < columns && len+(ptr-buf) == read_e 
					&& print_b + len != end){
				offset = len;
				memcpy(buf, ptr, offset);
				break;
			}

			zputb(ptr, len);
			lines++;

			ptr 	+= len;
			print_b	+= len;

			if(lines >= current_rows_limit){
				if(ptr[-1] != NLC){
					zprintf(NL);
				}

				reset_cmd(lnflag, 1);
                reset_cmd(scflag, screen_rows_limit);
                reset_cmd(hscflag, half_rows_limit);

				total_lines += lines;

				lines = 0;
				
				print_prompt(prompt, print_b, end, in_tty);

REQ_CMD:		i = 0;
				enum CMD cmd = wait_for_a_command(&i);
				
				if(cmd != CMD_REPEAT){ previous_command = cmd; }
				clean_prompt();
EXEC_CMD:		switch(cmd) {
					case CMD_EXIT:
					    reset_tty();
					    return 0;
					    break;
					case CMD_LINE:
					    lnflag = 1;
					    screen_def_rows_limit = i?i:screen_def_rows_limit;
					    screen_rows_limit = i?i:screen_rows_limit;
					    current_rows_limit = i?i:line_rows_limit;
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
					    print_prompt(prompt, print_b, end, in_tty);
						goto REQ_CMD;
					    break;
					case CMD_REPEAT:
					    cmd = previous_command;
					    goto EXEC_CMD;
					    break; 
					case CMD_INV:
					    clean_prompt();
					    zprintf("Invalid command");
					default:
					    goto REQ_CMD;
					    break;
				}
			}
		}
	}

	/* -w flag implementation */
	if(flags & W_FLAG){
		print_prompt("No-more", 0, 0, 1);
		i = 0;
		(void)wait_for_a_command(&i);
		clean_prompt();
	}
	
	free(buf);
	close(read_fd);
	reset_tty();	
	return 0;
}
