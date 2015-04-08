obj-m += chardev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o tree tree.c
	sudo insmod ./chardev.ko
	sudo mknod /dev/chardev c 77 0

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm tree
	sudo rmmod chardev
	sudo rm /dev/chardev
