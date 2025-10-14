# DataHub Integration Tests

This document describes the DataHub integration tests for the OMNI module.

## Overview

The DataHub tests verify that OMNI can properly detect and use DataHub as a metadata store when configured. The tests include both unit tests and integration tests.

## Test Files

### 1. `test_datahub.cpp` - Unit Tests

C++ unit test that directly tests the DataHub functions:

- **Test 1**: `CheckDataHubConfig()` returns false when config doesn't exist
- **Test 2**: `CheckDataHubConfig()` returns false when config exists but doesn't contain "MetaStore DataHub"
- **Test 3**: `CheckDataHubConfig()` returns true when config contains "MetaStore DataHub"
- **Test 4**: `ReadConfigFile()` returns empty string for non-existent file
- **Test 5**: `ReadConfigFile()` returns content for existing file

**Run with:**
```bash
ctest -R datahub_unit -V
```

### 2. `test_datahub.sh` - Integration Test

Bash script that tests the complete DataHub workflow:

1. Creates `~/.wrp/config` with "MetaStore DataHub" setting
2. Runs `wrp put` with a test YAML file
3. Verifies that:
   - Config is detected
   - DataHub registration is attempted (if DataHub is available)
   - OMNI workflow completes successfully
4. Cleans up test files and restores original config

**Run with:**
```bash
ctest -R datahub_integration -V
```

### 3. `datahub.yml` - Test YAML File

Simple test dataset configuration for DataHub testing:
- Name: `datahub_test_dataset`
- Tags: `test`, `datahub`, `integration`
- Source: Sample CSV file
- Reads 100 bytes from offset 0

## Configuration

### Enabling DataHub

To enable DataHub metadata registration, create `~/.wrp/config` with:

```
MetaStore DataHub
```

### DataHub Requirements

For full integration testing (with actual API registration):
- DataHub must be running at `http://localhost:8080`
- DataHub GMS must be accessible
- Tests will still pass if DataHub is not available (they test config detection only)

## Running the Tests

### All DataHub Tests

```bash
ctest -R datahub -V
```

### Individual Tests

```bash
# Unit tests only
ctest -R datahub_unit -V

# Integration test only
ctest -R datahub_integration -V

# Test that DataHub is NOT triggered without config
ctest -R datahub_disabled -V
```

## Test Dependencies

- **POCO**: Required for HTTP operations
- **yaml-cpp**: For YAML parsing
- CMake with `USE_POCO=ON`

## Test Behavior

### When DataHub is Running

If DataHub is available at `http://localhost:8080/health`:
- Integration test will attempt actual registration
- Test verifies both config detection AND API registration

### When DataHub is Not Running

If DataHub is not available:
- Integration test will still verify config detection
- Test will show a warning but still pass
- This ensures OMNI continues to work even if DataHub is unavailable

## Expected Output

### Successful Unit Test

```
=========================================
DataHub Unit Tests
=========================================

Test 1: Config file doesn't exist... PASSED
Test 2: Config exists but no MetaStore DataHub... PASSED
Test 3: Config contains MetaStore DataHub... PASSED
Test 4: ReadConfigFile with non-existent file... PASSED
Test 5: ReadConfigFile with existing file... PASSED

=========================================
Test Results
=========================================
Passed: 5
Failed: 0

ALL TESTS PASSED!
```

### Successful Integration Test

```
=========================================
DataHub Integration Test
=========================================
Using wrp binary: ./wrp

Step 1: Creating test config file
Config file: /home/user/.wrp/config
✓ Config file created

Step 2: Checking DataHub availability
✓ DataHub is available at http://localhost:8080

Step 3: Running wrp with DataHub config
Command: ./wrp put test/datahub.yml
...
DataHub metastore detected in config
Registering 'datahub_test_dataset' with DataHub...done
...

Step 4: Verifying test results
✓ Test 1 PASSED: DataHub config was detected
✓ Test 2 PASSED: DataHub registration was attempted
✓ Test 3 PASSED: wrp completed successfully

=========================================
Test Summary
=========================================
ALL TESTS PASSED
```

## Troubleshooting

### Test Failures

1. **Config not detected**: Check that `~/.wrp/config` is being created properly
2. **Registration fails**: Verify DataHub is running and accessible
3. **Permission errors**: Ensure `~/.wrp/` directory is writable

### Debugging

Run with verbose output:
```bash
ctest -R datahub -VV
```

Check DataHub availability:
```bash
curl http://localhost:8080/health
```

## Cleanup

Tests automatically clean up after themselves:
- Backup and restore any existing `~/.wrp/config`
- Remove test data files
- Remove `.blackhole` directory

## Integration with CI/CD

These tests can be run in CI/CD pipelines. They will:
- Pass without DataHub (testing config detection only)
- Pass with DataHub (testing full registration workflow)
- Fail only if the code itself is broken

To run in CI without DataHub:
```bash
ctest -R datahub_unit -V  # Only run unit tests
```
