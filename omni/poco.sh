rm -rf poco
mkdir poco
cd poco
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/hyoklee/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_COVERAGE=ON \
    -DUSE_POCO=ON -DSITE="ares" -DBUILDNAME="omni/r/poco" ..
ctest -T Build
ctest -C Release -T  Test
cd ..







