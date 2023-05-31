ifneq  ($(KERNELRELEASE),)
obj-m:=pictest_driver.o
else
KDIR := /home/yu/linux-6.1.28
PWD:=$(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.ko *.o *.symvers *.cmd *.cmd.o
endif