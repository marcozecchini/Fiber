#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h> //TODO this with proc. It should be init in fiber_init and fiber_exit
#include <linux/signal.h>
#include <linux/mm.h>
#include <asm/thread_info.h>

typedef unsigned int DWORD; 
typedef void* FLS_data_t;
typedef void* fiber_data_t;
/*
 * struct used to manage fiber
 */
typedef struct {
	
	unsigned short fid;
	fiber_data_t fiber_data;
	signal_struct *signal; //exception list
	sighand_struct *sighand;
	sigset_t blocked;
	unsigned long stack_base; //stack data virtual memory base address
	unsigned long stack_limit;
	thread_info* info; //context information
	FLS_data_t FLS_data;
	proc_dir_entry *fiber_entry;
	
} fiber_t;
	
/*
 * functions used to initialize fibers
 */
static void __init fiber_init(void);
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
	
