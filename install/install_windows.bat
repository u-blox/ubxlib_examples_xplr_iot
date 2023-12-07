@echo off
rem Copyright 2022-2023 u-blox
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

setlocal

set NCS_ROOT=%SystemDrive%\ncs
if "%NCS_VERS%" == "" (set NCS_VERS=v2.2.0)

set ADMIN_SUCC_FILE=%TEMP%\%~n0_admin.log
set ERR_FILE=%TEMP%\_err.txt
set PIP_COM=pip3 --disable-pip-version-check install -q

net session >nul 2>nul && (set IS_ADMIN=1)
if "%1" == "admin" (goto AdminInstall)
if defined IS_ADMIN (echo ** This script should not be started as administrator. & pause & exit /b 1)

if "%1" == "sdk" (goto SDKInstall)

echo.
echo === Installing all the required tools and packages for XPLR-IOT-1 examples
echo === This will be a lengthy process (10-20 min), please be patient
echo.

echo Started at: %date% %time%

echo Installing tools which require administrator priviledges...
del %ADMIN_SUCC_FILE% 2>NUL
call powershell -Command "Start-Process -Wait -Verb RunAs '%~f0' admin" 2>nul
if NOT exist %ADMIN_SUCC_FILE% goto AdminFailed

set PATH=%ProgramFiles%\Microsoft VS Code\bin;%PATH%
echo Installing Visual Studio Code extensions...
call :SilentCom "code --install-extension ms-vscode.cpptools-extension-pack"
call :SilentCom "code --install-extension marus25.cortex-debug"

mkdir %NCS_ROOT% >nul 2>&1
cd /D %NCS_ROOT%
set MGR_CMD=nrfutil-toolchain-manager.exe
set MGR_VERS=v1.2.6
if NOT exist %MGR_CMD% (
  curl -s https://raw.githubusercontent.com/NordicSemiconductor/pc-nrfconnect-toolchain-manager/%MGR_VERS%/resources/nrfutil-toolchain-manager/win32/%MGR_CMD% -o %MGR_CMD%
  curl -s https://raw.githubusercontent.com/NordicSemiconductor/pc-nrfconnect-toolchain-manager/%MGR_VERS%/resources/nrfutil-toolchain-manager/win32/vcruntime140.dll -o vcruntime140.dll
)
if NOT exist %NCS_ROOT%\toolchains\%NCS_VERS% (
  echo Installing nrf-connect environment tools...
  %MGR_CMD% install --ncs-version %NCS_VERS% --install-dir %NCS_ROOT%
)

%MGR_CMD% launch --install-dir %NCS_ROOT% --toolchain-path %NCS_ROOT%\toolchains\%NCS_VERS% "%~f0" sdk

goto eof

:SDKInstall
set NCS_DIR=%NCS_ROOT%\%NCS_VERS%
if NOT exist %NCS_DIR% (
  echo Installing nrf-connect SDK version %NCS_VERS% ...
  mkdir %NCS_DIR%
  cd /D %NCS_DIR%
  call :SilentCom "west init -m https://github.com/nrfconnect/sdk-nrf --mr %NCS_VERS%"
  call :SilentCom "west update"
  call :SilentCom "west zephyr-export"

  echo Installing additional Python requirements...
  %PIP_COM% -r zephyr/scripts/requirements.txt >nul 2>&1
  %PIP_COM% -r nrf/scripts/requirements.txt >nul 2>&1
  %PIP_COM% -r bootloader/mcuboot/scripts/requirements.txt >nul 2>&1
)

cd %USERPROFILE%

set REPO_NAME=ubxlib_examples_xplr_iot
if NOT exist %REPO_NAME% (
  echo Getting the source code repositories...
  call git clone --recursive -q https://github.com/u-blox/%REPO_NAME%
)
cd /D %REPO_NAME%

set NEWT_FILE=newtmgr.exe
set NEWT_TAR_FILE=%TEMP%\newtmgr.tar.gz
if NOT exist %NEWT_FILE% (
  curl -s https://archive.apache.org/dist/mynewt/apache-mynewt-1.4.1/apache-mynewt-newtmgr-bin-windows-1.4.1.tgz >%NEWT_TAR_FILE%
  call tar -O -xf %NEWT_TAR_FILE% "*%NEWT_FILE%" >%NEWT_FILE%
  del %NEWT_TAR_FILE%
)

set DO_CMD=do.bat
echo @set NCS_DIR=%NCS_DIR%>%DO_CMD%
echo @set PATH=^%%PATH^%%;^%%ProgramFiles^%%\Microsoft VS Code\bin>>%DO_CMD%
echo @%NCS_ROOT%\%MGR_CMD% launch --ncs-version %NCS_VERS% --install-dir %NCS_ROOT% cmd /c -- python do.py %%* >>%DO_CMD%

echo Ended at: %date% %time%
echo.
echo ======= All done! =======
echo.
pause
goto eof

:AdminInstall
if NOT defined IS_ADMIN (echo ** This part must be run as administrator. & pause & exit /b 1)

rem Avoid possible path length problem
call :SilentCom "reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystem /f /v LongPathsEnabled /t REG_DWORD /d 1"

cd %TEMP%
set CODE_INSTALLER=code_inst.exe
if NOT exist "%ProgramFiles%\Microsoft VS Code" (
  echo Installing Visual Studio Code...
  curl -s -L "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64" >%CODE_INSTALLER%
  %CODE_INSTALLER% /VERYSILENT /NORESTART /MERGETASKS=!runcode
  del %CODE_INSTALLER%
)

set JLINK_INSTALLER=JLink_Windows_x86_64.exe
if NOT exist "%ProgramFiles%\SEGGER\JLink" (
  echo Installing JLink software...
  curl -s -X POST https://www.segger.com/downloads/jlink/%JLINK_INSTALLER% ^
         -H "Content-Type: application/x-www-form-urlencoded" ^
         -d "accept_license_agreement=accepted" >%JLINK_INSTALLER%
  start /wait %JLINK_INSTALLER% /S
  del %JLINK_INSTALLER%
)

set NRF_CMD_DIR=%ProgramFiles%\Nordic Semiconductor\nrf-command-line-tools\bin
set NRF_CMD_INSTALLER=nrf-command-line-tools-10.23.0-x64.exe
if NOT exist "%NRF_CMD_DIR%" (
  echo Installing Nordic command line tools...
  curl -s -L https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-23-0/%NRF_CMD_INSTALLER% >%NRF_CMD_INSTALLER%
  start /wait %NRF_CMD_INSTALLER% /passive /quiet
  del %NRF_CMD_INSTALLER%
  setx PATH "%PATH%;%NRF_CMD_DIR%" >nul
)

echo Installing serial port driver...
set DRIVER_NAME=CP210x_Universal_Windows_Driver
set DRIVER_ZIP=%DRIVER_NAME%.zip
curl -s -L https://www.silabs.com/documents/public/software/%DRIVER_ZIP% >%DRIVER_ZIP%
mkdir %DRIVER_NAME%
call tar -C %DRIVER_NAME% -xf %DRIVER_ZIP%
del %DRIVER_ZIP%
cd %DRIVER_NAME%
call :SilentCom "pnputil /add-driver silabser.inf"
cd ..
rmdir /S /Q %DRIVER_NAME%

echo OK >%ADMIN_SUCC_FILE%

goto eof

:AdminFailed
echo Installation aborted!
pause
goto eof

:SilentCom
rem Execute a command without any printouts unless an error occurs
%~1 1>nul 2>%ERR_FILE% || (type %ERR_FILE% & pause & exit 1)
exit /b 0

:eof