# [CHÚ THÍCH] Khai báo các biến cục bộ
$cli_path = "C:\Users\dangh\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
$build_dir = ".\build\esp32.esp32.XIAO_ESP32S3"
$elf_file = "$build_dir\face_rbot.ino.elf"

# [CHÚ THÍCH] BƯỚC 1: DỌN DẸP
if (Test-Path $build_dir) {
    Write-Host "[INFO] Dọn dẹp thư mục build cũ..." -ForegroundColor Yellow
    Remove-Item -Path $build_dir -Recurse -Force -ErrorAction SilentlyContinue
}

# [CHÚ THÍCH] BƯỚC 2: BIÊN DỊCH
Write-Host "[INFO] Bắt đầu biên dịch (Compile)..." -ForegroundColor Cyan
& $cli_path compile --fqbn esp32:esp32:XIAO_ESP32S3 --build-path $build_dir face_rbot.ino

# [CHÚ THÍCH] BƯỚC 3: KIỂM TRA LỖI (FAIL-FAST)
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Quá trình biên dịch thất bại! Dừng việc nạp Flash." -ForegroundColor Red
    exit $LASTEXITCODE
}

# [CHÚ THÍCH] BƯỚC 4: NẠP FLASH VÀ MỞ MONITOR
Write-Host "[INFO] Biên dịch thành công. Bắt đầu nạp Flash qua espflash..." -ForegroundColor Green
espflash flash --monitor --chip esp32s3 $elf_file