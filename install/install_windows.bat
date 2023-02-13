@echo off
rem Copyright 2022 u-blox
rem
rem Licensed under the Apache License, Version 2.0 (the "License");
rem you may not use this file except in compliance with the License.
rem You may obtain a copy of the License at
rem
rem  http://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by applicable law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and

net session >nul 2>nul || (echo ** This script must be run as administrator. & pause & exit /b 1)
echo.
echo === Installing all the required tools and packages for XPLR-IOT-1 examples
echo === This will be a lengthy process, please be patient
echo.

setlocal
set ERR_FILE=%TEMP%\_err.txt
set ROOT_DIR=%USERPROFILE%\xplriot1
set ENV_DIR=%ROOT_DIR%\env
set GIT_ENV_DIR=%ENV_DIR:\=/%
set NCS_VERS=v2.1.0

echo Started at: %date% %time%

echo Installing packages...
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass ^
   -Command "[System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))" ^
   1>nul 2>%ERR_FILE% || (type %ERR_FILE% & exit /b 1)
set PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin
call :SilentCom "choco install -y --no-progress cmake --installargs 'ADD_CMAKE_TO_PATH=System'"
call :SilentCom "choco install -y --no-progress ninja gperf python3 git dtc-msys2 wget unzip nrfjprog tartool ccache"
call refreshenv >nul
pip3 install -q west
set TarFile=newtmgr.tar.gz
curl -s https://archive.apache.org/dist/mynewt/apache-mynewt-1.4.1/apache-mynewt-newtmgr-bin-windows-1.4.1.tgz >%TarFile%
call tar -O -xf %TarFile% "*newtmgr.exe" >%ALLUSERSPROFILE%\chocolatey\bin\newtmgr.exe
del %TarFile%

echo Installing nRF Connect SDK...
mkdir %ENV_DIR%\ncs
cd /d  %ENV_DIR%\ncs
call :SilentCom "west init -m https://github.com/nrfconnect/sdk-nrf --mr %NCS_VERS%"
call :SilentCom "west update"
call :SilentCom "west zephyr-export"

echo Installing additional Python requirements...
pip3 install -q -r zephyr/scripts/requirements.txt >nul 2>&1
pip3 install -q -r nrf/scripts/requirements.txt >nul 2>&1
pip3 install -q -r bootloader/mcuboot/scripts/requirements.txt >nul 2>&1

echo Installing ARM compiler...
cd ..
set GCCLoc=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10-2020q4
set GCCName=gcc-arm-none-eabi-10-2020-q4-major
set GCCZip=%GCCName%-win32.zip
wget -q %GCCLoc%/%GCCZip%
call unzip -q -o %GCCZip%
del %GCCZip%

echo Installing Visual Studio Code...
set CodeInstaller=code_inst.exe
curl -s -L "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64" >%CodeInstaller%
%CodeInstaller% /VERYSILENT /NORESTART /MERGETASKS=!runcode
call refreshenv >nul
del %CodeInstaller%
echo Installing extensions...
call :SilentCom "code --install-extension ms-vscode.cpptools-extension-pack"
call :SilentCom "code --install-extension marus25.cortex-debug"
call :SilentCom "code --uninstall-extension ms-vscode.cmake-tools"

echo Installing JLink software...
set JLinkExe=JLink_Windows_x86_64.exe
curl -s -X POST https://www.segger.com/downloads/jlink/%JLinkExe% ^
       -H "Content-Type: application/x-www-form-urlencoded" ^
       -d "accept_license_agreement=accepted" >%JLinkExe%
%JLinkExe% /S
del %JLinkExe%

echo Installing serial port driver...
set DriverName=CP210x_Universal_Windows_Driver
set DriverZip=%DriverName%.zip
wget -q https://www.silabs.com/documents/public/software/%DriverZip%
call unzip -q -o -d %DriverName% %DriverZip%
del %DriverZip%
cd %DriverName%
call :SilentCom "pnputil /add-driver silabser.inf"
cd ..
rmdir /S /Q %DriverName%

cd ..
echo Getting the source code repositories...
call git clone --recursive -q https://github.com/u-blox/XPLR-IoT-1-ApplicationFramework
cd XPLR-IoT-1-ApplicationFramework
call python do -n %ENV_DIR%\ncs -t %ENV_DIR%\%GCCName% save
rem Avoid owner protection problems as we have cloned as admin
call :SilentCom "takeown /r /f %ROOT_DIR%\ubxlib_examples_xplr_iot"
call git config --global --add safe.directory %GIT_ENV_DIR%/ncs/zephyr

echo Ended at: %date% %time%
echo.
echo ======= All done! =======
echo.
pause
goto:eof

:SilentCom
%~1 1>nul 2>%ERR_FILE% || (type %ERR_FILE% & exit /b 1)
exit /b 0