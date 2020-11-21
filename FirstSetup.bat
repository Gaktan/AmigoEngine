@echo off

rem Get submodules at the current revision
git submodule update --init --recursive

rem Update submodules to the latest revision
rem git submodule update --recursive --remote

rem Compile DirectXTex
cd External\DirectXTex\
rem Use Sharpmake's CompileSharpmake.bat because it's pretty good
call ..\Sharpmake\CompileSharpmake.bat ..\DirectXTex\DirectXTex\DirectXTex_Desktop_2017.vcxproj Debug x64
call ..\Sharpmake\CompileSharpmake.bat ..\DirectXTex\DirectXTex\DirectXTex_Desktop_2017.vcxproj Release x64
cd ..\..\

rem Compile Sharpmake and deploy it
cd External\Sharpmake\
call bootstrap-sharpmake.bat
call CompileSharpmake.bat Sharpmake.Application/Sharpmake.Application.csproj Release AnyCPU
py deploy_binaries.py --config Release --target-dir=..\..\Tools\Sharpmake\
cd ..\..\