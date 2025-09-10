rmdir /q /s poco
mkdir poco
cd poco
cmake  -DUSE_POCO=ON -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DSITE="win-11" -DBUILDNAME="omni/r" ..
REM ctest -D Experimental -C Release
ctest -T Build
ctest -C Release -T  Test
cd ..







