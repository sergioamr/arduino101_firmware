@echo off

:: Flash Arduino 101 firmware via USB and dfu-util
cls
setlocal ENABLEDELAYEDEXPANSION

set FLASH_BOOTLOADER=0

:start_parse_params
if "%1" == ""  goto:end_parse_params
if /i "%1" == "-b" (
  set FLASH_BOOTLOADER=1
  call:bl_warning
  if !ERRORLEVEL! NEQ 1 goto:error
)
if /i "%1" == "-s" (
  set SER_NUM=%2
  if "!SER_NUM!" == "" goto:help	  
  set SER_NUM_ARG=-S !SER_NUM!
)

SHIFT
goto:start_parse_params

:end_parse_params

set DFU=bin\dfu-util.exe %SER_NUM_ARG% -d,8087:0ABA
set IMG=images/firmware

if NOT "%SER_NUM%" == "" echo Flashing board S/N: %SER_NUM%
echo.
echo Reset the board before proceeding...
REM wait for DFU device
set X=
:loop
  for /f "delims== tokens=8" %%i in ('%DFU% -l 2^>NUL ^|find "sensor"') do (
    set X=%%i
  )
  REM extract the serial number from unknown discovered device
  if "!X!" == "" goto:loop
  if "%SER_NUM%" == "" (
    set SER_NUM=!X:*serial=!
	set SER_NUM=!SER_NUM:"=!
	echo Using board S/N: !SER_NUM!...
	set DFU=%DFU% -S !SER_NUM!
  ) 

if %FLASH_BOOTLOADER% EQU 1 (
  call:flash_bl
)
call:flash

if %ERRORLEVEL% NEQ 0 goto:error

echo.
echo ---SUCCESS---
pause
exit /b 0

:flash_bl
  echo.
  echo ** Flashing Bootloader **
  echo.
  %DFU% -a 7 -D %IMG%/bootloader_quark.bin
    if !ERRORLEVEL! NEQ 0 goto:error
  %DFU% -a 2 -R -D %IMG%/bootupdater.bin
    if !ERRORLEVEL! NEQ 0 goto:error
  echo *** Sleeping for 12 seconds...
  call:delay 12  
goto:eof
  
:flash
  echo.
  echo ** Flashing Quark **
  echo.
  %DFU% -a 2 -D %IMG%/quark.bin
    if !ERRORLEVEL! NEQ 0 goto:error
  %DFU% -a 7 -D %IMG%/arc.bin -R
    if !ERRORLEVEL! NEQ 0 goto:error
goto:eof

REM Ugly Windows equivalent of 'sleep'
:delay
  choice /c:Z /d:Z /t %1 > nul
goto:eof

REM Bootloader flashing warning
:bl_warning
echo.
echo ***************************************************************************
echo *                           *** WARNING ***                               *
echo * Flashing a bootloader may brick your board.                             *
echo * Do not flash bootloader unless you were explicitly instructed to do so. *
echo *                       PROCEED AT YOUR OWN RISK                          *
echo ***************************************************************************
echo.
choice /M "Proceed with flashing bootloader? " /c YN
if NOT errorlevel 1 exit 1
goto:eof

REM Usage message
:help
echo Usage: %~nx0 [options]
echo               -b                Flash bootloader
echo               -s serial_number  Only flash to board with specified serial number
exit /b 1
goto:eof

REM Return error message
:error
echo.
echo ***ERROR***
pause
exit /b 1
goto:eof
