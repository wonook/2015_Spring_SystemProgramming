/* 
 * __init, __exit, AND __initdata
 */

#include <linux/module.h> //needed by all modules
#include <linux/kernel.h> //needed for KERN_INFO
#include <linux/init.h> //needed for macros

static int hello3_data __initdata = 3;

static int __init hello_3_init(void) {
  printk(KERN_INFO "Hello, m world %d\n", hello3_data);
  return 0;
}

static void __exit hello_3_exit(void) {
  printk(KERN_INFO "Goodbye, m world 3\n");
}

module_init(hello_3_init);
module_exit(hello_3_exit);
