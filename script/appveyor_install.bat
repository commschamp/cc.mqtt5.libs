IF "%APPVEYOR_BUILD_WORKER_IMAGE%"=="Visual Studio 2017" (
    set TOOLCHAIN=msvc15
    set BOOST_VER=1_69_0
    set CMAKE_GENERATOR="Visual Studio 15 2017"
   
) ELSE IF "%APPVEYOR_BUILD_WORKER_IMAGE%"=="Visual Studio 2019" (
    set TOOLCHAIN=msvc16
    set BOOST_VER=1_77_0
    set CMAKE_GENERATOR="Visual Studio 16 2019"
) ELSE IF "%APPVEYOR_BUILD_WORKER_IMAGE%"=="Visual Studio 2022" (
    set TOOLCHAIN=msvc17
    set BOOST_VER=1_83_0
    set CMAKE_GENERATOR="Visual Studio 17 2022"
) ELSE (
    echo Toolchain %TOOLCHAIN% is not supported
    exit -1
)

IF "%PLATFORM%"=="x86" (
    set CMAKE_PLATFORM="Win32"
) ELSE (
    set CMAKE_PLATFORM="x64"
)

set BOOST_DIR=C:\Libraries\boost_%BOOST_VER%
