///
/// test_datahub.cpp
/// Unit test for DataHub integration functions
///

#include "OMNI.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#else
#include <windows.h>
#include <shlobj.h>
#endif

// Helper function to get home directory
std::string get_home_dir() {
#ifdef _WIN32
    char* home_path = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&home_path, &len, "USERPROFILE");
    if (err == 0 && home_path != nullptr) {
        std::string result = home_path;
        free(home_path);
        return result;
    }
    return "";
#else
    const char* home_path = std::getenv("HOME");
    if (home_path == nullptr) {
        struct passwd* pw = getpwuid(getuid());
        if (pw == nullptr) {
            return "";
        }
        return pw->pw_dir;
    }
    return home_path;
#endif
}

// Helper function to create directory
bool create_directory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

// Test 1: CheckDataHubConfig returns false when config doesn't exist
bool test_config_not_exists() {
    std::cout << "Test 1: Config file doesn't exist..." << std::flush;

    cae::OMNI omni;
    omni.SetQuiet(true);

    // Ensure config doesn't exist (back it up if it does)
    std::string home = get_home_dir();
    std::string config_path = home + "/.wrp/config";
    std::string backup_path = config_path + ".test_backup";

    bool had_config = false;
    std::ifstream check(config_path);
    if (check.is_open()) {
        check.close();
        std::rename(config_path.c_str(), backup_path.c_str());
        had_config = true;
    }

    bool result = omni.CheckDataHubConfig();

    // Restore backup if it existed
    if (had_config) {
        std::rename(backup_path.c_str(), config_path.c_str());
    }

    if (result == false) {
        std::cout << " PASSED" << std::endl;
        return true;
    } else {
        std::cout << " FAILED" << std::endl;
        return false;
    }
}

// Test 2: CheckDataHubConfig returns false when config exists but doesn't contain "MetaStore DataHub"
bool test_config_no_metastore() {
    std::cout << "Test 2: Config exists but no MetaStore DataHub..." << std::flush;

    cae::OMNI omni;
    omni.SetQuiet(true);

    std::string home = get_home_dir();
    std::string wrp_dir = home + "/.wrp";
    std::string config_path = wrp_dir + "/config";
    std::string backup_path = config_path + ".test_backup";

    // Create .wrp directory if needed
    create_directory(wrp_dir);

    // Backup existing config
    bool had_config = false;
    std::ifstream check(config_path);
    if (check.is_open()) {
        check.close();
        std::rename(config_path.c_str(), backup_path.c_str());
        had_config = true;
    }

    // Create test config without MetaStore DataHub
    std::ofstream config(config_path);
    config << "# Test config\n";
    config << "Some other setting\n";
    config.close();

    bool result = omni.CheckDataHubConfig();

    // Cleanup
    std::remove(config_path.c_str());
    if (had_config) {
        std::rename(backup_path.c_str(), config_path.c_str());
    }

    if (result == false) {
        std::cout << " PASSED" << std::endl;
        return true;
    } else {
        std::cout << " FAILED" << std::endl;
        return false;
    }
}

// Test 3: CheckDataHubConfig returns true when config contains "MetaStore DataHub"
bool test_config_with_metastore() {
    std::cout << "Test 3: Config contains MetaStore DataHub..." << std::flush;

    cae::OMNI omni;
    omni.SetQuiet(true);

    std::string home = get_home_dir();
    std::string wrp_dir = home + "/.wrp";
    std::string config_path = wrp_dir + "/config";
    std::string backup_path = config_path + ".test_backup";

    // Create .wrp directory if needed
    create_directory(wrp_dir);

    // Backup existing config
    bool had_config = false;
    std::ifstream check(config_path);
    if (check.is_open()) {
        check.close();
        std::rename(config_path.c_str(), backup_path.c_str());
        had_config = true;
    }

    // Create test config with MetaStore DataHub
    std::ofstream config(config_path);
    config << "# Test config\n";
    config << "MetaStore DataHub\n";
    config << "Other settings\n";
    config.close();

    bool result = omni.CheckDataHubConfig();

    // Cleanup
    std::remove(config_path.c_str());
    if (had_config) {
        std::rename(backup_path.c_str(), config_path.c_str());
    }

    if (result == true) {
        std::cout << " PASSED" << std::endl;
        return true;
    } else {
        std::cout << " FAILED" << std::endl;
        return false;
    }
}

// Test 4: ReadConfigFile returns empty string for non-existent file
bool test_read_config_not_exists() {
    std::cout << "Test 4: ReadConfigFile with non-existent file..." << std::flush;

    cae::OMNI omni;
    omni.SetQuiet(true);

    std::string result = omni.ReadConfigFile("/nonexistent/path/config");

    if (result.empty()) {
        std::cout << " PASSED" << std::endl;
        return true;
    } else {
        std::cout << " FAILED" << std::endl;
        return false;
    }
}

// Test 5: ReadConfigFile returns content for existing file
bool test_read_config_exists() {
    std::cout << "Test 5: ReadConfigFile with existing file..." << std::flush;

    cae::OMNI omni;
    omni.SetQuiet(true);

    // Create a temporary test file
#ifdef _WIN32
    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    std::string test_file = std::string(temp_path) + "test_config_" + std::to_string(GetCurrentProcessId());
#else
    std::string test_file = "/tmp/test_config_" + std::to_string(getpid());
#endif
    std::ofstream temp(test_file);
    temp << "Test content\nLine 2\n";
    temp.close();

    std::string result = omni.ReadConfigFile(test_file);

    // Cleanup
    std::remove(test_file.c_str());

    if (result.find("Test content") != std::string::npos &&
        result.find("Line 2") != std::string::npos) {
        std::cout << " PASSED" << std::endl;
        return true;
    } else {
        std::cout << " FAILED" << std::endl;
        return false;
    }
}

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "DataHub Unit Tests" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << std::endl;

    int passed = 0;
    int failed = 0;

    if (test_config_not_exists()) passed++; else failed++;
    if (test_config_no_metastore()) passed++; else failed++;
    if (test_config_with_metastore()) passed++; else failed++;
    if (test_read_config_not_exists()) passed++; else failed++;
    if (test_read_config_exists()) passed++; else failed++;

    std::cout << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Test Results" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << std::endl;

    if (failed == 0) {
        std::cout << "ALL TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "SOME TESTS FAILED!" << std::endl;
        return 1;
    }
}
