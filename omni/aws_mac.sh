rm -rf aws
mkdir aws
cd aws
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/Users/hyoklee/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake \
    -DUSE_AWS=ON -DUSE_POCO=ON -DSITE="mac-12" -DBUILDNAME="omni/r/poco+aws" \
    ..
ctest -T Build
ctest -C Release -T  Test
cd ..







