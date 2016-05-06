#pragma once
#include "tty.h"
#include "string.h"


#define reset_cmd(flag, v) do{ if(flag){ flag = 0; current_rows_limit = v;}} while(0)
#define cmd_case(N,M) case N: return CMD_##M; break

#define W_FLAG 1
#define L_FLAG 2
#define C_FLAG 4

int option_rows;


enum CMD{
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
};		

char **parse_flags(int *flags, char **argv);
enum CMD wait_for_a_command(size_t *k);
