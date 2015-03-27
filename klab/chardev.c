#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file*, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "spchardev"
#define BUF_LEN 80

static int Major;
static int Device_Open = 0;

static char msg[BUF_LEN];
static char *msg_Ptr;

static struct file_operaitons fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

void cleanup_module(void){
	int ret = unregister_chardev(Major, DEVICE_NAME);
	if (ret < 0) printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
}

static int device_open(struct inode *inode, struct file *file){
	static int counter = 0;

	if (Device_Open) return -EBUSY;

	Device_Open++;
	sprintf(msg, "I already told you %d times Hello world! \n", counter++);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file){
	Device_Open--;

	module_put(THIS_MODULE);

	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t * offset){
	int bytes_read = 0;
	if (*msg_Ptr == 0) return 0;

	while (length && *msg_Ptr){
		put_user(*(msg_Ptr++), buffer++);

		length--;
		bytes_read++;
	}

	return bytes_read;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off){
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}
