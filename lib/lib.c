#include "lib.h"
#include "user_lib.h"


void fill_cmd(){
	char buf[256];
	int ret = 0;
	int bytes_read = 0;
	while ((ret = read(fiber_fd, buf+bytes_read, sizeof(buf)-bytes_read)) != 0){
		if (ret < 0 && errno == -EINTR)
			continue;
		if (ret < 0)
			return;
		bytes_read += ret;
	}
	
	char *token;
	char *ptr;
	const char delimiter = ',';
	
	token = strtok(buf, delimiter);
	int i = 0;
	while (token != NULL){
			cmd[i] = strtol(token, &ptr, 10);
			token = strtok(NULL, delimiter);
			i++;
			if (i == CMDS)
				break;
	}
}

void init_lib(){
	fiber_fd = open("/dev/fiber_dev", O_RDWR);
	if (fiber_fd < 0){
		printf("Error in opening the device: %s\n", strerror(errno));
		close(fiber_fd);
	}
	
	char buf[256];
	int ret = 0;
	int bytes_read = 0;
	while ((ret = read(fiber_fd, buf+bytes_read, sizeof(buf)-bytes_read)) != 0){
		if (ret < 0 && errno == -EINTR)
			continue;
		if (ret < 0)
			return;
		bytes_read += ret;
	}
	
	char *token;
	char *ptr;
	const char delimiter = ',';
	
	token = strtok(buf, delimiter);
	int i = 0;
	while (token != NULL){
			cmd[i] = strtol(token, &ptr, 10);
			token = strtok(NULL, delimiter);
			i++;
			if (i == CMDS)
				break;
	}
}

void close_lib(){
	close(fiber_fd);
}

long SwitchToFiber(pid_t fiber) { 
	arguments_fiber f = {
		.fid = fiber,
	};
	return ioctl(fiber_fd, cmd[U_SWITCH_FIBER], &f); 
}

pid_t ConvertThreadToFiber(){
	return (pid_t)ioctl(fiber_fd, cmd[U_CONVERT_THREAD], 0);
}

pid_t CreateFiber(unsigned long size, fiber_function routine, void* data){ 
	arguments_fiber f = {
		.stack_size = size,
		.routine_pointer = routine,
		.function_arguments = data,
	};
	if (posix_memalign(&(f.stack_base), 16, size))
		return -1;
	bzero(f.stack_base, size);
	
	return (pid_t)ioctl(fiber_fd, cmd[U_CREATE_FIBER], &f);
}

long FlsAlloc(){
	return ioctl(fiber_fd, cmd[U_FLS_ALLOC], 0);
}

bool FlsFree(long i){
		arguments_fiber f = {
			.index = i,
		};
		
		return ioctl(fiber_fd, cmd[U_FLS_FREE], &f);
}

void FlsSetValue(long long buf, long i){
		arguments_fiber f = {
				.index = i,
				.buffer = buf,
		};
		
		ioctl(fiber_fd, cmd[U_FLS_SET], &f);
}

long long FlsGetValue(long i){
	long long ret = 0;
	arguments_fiber f = {
			.index = i,
			.buffer = (unsigned long)&ret,
	};
	
	ioctl(fiber_fd, cmd[U_FLS_GET], &f);
	return ret;
}
