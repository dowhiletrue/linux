set -e
export ARCH=arm 
export CROSS_COMPILE=arm-linux-gnueabihf- 
export INSTALL_MOD_PATH="$(pwd)/4.3.0-rc1"
make clean
make olddefconfig
make zImage
make dtbs
cd arch/arm/boot
cat zImage dts/dove-cubox.dtb > zImage.cubox
mkimage -A arm -O linux -C none  -T kernel -a 0x00008000 -e 0x00008000 -n 'Linux-cubox' -d zImage.cubox uImage
cd -
make modules
make modules_install
depmod -b "4.3.0-rc1" -F System.map 4.3.0-rc1
