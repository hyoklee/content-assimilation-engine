rmdir /q /s hermes
mkdir hermes
cd hermes
cmake -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg.microsoft/scripts/buildsystems/vcpkg.cmake -DUSE_HERMES=ON -DSITE="windows-11" -DBUILDNAME="omni/r/hermes" ..
REM ctest -D Experimental -C Release
ctest -T Build
ctest -C Release -T  Test
cd ..







