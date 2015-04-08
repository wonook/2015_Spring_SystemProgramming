/*
 * hello-2.c - module_init and module_exit macros.
 * preferred over using init_module() and cleanup_module().
 * */

#include <linux/module.h> //neede by all modules
#include <linux/kernel.h> //for KERN_INFO
#include <linux/init.h> //for macros

static int __init hello_2_init(void) {
  printk(KERN_INFO "<2> Hello, m world 2\n");
  return 0;
}

static void __exit hello_2_exit(void) {
  printk(KERN_INFO "Goodbye, m world 2\n");
}

module_init(hello_2_init);
module_exit(hello_2_exit);
