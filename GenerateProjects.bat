cd external\Sharpmake\
py deploy_binaries.py --config Release --target-dir=..\..\tools\Sharpmake\
cd ..\..\
rem py tools\Sharpmake\CopySharpmakeExecutables.bat
tools\Sharpmake\Sharpmake.Application.exe /sources("projects/sharpmake/main.sharpmake.cs")
