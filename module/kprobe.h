#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/percpu-defs.h>
#include "fibers.h"


#define end_not_our_fiber(prev_task, temp_task) do { \
	temp_prev = get_cpu_var(prev_task);              \
	this_cpu_write(prev_task, current);				 \
	put_cpu_var(prev_task);							 \
} while(0)

extern struct hlist_head processes;
extern thread_t* find_thread(pid_t pid, process_t *process);
extern fiber_t *find_fiber(pid_t fid, process_t *process);
extern process_t *find_process(pid_t pid);
extern void do_exit(long);

struct kprobe kp_exit;
struct kretprobe kp_time;

//To register and unregister kprobes
int register_probe_exit(void);
int unregister_probe_exit(void);
int register_kprobe_time(void);
int unregister_kprobe_time(void);

// Handlers
int exit_cleanup(struct kprobe *k, struct pt_regs *r);
int fiber_time(struct kretprobe_instance *k, struct pt_regs *r);

