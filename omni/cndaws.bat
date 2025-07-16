rmdir /q /s cndaws
mkdir cndaws
cd cndaws
set CMAKE_PREFIX_PATH=%CONDA_PREFIX%
set OPENSSL_ROOT_DIR=%CONDA_PREFIX%\Library
cmake -DAWS=ON -DPOCO=ON  -DSITE="win-11" -DBUILDNAME="conda/omni/r/aws+poco" ..
REM ctest -D Experimental -C Release
ctest -T Build
ctest -C Release -T  Test
cd ..







