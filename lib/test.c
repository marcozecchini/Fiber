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

int i = 0;
void hello_1(void* par){
	int *fibers = (int*)par;
	SwitchToFiber(*fibers+2);
	SwitchToFiber(*fibers);
}

void hello_2(void *par){
	int *fibers = (int*)par;
	for (int j = 0; j < 100000000;j++)
		printf("%d", j);
	SwitchToFiber(*fibers);
	SwitchToFiber(*fibers+1);
}

int main(){
	init_dev();
	
	int myFiber = ConvertThreadToFiber();
	int c[3];
	c[0] = myFiber;
	c[1] = myFiber + 1;
	c[2] = myFiber + 2;
	/*
	 * Test 1 - Switch and Create
	 */
	
	int new_fiber = CreateFiber(256, hello_1, c);
	printf("new_fiber is %d\n", new_fiber);
	int new_fiber1 = CreateFiber(256, hello_2, c);
	printf("new_fiber is %d\n", new_fiber1);
	SwitchToFiber(new_fiber);
	SwitchToFiber(new_fiber1);
	printf("i e' %d\n", i);
	printf("My PID is: %d\n", getpid());
	scanf("\\n");
	/*
	 * Test 2 -Test Fls
	 */
	 /*
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
	*/
	close_lib();
	return 0;

}
