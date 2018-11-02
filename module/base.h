#define NOD(NAME, MODE, IOP, FOP, OP) {			\
	.name = (NAME),					\
	.len  = sizeof(NAME) - 1,			\
	.mode = MODE,					\
	.iop  = IOP,					\
	.fop  = FOP,					\
	.op   = OP,					\
}

#define DIR(NAME, MODE, iops, fops)	\
	NOD(NAME, (S_IFDIR|(MODE)), &iops, &fops, {} )

#define REG(NAME, MODE, fops)				\
	NOD(NAME, (S_IFREG|(MODE)), NULL, &fops, {})


union proc_op {
	int (*proc_get_link)(struct dentry *, struct path *);
	int (*proc_show)(struct seq_file *m,
		struct pid_namespace *ns, struct pid *pid,
		struct task_struct *task);
};


struct pid_entry {
	const char *name;
	int len;
	umode_t mode;
	const struct inode_operations *iop;
	const struct file_operations *fop;
	union proc_op op;
};

struct proc_inode {
	struct pid *pid;
	unsigned int fd;
	union proc_op op;
	struct proc_dir_entry *pde;
	struct ctl_table_header *sysctl;
	struct ctl_table *sysctl_entry;
	const struct proc_ns_operations *ns_ops;
	struct inode vfs_inode;
};

static inline struct proc_inode *PROC_I(const struct inode *inode)
{
	return container_of(inode, struct proc_inode, vfs_inode);
}

static inline struct pid *proc_pid(struct inode *inode)
{
	return PROC_I(inode)->pid;
}

static inline struct task_struct *get_proc_task(struct inode *inode)
{
	return get_pid_task(proc_pid(inode), PIDTYPE_PID);
}


typedef int (*proc_pident_readdir_t)(struct file *file, struct dir_context *ctx, const struct pid_entry *ents, unsigned int nents);
typedef struct dentry * (*proc_pident_lookup_t)(struct inode *dir, struct dentry *dentry, const struct pid_entry *ents, unsigned int nents);
typedef int (*pid_getattr_t)(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat);
typedef int (*proc_setattr_t)(struct dentry *dentry, struct iattr *attr);
