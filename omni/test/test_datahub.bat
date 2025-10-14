@echo off
REM DataHub integration test script
REM This script tests the DataHub registration functionality

setlocal enabledelayedexpansion

echo =========================================
echo DataHub Integration Test
echo =========================================

REM Get the directory where this script is located
set SCRIPT_DIR=%~dp0
set YAML_FILE=%SCRIPT_DIR%datahub.yml

REM Find the wrp binary
set WRP_BIN=
if exist "%SCRIPT_DIR%..\wrp.exe" (
    set WRP_BIN=%SCRIPT_DIR%..\wrp.exe
) else if exist "%SCRIPT_DIR%..\Release\wrp.exe" (
    set WRP_BIN=%SCRIPT_DIR%..\Release\wrp.exe
) else if exist ".\Release\wrp.exe" (
    set WRP_BIN=.\Release\wrp.exe
) else if exist ".\wrp.exe" (
    set WRP_BIN=.\wrp.exe
) else (
    echo Error: wrp binary not found in expected locations
    exit /b 1
)

echo Using wrp binary: !WRP_BIN!

REM Create .wrp directory if it doesn't exist
set WRP_DIR=%USERPROFILE%\.wrp
set CONFIG_FILE=%WRP_DIR%\config
set BACKUP_FILE=%CONFIG_FILE%.backup_%RANDOM%

if not exist "%WRP_DIR%" mkdir "%WRP_DIR%"

REM Backup existing config file if it exists
if exist "%CONFIG_FILE%" (
    echo Backing up existing config to %BACKUP_FILE%
    copy "%CONFIG_FILE%" "%BACKUP_FILE%" >nul
)

echo.
echo Step 1: Creating test config file
echo Config file: %CONFIG_FILE%
(
echo # Test configuration for DataHub integration
echo MetaStore DataHub
) > "%CONFIG_FILE%"

echo [OK] Config file created

echo.
echo Step 2: Checking DataHub availability
set DATAHUB_AVAILABLE=false
where curl >nul 2>&1
if !errorlevel! equ 0 (
    curl -s -o nul -w "%%{http_code}" http://localhost:8080/health | findstr "200" >nul 2>&1
    if !errorlevel! equ 0 (
        echo [OK] DataHub is available at http://localhost:8080
        set DATAHUB_AVAILABLE=true
    ) else (
        echo [WARNING] DataHub is not available at http://localhost:8080
        echo [WARNING] Test will verify config detection but skip API registration
    )
) else (
    echo [WARNING] curl not available, skipping DataHub health check
)

echo.
echo Step 3: Running wrp with DataHub config
echo Command: !WRP_BIN! put %YAML_FILE%

REM Run wrp and capture output
set TEMP_OUTPUT=%TEMP%\wrp_output_%RANDOM%.txt
call "!WRP_BIN!" put "%YAML_FILE%" > "%TEMP_OUTPUT%" 2>&1
set TEST_EXIT_CODE=!errorlevel!

type "%TEMP_OUTPUT%"

REM Check for expected output
echo.
echo Step 4: Verifying test results

REM Test 1: Check if DataHub config was detected
set TEST1_PASSED=false
findstr /C:"DataHub metastore detected in config" "%TEMP_OUTPUT%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [OK] Test 1 PASSED: DataHub config was detected
    set TEST1_PASSED=true
) else (
    echo [FAIL] Test 1 FAILED: DataHub config not detected
)

REM Test 2: Check if registration was attempted when DataHub is available
set TEST2_PASSED=true
if "!DATAHUB_AVAILABLE!" equ "true" (
    findstr /C:"Registering 'datahub_test_dataset' with DataHub" "%TEMP_OUTPUT%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo [OK] Test 2 PASSED: DataHub registration was attempted
    ) else (
        echo [FAIL] Test 2 FAILED: DataHub registration not attempted
        set TEST2_PASSED=false
    )
) else (
    echo [SKIP] Test 2 SKIPPED: DataHub not available
)

REM Test 3: Verify wrp completed successfully
set TEST3_PASSED=false
if !TEST_EXIT_CODE! equ 0 (
    echo [OK] Test 3 PASSED: wrp completed successfully
    set TEST3_PASSED=true
) else (
    echo [FAIL] Test 3 FAILED: wrp exited with code !TEST_EXIT_CODE!
)

echo.
echo =========================================
echo Test Summary
echo =========================================

REM Cleanup
del "%TEMP_OUTPUT%" >nul 2>&1
if exist "%BACKUP_FILE%" (
    echo Restoring original config
    move /y "%BACKUP_FILE%" "%CONFIG_FILE%" >nul
) else (
    REM Remove test config if no backup exists
    del "%CONFIG_FILE%" >nul 2>&1
)

REM Clean up test data
if exist datahub_test_dataset del datahub_test_dataset >nul 2>&1
if exist .blackhole rmdir /s /q .blackhole >nul 2>&1

REM Overall result
if "!TEST1_PASSED!" equ "true" if "!TEST2_PASSED!" equ "true" if "!TEST3_PASSED!" equ "true" (
    echo ALL TESTS PASSED
    exit /b 0
) else (
    echo SOME TESTS FAILED
    exit /b 1
)
