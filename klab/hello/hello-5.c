#include <linux/module.h>
#include <linux/moduleparam.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/stat.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lee Sanha");

static short int myshort = 1;
static int myint = 420;
static long int mylong = 9999;
static char *mystring = "sffuck";
static int myintArray[2] = { -1. -1};
static int arr_argc = 0;

module_param(myshort, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myshort, "A short integer");
module_param(myint, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(myint, "An integer");
module_param(mylong, long, S_IRUSR);
MODULE_PARM_DESC(mylong, "A long integer");
module_param(mystring, charp, 0000);
MODULE_PARM_DESC(mystring, "A character string");

module_param_array(myintArray, int, &arr_argc, 0000);
MODULE_PARM_DESC(myintArray, "An array of integers");

static int __init hello_5_init(void){
	int i;
	printk(KERN_INFO "Hello, world 5\n============\n");
	printk(KERN_INFO "Myshort is a short integer: %hd\n", myshort);
	printk(KERN_INFO "myint is an integer: %d\n", myint);
	printk(KERN_INFO "mylong is a long integer: %ld\n", mylong);
	printk(KERN_INFO "mystring is a string: %s\n", mystring);
	for (i = 0; i < (sizeof myintArray / sizeof (int)); i++)
		printk(KERN_INFO "myintArray[%d] = %d\n", i, myintArray[i]);
	printk(KERN_INFO "Got %d arguments for myintArray.\n", arr_argc);
	return 0;
}

static void __exit hello_5_exit(void){
	printk(KERN_INFO "Goodbye, world 5\n");
}

module_init(hello_5_init);
module_exit(hello_5_exit);
