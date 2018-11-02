#include "device.h"
#include "fibers.h"
int Major = 0;
char buf_message[MSG_LEN] = "";

int device_init(void){
	Major = register_chrdev(0, DEVICE_NAME, &fops) ;
	printk("Major has this value: %d", Major);
	if (Major < 0){
			printk("Error in creation of the char device");
			return Major;
	}
	fiber_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(fiber_class)){
			unregister_chrdev(Major, DEVICE_NAME);
			printk(KERN_ALERT "ERROR in creation of device class");
			return PTR_ERR(fiber_class);
	}
	fiber_device = device_create(fiber_class, NULL, MKDEV(Major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(fiber_device)){
		class_destroy(fiber_class);
		unregister_chrdev(Major, DEVICE_NAME);
		printk(KERN_ALERT "ERROR in creation of device");
		return PTR_ERR(fiber_device);
	}
	print_commands();
	return 0;
}

static ssize_t fiber_read(struct file *f, char __user *buf, size_t len, loff_t *off){
	int i;
	if (*off > strnlen(buf_message, 256)) return 0;
	i = min_t(size_t, len, strnlen(buf_message, 256));
	if (copy_to_user(buf, (buf_message+*off), i)) {
		return -EFAULT;
	}
	*off += i;
	return i;
}

int device_unregister_connection(void){
	device_destroy(fiber_class, MKDEV(Major,0));
	class_unregister(fiber_class);
	class_destroy(fiber_class);
	unregister_chrdev(Major, DEVICE_NAME);
	return 0;
}

int fiber_open(struct inode* inode, struct file *file){
	if (!try_module_get(THIS_MODULE))
		return -1;
	return 0;
}

int fiber_release(struct inode *inode, struct file *file){
	cleanup_memory();
	module_put(THIS_MODULE);
	return 0;
}

long fiber_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
	pid_t tid = current->tgid;
	long ret = 0;
	arguments_fiber arg_fiber;
	if (ioctl_num == CONVERT_THREAD){
			ret = (long) ConvertThreadToFiber(tid);
			return ret;
	}
	else if (ioctl_num ==  CREATE_FIBER){
			get_arguments(arg_fiber, ioctl_param);
			ret = (long) CreateFiber(arg_fiber.stack_base, arg_fiber.stack_size, arg_fiber.routine_pointer, arg_fiber.function_arguments, tid);
			return ret;
	}
	else if (ioctl_num == SWITCH_FIBER){
			get_arguments(arg_fiber, ioctl_param);
			ret = (long) SwitchToFiber(arg_fiber.fid, tid);
			return ret;
	}
	else if (ioctl_num == FLS_ALLOC){
			ret = (long) FlsAlloc(tid);
			return ret;
	}
	else if (ioctl_num == FLS_FREE){
			get_arguments(arg_fiber, ioctl_param);
			ret = (long) FlsFree(arg_fiber.index, tid);
			return ret;
	}
	else if (ioctl_num ==  FLS_GET){
			get_arguments(arg_fiber, ioctl_param);
			ret = FlsGetValue(arg_fiber.index, tid);
			if (copy_to_user((void*)arg_fiber.buffer, &ret, sizeof(long long))){
				return -1;
			}
			return 0;
	}
	else if (ioctl_num ==  FLS_SET){
			get_arguments(arg_fiber, ioctl_param);
			FlsSetValue(arg_fiber.index, arg_fiber.buffer, tid);
			return 0;
	}
	else {
			printk("Command not recognized");
			return -EINVAL;
		
	}
}
