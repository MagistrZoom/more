#include "include/command.h"

/* set up no-echo and canonical mode
 * block signals SIGINT and SIGTSTP
 * read one char
 * unblock signals SIGINT and SIGTSTP
 * set up echo and non-canonical mode
 */
char **parse_flags(int *flags, char **argv){
	int index = 1;
	unsigned char fs = 1;
	unsigned char off = 1;
	while(fs){
		switch(*argv[index]){
			case '-':
				switch(argv[index][1]){
					case 'c':
						*flags |= C_FLAG;
						index++;
						break;
					case 'w':
						*flags |= W_FLAG;
						index++;
						break;
					case '-':
						fs = 0;	
						break;
					case 'l':
						off=2;	
						if(!argv[index][2]){
							index++;
							off=0;
						}
					default:
						*flags |= L_FLAG;	
						char *r;
						option_rows = strtol(&argv[index][off], &r, 10);
						if(option_rows <= 0 || r == &argv[index][off]){
							errno = EINVAL;
							zassert(1)
						}
						off = 1;
						index++;
						break;
				}
				break;
			default:
				fs = 0;	
				break;
		}
	}


	return &argv[index];
}
enum CMD wait_for_a_command(size_t *k){
	term_mode(COMMAND_SET, &term);	

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
	term_mode(COMMAND_UNSET, &term);	

	return CMD_INV;
}
