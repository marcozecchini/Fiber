#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h> 
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/mm.h>
#include <asm/thread_info.h>
#include <asm/page.h>

typedef unsigned int DWORD; 
typedef void* FLS_data_t;
typedef void* fiber_data_t;
typedef void (*fiber_function) (void* fiber_data_t);
typedef fiber_function fiber_function_t;
/*
 * struct used to manage fiber
 */
typedef struct {
	
	unsigned short fid;
	fiber_data_t fiber_data;
	struct signal_struct *signal; //exception list
	struct sighand_struct *sighand;
	sigset_t blocked;
	unsigned long stack_base; //stack data virtual memory base address
	unsigned long stack_end;
	struct task_struct* info; 
	fiber_function_t function;
	unsigned long next_instruction; 
	FLS_data_t FLS_data;
	struct proc_dir_entry *fiber_entry;
	
} fiber_t;

/*
 * functions used to initialize fibers
 */
static int __init fiber_init(void);
static void __exit fiber_exit(void);

/*
 * Functions that manages Fibers
 */ 
void* ConvertThreadToFiber(fiber_data_t fiber_data, DWORD flag);

void* CreateFiber(size_t stack_size, void (*routine_pointer), fiber_data_t fiber_data);

void SwichToFiber(fiber_t* fiber);

void DeleteFiber(fiber_t* fiber);

/*
 * Functions to manage Fiber Local Storage
 */
	
