@echo off
echo Building OMNI with Memcached support (USE_POCO=ON)
rmdir /q /s memc
mkdir memc
cd memc
cmake -DUSE_MEMCACHED=ON -DUSE_POCO=ON -DUSE_HERMES=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DSITE="windows-11" -DBUILDNAME="omni/r/memc" ..
ctest -T Build
ctest -C Release -T Test
cd ..

echo.
echo Building OMNI with Memcached support (USE_POCO=OFF)
rmdir /q /s memc_no_poco
mkdir memc_no_poco
cd memc_no_poco
cmake -DUSE_MEMCACHED=ON -DUSE_POCO=OFF -DUSE_HERMES=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DSITE="windows-11" -DBUILDNAME="omni/r/memc_no_poco" ..
ctest -T Build
ctest -C Release -T Test
cd ..

echo.
echo Memcached build tests completed!

