rm -rf run
mkdir run
cd run
cmake -DCMAKE_TOOLCHAIN_FILE=/Users/hyoklee/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DSITE="mac-12" -DBUILDNAME="omni/r" ..
ctest -T Build
ctest -C Release -T  Test
cd ..







