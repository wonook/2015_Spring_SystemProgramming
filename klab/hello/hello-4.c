/* 
 * __init, __exit, AND __initdata
 */

#include <linux/module.h> //needed by all modules
#include <linux/kernel.h> //needed for KERN_INFO
#include <linux/init.h> //needed for macros
#define DRIVER_AUTHOR "Wonook Song <wsong0512@gmail.com>"
#define DRIVER_DESC "A sample driver"

static int hello3_data __initdata = 4;

static int __init hello_4_init(void) {
  printk(KERN_INFO "Hello, m world %d\n", hello3_data);
  return 0;
}

static void __exit hello_4_exit(void) {
  printk(KERN_INFO "Goodbye, m world 4\n");
}

module_init(hello_4_init);
module_exit(hello_4_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
/*  
 * This module uses /dev/testdevice.  The MODULE_SUPPORTED_DEVICE macro might
 * be used in the future to help automatic configuration of modules, but is 
 * currently unused other than for documentation purposes.
 * */
MODULE_SUPPORTED_DEVICE("testdevice");
