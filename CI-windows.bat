@echo off
REM Check versions
qmake -v
cmake --version
ninja --version
pandoc --version
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

mkdir cmakebuild
cd cmakebuild

REM DISABLE Warnings as errors for TRENCH build stability
REM set CXXFLAGS="/WX" 

call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Added -DTB_BUILD_UNIT_TESTS=OFF to skip building unnecessary tests
cmake .. -GNinja -DCMAKE_PREFIX_PATH="%QT_ROOT_DIR%" -DCMAKE_BUILD_TYPE=Release -DTB_ENABLE_PCH=0 -DTB_ENABLE_CCACHE=0 -DTB_BUILD_UNIT_TESTS=OFF

IF %ERRORLEVEL% NEQ 0 GOTO ERROR

cmake --build . --config Release

IF %ERRORLEVEL% NEQ 0 GOTO ERROR

set BUILD_DIR="%cd%"

REM --- UNIT TESTS REMOVED FOR TRENCH SDK ---
REM Tests are skipped because they depend on removed game configurations (Quake/etc)
REM -----------------------------------------

"C:\Program Files\CMake\bin\cpack.exe"

IF %ERRORLEVEL% NEQ 0 GOTO ERROR

call generate_checksum.bat

GOTO END

:ERROR
echo "Building TRENCH failed"
exit /b 1

:END