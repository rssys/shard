git clone https://github.com/qemu/qemu
cd qemu
git checkout 9bb68d34dda9be60335e73e65c8fb61bca035362
cp ../*.c accel/kvm/.
cp ../*.h accel/kvm/.

mkdir build
cd build

../configure --enable-curses --enable-gtk --python=python2.7 --target-list=x86_64-softmmu
make

cp ../../run_qemu x86_64-softmmu/.
