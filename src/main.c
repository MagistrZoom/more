#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "include/zprintf.h"

int in_tty = 0;
int piped_fd = 0;

void perr(int err){
	char *err_ptr = strerror(err);
	dzprintf(STDERR_FILENO, "%s\n", err_ptr);
}

int main(int argc, char *argv[]) {

	/*TODO: argument and options parsing			*/





	/* reopen control STDIN descriptor and save stdin from pipe */
	if(!isatty(STDIN_FILENO)){
		in_tty = 1;
		piped_fd = dup(STDIN_FILENO);
		close(STDIN_FILENO);
		int fd = open("/dev/tty", O_RDONLY | O_LARGEFILE);
		if(errno != 0 && fd == -1){
			perr(errno);	
			return errno;
		}
	}

	/*let's start to read regular files using mmap	*/
	if(!in_tty){
		struct stat read_stat = { 0 };
	
		stat(argv[1], &read_stat);
		int read_fd = open(argv[1], O_RDONLY | O_LARGEFILE);
		char *read_ptr = mmap(NULL, read_stat.st_size, PROT_READ, 
				MAP_PRIVATE, read_fd, 0);
	}


	return 0;
}

