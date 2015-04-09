obj-m += msrdrv.o

all: msrdrv leaf0ah PMU

msrdrv:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod ./msrdrv.ko
	sudo mknod /dev/msrdrv c 33 0

leaf0ah: leaf0ah.o
	ld --dynamic-linker=/lib64/ld-linux-x86-64.so.2 -o leaf0ah -lc leaf0ah.o

leaf0ah.o: leaf0ah.s
	as -g -o leaf0ah.o leaf0ah.s

PMU:
	gcc -I. -O2 -o PMU PMU.c

run2:
	sudo ./PMU

clean:
	rm -f leaf0ah PMU *.o *.so *.order *.symvers
	sudo rmmod msrdrv
	sudo rm /dev/msrdrv
