#include <linux/module.h>
#include <linux/kernel.h>

// These two modules are always needed in a kernel module.

int init_module(void) {
  printk(KERN_INFO "<1>Hello m world 1.\n");

  return 0;
}

void cleanup_module(void) {
  printk(KERN_INFO "Goodbye m world 1.\n");
}

