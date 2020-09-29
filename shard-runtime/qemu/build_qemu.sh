git clone https://github.com/qemu/qemu
cd qemu
cp ../*.c accel/kvm/.
cp ../*.h accel/kvm/.

../configure --enable-curses --enable-gtk --python=python2.7 --target-list=x86_64-softmmu
mkdir build
cmake ..

cp ../run_qemu x86_64-softmmu/.