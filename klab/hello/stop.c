#include <linux/kernel.h>
#include <linux/module.h>

void cleanup_module() {
  printk(KERN_INFO "Short if the life of a kernel module\n");
}
