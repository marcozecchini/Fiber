#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>

#include "fibers.h"
#include "base.h"

extern struct hlist_head processes;
extern thread_t* find_thread(pid_t pid, process_t *process);
extern fiber_t *find_fiber(pid_t fid, process_t *process);
extern process_t *find_process(pid_t pid);


/*
 * Proc operations
 */
ssize_t proc_fiber_read(struct file *f, char __user *c, size_t len, loff_t *off);
int proc_fiber_base_readdir(struct file *f, struct dir_context *cxt);
struct dentry * proc_fiber_base_lookup(struct inode *i, struct dentry *d, unsigned int ui);

/*
 * Pointer to functions catched with kallsyms
 */
proc_pident_readdir_t readdir;
proc_pident_lookup_t lookup;
pid_getattr_t getattr;
proc_setattr_t setattr;

/*
 * Kretprobes then Handlers and entry handlers
 */
 
int register_kret_proc_dir(void);
int unregister_kret_proc_dir(void);
struct kretprobe readdir_krp;
struct kretprobe lookup_krp;
 
int entry_proc_readdir(struct kretprobe_instance *k, struct pt_regs *r);
int proc_readdir(struct kretprobe_instance *k, struct pt_regs *r);
int entry_proc_lookup(struct kretprobe_instance *k, struct pt_regs *r);
int proc_lookup(struct kretprobe_instance *k, struct pt_regs *r);

/*
 * Kretprobe data struct 
 */
typedef struct {
	struct file *file;
	struct dir_context *cxt;
} tgid_dir_t;

typedef struct {
	struct dentry *dentry;
	struct inode *inode;
} tgid_lookup_t;

/*
 * spinlock to manage number of entries
 */
spinlock_t check_number_entries = __SPIN_LOCK_UNLOCKED(check_number_entries);
int number_entries = 0;


/*
 * File operations for proc_file, proc_dir and new pid_entry definition
 * to be added as subfolder
 */
struct file_operations dir_ops ={
	.read = generic_read_dir,
	.iterate_shared = proc_fiber_base_readdir,
	.llseek = generic_file_llseek,
};

struct inode_operations inode_dir_ops = {
	.lookup = proc_fiber_base_lookup,
};

struct file_operations file_ops = {
	.read = proc_fiber_read,
};

const struct pid_entry fiber_subfolder[] = {
	DIR("fibers", S_IRUGO | S_IXUGO, inode_dir_ops, dir_ops),
};
