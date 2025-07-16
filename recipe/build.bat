:: @echo off
setlocal EnableDelayedExpansion
set CMAKE_PREFIX_PATH=%CONDA_PREFIX%
set OPENSSL_ROOT_DIR=%CONDA_PREFIX%\Library

:: Create and enter build directory
mkdir build
cd build

:: Configure the project with CMake using Ninja generator
cmake -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_INSTALL_PREFIX=%OPENSSL_ROOT_DIR% ^
      -DPOCO=ON ^
      ../.. || exit /b 1

:: Build the project
cmake --build . --config Release || exit /b 1

:: Install the project
cmake --install . --config Release || exit /b 1

:: Exit successfully
exit /b 0

