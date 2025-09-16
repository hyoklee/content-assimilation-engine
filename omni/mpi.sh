rm -rf mpi
mkdir mpi
cd mpi
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/hyoklee/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DUSE_MPI=ON -DSITE="ares" -DBUILDNAME="omni/r/mpi" ..
ctest -T Build
ctest -C Release -T  Test
cd ..