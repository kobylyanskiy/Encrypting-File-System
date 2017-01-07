obj-m := encrypt_fs.o

encrypt_fs-objs := module.o

all: ko main

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

main_SOURCES: 
	main.c simple.h

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm main
