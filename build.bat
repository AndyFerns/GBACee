@echo off

@REM Create build folder if it doesnt exist
if not exist build (
    mkdir build
)

echo [*] Building GBCee...
gcc -Wall -Wextra -std=c11 -Iincludes -Iincludes\core -Iincludes\hardware -Iincludes\platform -Iincludes\debug -g src\*.c src\core\*.c src\hardware\*.c src\platform\*.c src\debug\*.c -o build\gbcee.exe -IC:\msys64\mingw64\include\SDL2 -LC:\msys64\mingw64\lib -lSDL2

if %errorlevel% neq 0 (
    echo [!] Build failed.
    exit /b %errorlevel%
)

echo [!] Build successful. Running...

build\gbcee.exe