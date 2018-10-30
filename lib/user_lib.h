#include <unistd.h>
#include <stdbool.h>

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

typedef void* fiber_data_t;
typedef void (*fiber_function) (fiber_data_t);

long SwitchToFiber(pid_t fiber);
pid_t ConvertThreadToFiber();
pid_t CreateFiber(unsigned long size, fiber_function routine, void* data);
long FlsAlloc();
bool FlsFree(long i);
void FlsSetValue(long long buf, long i);
long long FlsGetValue(long i);
void  close_lib();
