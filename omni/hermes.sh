module load openmpi/5.0.5-x2kvx5l
module load iowarp/main-resnye4
rm -rf hermes
mkdir hermes
cd hermes
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/hyoklee/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_HERMES=ON \
    -DUSE_POCO=OFF \
    -DSITE="ares" -DBUILDNAME="omni/r/hermes" ..
ctest -C Release -D Experimental
cd ..







