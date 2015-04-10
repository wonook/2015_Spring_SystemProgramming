obj-m += msrdrv.o

all: msrdrv cpu_pmc_ver leaf0ah msrtest

msrdrv:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod ./msrdrv.ko
	sudo mknod /dev/msrdrv c 33 0

cpu_pmc_ver:
	gcc -o cpu_pmc_ver cpu_pmc_ver.c

leaf0ah: leaf0ah.o
	ld --dynamic-linker=/lib64/ld-linux-x86-64.so.2 -o leaf0ah -lc leaf0ah.o

leaf0ah.o: leaf0ah.s
	as -g -o leaf0ah.o leaf0ah.s

msrtest:
	gcc -g -I. -O2 -o msrtest msrtest.c

run2:
	sudo ./msrtest

clean:
	rm -f leaf0ah cpu_pmc_ver msrtest *.ko *.o *.so *.order *.symvers
	rm *.mod.* .msrdrv.*
	rm -rf .tmp_versions
	sudo rmmod msrdrv
	sudo rm /dev/msrdrv
