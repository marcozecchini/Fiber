#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h> 
#include <linux/types.h>
#include <asm/processor.h>
#include <asm/fpu/types.h>
#include <asm/ptrace.h>
#include <asm/fpu/internal.h>
#include <linux/bitmap.h>
#include <linux/ptrace.h>
#include <linux/spinlock.h>
#include <linux/hashtable.h>
#include <asm/atomic.h>

#define STACK_SIZE 4096
#define MAX_FLS 4096
#define FAILURE -1

typedef void* FLS_data_t;
typedef void* fiber_data_t;
typedef void (*fiber_function) (fiber_data_t);

/*
 * structs to better manage process and thread data
 */ 

typedef struct {
	pid_t pid;
	DECLARE_HASHTABLE(threads, 10);
	DECLARE_HASHTABLE(fibers, 10);
	struct hlist_node node;
	atomic64_t last_fiber_id;
	atomic64_t active_threads;
	
} process_t;

typedef struct {
		pid_t thread_id;
		struct hlist_node node;
		process_t *parent_process;
		void *current_fiber;
} thread_t;

/*
 * struct used to manage fiber
 */
typedef struct {
	
	pid_t fid;
	char name[128];
	
	spinlock_t lock;
	
	void* stack_base; //stack data virtual memory base address
	unsigned long stack_size;
	//context cpu
	struct pt_regs regs;
	struct fpu fpu;
	
	//to interface with threads and processes
	struct hlist_node node;
	thread_t *attached_thread;
	process_t *parent;
	
	//to manage fls
	long long FLS_data[MAX_FLS];
	DECLARE_BITMAP(fls_bitmap, MAX_FLS);
	//to deal with proc 
	struct proc_dir_entry *fiber_entry;
	//To know all the other active fiber
	struct list_head list;
	
	//For statistics to be inserted into /proc files
	void* start_address;
	pid_t creator_thread;
	unsigned long activation_counter;
	atomic64_t failed_counter;
	unsigned long total_time;
	
} fiber_t;

/*
 * Struct to pass data to IOCTL calls
 */ 
typedef struct {
	unsigned long stack_size;
	void* stack_base;
	fiber_function routine_pointer;
	fiber_data_t function_arguments;
	long index;
	long long buffer;
	pid_t fid;
} arguments_fiber;

/*
 * functions used to initialize fibers
 */
static int __init fiber_init(void);
static void __exit fiber_exit(void);

/*
 * Functions that manages Fibers
 */ 
pid_t ConvertThreadToFiber(pid_t tid);
pid_t CreateFiber(void* stack_base, unsigned long stack_size, fiber_function routine_pointer, fiber_data_t __user fiber_data, pid_t tid);
int SwitchToFiber(pid_t fid, pid_t tid);

/*
 * Functions to manage Fiber Local Storage
 */
long FlsAlloc(pid_t tid);
bool FlsFree(long i, pid_t tid);
long long FlsGetValue(long i, pid_t tid);
void FlsSetValue(long i, long long value, pid_t tid);
	
/*
 * Util function
 */
extern thread_t* find_thread(pid_t pid, process_t *process);
extern fiber_t *find_fiber(pid_t fid, process_t *process);
extern process_t *find_process(pid_t pid);
int cleanup_memory(void);

/** 
 * External functions
 */
extern int register_probe_exit(void);
extern int register_kret_proc_dir(void);
extern int unregister_kret_proc_dir(void);
extern int unregister_probe_exit(void);
extern int register_kprobe_time(void);
extern int unregister_kprobe_time(void);

