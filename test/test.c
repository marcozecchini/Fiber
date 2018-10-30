#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "lib.h"

typedef struct {
		int i;
} pfiber_data, *pfiber_data_t;

#define FIBERS 3
#define PRIMARY_FIBER 0
#define SECOND_FIBER 1
#define THIRD_FIBER 2
#define STACK_SIZE 128

int *fibers[FIBERS];

void hello_1(void* par){
	pfiber_data_t data = (pfiber_data_t)par;
	printf("%d", data->i);
	SwitchToFiber(fibers[THIRD_FIBER]);
}

void hello_2(void *par){
	pfiber_data_t data = (pfiber_data_t)par;
	printf("%d", data->i);
	SwitchToFiber(fibers[PRIMARY_FIBER]);
}

int main(){
	printf("Allocating first data\n");
	pfiber_data_t data = (pfiber_data_t)malloc(sizeof(pfiber_data));
	if (data == NULL){
			printf("ERROR in allocatin data");
			close(fiber_fd);
			return errno;
	}
	
	data->i = 2;
	printf("Converting this thread to a fiber\n");
	fibers[PRIMARY_FIBER] = ConvertThreadToFiber(data, 1);
	if (fibers[PRIMARY_FIBER] < 0){
			printf("ERROR in creation of the first fiber");
			close(fiber_fd);
			return -1;
	}
	
	printf("Creating the second fiber\n");
	pfiber_data_t data2 = (pfiber_data_t)malloc(sizeof(pfiber_data));
	data2->i = 3;
	fibers[SECOND_FIBER] = CreateFiber(0, hello_1, data2);
	if (fibers[SECOND_FIBER] <0  ){
			printf("ERROR in creation of the second fiber\n");
			close(fiber_fd);
			return -1;
	}
	
	printf("Creating the third fiber\n");
	pfiber_data_t data3 = (pfiber_data_t)malloc(sizeof(pfiber_data));
	data3->i = 4;
	fibers[THIRD_FIBER] = CreateFiber(0, hello_2, data3);
	if (fibers[THIRD_FIBER] < 0){
			printf("ERROR in creation of the third fiber");
			close(fiber_fd);
			return -1;
	}
	
	printf("First number: %d", data->i);
	
	SwitchToFiber(fibers[SECOND_FIBER]);
	close(fd);
	free(data);
	free(data2);
	free(data3);
	return 0;
}
