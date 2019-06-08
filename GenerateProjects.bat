@echo off
cd external\Sharpmake\
bootstrap-sharpmake.bat
CompileSharpmake.bat Sharpmake.Application/Sharpmake.Application.csproj Release AnyCPU
py deploy_binaries.py --config Release --target-dir=..\..\tools\Sharpmake\
cd ..\..\

rem py tools\Sharpmake\CopySharpmakeExecutables.bat
tools\Sharpmake\Sharpmake.Application.exe /sources("projects/sharpmake/main.sharpmake.cs")
