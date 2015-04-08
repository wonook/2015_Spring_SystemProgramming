obj-m += msrdrv.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod ./msrdrv.ko
	sudo mknod /dev/msrdrv c 33 0

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rmmod msrdrv
	sudo rm /dev/msrdrv
