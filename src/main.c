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

/*
 * @function read_data
 * @description read @limit bytes into @buf
 *
 * @param (int) fd file descriptor
 * @param (char*)  	buf
 * @param (off_t) 	offset
 * @param (size_t) 	limit
 *
 * @return (size_t) @limit or less written in buf
 */
size_t read_data(int fd, char *buf, off_t *offset, size_t limit){
	size_t o_data, o_hole, read_limit = 0, current_read = 0, max_read = 0;
A:
	o_data = lseek(fd, *offset, SEEK_DATA);
	if(o_data == *offset){
		goto B;
	}
C:
	*offset = lseek(fd, *offset, SEEK_HOLE);
	*offset = lseek(fd, *offset, SEEK_DATA);
B:
	o_hole = lseek(fd, *offset, SEEK_HOLE);
	lseek(fd, *offset, SEEK_SET);
	max_read = o_hole - *offset;
	if(max_read > limit)
		max_read = limit;
	current_read = read(fd, buf + read_limit, max_read);
	if(current_read > 0){
		read_limit += current_read;
		*offset = lseek(fd, 0, SEEK_CUR);
		goto A;
	}
E:
	return read_limit;
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


		/*
		 *
		 * lseek + read
		 * Punch holes away
		 * 
		 */
		off_t offset = 0;
		char *buffer = malloc(sizeof(char)*page_size);
		size_t data = read_data(read_fd, buffer, &offset, page_size-1);	
		
		if(data != page_size - 1){
			/* end_of_reading() */
			return 0;
		}
	}
	return 0;
}


