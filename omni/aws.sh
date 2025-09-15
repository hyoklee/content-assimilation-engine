rm -rf aws
mkdir aws
cd aws
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/hyoklee/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DUSE_AWS=ON -DUSE_POCO=ON -DSITE="ares" -DBUILDNAME="omni/r/poco+aws" \
    ..
ctest -T Build
ctest -C Release -T  Test
cd ..







