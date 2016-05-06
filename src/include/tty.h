#pragma once
#include <unistd.h>

#include <termios.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

#include "zassert.h"
#include "signal.h"
#include "string.h"
#include "zprintf.h"

#define COMMAND_SET (1)
#define COMMAND_UNSET (0)

struct termios term, def_term;

void handle_stdin(int *in_tty, int *pipe_fd, int *read_fd);
void reset_tty();
void term_mode(int action, struct termios *term);
void clear_screen();
