@echo off

SET USE_WEST=0
SET CMAKE_OPTIONS=-DLOCAL_INSTALL=ON
SET CMAKE_BUILD=Release
SET WHEEL=0

REM Parse command line
:loop
IF NOT "%1"=="" (
    IF "%1"=="--help" (
        GOTO :Help
    ) ELSE IF "%1"=="-h" (
        GOTO :Help
	) ELSE IF "%1"=="--debug" (
        SET CMAKE_BUILD=Debug
	) ELSE IF "%1"=="-d" (
        SET CMAKE_BUILD=Debug
    ) ELSE IF "%1"=="--with" (
        SET URL=%2
		SHIFT
		cd plugins
		CALL :clone_pull %URL%
		cd ..
    ) ELSE IF "%1"=="--build-wheel" (
        SET WHEEL=1
    ) ELSE IF "%1"=="--global" (
		SET CMAKE_OPTIONS=-DLOCAL_INSTALL=OFF
	) ELSE (
		GOTO :Help
	)
    SHIFT
    GOTO :loop
)



if not exist "build" (
mkdir build
)
cd build
cmake .. %CMAKE_OPTIONS%
cmake --build . --target ALL_BUILD --config %CMAKE_BUILD% -- /nologo /verbosity:minimal /maxcpucount
cmake --build . --target INSTALL --config %CMAKE_BUILD% -- /nologo /verbosity:minimal /maxcpucount

REM Build wheel if necessary
IF "%WHEEL%" == "1" (
cd install
python -c "import librir;print('librir is importable !')"
python setup.py bdist_wheel
cd ..
)
cd ..

EXIT /B 0

:Help
echo "Build script for librir"
echo
echo "Usage:"
echo "--help		display help"
echo "--with	download and compile the given plugin using its git url"
echo "--debug	debug build only (default is release only)"
echo "--build-wheel	build Python wheel package"
echo "--global	global installation instead of local one"
echo
EXIT /B 0

:clone_pull 
REM for /F "delims=" %%i in (%~1) do set basename="%%~ni"
set basename=%~n1
echo "basename is %basename%"
if not exist "%basename%" (
	git clone %~1
) else (
	cd %basename%
	git pull
	cd ..
)
EXIT /B 0
