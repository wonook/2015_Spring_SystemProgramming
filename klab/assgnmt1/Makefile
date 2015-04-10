obj-m += chardev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo mknod /dev/char_dev c 58 0
	sudo insmod chardev.ko
	gcc -o pptree pptree.c

run1:
	sudo ./pptree

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rmmod chardev
	sudo rm /dev/char_dev
	rm pptree
