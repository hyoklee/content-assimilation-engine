rmdir /q /s poco
mkdir poco
cd poco
cmake  -DUSE_POCO=ON -DUSE_HERMES=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DSITE="windows-11" -DBUILDNAME="omni/r/poco" ..
REM ctest -D Experimental -C Release
ctest -T Build
ctest -C Release -T  Test
cd ..







