@echo off
setlocal

REM [CHÚ THÍCH] Khai báo các biến cục bộ
set CLI_PATH="C:\Users\dangh\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
set BUILD_DIR=.\build\esp32.esp32.XIAO_ESP32S3
set ELF_FILE=%BUILD_DIR%\face_rbot.ino.elf

REM [CHÚ THÍCH] BƯỚC 1: DỌN DẸP
echo [INFO] Don dep thu muc build cu...
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%

REM [CHÚ THÍCH] BƯỚC 2: BIÊN DỊCH
echo [INFO] Bat dau bien dich (Compile)...
%CLI_PATH% compile --fqbn esp32:esp32:XIAO_ESP32S3 --build-path %BUILD_DIR% face_rbot.ino

REM [CHÚ THÍCH] BƯỚC 3: KIỂM TRA LỖI (FAIL-FAST)
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Qua trinh bien dich that bai! Dung viec nap Flash.
    exit /b %ERRORLEVEL%
)

REM [CHÚ THÍCH] BƯỚC 4: NẠP FLASH VÀ MỞ MONITOR
echo [INFO] Bien dich thanh cong. Bat dau nap Flash qua espflash...
espflash flash --monitor --chip esp32s3 "%ELF_FILE%"

endlocal