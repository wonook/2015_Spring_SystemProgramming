obj-m += chardev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo mknod char_dev c 58 0
	sudo insmod chardev.ko
	gcc -o pptree pptree.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rmmod chardev
	sudo rm char_dev
	rm pptree
