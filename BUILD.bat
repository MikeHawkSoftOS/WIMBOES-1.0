@echo off
echo ========================================
echo Wimboes Enhanced Build Script
echo ========================================
echo.

REM Check if WSL is available
wsl --version >nul 2>&1
if %errorlevel% equ 0 (
    echo WSL detected! Building with WSL...
    echo.
    wsl bash -c "cd '/mnt/d/___SYNO/Downloads/wimboes src' && make clean && make && echo '' && echo 'Build complete! Image at: build/wimboes.img' && echo 'Size:' && ls -lh build/wimboes.img | awk '{print $5}'"
    
    if exist "build\wimboes.img" (
        echo.
        echo ========================================
        echo SUCCESS! Image created.
        echo ========================================
        echo.
        echo To run in VirtualBox:
        echo 1. Create VM: Other/Unknown 32-bit, 128MB RAM
        echo 2. Storage -^> Floppy Controller -^> Add Floppy
        echo 3. Select: build\wimboes.img
        echo 4. Boot Order: Enable Floppy first
        echo 5. Start VM!
        echo.
        echo New games available:
        echo - Snake, Tetris, Pong
        echo - Find in Start Menu!
        echo.
    ) else (
        echo.
        echo ========================================
        echo BUILD FAILED!
        echo ========================================
        echo Check errors above.
        echo.
    )
) else (
    echo WSL not found!
    echo.
    echo Please install WSL to build:
    echo   wsl --install
    echo.
    echo Or use MinGW/MSYS2:
    echo 1. Install from https://www.msys2.org/
    echo 2. Open MSYS2 MinGW 32-bit terminal
    echo 3. Run: pacman -S mingw-w64-i686-gcc make
    echo 4. Navigate to this folder and run: make
    echo.
)

pause
