#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "include/string.h"
#include "include/zprintf.h"

#include <malloc.h>

#define NL "\n"
#define COLUMNS (80)
#define LINES (24)

int in_tty = 0;
int piped_fd = 0;

/* TODO: make header*/
int perr(int err){
	char *err_ptr = strerror(err);
	dzprintf(STDERR_FILENO, "%s\n", err_ptr);
	return err;
}

/* TODO: make header*/
void put_header(char *filename){
	zprintf("::::::::::::\n%s\n::::::::::::\n", filename);	
}

int main(int argc, char *argv[]) {
	/*TODO: argument and options parsing			*/

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

		int lines = 2 + strastr(argv[1], NL);

		put_header(argv[1]);	

		char *buffer = malloc(sizeof(char)*page_size);
		
		int r_code;
		size_t read_bytes = 0;

		//way to get filesize
		off_t end = lseek(read_fd, 0, SEEK_END);
		if(end == -1) { return perr(errno); }
		off_t start = lseek(read_fd, 0, SEEK_SET);
		if(start == -1) { return perr(errno); }

		while(read_bytes < end){
			r_code = read(read_fd, buf, page_size-1);
			if(!r_code) { return 0; /* end of file */ }
			if(r_code == -1){ return perr(errno); }
			read_bytes += r_code;


		}
		
	}
	return 0;
}


