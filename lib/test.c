#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "user_lib.h"
#include "lib.h"

#define FIBERS 3
#define PRIMARY_FIBER 0
#define SECOND_FIBER 1
#define THIRD_FIBER 2
#define STACK_SIZE 128


void hello_1(void* par){
	printf("here");
}

void hello_2(void *par){

}

int main(){
	init_dev();
	
	int myFiber = ConvertThreadToFiber();
	char c[2] = "ch";
	/*
	 * Test Switch and Create
	 */
	
	/*int new_fiber = CreateFiber(16, hello_1, c);
	printf("new_fiber is %d\n", new_fiber);
	SwitchToFiber(new_fiber);*/
	printf("Exit\n");
	
	/*
	 * Test 2 -Test Fls
	 */
	long addr = FlsAlloc();
	long long val = 11111;
	FlsSetValue(val, addr);
	long long val2;
	val2 = FlsGetValue(addr);
	printf("%lld\n", val2);
	
	
	long addr2 = FlsAlloc();
	val = 22222;
	FlsSetValue(val, addr2);
	val2 = FlsGetValue(addr2);
	printf("%lld\n", val2);
	
	if (!FlsFree(addr)) printf("Error\n");
	if (!FlsFree(addr2)) printf("Error\n");
	close_lib();
	return 0;

}
