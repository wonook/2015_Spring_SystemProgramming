obj-m += msrdrv.o
obj-m += chardev.o

all: driver pptree msrtest

driver:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod chardev.ko
	sudo insmod msrdrv.ko
	sudo mknod /dev/msrdrv c 243 0
	sudo mknod /dev/char_dev c 158 0

pptree:
	gcc -o pptree pptree.c

msrtest:
	gcc -g -I. -O2 -o msrtest msrtest.c

run1:
	sudo ./pptree

run2:
	sudo ./msrtest

clean:
	rm -f pptree msrtest *.ko *.o *.so *.order *.symvers
	rm *.mod.* .msrdrv.*
	rm -rf .tmp_versions
	sudo rmmod msrdrv
	sudo rmmod chardev
	sudo rm /dev/msrdrv
	sudo rm /dev/char_dev
