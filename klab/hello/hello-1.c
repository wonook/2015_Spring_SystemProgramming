#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void){
    printk(KERN_INFO "Hello module world 1!\n");

    return 0;
}

void cleanup_module(void){
    printk(KERN_INFO "Goodbye module world 1!\n");
}

