#include <linux/kernel.h>
#include <linux/module.h>

int init_module(void){
	printk(KERN_INFO "Hellom world - this is the kernel speaking\n");
	return 0;
}
