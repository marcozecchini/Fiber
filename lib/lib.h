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

#define init_dev() do{\
	fiber_fd = open("/dev/fiber_dev", O_RDWR);\
	if (fiber_fd < 0){\
		printf("Error in opening the device: %s\n", strerror(errno));\
		close(fiber_fd); }\
	char buf[256];\
	int ret = 0;\
	int bytes_read = 0;\
	while ((ret = read(fiber_fd, buf+bytes_read, sizeof(buf)-bytes_read)) != 0){\
		if (ret < 0 && errno == -EINTR)\
			continue;\
		if (ret < 0)\
			return;\
		bytes_read += ret;}\
	char *token;\
	char *ptr;\
	const char delimiter[2] = "\n";\
	token = strtok(buf, delimiter);\
	int i = 0;\
	while (token != NULL){\
			cmd[i] = strtol(token, &ptr, 10);\
			token = strtok(NULL, delimiter);\
			i++;\
			if (i == CMDS)\
				break;}\
} while (0) 

#define close_lib() close(fiber_fd);

typedef void* fiber_data_t;
typedef void (*fiber_function) (fiber_data_t);

long SwitchToFiber(pid_t fiber);
pid_t ConvertThreadToFiber();
pid_t CreateFiber(unsigned long size, fiber_function routine, void* data);
long FlsAlloc();
bool FlsFree(long i);
void FlsSetValue(long long buf, long i);
long long FlsGetValue(long i);


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


