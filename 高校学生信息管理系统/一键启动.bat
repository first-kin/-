@echo off
setlocal EnableExtensions
title Student System - One Click Start

:: ============================================================
::  One-click start: MySQL + C++ backend (8080) + Vue frontend (5173)
::  Usage: double click this file, click "Yes" on the UAC popup.
::  Keep this file ASCII-only to avoid cmd encoding problems.
:: ============================================================

:: ---- self-elevate (needed for net start / taskkill) ----
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator rights, click "Yes" in the popup...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "ROOT=%~dp0"
cd /d "%ROOT%"

echo ================================================
echo   Student Info Management System - Starting
echo ================================================

:: ---- [1/6] stop old backend / free ports ----
echo [1/6] Stopping old backend and freeing ports 8080 / 5173 ...
taskkill /f /im student_backend.exe >nul 2>&1
for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":8080" ^| findstr LISTENING') do taskkill /f /pid %%a >nul 2>&1
for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":5173" ^| findstr LISTENING') do taskkill /f /pid %%a >nul 2>&1
echo       done.

:: ---- [2/6] MySQL service ----
echo [2/6] Checking MySQL service ...
set "MYSVC="
for %%s in (MySQL80 MySQL84 MySQL57 MySQL) do (
    if not defined MYSVC (
        sc query %%s >nul 2>&1 && set "MYSVC=%%s"
    )
)
if not defined MYSVC (
    echo       [ERROR] No MySQL service found ^(tried MySQL80/84/57/MySQL^).
    pause
    exit /b 1
)
sc query "%MYSVC%" | find "RUNNING" >nul
if %errorlevel% equ 0 (
    echo       %MYSVC% is already running.
) else (
    net start "%MYSVC%"
    if errorlevel 1 (
        echo       [ERROR] Failed to start service %MYSVC%.
        pause
        exit /b 1
    )
    echo       %MYSVC% started.
)

:: ---- [3/6] build backend (incremental, applies latest code) ----
echo [3/6] Building backend ...
pushd "%ROOT%backend"
if not exist build (
    cmake -B build -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake
)
cmake --build build --config Release
popd
if not exist "%ROOT%backend\build\Release\student_backend.exe" (
    echo       [ERROR] Build failed. Run this in backend folder to see details:
    echo         cmake --build build --config Release
    pause
    exit /b 1
)
echo       backend built.

:: ---- [4/6] start backend ----
echo [4/6] Starting backend on port 8080 ...
start "backend-8080" /d "%ROOT%backend" cmd /k "build\Release\student_backend.exe"

set /a WAIT=0
:WAIT_BACKEND
timeout /t 1 /nobreak >nul
netstat -ano | findstr ":8080" | findstr LISTENING >nul
if %errorlevel% equ 0 goto BACKEND_UP
set /a WAIT+=1
if %WAIT% lss 20 goto WAIT_BACKEND
echo       [WARN] Port 8080 is not listening yet.
echo              Check the "backend-8080" window for errors
echo              ^(most likely MySQL password in backend\config\app.json^).
goto START_FRONTEND

:BACKEND_UP
echo       backend is listening on 8080.

:START_FRONTEND
:: ---- [5/6] frontend ----
echo [5/6] Starting frontend on port 5173 ...
where npm >nul 2>&1
if errorlevel 1 (
    echo       [ERROR] npm not found in PATH. Install Node.js LTS first.
    pause
    exit /b 1
)
if exist "%ROOT%frontend\node_modules" (
    start "frontend-5173" /d "%ROOT%frontend" cmd /k "npm run dev"
) else (
    echo       First run: npm install, takes a few minutes, do not close.
    start "frontend-install" /d "%ROOT%frontend" cmd /k "npm install && npm run dev"
)

set /a WAIT=0
:WAIT_FRONTEND
timeout /t 1 /nobreak >nul
netstat -ano | findstr ":5173" | findstr LISTENING >nul
if %errorlevel% equ 0 goto FRONTEND_UP
set /a WAIT+=1
if %WAIT% lss 60 goto WAIT_FRONTEND
echo       [WARN] Port 5173 is not listening yet, open it manually later.
goto OPEN_BROWSER

:FRONTEND_UP
echo       frontend is listening on 5173.

:OPEN_BROWSER
:: ---- [6/6] browser ----
echo [6/6] Opening browser ...
start "" http://localhost:5173

echo.
echo ================================================
echo   ALL STARTED.
echo.
echo   Login:  admin  /  password
echo.
echo   To stop: close "backend-8080" and
echo            "frontend-5173" windows.
echo ================================================
pause
