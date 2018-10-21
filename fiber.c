#include "fiber.h"

#define BAD 999

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Zecchini <marcozecchini.2594@gmail.com>");
MODULE_DESCRIPTION("This module was created to build a Fiber infrustructure"
					"that can be used by the system");

static struct proc_dir_entry *fiber_folder;
static unsigned short next_fid;

unsigned short find_fid(void){
	return 0;
}


void* ConvertThreadToFiber(fiber_data_t fiber_data, DWORD flag){
		fiber_t *fiber;
		struct task_struct *info;
		char *proc_name;
		
		if (flag == BAD){
			printk(KERN_ERR "Returning, bad flag", KBUILD_MODNAME);
			return NULL;
		}
		
		info = current;
		
		fiber = kmalloc(sizeof(fiber_t), GFP_KERNEL);
		if (!fiber){
			printk(KERN_ERR "Not enough memory to allocate fiber instance", KBUILD_MODNAME);
			return NULL;
		}
		
		//Copy values into fiber struct
		fiber->fiber_data = fiber_data;
		fiber->info = info;
		fiber->signal = info->signal;
		fiber->sighand = info->sighand;
		fiber->blocked = info->blocked;
		fiber->stack_base = info->mm->start_stack;
		fiber->stack_end = info->mm->mmap_base - 1;
		fiber->function = NULL; 
		fiber->next_instruction = (unsigned long) __builtin_return_address(0); 
		//create instances in proc filesystem
		fiber->fid = find_fid();
		//TODO last is file_operations
		snprintf(proc_name, 2, "%d", fiber->fid);
		fiber->fiber_entry = proc_create(proc_name, 0666, fiber_folder, NULL); 
		
		return (void*)fiber;
		
}


void* CreateFiber(size_t stack_size, void (*routine_pointer), fiber_data_t fiber_data){

	fiber_t *fiber;
	struct task_struct *info;
	char *proc_name;
	void *base_address_stack;
		
	if (stack_size > PAGE_SIZE){
		printk(KERN_ERR "Returning, bad initialization", KBUILD_MODNAME);
		return NULL;
	}
	
	fiber = kmalloc(sizeof(fiber_t), GFP_KERNEL);
	if (!fiber){
		printk(KERN_ERR "Not enough memory to allocate fiber instance", KBUILD_MODNAME);
		return NULL;
	}
	
	base_address_stack = kmalloc(sizeof(stack_size), GFP_KERNEL);
	if (!base_address_stack){
		printk(KERN_ERR "Not enough memory to allocate the new stack\nDeallocate all", KBUILD_MODNAME);
		kfree(fiber);
		return NULL;
	}
	
	//Copy values into fiber struct
	fiber->fiber_data = fiber_data;
	fiber->info = info;
	fiber->signal = info->signal;
	fiber->sighand = info->sighand;
	fiber->blocked = info->blocked;
	fiber->stack_base = (unsigned long)base_address_stack; 
	fiber->stack_end = (unsigned long)base_address_stack + stack_size;
	fiber->function = routine_pointer; 
	fiber->next_instruction = (unsigned long)routine_pointer; 
	//create instances in proc filesystem
	fiber->fid = find_fid();
	//TODO last is file_operations
	snprintf(proc_name, 2, "%d", fiber->fid);
	fiber->fiber_entry = proc_create(proc_name, 0666, fiber_folder, NULL); 
	
	return (void*)fiber;
	
}

static int __init fiber_init(void){
	fiber_folder = proc_mkdir("fiber", NULL); //Null is the parent
	if (!fiber_folder){
			printk(KERN_ERR "Error in the creation of fiber proc folder", KBUILD_MODNAME);
			return -ENOMEM;
	}
	next_fid = 0;
	printk(KERN_INFO "fiber installed", KBUILD_MODNAME);
	return 0;
}

static void __exit fiber_exit(void){
	printk(KERN_INFO "Cleaning up fiber interface", KBUILD_MODNAME);
	next_fid = 0;
	remove_proc_entry("fiber", NULL);
}	

module_init(fiber_init);
module_exit(fiber_exit);
