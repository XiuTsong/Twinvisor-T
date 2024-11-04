obj-m += reserve.o
obj-m += migrate.o
obj-m += unmap.o

all:
	make ARCH=arm64 CROSS_COMPILE=$(PWD)/../../toolchains/aarch64/bin/aarch64-linux-gnu- -C .. M=$(PWD) modules

clean:
	make ARCH=arm64 CROSS_COMPILE=$(PWD)/../../toolchains/aarch64/bin/aarch64-linux-gnu- -C .. M=$(PWD) clean
