obj-m := myled.o

myled.ko: myled.c
	make -C /usr/src/linux-headers-$(shell uname -r) M=$(PWD) V=1 modules
clean:
	make -C /usr/src/linux-headers-$(shell uname -r) M=$(PWD) V=1 clean
