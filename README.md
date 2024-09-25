Fun with DES


Tested with:
clang version 20.0.0git (https://github.com/llvm/llvm-project.git bcbdf7ad6b571d11c102d018c78ee0fbf71e3e2c)
Target: x86_64-unknown-linux-gnu
Thread model: posix
InstalledDir: /home/bswenson3/software/llvm/bin
Build config: +assertions

Note: make -j needs to be fixed but the number of targets is small so just use `make`

git clone git@github.com:bpswenson/fracture.git

cd fracture && mkdir build && cd build && cmake .. && make

./test/test_lib

Before function call: 1

After function call: 2

Before undo function call: 2

After undo function call: 1


