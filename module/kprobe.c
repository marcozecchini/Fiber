#include "kprobe.h"

int register_probe_exit(void){
		kp_exit.pre_handler = exit_cleanup;
		kp_exit.addr = (kprobe_opcode_t*)&(do_exit);
		register_kprobe(&kp_exit);
		return 0;
}

int unregister_probe_exit(void){
	unregister_kprobe(&kp_exit);
	return 0;
}

int exit_cleanup(struct kprobe *k, struct pt_regs *r){
		process_t *process;
		thread_t *thread;
		fiber_t *prev_fiber;
		struct pt_regs *prev_regs;
		struct fpu *prev_fpu;
		long ret;
		
		process = find_process(current->tgid);
		if (process == NULL) return 0;
		
		thread = find_thread(current->pid, process);
		if (thread == NULL) return 0;
		
		ret = atomic64_dec_return(&(thread->parent_process->active_threads));
		if (thread->current_fiber != NULL && ret != 0){
			//save CPU state
			preempt_disable();
			prev_regs = current_pt_regs();
			prev_fiber = (fiber_t*) thread->current_fiber;
			
			//save CPU previous state in fiber struct
			memcpy(&prev_fiber->regs, prev_regs, sizeof(struct pt_regs));
			
			//save FPU regs
			prev_fpu = &(prev_fiber->fpu);
			copy_fxregs_to_kernel(prev_fpu);
			
			prev_fiber->attached_thread = NULL;
			preempt_enable();
			
			spin_unlock(&prev_fiber->lock);
			
		}
		hash_del_rcu(&(thread->node));
		kfree(thread);
		
		if (ret == 0) {
				//This means that this is the last thread
				int i;
				fiber_t *f;
				hash_for_each_rcu(process->fibers, i, f, node){
						if (f == NULL) break;
						kfree(f);
				}
				hash_del_rcu(&(process->node));
				kfree(process);
		}
		return 0;
}
