obj-m += fsplit.o
fsplit-objs := ./src/fsplit.o

KERNEL_DIR = /lib/modules/$(shell uname -r)/build

SRC_DIR = ./src
INT_DIR = ./int
BIN_DIR = ./bin


all: clean setup kernel daemon
	clear

clean:
	rm -rf $(INT_DIR)
	rm -rf $(BIN_DIR)

setup:
	mkdir $(INT_DIR)
	mkdir $(BIN_DIR)

daemon:
	gcc -o $(BIN_DIR)/fsplit_daemon.elf $(SRC_DIR)/fsplit_daemon.c

kernel:
	make -C $(KERNEL_DIR) M=$(shell pwd) modules
	mv -f *.o $(INT_DIR)
	mv -f *.ko $(BIN_DIR)
	rm -rf *.mod.* *.mod *.cmd .module* modules* Module* .*.cmd .tmp* $(SRC_DIR)/*.o $(SRC_DIR)/.*.cmd