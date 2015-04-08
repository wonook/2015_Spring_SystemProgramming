obj-m += chardev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo mknod char_dev c 100 0
	sudo insmod chardev.ko
	gcc -o ioctl ioctl.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm ioctl
	sudo rmmod chardev
	sudo rm char_dev
