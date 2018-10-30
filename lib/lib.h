#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define U_CONVERT_THREAD 0
#define U_CREATE_FIBER  1
#define U_SWITCH_FIBER 2
#define U_FLS_ALLOC 3
#define U_FLS_FREE 4
#define U_FLS_GET 5
#define U_FLS_SET 6
#define CMDS 7


int fiber_fd;
long cmd[CMDS];

typedef void* fiber_data_t;
typedef void (*fiber_function) (fiber_data_t);
typedef struct {
	unsigned long stack_size;
	void* stack_base;
	fiber_function routine_pointer;
	fiber_data_t function_arguments;
	long index;
	long long buffer;
	pid_t fid;
} arguments_fiber;


