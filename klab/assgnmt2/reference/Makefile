obj-m += msrdrv.o

all: msrdrv cpu_pmc_ver leaf0ah 
	#PMU

msrdrv:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod ./msrdrv.ko
	sudo mknod /dev/msrdrv c 223 0

cpu_pmc_ver:
	gcc -o cpu_pmc_ver cpu_pmc_ver.c

leaf0ah: leaf0ah.o
	ld --dynamic-linker=/lib64/ld-linux-x86-64.so.2 -o leaf0ah -lc leaf0ah.o

leaf0ah.o: leaf0ah.s
	as -g -o leaf0ah.o leaf0ah.s

#PMU:
	#gcc -I. -O2 -o PMU_prac PMU_prac.c

#run2:
	#sudo ./PMU

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rmmod msrdrv
	sudo rm /dev/msrdrv
	rm cpu_pmc_ver
	rm -f leaf0ah PMU_prac *.ko *.o *.so *.order *.symvers
