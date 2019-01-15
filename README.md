How to cross compile linux kernel 4.3 for cubox 1st gen

* build Dockerfile in /docker using docker build . or use dowhiletrue/cross-compiler-cubox from dockerhub
* docker run -it -v "$(pwd):/work" --user $(id -u):$(id -g) dowhiletrue/cross-compiler-cubox  bash -c "cd /work && ./cross-compile-cubox-kernel-and-modules.sh"
* scp arch/arm/boot/uImage cubox@cubox:
* scp -r 4.3.0-rc1/lib/modules/4.3.0-rc1 cubox@cubox:
* ssh cubox@cubox
* su
* mv /home/cubox/4.3.0-rc1 /lib/modules/
* mv /home/cubox/uImage /boot/
* maybe add boot.scr according to howto.html as well


How to cross compile rtl8188eu usb wlan

* git clone https://github.com/lwfinger/rtl8188eu.git
* make sure that kernel source is located in the same base directory as rtl8188eu (siblings)
* docker run -it -v "$(pwd):/work" --user $(id -u):$(id -g) dowhiletrue/cross-compiler-cubox  bash -c "cd /work/rtl8188eu && make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KSRC=../linux/ all"
* scp rtl8188eu/rtl8188eufw.bin  cubox@cubox:
* scp rtl8188eu/8188eu.ko  cubox@192.168.0.96:
* ssh cubox@cubox
* su
* install 8188eu.ko /lib/modules/4.3.0-rc1/kernel/drivers/net/wireless
* cp rtl8188eufw.bin /lib/firmware/
* mkdir -p /lib/firmware/rtlwifi
* cp rtl8188eufw.bin /lib/firmware/rtlwifi/.
* depmod -a "$(uname -r)"
* modinfo 8188eu
* modprobe -v 8188eu

