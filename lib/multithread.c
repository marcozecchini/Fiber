
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "user_lib.h"
#include "lib.h"


struct thread_info {
        unsigned long data;
        unsigned long data2;
};

void alarm_handler(int signal)
{
  alarm(3);
  printf("ALARM HANDLER: Trying to switch to fiber 10\n");
  int ret = SwitchToFiber(2);
  if (ret == -1)
      printf("CANNOT SWITCH TO THREAD 10!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

void foo (void * param)
{
        struct thread_info  *t = (struct thread_info *) param;

        long counter = 0;

        printf("FIBER %d: old thread %d\n", (int)t->data2, (int)t->data);
        SwitchToFiber(t->data);

        long index = FlsAlloc();
        printf("FIBER %d: index given is %ld\n", (int)t->data2, index);
        if (index == -1)
                exit(0);
        FlsSetValue(++counter, index);
        long long res = FlsGetValue(index);
        printf("FIBER %d: trying to free index %ld\n", (int)t->data2, index);
        if (!FlsFree(index))
                printf("FIBER %d: cannot free index %ld\n", (int)t->data2, index);

        printf("FIBER %d: returned value %lld\n", (int)t->data2, res);
        exit(0);
}


void * thread_function(void * param){
        struct thread_info *t = (struct thread_info*) param;
        int k = ConvertThreadToFiber();
        printf("%d", k);
        struct thread_info t_info ={ .data = k };
        int new_fiber = CreateFiber(2*4096, foo, (void*)&t_info);
        t_info.data2 = new_fiber;
        printf("THREAD %ld: CreateFiber done, new fiber_id %d\n", t->data, new_fiber);
        SwitchToFiber(new_fiber);
        printf("qui");
        SwitchToFiber(new_fiber);
        return NULL;
}

#define NUM_THREADS 10
int main()
{	init_dev();
        printf("MAIN: Starting main...\n");
        //int myfiber = ConvertThreadToFiber();
        //printf("MAIN: ConvertThreadToFiber done, main fiber id %d\n", myfiber);
        pthread_t threads[NUM_THREADS];
        struct thread_info t_info[NUM_THREADS];
        unsigned long i = 0;
        for (i = 0; i < NUM_THREADS; i++) {
                t_info[i].data = i;
                pthread_create(&threads[i], NULL, thread_function, (void*) &t_info[i]);
        }
        
        signal(SIGALRM, alarm_handler);
        alarm(3);
        for (i = 0; i < NUM_THREADS; i++) {
                pthread_join(threads[i], NULL);
        }
       close_lib();
}
