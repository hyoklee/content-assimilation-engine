rm -rf poco
mkdir poco
cd poco
cmake -DCMAKE_TOOLCHAIN_FILE=/Users/hyoklee/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DUSE_POCO=ON -DSITE="mac-12" -DBUILDNAME="omni/r" ..
ctest -T Build
ctest -C Release -T  Test
cd ..







