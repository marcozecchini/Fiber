#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/percpu-defs.h>
#include "fibers.h"

extern struct hlist_head processes;
extern thread_t* find_thread(pid_t pid, process_t *process);
extern fiber_t *find_fiber(pid_t fid, process_t *process);
extern process_t *find_process(pid_t pid);
extern void do_exit(long);

struct kprobe kp_exit;

//To register and unregister kprobes
int register_probe_exit(void);
int unregister_probe_exit(void);

// Handler
int exit_cleanup(struct kprobe *k, struct pt_regs *r);
