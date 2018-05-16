obj-m += pack_handler.o

all:
	make -C /home/ibarantseva/Downloads/linux-4.14/ M=$(PWD) modules

clean:
	make -C /home/ibarantseva/Downloads/linux-4.14/ M=$(PWD) clean
