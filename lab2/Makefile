obj-m :=my_blk_dev.o
all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
do:
	sudo insmod my_blk_dev.ko
rm:
	sudo rmmod my_blk_dev.ko
