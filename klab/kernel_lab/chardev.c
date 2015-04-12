/*
 * chardev.c - Create a character device
 */

#include <linux/kernel.h>	
#include <linux/module.h>	
#include <linux/fs.h>
#include <linux/sched.h>	/* for using task_struct */
#include <asm/uaccess.h>	/* for using put_user */

#include "chardev.h"
#define SUCCESS 0

/* 
 * This function is called when a process attempts to open the device file 
 */
static int device_open (struct inode *inode, struct file *file)
{
    	// But we don't need this.
	return SUCCESS;
}

/*
 * This function is called when a process attempts to close the device file
 */
static int device_release (struct inode *inode, struct file *file)
{
	// But we don't need this
	return SUCCESS;
}

/* 
 * This function is called when a process which has already opened the device file attempts to read from it.
 */
static ssize_t device_read (struct file *file, char __user * buffer, size_t length, loff_t * offset)
{
    	// But we don't need this
	return SUCCESS;
}

/* 
 * This function is called when somebody tries to write into our device file. 
 */
static ssize_t
device_write (struct file *file, const char __user * buffer, size_t length, loff_t * offset)
{
    	// But we don't need this
	return SUCCESS;
}

/*
 * My own struct to pass Process's ID and name.
 */
struct pinfo {
	char pname[16];
	int processid;
};

/*
 * This function is called whenever a process tries to do an ioctl on our device file.
 * We have three parameter. Firsty, file_desc is passed by user process to *file.
 * Secondly, IOCTL_CHR_CMDS is is passed by user process to ioctl_num.
 * Lastly, pointer pointing the struct pinfo's array is passed by user process to ioctl_param.
 */
static long device_ioctl (struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	printk (KERN_ALERT "structing process tree...\n");

	struct pinfo * pinfo_p = ioctl_param;			// Cast ioctl_param to struct array pointer.
	struct task_struct *cur_task = current;			// Make current task's task_struct pointer.
	int i;							// Iterator.

	if (ioctl_num != IOCTL_CHR_CMDS) return 0;		// Check the ioctl param.

	for (;;){
		for (i=0; i<16; i++)				// Copy process name.
			pinfo_p->pname[i] = cur_task->comm[i];

		put_user(cur_task->pid, &(pinfo_p->processid));	// Copy process id.
		if (cur_task->pid == NULL) break;		// If the process is parent, quit.
		cur_task = cur_task->parent;			// Iterating.
		pinfo_p++;
	}
	return SUCCESS;
}

/* 
 * This structure will hold the functions to be called
 * when a process does something to the device we created.
 * Since a pointer to this structure is kept in the devices table,
 * it can't be local to init_module. NULL is for unimplemented functions. 
 */
struct file_operations Fops = {
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
};

/* 
 * Initialize the module - Register the character device 
 */
int init_module()
{
	int ret_val;
	/* 
	 * Register the character device (atleast try) 
	 */
	ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

	/* 
	 * Negative values signify an error 
	 */
	if (ret_val < 0) {
		printk(KERN_ALERT "%s failed with %d\n",
		       "Sorry, registering the character device ", ret_val);
		return ret_val;
	}

	printk(KERN_INFO "%s The major device number is %d.\n",
	       "Registeration is a success", MAJOR_NUM);
	printk(KERN_INFO "If you want to talk to the device driver,\n");
	printk(KERN_INFO "you'll have to create a device file. \n");
	printk(KERN_INFO "We suggest you use:\n");
	printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
	printk(KERN_INFO "The device file name is important, because\n");
	printk(KERN_INFO "the ioctl program assumes that's the\n");
	printk(KERN_INFO "file you'll use.\n");

	return 0;
}

/* 
 * Cleanup - unregister the appropriate file from /proc 
 */
void cleanup_module()
{
	/* 
	 * Unregister the device 
	 */
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}
