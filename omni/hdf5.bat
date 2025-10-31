rmdir /q /s hdf5
mkdir hdf5
cd hdf5
cmake  -DUSE_HDF5=ON -DUSE_MPI=ON -DUSE_HERMES=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DSITE="windows-11" -DBUILDNAME="omni/r/hdf5" ..
REM ctest -D Experimental -C Release
ctest -T Build
ctest -C Release -T  Test
cd ..
