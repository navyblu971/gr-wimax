obj-m += ethWimax.o

all:
	make -C /lib/modules/3.16.0-4-686-pae/build M=$(PWD) modules

clean:
	make -C /lib/modules/3.16.0-4-686-pae/build M=$(PWD) clean
