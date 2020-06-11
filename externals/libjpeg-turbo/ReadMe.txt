The folder contains build of the libjpeg-turbo library, MSVC 2015.

MSVC solution files were generated using CMake command below.

32 bit
cmake -G "Visual Studio 14 2015" -D ENABLE_SHARED:BOOL=ON -D WITH_ARITH_DEC:BOOL=ON -D WITH_ARITH_ENC:BOOL=ON -D WITH_JPEG7:BOOL=ON -D WITH_JPEG8:BOOL=ON -D WITH_MEM_SRCDST:BOOL=ON -D WITH_SIMD:BOOL=ON -D WITH_TURBOJPEG:BOOL=ON ../

64 bit
cmake -G "Visual Studio 14 2015 Win64" -D ENABLE_SHARED:BOOL=ON -D WITH_ARITH_DEC:BOOL=ON -D WITH_ARITH_ENC:BOOL=ON -D WITH_JPEG7:BOOL=ON -D WITH_JPEG8:BOOL=ON -D WITH_MEM_SRCDST:BOOL=ON -D WITH_SIMD:BOOL=ON -D WITH_TURBOJPEG:BOOL=ON ../


Version 2.0.4  (2019-12-31)
https://github.com/libjpeg-turbo/libjpeg-turbo
