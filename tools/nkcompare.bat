@ECHO off



REM check for command line arguments
IF NOT [%2] == [] IF [%3] == [] (

	GOTO :START

)



ECHO usage: nkcompare.bat original_firmware.hex exported_firmware.bin

GOTO :EOF



:START
SETLOCAL enabledelayedexpansion

REM generate comparable binaries
srec_cat %1 -Intel -offSET=0x80000000 -fill 0xFF 0x00000 0x3E000 -output %TEMP%\firmware_orig_filled.bin -binary

srec_cat %2 -binary -offSET=0x00000000 -fill 0xFF 0x00000 0x3E000 -output %TEMP%\firmware_exported_filled.bin -binary

REM generate hash values
SET /a count=1 
FOR /f "skip=1 delims=:" %%a IN ('CertUtil -hashfile %TEMP%/firmware_orig_filled.bin SHA256') DO (
  IF !count! EQU 1 SET "orig_hash=%%a"
  SET /a count+=1
)
SET "orig_hash=%orig_hash: =%

SET /a count=1 
for /f "skip=1 delims=:" %%a IN ('CertUtil -hashfile %TEMP%/firmware_exported_filled.bin SHA256') DO (
  IF !count! EQU 1 SET "exp_hash=%%a"
  SET/a count+=1
)
SET "exp_hash=%exp_hash: =%

REM remove temporary output files
DEL %TEMP%\firmware_orig_filled.bin
DEL %TEMP%\firmware_exported_filled.bin

IF %exp_hash% EQU %orig_hash% (
	ECHO Firmware binary match
	ECHO SHA512: %exp_hash%
	GOTO :EOF
)

ECHO Firmware binary mismatch
ECHO Original File SHA256: %orig_hash%
ECHO Exported File SHA256: %exp_hash%

ENDLOCAL
:EOF
