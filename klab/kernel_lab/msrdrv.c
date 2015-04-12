/*
 * msrdrv.c - Create a character device
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include "msrdrv.h"
#define SUCCESS 0

/*
 * This funciton is called when a process attempts to open the device file
 */
static int msrdrv_open (struct inode *i, struct file *f){
	// But we don't need this.
	return SUCCESS;
}

/*
 * This function is called when a process attempts to close the device file.
 */
static int msrdrv_release (struct inode *i, struct file *f){
	// But we don't need this.
    	return 0;
}

/*
 * This function is called when a process which has already opened the device file attempts to read from it.
 */
static ssize_t msrdrv_read (struct file *f, char *b, size_t c, loff_t* o){
	// But we don't need this.
    	return 0;
}

/*
 * This function is called when somebody tries to write into our device file.
 */
static ssize_t msrdrv_write (struct file *f, const char *b, size_t c, loff_t *o){
	// But we don't need this.
    	return 0;
}

/*
 * Pre-declared ioctl function.
 */
static long msrdrv_ioctl (struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

dev_t msrdrv_dev;
struct cdev *msrdrv_cdev;

/* 
 * This structure will hold the functions to be called
 * when a process does something to the device we created.
 * Since a pointer to this structure is kept in the devices table,
 * it can't be local to init_module. NULL is for unimplemented functions. 
 */
struct file_operations msrdrv_fops = {
	.owner 		= THIS_MODULE,
	.read 		= msrdrv_read,
	.write 		= msrdrv_write,
	.open 		= msrdrv_open,
	.release 	= msrdrv_release,
	.unlocked_ioctl = msrdrv_ioctl,
	.compat_ioctl   = NULL,
};

static long long read_msr (unsigned int ecx) {
	unsigned int edx = 0, eax = 0;
	unsigned long long result = 0;
	__asm__ __volatile__("rdmsr" : "=a"(eax), "=d"(edx) : "c"(ecx));
	result = eax | (unsigned long long)edx << 0x20;
	printk (KERN_ALERT "Module msrdrv: Read 0x%016llx (0x%08x:0x%08x) from MSR 0x%08x\n", result, edx, eax, ecx);
	return result;
}

static void write_msr (int ecx, unsigned int eax, unsigned int edx) {
	printk (KERN_ALERT "Module msrdrv: Writing 0x%08x:0x%08x to MSR 0x%04x\n", edx, eax, ecx);
	__asm__ __volatile__("wrmsr" : : "c"(ecx), "a"(eax), "d"(edx));
}

static long long read_tsc(void) {
	unsigned eax, edx;
	long long result;
	__asm__ __volatile__("rdtsc" : "=a"(eax), "=d"(edx));
	result = eax | (unsigned long long)edx << 0x20;
	printk(KERN_ALERT "Module msrdrv: Read 0x%016llx (0x%08x:0x%08x) from TSC\n", result, edx, eax);
	return result;
}

/*
 * This function is called whenever a process tries to do an ioctl on our device file.
 * We have three parameter. Firsty, file_desc is passed by user process to *file.
 * Secondly, IOCTL_MSR_CMDS is is passed by user process to ioctl_num.
 * Lastly, pointer pointing the struct MsrInOut's array is passed by user process to ioctl_param.
 */
static long msrdrv_ioctl (struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
	struct MsrInOut *msrops;
	int i;
	if (ioctl_num != IOCTL_MSR_CMDS) return 0;	// Check the ioctl_param
	msrops = (struct MsrInOut*)ioctl_param;
	
	for (i = 0; i <= MSR_VEC_LIMIT ; i++, msrops++) {
		switch (msrops->op) {
		case MSR_NOP:
			printk (KERN_ALERT "Module " DEVICE_NAME ": seen MSR_NOP commnad\n");
			break;
		case MSR_STOP:
			printk (KERN_ALERT "Module " DEVICE_NAME ": seen MSR_STOP command\n");
			goto label_end;
		case MSR_READ:
                        printk (KERN_ALERT "Module " DEVICE_NAME ": seen MSR_READ command\n");
			msrops->value = read_msr(msrops->ecx);
			break;
		case MSR_WRITE:
			printk (KERN_ALERT "Module " DEVICE_NAME ": seen MSR_WRITE commnad\n");
			write_msr (msrops->ecx, msrops->eax, msrops->edx);
			break;
		case MSR_RDTSC:
			printk (KERN_ALERT "Module " DEVICE_NAME ": seen MSR_RDTSC command\n");
			msrops->value = read_tsc();
		default:
			printk (KERN_ALERT "Module " DEVICE_NAME ": Unknown option 0x%x\n", msrops->op);
			return 1;
		}
	}
	label_end:

	return 0;
}

static int msrdrv_init(void) {
	long int val;
	msrdrv_dev = MKDEV (MAJOR_NUM, MINOR_NUM);
	register_chrdev_region (msrdrv_dev, 1, DEVICE_NAME);
	msrdrv_cdev = cdev_alloc();
	msrdrv_cdev->owner = THIS_MODULE;
	msrdrv_cdev->ops = &msrdrv_fops;
	cdev_init (msrdrv_cdev, &msrdrv_fops);
	cdev_add (msrdrv_cdev, msrdrv_dev, 1);
	printk (KERN_ALERT "Module " DEVICE_NAME " loaded\n");
	return 0;
}

static void msrdrv_exit (void) {
	long int val;
	cdev_del (msrdrv_cdev);
	unregister_chrdev_region (msrdrv_dev, 1);
	printk (KERN_ALERT "Module " DEVICE_NAME " unloaded\n");
}

module_init (msrdrv_init);
module_exit (msrdrv_exit);
