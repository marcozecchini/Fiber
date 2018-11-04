#include "device.h"
#include "fibers.h"
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Zecchini <marcozecchini.2594@gmail.com>");
MODULE_DESCRIPTION("This module was created to build a Fiber infrustructure"
					"that can be used by the system");

DEFINE_HASHTABLE(processes, 10);
spinlock_t spin_process = __SPIN_LOCK_UNLOCKED(spin_process);
unsigned long spin_flag;

process_t *find_process(pid_t pid){
	process_t* result;
	hash_for_each_possible_rcu(processes, result, node , pid){
			if (result == NULL)
				return NULL;
			if (result->pid == pid)
				return result;
	}
	return NULL;	
}

thread_t* find_thread(pid_t pid, process_t *process){
	thread_t *result;
	hash_for_each_possible_rcu(process->threads, result, node, pid){
		if (result == NULL)
			return NULL;
		if (result->thread_id==pid)
			return result;
	}
	return NULL;
}


fiber_t *find_fiber(pid_t fid, process_t *process){
	fiber_t *result;
	hash_for_each_possible_rcu(process->fibers, result, node, fid){
		if (result == NULL)
			return NULL;
		if (result->fid==fid)
			return result;
	}
	return NULL;
}



pid_t ConvertThreadToFiber(pid_t tid){
		fiber_t *fiber;
		thread_t *thread;
		process_t *process;
		
		//First check if the process is already in the hash otherwise add it
		spin_lock_irqsave(&(spin_process), spin_flag);
		process = find_process(current->tgid);
		if (process == NULL){
				process = kmalloc(sizeof(process_t), GFP_KERNEL);
				if (!process){
					printk("Not enough memory to allocate process_t instance", KBUILD_MODNAME);
					return NULL;
				}
		
				process->pid = current->tgid;
				atomic64_set(&(process->last_fiber_id), 0);
				atomic64_set(&(process->active_threads), 0);
				hash_init(process->threads);
				hash_init(process->fibers);
				hash_add_rcu(processes, &(process->node), process->pid);
		}
		spin_unlock_irqrestore(&(spin_process), spin_flag);
		
		//now check if the pid has already been used 
		thread = find_thread(tid, process);
		if (thread != NULL)
			return -1;
		
		//init thread 
		thread = kmalloc(sizeof(thread_t), GFP_KERNEL);
		if (!thread){
			printk("Not enough memory to allocate thread_t instance", KBUILD_MODNAME);
			return NULL;
		}
		thread->thread_id = tid;
		thread->parent_process = process;
		thread->current_fiber = NULL;
		atomic64_inc(&(thread->parent_process->active_threads));
		hash_add_rcu(process->threads, &(thread->node), tid);
		
		//init fiber
		fiber = kmalloc(sizeof(fiber_t), GFP_KERNEL);
		if (!fiber){
			printk("Not enough memory to allocate fiber_t instance", KBUILD_MODNAME);
			return NULL;
		}
		//Copy values into fiber struct
		fiber->lock = __SPIN_LOCK_UNLOCKED(lock);
		fiber->attached_thread = thread;
		fiber->parent = process;
		memset(fiber->FLS_data, 0, sizeof(long long) * MAX_FLS);
		bitmap_zero(fiber->fls_bitmap, MAX_FLS);
		memcpy(&(fiber->regs), current_pt_regs(), sizeof(struct pt_regs));
		fiber->fid = atomic64_inc_return(&process->last_fiber_id);
		hash_add_rcu(process->fibers, &(fiber->node), fiber->fid);
		
		fiber->stats.start_address = (void*) current_pt_regs()->ip;
		fiber->stats.creator_thread = current->pid;
		fiber->stats.activation_counter = 1;
		atomic64_set(&(fiber->stats.failed_counter), 0);
		copy_fxregs_to_kernel(&(fiber->fpu));
		fiber->stats.total_time = 0;
		snprintf(fiber->name, 128, "%d", fiber->fid);
		
		fiber->regs.sp = current_pt_regs()->sp;
		fiber->regs.bp = current_pt_regs()->bp;
		
		thread->current_fiber = fiber;
		
		if (!spin_trylock(&fiber->lock)) return -1;
		
		printk("Thread converted with this fid: %d", fiber->fid);
		return fiber->fid;
}

pid_t CreateFiber(void* stack_base, unsigned long stack_size, fiber_function routine_pointer, fiber_data_t __user fiber_data, pid_t tid){
	fiber_t *fiber;
	thread_t *thread;
	process_t *process;
	
	process = find_process(current->tgid);
	if (process == NULL) return -1;
	
	thread = find_thread(tid, process);
	if (thread == NULL) return -1;
	
	fiber = kmalloc(sizeof(fiber_t), GFP_KERNEL);
	if (!fiber){
		printk("Not enough memory to allocate fiber instance", KBUILD_MODNAME);
		return -1;
	}
	
	//Copy values into fiber struct
	fiber->lock = __SPIN_LOCK_UNLOCKED(lock);
	fiber->attached_thread = NULL;
	fiber->parent = process;
	memset(fiber->FLS_data, 0, sizeof(long long) * MAX_FLS);
	bitmap_zero(fiber->fls_bitmap, MAX_FLS);
	memcpy(&(fiber->regs), task_pt_regs(current), sizeof(struct pt_regs));
	fiber->fid = atomic64_inc_return(&process->last_fiber_id); //new fiber id
	hash_add_rcu(process->fibers, &(fiber->node), fiber->fid); //add the fiber to the hash
	//Setup the new stack
	fiber->stack_base = stack_base;
	fiber->stack_size = stack_size;
	fiber->regs.sp = (long) (fiber->stack_base)+(fiber->stack_size)-8;
	fiber->regs.bp = fiber->regs.sp;
	
	//set routine pointer and parameters in the proper fields
	fiber->regs.ip = (long) routine_pointer;
	fiber->regs.di = (long) fiber_data;
	
	//Statistics
	fiber->stats.start_address = (void*) routine_pointer;
	printk("Fiber created with stack_base %#016x, and ip %#016x", fiber->stack_base, fiber->regs.ip);
	fiber->stats.creator_thread = current->pid;
	fiber->stats.activation_counter = 1;
	atomic64_set(&(fiber->stats.failed_counter), 0);
	copy_fxregs_to_kernel(&(fiber->fpu));
	fiber->stats.total_time = 0;
	snprintf(fiber->name, 128, "%d", fiber->fid);
	
	
	printk("Fiber %d created", fiber->fid);
	return fiber->fid;
	
}

int SwitchToFiber(pid_t fid, pid_t tid){

	process_t *process;
	thread_t *thread;
	fiber_t *next_fiber, *prev_fiber;
	struct pt_regs *curr_regs;
	struct fpu *prev_fpu, *next_fpu;
	struct fxregs_state *next_fx;
	
	process = find_process(current->tgid);
	if (process == NULL) return FAILURE;
	
	thread = find_thread(tid, process);
	if (thread == NULL) return FAILURE;
	
	next_fiber = find_fiber(fid, process);
	if (next_fiber == NULL) return FAILURE;
	if (!spin_trylock(&next_fiber->lock) || next_fiber->attached_thread != NULL){
		atomic64_inc(&(next_fiber->stats.failed_counter));
		return FAILURE;
	}	
	next_fiber->attached_thread = thread;
	next_fiber->stats.activation_counter++;
	
	prev_fiber = (fiber_t*)thread->current_fiber;
	thread->current_fiber = next_fiber;
	
	curr_regs = task_pt_regs(current);
	memcpy(&(prev_fiber->regs), curr_regs, sizeof(struct pt_regs));
	//save FPU
	prev_fpu = &(prev_fiber->fpu);
	copy_fxregs_to_kernel(prev_fpu);
	
	//remove prev_fiber from thread
	prev_fiber->attached_thread = NULL;
	prev_fiber->stats.total_time += current->utime;
	spin_unlock(&prev_fiber->lock);
	printk("Passing from fiber %d to fiber %d", prev_fiber->fid, next_fiber->fid);
	//change cpu context
	memcpy(curr_regs, &(next_fiber->regs), sizeof(struct pt_regs));
	//restore next FPU
	next_fpu = &(next_fiber->fpu);
	next_fx = &(next_fpu->state.fxsave);
	copy_kernel_to_fxregs(next_fx);
	return 0; 
}

long FlsAlloc(pid_t tid){
	process_t *process;
	thread_t *thread;
	fiber_t* fiber;
	long i;
	process = find_process(current->tgid);
	if (process == NULL){
		return -1;
	}
	
	thread = find_thread(tid, process);
	if (thread == NULL){
		return -1;
	}
	
	fiber = (fiber_t*) thread->current_fiber;
	
	//Look for space 
	i = find_first_zero_bit(fiber->fls_bitmap, MAX_FLS);
	printk("First zero at %ld", i);
	if (i == MAX_FLS){
			printk("No more space to allocate memory");
			return -1;
	}
	change_bit(i, fiber->fls_bitmap);
	return i;
}

bool FlsFree(long i, pid_t tid){
	process_t *process;
	thread_t *thread;
	fiber_t* fiber;
	
	process = find_process(current->tgid);
	if (process == NULL){
		return false;
	}
	
	thread = find_thread(tid, process);
	if (thread == NULL){
		return false;
	}
	
	fiber = (fiber_t*) thread->current_fiber;
	
	if ( i > MAX_FLS || i < 0) return false;
	
	if (test_bit(i, fiber->fls_bitmap)==0) return false;
	
	change_bit(i, fiber->fls_bitmap);
	
	return true;
}
long long FlsGetValue(long i, pid_t tid){
	process_t *process;
	thread_t *thread;
	fiber_t* fiber;
	
	process = find_process(current->tgid);
	if (process == NULL){
		return -1;
	}
	
	thread = find_thread(tid, process);
	if (thread == NULL){
		return -1;
	}
	
	fiber = (fiber_t*) thread->current_fiber;
	
	if ( i > MAX_FLS || i < 0) return -1;
	
	if (test_bit(i, fiber->fls_bitmap)==0) return -1;
	
	return fiber->FLS_data[i];
}

void FlsSetValue(long i, long long value, pid_t tid){
	process_t *process;
	thread_t *thread;
	fiber_t* fiber;
	
	process = find_process(current->tgid);
	if (process == NULL)
		return;
	
	thread = find_thread(tid, process);
	if (thread == NULL)
		return;
	
	fiber = (fiber_t*) thread->current_fiber;
	
	if ( i > MAX_FLS || i < 0) return;
	
	if (test_bit(i, fiber->fls_bitmap)==0) return;
	
	fiber->FLS_data[i] = value;
}
	
int cleanup_memory(void){
		process_t *process;
		thread_t *thread;
		fiber_t *fiber;
		int i;
		
		process = find_process(current->tgid);
		if (process == NULL)
			return 0;
			
		hash_for_each_rcu(process->threads, i, thread, node){
				kfree(thread);
		}
		
		hash_for_each_rcu(process->fibers, i, fiber, node){
				kfree(fiber);
		}
		
		hash_del_rcu(&(process->node));
		kfree(process);
		printk("Process with pid %d successfully deleted", current->tgid);
		return 0;
		
}

/*
 * Init and cleanup of fibers module.
 */ 

static int fiber_init(void){
	device_init();
	register_probe_exit();
	register_kret_proc_dir();
	printk("fiber installed", KBUILD_MODNAME);

	return 0;
}

static void __exit fiber_exit(void){
	printk("Cleaning up fiber interface", KBUILD_MODNAME);
	unregister_kret_proc_dir();
	unregister_probe_exit();
	device_unregister_connection();
}	

module_init(fiber_init);
module_exit(fiber_exit);
