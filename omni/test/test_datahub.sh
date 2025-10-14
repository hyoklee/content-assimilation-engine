#!/bin/bash
# DataHub integration test script
# This script tests the DataHub registration functionality

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "DataHub Integration Test"
echo "========================================="

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
YAML_FILE="${SCRIPT_DIR}/datahub.yml"

# Find the wrp binary
if [ -x "${SCRIPT_DIR}/../wrp" ]; then
    WRP_BIN="${SCRIPT_DIR}/../wrp"
elif [ -x "${SCRIPT_DIR}/../build/omni/wrp" ]; then
    WRP_BIN="${SCRIPT_DIR}/../build/omni/wrp"
elif [ -x "./omni/wrp" ]; then
    WRP_BIN="./omni/wrp"
elif [ -x "./wrp" ]; then
    WRP_BIN="./wrp"
else
    echo -e "${RED}Error: wrp binary not found in expected locations${NC}"
    exit 1
fi

echo "Using wrp binary: ${WRP_BIN}"

# Create ~/.wrp directory if it doesn't exist
WRP_DIR="${HOME}/.wrp"
CONFIG_FILE="${WRP_DIR}/config"
BACKUP_FILE="${CONFIG_FILE}.backup_$$"

mkdir -p "${WRP_DIR}"

# Backup existing config file if it exists
if [ -f "${CONFIG_FILE}" ]; then
    echo "Backing up existing config to ${BACKUP_FILE}"
    cp "${CONFIG_FILE}" "${BACKUP_FILE}"
fi

# Cleanup function to restore config
cleanup() {
    echo ""
    echo "Cleaning up..."
    if [ -f "${BACKUP_FILE}" ]; then
        echo "Restoring original config"
        mv "${BACKUP_FILE}" "${CONFIG_FILE}"
    else
        # Remove test config if no backup exists
        rm -f "${CONFIG_FILE}"
    fi
    # Clean up test data
    rm -f datahub_test_dataset
    rm -rf .blackhole
}

# Register cleanup on exit
trap cleanup EXIT INT TERM

echo ""
echo "Step 1: Creating test config file"
echo "Config file: ${CONFIG_FILE}"
cat > "${CONFIG_FILE}" << 'EOF'
# Test configuration for DataHub integration
MetaStore DataHub
EOF

echo -e "${GREEN}✓ Config file created${NC}"

echo ""
echo "Step 2: Checking DataHub availability"
if command -v curl &> /dev/null; then
    if curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/health | grep -q "200"; then
        echo -e "${GREEN}✓ DataHub is available at http://localhost:8080${NC}"
        DATAHUB_AVAILABLE=true
    else
        echo -e "${YELLOW}⚠ DataHub is not available at http://localhost:8080${NC}"
        echo -e "${YELLOW}  Test will verify config detection but skip API registration${NC}"
        DATAHUB_AVAILABLE=false
    fi
else
    echo -e "${YELLOW}⚠ curl not available, skipping DataHub health check${NC}"
    DATAHUB_AVAILABLE=false
fi

echo ""
echo "Step 3: Running wrp with DataHub config"
echo "Command: ${WRP_BIN} put ${YAML_FILE}"

# Run wrp and capture output
OUTPUT=$(${WRP_BIN} put "${YAML_FILE}" 2>&1) || TEST_EXIT_CODE=$?

echo "${OUTPUT}"

# Check for expected output
echo ""
echo "Step 4: Verifying test results"

# Test 1: Check if DataHub config was detected
if echo "${OUTPUT}" | grep -q "DataHub metastore detected in config"; then
    echo -e "${GREEN}✓ Test 1 PASSED: DataHub config was detected${NC}"
    TEST1_PASSED=true
else
    echo -e "${RED}✗ Test 1 FAILED: DataHub config not detected${NC}"
    TEST1_PASSED=false
fi

# Test 2: Check if registration was attempted when DataHub is available
if [ "${DATAHUB_AVAILABLE}" = true ]; then
    if echo "${OUTPUT}" | grep -q "Registering 'datahub_test_dataset' with DataHub"; then
        echo -e "${GREEN}✓ Test 2 PASSED: DataHub registration was attempted${NC}"
        TEST2_PASSED=true
    else
        echo -e "${RED}✗ Test 2 FAILED: DataHub registration not attempted${NC}"
        TEST2_PASSED=false
    fi
else
    echo -e "${YELLOW}⊘ Test 2 SKIPPED: DataHub not available${NC}"
    TEST2_PASSED=true  # Don't fail if DataHub is not available
fi

# Test 3: Verify wrp completed successfully (regardless of DataHub registration)
if [ -z "${TEST_EXIT_CODE}" ] || [ "${TEST_EXIT_CODE}" -eq 0 ]; then
    echo -e "${GREEN}✓ Test 3 PASSED: wrp completed successfully${NC}"
    TEST3_PASSED=true
else
    echo -e "${RED}✗ Test 3 FAILED: wrp exited with code ${TEST_EXIT_CODE}${NC}"
    TEST3_PASSED=false
fi

echo ""
echo "========================================="
echo "Test Summary"
echo "========================================="

# Overall result
if [ "${TEST1_PASSED}" = true ] && [ "${TEST2_PASSED}" = true ] && [ "${TEST3_PASSED}" = true ]; then
    echo -e "${GREEN}ALL TESTS PASSED${NC}"
    exit 0
else
    echo -e "${RED}SOME TESTS FAILED${NC}"
    exit 1
fi
