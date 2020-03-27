@echo off

if exist build rmdir /S /Q build
if not exist lib mkdir lib
if not exist lib\x64_Debug mkdir lib\x64_Debug
if not exist lib\x64_Release mkdir lib\x64_Release

mkdir build
cd build

cmake ..
msbuild librf.sln /p:Configuration=Debug /p:Platform=x64 /m
msbuild librf.sln /p:Configuration=Release /p:Platform=x64 /m

xcopy /Y Debug\librf.* ..\lib\x64_Debug\
xcopy /Y Release\librf.* ..\lib\x64_Release\

cd ..

rmdir /S /Q build
