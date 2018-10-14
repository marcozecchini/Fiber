#include "fiber.h"
#include <linux/kernel.h>
#define BAD 999
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Zecchini <marcozecchini.2594@gmail.com>");
MODULE_DESCRIPTION("This module was created to build a Fiber infrustructure"
					"that can be used by the system");

static struct proc_dir_entry *fiber_folder;
static unsigned short next_fid;


void* ConvertThreadToFiber(fiber_data_t fiber_data, DWORD flag){
		fiber_t *fiber;
		thread_info* info_thread;
		
		if (flag == BAD){
			errno = EBADMSG;
			printk(KER_INFO "Returning, bad flag", KBUILD_MODNAME);
			return NULL;
		}
		
		info_thread = __current_thread_info();
		
		fiber = kmalloc(sizeof(fiber_t), GFP_KERNEL);
		if (!fiber){
			errno = ENOMEM;
			printk(KER_INFO "Not enough memory to allocate fiber instance", KBUILD_MODNAME);
			return NULL;
		}
		
		//Copy values into fiber struct
		fiber->fiber_data = fiber_data;
		fiber->info = info_thread;
		fiber->signal = info_thread->task->signal;
		fiber->sighand = info_thread->task->sighand;
		fiber->blocked = info_thread->task->blocked;
		fiber->stack_base = info_thread->task->mm->start_stack;
		fiber->stack_limit = info_thread->addr_limit;
		//create instances in proc filesystem
		fiber->fid = find_fid();
		fiber->fiber_entry = create_proc_entry(fiber->fid, "fiber");
		
		return (void*)fiber;
		
}

unsigned short find_fid(void){
	
}

static void __init fiber_init(void){
	fiber_folder = proc_mkdir("fiber", NULL); //Null is the parent
	if (!fiber_folder){
			printk(KER_ERR "Error in the creation of fiber proc folder", KBUILD_MODNAME);
			return -ENOMEM;
	}
	next_fid = 0;
}

static void __exit fiber_exit(void){
	printk(KER_INFO "Cleaning up fiber interface", KBUILD_MODNAME);
	next_fid = 0;
	remove_proc_entry("fiber", NULL);
}	

module_init(fiber_init);
module_exit(fiber_exit);
