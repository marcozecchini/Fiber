#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/string.h>
/*
 * API to interact with fibers
 */
#define DEVICE_NAME "fiber_dev"
#define CLASS_NAME "fiber_class"
#define MSG_LEN 256
/*
 * To deal with IOCTL function
 */
#define CONVERT_THREAD _IO(Major, 0)
#define CREATE_FIBER _IO(Major, 1)
#define SWITCH_FIBER _IO(Major, 2)
#define FLS_ALLOC _IO(Major, 3)
#define FLS_FREE _IO(Major, 4)
#define FLS_GET _IO(Major, 5)
#define FLS_SET _IO(Major, 6)
//Get fiber_arguments
#define get_arguments(fiber_arg, arg) do {\
	if (!access_ok(VERIFY_READ, arg, sizeof(arguments_fiber))){ \
		printk("Bad pointer to IOCTL");							\
		return -1;												\
	}															\
	if (copy_from_user(&fiber_arg, (void*) arg, sizeof(arguments_fiber))){\
		printk("Error in coping from user");					\
		return -1;												\
	}															\
} while(0)
	
#define print_commands() snprintf(buf_message, MSG_LEN, "%ld\n%ld\n%ld\n%ld\n%ld\n%ld\n%ld\n", CONVERT_THREAD, CREATE_FIBER, SWITCH_FIBER, FLS_ALLOC, FLS_FREE, FLS_GET, FLS_SET)
	
static struct class* fiber_class = NULL;
static struct device* fiber_device = NULL;

extern char buf_message[MSG_LEN];
extern int Major;
extern int cleanup_memory(void);
int device_init(void);

int device_unregister_connection(void);

int fiber_open(struct inode* inode, struct file *file);

static ssize_t fiber_read(struct file *f, char __user *buf, size_t len, loff_t *off);

int fiber_release(struct inode *inode, struct file *file);

long fiber_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

static struct file_operations fops = {
		.owner = THIS_MODULE,
		.open = fiber_open,
		.release = fiber_release, 
		.read = fiber_read,
		.unlocked_ioctl = fiber_ioctl
};
