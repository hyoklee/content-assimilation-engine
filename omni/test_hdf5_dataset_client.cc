///
/// test_hdf5_dataset_client.cpp
/// Comprehensive unit tests for HDF5 dataset client functionality
///

#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <vector>
#include <hdf5.h>
#include <sys/stat.h>  // For chmod
#include "format/hdf5_dataset_client.h"
#include "format/dataset_config.h"
#include "format/format_client.h"

using namespace cae;

// Helper function to run a test case
void test_assert(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "[PASS] " << test_name << std::endl;
    } else {
        std::cerr << "[FAIL] " << test_name << std::endl;
        exit(1);
    }
}

// Helper to create a test HDF5 file with sample datasets
std::string create_test_hdf5_file() {
    std::string filename = "test_hdf5_file.h5";

    // Create file
    hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        std::cerr << "Failed to create test HDF5 file" << std::endl;
        return "";
    }

    // Create a simple 1D integer dataset
    {
        hsize_t dims[1] = {100};
        hid_t dataspace_id = H5Screate_simple(1, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/int_dataset", H5T_NATIVE_INT,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<int> data(100);
        for (int i = 0; i < 100; ++i) {
            data[i] = i * 10;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    // Create a 2D double dataset
    {
        hsize_t dims[2] = {10, 20};
        hid_t dataspace_id = H5Screate_simple(2, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/double_dataset", H5T_NATIVE_DOUBLE,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<double> data(200);
        for (int i = 0; i < 200; ++i) {
            data[i] = i * 1.5;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    // Create a float dataset
    {
        hsize_t dims[1] = {50};
        hid_t dataspace_id = H5Screate_simple(1, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/float_dataset", H5T_NATIVE_FLOAT,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<float> data(50);
        for (int i = 0; i < 50; ++i) {
            data[i] = i * 2.5f;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    // Create a long dataset
    {
        hsize_t dims[1] = {30};
        hid_t dataspace_id = H5Screate_simple(1, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/long_dataset", H5T_NATIVE_LONG,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<long> data(30);
        for (int i = 0; i < 30; ++i) {
            data[i] = i * 100L;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_LONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    // Create int64 dataset
    {
        hsize_t dims[1] = {40};
        hid_t dataspace_id = H5Screate_simple(1, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/int64_dataset", H5T_NATIVE_INT64,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<int64_t> data(40);
        for (int i = 0; i < 40; ++i) {
            data[i] = i * 1000LL;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    // Create uint64 dataset
    {
        hsize_t dims[1] = {40};
        hid_t dataspace_id = H5Screate_simple(1, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/uint64_dataset", H5T_NATIVE_UINT64,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<uint64_t> data(40);
        for (int i = 0; i < 40; ++i) {
            data[i] = i * 2000ULL;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    // Create a string dataset
    {
        hsize_t dims[1] = {5};
        hid_t dataspace_id = H5Screate_simple(1, dims, NULL);

        // Create fixed-length string datatype
        hid_t string_type = H5Tcopy(H5T_C_S1);
        H5Tset_size(string_type, 20);

        hid_t dataset_id = H5Dcreate2(file_id, "/string_dataset", string_type,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        char data[5][20] = {"hello", "world", "test", "data", "string"};
        H5Dwrite(dataset_id, string_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

        H5Dclose(dataset_id);
        H5Tclose(string_type);
        H5Sclose(dataspace_id);
    }

    // Create a large dataset for testing partial printing (> 100 elements)
    {
        hsize_t dims[1] = {500};
        hid_t dataspace_id = H5Screate_simple(1, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/large_dataset", H5T_NATIVE_INT,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<int> data(500);
        for (int i = 0; i < 500; ++i) {
            data[i] = i;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    // Create a 2D large dataset for testing partial printing
    {
        hsize_t dims[2] = {50, 50};
        hid_t dataspace_id = H5Screate_simple(2, dims, NULL);
        hid_t dataset_id = H5Dcreate2(file_id, "/large_2d_dataset", H5T_NATIVE_INT,
                                     dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        std::vector<int> data(2500);
        for (int i = 0; i < 2500; ++i) {
            data[i] = i;
        }
        H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
    }

    H5Fclose(file_id);
    return filename;
}

// Test Import method
void test_import() {
    std::cout << "\n=== Testing Import() ===" << std::endl;

    Hdf5DatasetClient client;
    FormatContext ctx;
    ctx.filename_ = "test_file.h5";
    ctx.size_ = 1000;
    ctx.offset_ = 0;

    // This should just print info without errors
    client.Import(ctx);

    std::cout << "[PASS] Import method executed" << std::endl;
}

// Test Describe method
void test_describe() {
    std::cout << "\n=== Testing Describe() ===" << std::endl;

    Hdf5DatasetClient client;
    FormatContext ctx;
    ctx.filename_ = "test_file.h5";
    ctx.size_ = 2000;
    ctx.offset_ = 100;

    std::string desc = client.Describe(ctx);
    test_assert(desc.find("test_file.h5") != std::string::npos, "Describe contains filename");
    test_assert(desc.find("2000") != std::string::npos, "Describe contains size");
    test_assert(desc.find("100") != std::string::npos, "Describe contains offset");
}

// Test ReadDataset with valid file and dataset
void test_read_dataset_valid() {
    std::cout << "\n=== Testing ReadDataset() with valid data ===" << std::endl;

    std::string test_file = create_test_hdf5_file();
    test_assert(!test_file.empty(), "Test HDF5 file created");

    Hdf5DatasetClient client;
    DatasetConfig config;
    config.name = "test_read";
    config.uri = "hdf5://" + test_file + "/int_dataset";
    config.start = {0};
    config.count = {10};
    config.stride = {1};

    size_t buffer_size = 0;
    unsigned char* buffer = client.ReadDataset(config, buffer_size);

    test_assert(buffer != nullptr, "ReadDataset returned valid buffer");
    test_assert(buffer_size == 10 * sizeof(int), "Buffer size is correct");

    // Verify data
    int* data = reinterpret_cast<int*>(buffer);
    test_assert(data[0] == 0, "First element is correct");
    test_assert(data[9] == 90, "Last element is correct");

    delete[] buffer;
    std::remove(test_file.c_str());

    std::cout << "[PASS] ReadDataset with valid data" << std::endl;
}

// Test ReadDataset with invalid URI
void test_read_dataset_invalid_uri() {
    std::cout << "\n=== Testing ReadDataset() with invalid URI ===" << std::endl;

    Hdf5DatasetClient client;
    DatasetConfig config;
    config.name = "test_invalid";
    config.uri = "invalid://uri/format";
    config.start = {0};
    config.count = {10};
    config.stride = {1};

    size_t buffer_size = 0;
    unsigned char* buffer = client.ReadDataset(config, buffer_size);

    test_assert(buffer == nullptr, "ReadDataset with invalid URI returns nullptr");

    std::cout << "[PASS] ReadDataset with invalid URI" << std::endl;
}

// Test ReadDataset with non-existent file
void test_read_dataset_nonexistent_file() {
    std::cout << "\n=== Testing ReadDataset() with non-existent file ===" << std::endl;

    Hdf5DatasetClient client;
    DatasetConfig config;
    config.name = "test_nofile";
    config.uri = "hdf5://nonexistent_file.h5/dataset";
    config.start = {0};
    config.count = {10};
    config.stride = {1};

    size_t buffer_size = 0;
    unsigned char* buffer = client.ReadDataset(config, buffer_size);

    test_assert(buffer == nullptr, "ReadDataset with non-existent file returns nullptr");

    std::cout << "[PASS] ReadDataset with non-existent file" << std::endl;
}

// Test ReadDataset with invalid dataset name
void test_read_dataset_invalid_dataset() {
    std::cout << "\n=== Testing ReadDataset() with invalid dataset name ===" << std::endl;

    std::string test_file = create_test_hdf5_file();

    Hdf5DatasetClient client;
    DatasetConfig config;
    config.name = "test_invalid_ds";
    config.uri = "hdf5://" + test_file + "/nonexistent_dataset";
    config.start = {0};
    config.count = {10};
    config.stride = {1};

    size_t buffer_size = 0;
    unsigned char* buffer = client.ReadDataset(config, buffer_size);

    test_assert(buffer == nullptr, "ReadDataset with invalid dataset returns nullptr");

    std::remove(test_file.c_str());

    std::cout << "[PASS] ReadDataset with invalid dataset" << std::endl;
}

// Test ExecuteRunScript with successful execution
void test_execute_run_script_success() {
    std::cout << "\n=== Testing ExecuteRunScript() with success ===" << std::endl;

    // Create a simple success script
    std::string script_path = "test_success_script.sh";
    std::ofstream script(script_path);
    script << "#!/bin/bash\n";
    script << "echo 'Script executed successfully'\n";
    script << "exit 0\n";
    script.close();

    chmod(script_path.c_str(), 0755);

    Hdf5DatasetClient client;
    client.ExecuteRunScript(script_path, "input.txt", "output.txt");

    std::remove(script_path.c_str());

    std::cout << "[PASS] ExecuteRunScript with success" << std::endl;
}

// Test ExecuteRunScript with failure
void test_execute_run_script_failure() {
    std::cout << "\n=== Testing ExecuteRunScript() with failure ===" << std::endl;

    // Create a failing script
    std::string script_path = "test_fail_script.sh";
    std::ofstream script(script_path);
    script << "#!/bin/bash\n";
    script << "echo 'Script failed'\n";
    script << "exit 1\n";
    script.close();

    chmod(script_path.c_str(), 0755);

    Hdf5DatasetClient client;
    client.ExecuteRunScript(script_path, "input.txt", "output.txt");

    std::remove(script_path.c_str());

    std::cout << "[PASS] ExecuteRunScript with failure" << std::endl;
}

// Test with different data types (double, float, long, int64, uint64, string)
void test_different_datatypes() {
    std::cout << "\n=== Testing different data types ===" << std::endl;

    std::string test_file = create_test_hdf5_file();
    Hdf5DatasetClient client;

    // Test double dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/double_dataset";
        config.start = {0, 0};
        config.count = {2, 5};
        config.stride = {1, 1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "Double dataset read");
        test_assert(buffer_size == 10 * sizeof(double), "Double buffer size correct");

        delete[] buffer;
    }

    // Test float dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/float_dataset";
        config.start = {0};
        config.count = {10};
        config.stride = {1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "Float dataset read");
        test_assert(buffer_size == 10 * sizeof(float), "Float buffer size correct");

        delete[] buffer;
    }

    // Test long dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/long_dataset";
        config.start = {0};
        config.count = {15};
        config.stride = {1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "Long dataset read");
        test_assert(buffer_size == 15 * sizeof(long), "Long buffer size correct");

        delete[] buffer;
    }

    // Test int64 dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/int64_dataset";
        config.start = {0};
        config.count = {20};
        config.stride = {1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "Int64 dataset read");
        test_assert(buffer_size == 20 * sizeof(int64_t), "Int64 buffer size correct");

        delete[] buffer;
    }

    // Test uint64 dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/uint64_dataset";
        config.start = {0};
        config.count = {25};
        config.stride = {1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "Uint64 dataset read");
        test_assert(buffer_size == 25 * sizeof(uint64_t), "Uint64 buffer size correct");

        delete[] buffer;
    }

    // Test string dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/string_dataset";
        config.start = {0};
        config.count = {3};
        config.stride = {1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "String dataset read");
        test_assert(buffer_size == 3 * 20, "String buffer size correct");

        delete[] buffer;
    }

    std::remove(test_file.c_str());

    std::cout << "[PASS] Different data types" << std::endl;
}

// Test printing large datasets (> 100 elements)
void test_large_dataset_printing() {
    std::cout << "\n=== Testing large dataset printing ===" << std::endl;

    std::string test_file = create_test_hdf5_file();
    Hdf5DatasetClient client;

    // Test 1D large dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/large_dataset";
        config.start = {0};
        config.count = {150};
        config.stride = {1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "Large 1D dataset read");
        delete[] buffer;
    }

    // Test 2D large dataset
    {
        DatasetConfig config;
        config.uri = "hdf5://" + test_file + "/large_2d_dataset";
        config.start = {0, 0};
        config.count = {10, 15};
        config.stride = {1, 1};

        size_t buffer_size = 0;
        unsigned char* buffer = client.ReadDataset(config, buffer_size);

        test_assert(buffer != nullptr, "Large 2D dataset read");
        delete[] buffer;
    }

    std::remove(test_file.c_str());

    std::cout << "[PASS] Large dataset printing" << std::endl;
}

int main() {
    std::cout << "Starting HDF5 Dataset Client Comprehensive Unit Tests" << std::endl;
    std::cout << "======================================================" << std::endl;

    try {
        // Test basic methods
        test_import();
        test_describe();

        // Test ReadDataset with various scenarios
        test_read_dataset_valid();
        test_read_dataset_invalid_uri();
        test_read_dataset_nonexistent_file();
        test_read_dataset_invalid_dataset();

        // Test ExecuteRunScript
        test_execute_run_script_success();
        test_execute_run_script_failure();

        // Test different data types
        test_different_datatypes();

        // Test large datasets
        test_large_dataset_printing();

        std::cout << "\n======================================================" << std::endl;
        std::cout << "All HDF5 Dataset Client tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
