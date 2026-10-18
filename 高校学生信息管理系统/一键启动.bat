@echo off
title Student System - One Click Start

:: ============================================================
::  One-click start: MySQL + C++ backend (8080) + Vue frontend (5173)
::  Usage: double click. UAC prompt will appear - click "Yes".
::  NOTE: keep this file pure ASCII to avoid encoding issues.
:: ============================================================

:: self-elevate (needed for net start)
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator rights, click "Yes" in the popup...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "ROOT=%~dp0"

echo ================================================
echo   Student Info Management System - Starting
echo ================================================

echo [1/5] Cleaning up old processes / ports 8080 and 5173 ...
taskkill /f /im student_backend.exe >nul 2>&1
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :8080 ^| findstr LISTENING') do taskkill /f /pid %%a >nul 2>&1
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :5173 ^| findstr LISTENING') do taskkill /f /pid %%a >nul 2>&1
echo       done.

echo [2/5] Checking MySQL service ...
sc query MySQL80 | find "RUNNING" >nul
if %errorlevel% equ 0 (
    echo       MySQL already running, skip.
) else (
    net start MySQL80
    if errorlevel 1 (
        echo       [ERROR] Failed to start MySQL80 service.
        pause
        exit /b 1
    )
    echo       MySQL80 started.
)

echo [3/5] Starting backend (Drogon, port 8080) ...
if not exist "%ROOT%backend\build\Release\student_backend.exe" (
    echo       [ERROR] student_backend.exe not found. Build first:
    echo         cd backend  ^&^&  cmake --build build --config Release
    pause
    exit /b 1
)
start "backend-8080" cmd /k "cd /d %ROOT%backend && build\Release\student_backend.exe"
echo       backend window launched.

echo [4/5] Starting frontend (Vite, port 5173) ...
if exist "%ROOT%frontend\node_modules" (
    start "frontend-5173" cmd /k "cd /d %ROOT%frontend && npm run dev"
    echo       frontend window launched.
) else (
    echo       First run: installing deps, takes 2-5 minutes, do NOT close...
    start "frontend-install" cmd /k "cd /d %ROOT%frontend && npm install && npm run dev"
)

echo [5/5] Waiting for services, then opening browser ...
timeout /t 8 /nobreak >nul
start "" http://localhost:5173

echo.
echo ================================================
echo   ALL STARTED.
echo.
echo   Login:  admin  /  password
echo   (If browser opened too early during first
echo    npm install, refresh it in half a minute.)
echo.
echo   To stop: close "backend-8080" and
echo   "frontend-5173" windows.
echo ================================================
pause
