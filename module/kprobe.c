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

int register_kprobe_time(void){
		kp_time.handler = fiber_time;
		kp_time.kp.symbol_name = "finish_task_switch";
		register_kretprobe(&kp_time);
}

int unregister_kprobe_time(void){
	unregister_kretprobe(&kp_time);
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

DEFINE_PER_CPU(struct task_struct *, prev_task) = NULL;

int fiber_time(struct kretprobe_instance *k, struct pt_regs *r) {
	
	process_t *process;
	thread_t *thread;
	fiber_t *fiber;
	struct task_struct *temp_prev;
	
	temp_prev = get_cpu_var(prev_task);
	if (temp_prev == NULL){
		put_cpu_var(prev_task);
		end_not_our_fiber(prev_task, temp_task);
		return 0;
	}
	
	process = find_process(temp_prev->tgid);
	if (process == NULL){
			put_cpu_var(prev_task);
			end_not_our_fiber(prev_task, temp_task);
			return 0;
	}
	
	thread = find_thread(temp_prev->pid, process);
	if (thread == NULL){
			put_cpu_var(prev_task);
			end_not_our_fiber(prev_task, temp_task);
			return 0;
	}
	
	fiber = (fiber_t*)thread->current_fiber;
	if (fiber == NULL){
			put_cpu_var(prev_task);
			end_not_our_fiber(prev_task, temp_task);
			return 0;
	}
	
	fiber->total_time += temp_prev->utime;
	put_cpu_var(prev_task);
	end_not_our_fiber(prev_task, temp_task);
	return 0;
	
}
