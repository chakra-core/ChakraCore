mkdir Build
pushd Build
cmake -DCLANG_INCLUDE_DIRS:STRING=/usr/include/clang/3.8/ -DLLVM_CONFIG_EXECUTABLE:STRING=/usr/bin/llvm-config-3.8 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ ..
make
popd
