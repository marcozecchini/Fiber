#include "subfolder_proc.h"

int register_kret_proc_dir(void){
		readdir = (proc_pident_readdir_t)kallsyms_lookup_name("proc_pident_readdir");
		lookup = (proc_pident_lookup_t)kallsyms_lookup_name("proc_pident_lookup");
		setattr = (proc_setattr_t)kallsyms_lookup_name("proc_setattr");
		getattr = (pid_getattr_t)kallsyms_lookup_name("pid_getattr");
		
		readdir_krp.entry_handler = entry_proc_readdir;
		readdir_krp.handler = proc_readdir;
		readdir_krp.data_size = sizeof(tgid_dir_t);
		readdir_krp.kp.symbol_name = "proc_tgid_base_readdir";
		
		lookup_krp.entry_handler = entry_proc_lookup;
		lookup_krp.handler = proc_lookup;
		lookup_krp.data_size = sizeof(tgid_lookup_t);
		lookup_krp.kp.symbol_name = "proc_tgid_base_lookup";
		
		register_kretprobe(&readdir_krp);
		register_kretprobe(&lookup_krp);
		
		inode_dir_ops.getattr = getattr;
		inode_dir_ops.setattr = setattr;
		
		return 0;
}

int unregister_kret_proc_dir(void){
	unregister_kretprobe(&readdir_krp);
	unregister_kretprobe(&lookup_krp);
	return 0;
}

int entry_proc_readdir(struct kretprobe_instance *k, struct pt_regs *r){
			tgid_dir_t data;
			data.file = (struct file *)r->di;
			data.cxt = (struct dir_context*) r->si;
			
			memcpy(k->data, &data, sizeof(tgid_dir_t));
			return 0;
}
int proc_readdir(struct kretprobe_instance *k, struct pt_regs *r){
	tgid_dir_t *data = (tgid_dir_t*)(k->data);
	process_t *process;
	unsigned long flags;
	unsigned int pos; 
	
	struct task_struct *task = get_pid_task(proc_pid(file_inode(data->file)), PIDTYPE_PID);
	if (number_entries == 0){
			spin_lock_irqsave(&check_number_entries, flags);
			number_entries = data->cxt->pos;
			spin_unlock_irqrestore(&check_number_entries, flags);
	}
	
	process = find_process(task->tgid);
	if (process == NULL) return 0;
	
	pos = number_entries;
	readdir(data->file, data->cxt, fiber_subfolder - (pos-2), pos-1);
	return 0;
}

int entry_proc_lookup(struct kretprobe_instance *k, struct pt_regs *r){
	struct inode *inode = (struct inode *)r->di;
		struct dentry *dentry = (struct dentry *)r->si;
		
		tgid_lookup_t data;
		data.inode = inode;
		data.dentry = dentry;
		memcpy(k->data, &data, sizeof(tgid_lookup_t));
		return 0;
}

int proc_lookup(struct kretprobe_instance *k, struct pt_regs *r){
	tgid_lookup_t *data = (tgid_lookup_t*)(k->data);
	process_t *process;
	
	unsigned long pos;
	struct task_struct *task = get_proc_task(data->inode);
	
	process = find_process(task->tgid);
	if (process == NULL) return -1;
	
	if (number_entries == 0) return 0;
	
	pos = number_entries;
	lookup(data->inode, data->dentry, fiber_subfolder - (pos-2), pos-1);
	return 0;
}

/*
 * Proc operations
 */
ssize_t proc_fiber_read(struct file *f, char __user *c, size_t len, loff_t *off){
	char proc_string[512] = "";
	process_t *process;
	fiber_t *fiber;
	size_t i;
	unsigned long fiber_id;
	
	struct task_struct *task = get_pid_task(proc_pid(file_inode(f)), PIDTYPE_PID);
	if (task == NULL) return -1;
	
	process = find_process(task->tgid);
	if (process == NULL) return -1;
	
	int ret = kstrtoul(f->f_path.dentry->d_name.name, 10, &fiber_id);
	if (ret) return 0;
	
	fiber = find_fiber(fiber_id, process);
	if (fiber == NULL) return -1;
	
	snprintf(proc_string, 512, "Currently running: %s\nStart address: %#010x\nCreator thread: %d\nNumber of current activations: %lu\nNumber of failed activation: %lld\nTotal execution time: %lu\n",
				(fiber->attached_thread == NULL) ? "no" : "yes", (unsigned long) fiber->start_address, fiber->creator_thread, fiber->activation_counter, atomic64_read(&(fiber->failed_counter)), fiber->total_time);
	
	if (*off >= strnlen(proc_string, 512)) return 0;
	
	i = min_t(size_t, len, strnlen(proc_string, 512));
	
	if (copy_to_user(c, proc_string, i)) return -1;
	
	*off += i;
	return i;
}

int proc_fiber_base_readdir(struct file *f, struct dir_context *cxt){
	process_t *process;
	struct task_struct *task = get_pid_task(proc_pid(file_inode(f)), PIDTYPE_PID);
	unsigned long ents;
	struct pid_entry *fiber_fields;
	int i, ret, counter = 0;
	fiber_t *fiber;
	
	if (task == NULL || f == NULL || cxt == NULL)
		return -ENOENT;

	process = find_process(task->tgid);
	if (process == NULL)
		return 0;
	ents = atomic64_read(&(process->last_fiber_id));
	fiber_fields = kmalloc(ents * sizeof(struct pid_entry), GFP_KERNEL);
	memset(fiber_fields, 0, ents * sizeof(struct pid_entry));
	hash_for_each_rcu(process->fibers, i, fiber, node){
		if (fiber==NULL) break;
		fiber_fields[counter].fop = &file_ops;
		fiber_fields[counter].name = fiber->name;
		fiber_fields[counter].len = strlen(fiber->name);
		fiber_fields[counter].iop = NULL;
		
		fiber_fields[counter].mode = (S_IFREG|(S_IRUGO));
		counter++;
	
	}
	ret = readdir(f, cxt, fiber_fields, ents);
	kfree(fiber_fields);
	return ret; 
}

struct dentry * proc_fiber_base_lookup(struct inode *i, struct dentry *d, unsigned int ui){
	process_t *process;
	struct task_struct *task = get_pid_task(proc_pid(i), PIDTYPE_PID);
	unsigned long ents;
	struct pid_entry *fiber_fields;
	int index, counter = 0;
	fiber_t *fiber;
	struct dentry *final_dentry;
	
	if (task == NULL || i == NULL || d == NULL)
		return NULL;
	
	process = find_process(task->tgid);
	if (process == NULL)
		return 0;
	
	ents = atomic64_read(&(process->last_fiber_id));
	fiber_fields = kmalloc(ents * sizeof(struct pid_entry), GFP_KERNEL);
	memset(fiber_fields, 0, ents * sizeof(struct pid_entry));
	hash_for_each_rcu(process->fibers, index, fiber, node){
		if (fiber==NULL) break;
		
		fiber_fields[counter].name = fiber->name;
		fiber_fields[counter].len = strlen(fiber->name);
		fiber_fields[counter].iop = NULL;
		fiber_fields[counter].fop = &file_ops;
		fiber_fields[counter].mode = (S_IFREG|(S_IRUGO));
		counter++;
	
	}
	
	final_dentry = lookup(i, d, fiber_fields, ents);
	kfree(fiber_fields);
	return final_dentry;
}
