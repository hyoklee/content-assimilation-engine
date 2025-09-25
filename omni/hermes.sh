rm -rf hermes
mkdir hermes
cd hermes
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/hyoklee/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DUSE_HERMES=ON \
    -DSITE="ubu-24.04/WSL" -DBUILDNAME="omni/r" ..
ctest -T Build
ctest -C Release -T  Test
cd ..







