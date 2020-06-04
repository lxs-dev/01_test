KERNELDIR = /home/lxs/liunx/IMX6ULL/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek


all:
	make -C $(KERNELDIR) M=`pwd` modules 
	arm-linux-gnueabihf-gcc app.c -o app
clean:
	make -C $(KERNELDIR) M=`pwd` modules clean
	rm app -R
cp :
	cp  *.ko app   /home/lxs/liunx/nfs/rootfs/myfile/ -f
obj-m	+= platform_sleep.o