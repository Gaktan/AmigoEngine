@echo off

git submodule init
git submodule update
git submodule update --recursive --remote

rem Compile SharpMake and deploy it
cd External\Sharpmake\
call bootstrap-sharpmake.bat
call CompileSharpmake.bat Sharpmake.Application/Sharpmake.Application.csproj Release AnyCPU
py deploy_binaries.py --config Release --target-dir=..\..\Tools\Sharpmake\
cd ..\..\