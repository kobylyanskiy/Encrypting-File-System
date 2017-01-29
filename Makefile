obj-m := encrypt_fs.o

encrypt_fs-objs := module.o

all: ko mkfs efsck encrypt

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

encrypt:
	gcc -I/usr/lib/x86_64-linux-gnu -o encrypt main.c /usr/lib/x86_64-linux-gnu/libcrypto.a

mkfs:
	gcc -o mkfs mkfs.c

efsck:
	gcc -o efsck efsck.c


main_SOURCES: 
	mkfs.c simple.h

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm mkfs encrypt
