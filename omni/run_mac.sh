rm -rf run
mkdir run
cd run
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILDNAME=omni/r/poco \
    -DCMAKE_TOOLCHAIN_FILE=/Users/hyoklee/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake \
    -DSITE=mac-12\
    -DUSE_POCO=ON \
    ..
ctest -D Experimental
cd ..







