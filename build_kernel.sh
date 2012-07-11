export ARCH=arm

#export CROSS_COMPILE=<Local Android Path>/android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
#export CROSS_COMPILE=/home/user/Android-ICS/GAUDI-PROD3/android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
export CROSS_COMPILE=/opt/toolchains/arm-eabi-4.4.3/bin/arm-eabi-

make u1_na_spr_defconfig
make
