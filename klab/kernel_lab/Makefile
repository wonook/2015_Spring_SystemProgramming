obj-m += msrdrv.o
obj-m += chardev.o

all: modules tree PMU

modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod ./chardev.ko
	sudo insmod ./msrdrv.ko
	sudo mknod /dev/chardev c 77 0
	sudo mknod /dev/msrdrv c 33 0

tree:
	gcc -o tree tree.c

PMU:
	gcc -I. -O2 -o PMU PMU.c

run1:
	sudo ./tree

run2:
	sudo ./PMU

clean:
	rm -f tree PMU *.ko *.o *.so *.order *.symvers *.mod.*
	sudo rmmod chardev
	sudo rmmod msrdrv
	sudo rm /dev/chardev
	sudo rm /dev/msrdrv
