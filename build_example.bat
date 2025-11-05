@echo off

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvars64.bat" %1))

IF NOT EXIST build (
   mkdir build
)

pushd build
cl -Zi ../example/main.c /Fetest.exe /link /DEBUG:FULL 
popd
