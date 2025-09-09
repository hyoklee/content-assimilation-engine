rmdir /q /s h5
mkdir h5
cd h5
cmake -DUSE_HDF5=ON  -DUSE_AWS=ON -DUSE_POCO=ON -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DSITE="win-11" -DBUILDNAME="omni/r/aws+h5+poco" ..
REM ctest -D Experimental -C Release
ctest -T Build
ctest -C Release -T  Test
cd ..
