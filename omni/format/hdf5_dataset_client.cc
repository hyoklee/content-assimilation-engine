#include "hdf5_dataset_client.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <cstdint>

namespace cae {

void Hdf5DatasetClient::Import(const FormatContext &ctx) {
  std::cout << "HDF5 Dataset Import called for file: " << ctx.filename_ << std::endl;
  std::cout << "Size: " << ctx.size_ << " bytes" << std::endl;
  std::cout << "Offset: " << ctx.offset_ << " bytes" << std::endl;
  
  // This method is kept for compatibility with FormatClient interface
  // The main functionality is in ReadDataset method
}

void Hdf5DatasetClient::ReadDataset(const DatasetConfig& config) {
  std::cout << "Reading dataset: " << config.name << std::endl;
  std::cout << "URI: " << config.uri << std::endl;
  
  // Parse the HDF5 URI
  std::string file_path, dataset_name;
  if (!ParseHdf5Uri(config.uri, file_path, dataset_name)) {
    std::cerr << "Error: Invalid HDF5 URI format: " << config.uri << std::endl;
    return;
  }
  
  std::cout << "File path: " << file_path << std::endl;
  std::cout << "Dataset name: " << dataset_name << std::endl;
  
  // Open HDF5 file
  hid_t file_id = OpenHdf5File(file_path);
  if (file_id < 0) {
    std::cerr << "Error: Failed to open HDF5 file: " << file_path << std::endl;
    return;
  }
  
  // Get dataset information
  std::vector<hsize_t> dimensions;
  hid_t datatype;
  if (!GetDatasetInfo(file_id, dataset_name, dimensions, datatype)) {
    std::cerr << "Error: Failed to get dataset info for: " << dataset_name << std::endl;
    CloseHdf5File(file_id);
    return;
  }
  
  std::cout << "Dataset dimensions: ";
  for (size_t i = 0; i < dimensions.size(); ++i) {
    if (i > 0) std::cout << " x ";
    std::cout << dimensions[i];
  }
  std::cout << std::endl;
  
  // Allocate buffer for the hyperslab
  auto buffer = AllocateBuffer(config.count, datatype);
  if (!buffer) {
    std::cerr << "Error: Failed to allocate buffer for dataset" << std::endl;
    CloseHdf5File(file_id);
    return;
  }
  
  // Read the hyperslab
  if (!ReadDatasetHyperslab(file_id, dataset_name, config.start, config.count, config.stride, buffer.get(), datatype)) {
    std::cerr << "Error: Failed to read dataset hyperslab" << std::endl;
    CloseHdf5File(file_id);
    return;
  }
  
  std::cout << "Successfully read dataset hyperslab" << std::endl;
  
  // Print hyperslab values after reading
  PrintHyperslabValues(buffer.get(), config.count, datatype, dataset_name);
  
  // Call callback
  OnDatasetRead(dataset_name, config.count);
  
  // Execute run script if specified
  if (!config.run_script.empty()) {
    std::string temp_input_file = "/tmp/" + config.name + "_input.h5";
    std::string temp_output_file = "/tmp/" + config.name + "_output.parquet";
    
    // TODO: Save the hyperslab data to a temporary HDF5 file
    // For now, we'll just call the script with placeholder files
    ExecuteRunScript(config.run_script, temp_input_file, temp_output_file);
  }
  
  CloseHdf5File(file_id);
}

void Hdf5DatasetClient::ExecuteRunScript(const std::string& script_path, 
                                        const std::string& input_file, 
                                        const std::string& output_file) {
  std::cout << "Executing run script: " << script_path << std::endl;
  std::cout << "Input file: " << input_file << std::endl;
  std::cout << "Output file: " << output_file << std::endl;
  
  // Build command
  std::ostringstream cmd;
  cmd << script_path << " " << input_file << " " << output_file;
  
  // Execute the script
  int result = std::system(cmd.str().c_str());
  if (result == 0) {
    std::cout << "Run script executed successfully" << std::endl;
  } else {
    std::cerr << "Run script failed with exit code: " << result << std::endl;
  }
}

hid_t Hdf5DatasetClient::OpenHdf5File(const std::string& file_path) {
  // Check if file exists
  if (!std::filesystem::exists(file_path)) {
    std::cerr << "Error: File does not exist: " << file_path << std::endl;
    return -1;
  }
  
  // Open HDF5 file for reading
  hid_t file_id = H5Fopen(file_path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file_id < 0) {
    std::cerr << "Error: Failed to open HDF5 file: " << file_path << std::endl;
    return -1;
  }
  
  return file_id;
}

void Hdf5DatasetClient::CloseHdf5File(hid_t file_id) {
  if (file_id >= 0) {
    H5Fclose(file_id);
  }
}

bool Hdf5DatasetClient::GetDatasetInfo(hid_t file_id, const std::string& dataset_name,
                                      std::vector<hsize_t>& dimensions, hid_t& datatype) {
  // Open dataset
  hid_t dataset_id = H5Dopen2(file_id, dataset_name.c_str(), H5P_DEFAULT);
  if (dataset_id < 0) {
    std::cerr << "Error: Failed to open dataset: " << dataset_name << std::endl;
    return false;
  }
  
  // Print dataset values after opening
  PrintDatasetValues(dataset_id, dataset_name);
  
  // Get dataspace
  hid_t dataspace_id = H5Dget_space(dataset_id);
  if (dataspace_id < 0) {
    std::cerr << "Error: Failed to get dataspace for dataset: " << dataset_name << std::endl;
    H5Dclose(dataset_id);
    return false;
  }
  
  // Get dimensions
  int rank = H5Sget_simple_extent_ndims(dataspace_id);
  if (rank < 0) {
    std::cerr << "Error: Failed to get dataset rank" << std::endl;
    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
    return false;
  }
  
  dimensions.resize(rank);
  H5Sget_simple_extent_dims(dataspace_id, dimensions.data(), nullptr);
  
  // Get datatype
  datatype = H5Dget_type(dataset_id);
  if (datatype < 0) {
    std::cerr << "Error: Failed to get dataset datatype" << std::endl;
    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
    return false;
  }
  
  H5Sclose(dataspace_id);
  H5Dclose(dataset_id);
  
  return true;
}

bool Hdf5DatasetClient::ReadDatasetHyperslab(hid_t file_id, const std::string& dataset_name,
                                            const std::vector<hsize_t>& start,
                                            const std::vector<hsize_t>& count,
                                            const std::vector<hsize_t>& stride,
                                            void* buffer, hid_t datatype) {
  // Open dataset
  hid_t dataset_id = H5Dopen2(file_id, dataset_name.c_str(), H5P_DEFAULT);
  if (dataset_id < 0) {
    std::cerr << "Error: Failed to open dataset: " << dataset_name << std::endl;
    return false;
  }
  
  // Print dataset values after opening
  PrintDatasetValues(dataset_id, dataset_name);
  
  // Get dataspace
  hid_t dataspace_id = H5Dget_space(dataset_id);
  if (dataspace_id < 0) {
    std::cerr << "Error: Failed to get dataspace for dataset: " << dataset_name << std::endl;
    H5Dclose(dataset_id);
    return false;
  }
  
  // Select hyperslab
  herr_t status = H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, 
                                     start.data(), stride.data(), count.data(), nullptr);
  if (status < 0) {
    std::cerr << "Error: Failed to select hyperslab" << std::endl;
    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
    return false;
  }
  
  // Create memory dataspace
  hid_t memspace_id = H5Screate_simple(count.size(), count.data(), nullptr);
  if (memspace_id < 0) {
    std::cerr << "Error: Failed to create memory dataspace" << std::endl;
    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
    return false;
  }
  
  // Read data
  status = H5Dread(dataset_id, datatype, memspace_id, dataspace_id, H5P_DEFAULT, buffer);
  if (status < 0) {
    std::cerr << "Error: Failed to read dataset" << std::endl;
    H5Sclose(memspace_id);
    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
    return false;
  }
  
  H5Sclose(memspace_id);
  H5Sclose(dataspace_id);
  H5Dclose(dataset_id);
  
  return true;
}

std::unique_ptr<char[]> Hdf5DatasetClient::AllocateBuffer(const std::vector<hsize_t>& dimensions, hid_t datatype) {
  size_t total_size = CalculateDatasetSize(dimensions, datatype);
  if (total_size == 0) {
    return nullptr;
  }
  
  return std::make_unique<char[]>(total_size);
}

size_t Hdf5DatasetClient::CalculateDatasetSize(const std::vector<hsize_t>& dimensions, hid_t datatype) {
  // Get size of datatype
  size_t type_size = H5Tget_size(datatype);
  if (type_size == 0) {
    std::cerr << "Error: Failed to get datatype size" << std::endl;
    return 0;
  }
  
  // Calculate total size
  size_t total_size = type_size;
  for (hsize_t dim : dimensions) {
    total_size *= dim;
  }
  
  return total_size;
}

void Hdf5DatasetClient::PrintDatasetValues(hid_t dataset_id, const std::string& dataset_name) {
  std::cout << "=== Dataset Values for: " << dataset_name << " ===" << std::endl;
  
  // Get dataspace
  hid_t dataspace_id = H5Dget_space(dataset_id);
  if (dataspace_id < 0) {
    std::cerr << "Error: Failed to get dataspace for printing values" << std::endl;
    return;
  }
  
  // Get dimensions
  int rank = H5Sget_simple_extent_ndims(dataspace_id);
  if (rank < 0) {
    std::cerr << "Error: Failed to get dataset rank for printing" << std::endl;
    H5Sclose(dataspace_id);
    return;
  }
  
  std::vector<hsize_t> dims(rank);
  H5Sget_simple_extent_dims(dataspace_id, dims.data(), nullptr);
  
  // Get datatype
  hid_t datatype = H5Dget_type(dataset_id);
  if (datatype < 0) {
    std::cerr << "Error: Failed to get dataset datatype for printing" << std::endl;
    H5Sclose(dataspace_id);
    return;
  }
  
  // Calculate total size
  size_t total_elements = 1;
  for (hsize_t dim : dims) {
    total_elements *= dim;
  }
  
  // Limit the number of elements to print to avoid overwhelming output
  const size_t max_elements_to_print = 100;
  size_t elements_to_print = std::min(total_elements, max_elements_to_print);
  
  // Allocate buffer for reading
  size_t type_size = H5Tget_size(datatype);
  std::vector<char> buffer(elements_to_print * type_size);
  
  // Create memory dataspace for reading
  std::vector<hsize_t> read_dims = dims;
  if (total_elements > max_elements_to_print) {
    // For large datasets, read only the first few elements
    // Calculate how many elements to read from the first dimension
    size_t elements_per_dim = 1;
    for (size_t i = 1; i < dims.size(); ++i) {
      elements_per_dim *= dims[i];
    }
    if (elements_per_dim > 0) {
      size_t max_first_dim = max_elements_to_print / elements_per_dim;
      read_dims[0] = std::min(dims[0], static_cast<hsize_t>(max_first_dim));
    } else {
      read_dims[0] = std::min(dims[0], static_cast<hsize_t>(max_elements_to_print));
    }
  }
  
  hid_t memspace_id = H5Screate_simple(read_dims.size(), read_dims.data(), nullptr);
  if (memspace_id < 0) {
    std::cerr << "Error: Failed to create memory dataspace for printing" << std::endl;
    H5Tclose(datatype);
    H5Sclose(dataspace_id);
    return;
  }
  
  // Select hyperslab from file dataspace to match memory dataspace
  std::vector<hsize_t> start(dims.size(), 0);  // Start at origin
  std::vector<hsize_t> stride(dims.size(), 1); // Unit stride
  std::vector<hsize_t> count = read_dims;      // Count matches memory dataspace
  
  herr_t status = H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, 
                                     start.data(), stride.data(), count.data(), nullptr);
  if (status < 0) {
    std::cerr << "Error: Failed to select hyperslab for printing" << std::endl;
    H5Sclose(memspace_id);
    H5Tclose(datatype);
    H5Sclose(dataspace_id);
    return;
  }
  
  // Read data
  status = H5Dread(dataset_id, datatype, memspace_id, dataspace_id, H5P_DEFAULT, buffer.data());
  if (status < 0) {
    std::cerr << "Error: Failed to read dataset for printing" << std::endl;
    H5Sclose(memspace_id);
    H5Tclose(datatype);
    H5Sclose(dataspace_id);
    return;
  }
  
  // Print values based on datatype
  std::cout << "Dataset dimensions: ";
  for (size_t i = 0; i < dims.size(); ++i) {
    if (i > 0) std::cout << " x ";
    std::cout << dims[i];
  }
  std::cout << std::endl;
  
  std::cout << "First " << elements_to_print << " values: ";
  
  // Determine the native type and print accordingly
  H5T_class_t type_class = H5Tget_class(datatype);
  switch (type_class) {
    case H5T_INTEGER: {
      if (H5Tequal(datatype, H5T_NATIVE_INT)) {
        int* data = reinterpret_cast<int*>(buffer.data());
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else if (H5Tequal(datatype, H5T_NATIVE_LONG)) {
        long* data = reinterpret_cast<long*>(buffer.data());
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else {
        // Generic integer printing
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << "int_val_" << i;
        }
      }
      break;
    }
    case H5T_FLOAT: {
      if (H5Tequal(datatype, H5T_NATIVE_DOUBLE)) {
        double* data = reinterpret_cast<double*>(buffer.data());
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else if (H5Tequal(datatype, H5T_NATIVE_FLOAT)) {
        float* data = reinterpret_cast<float*>(buffer.data());
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else {
        // Generic float printing
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << "float_val_" << i;
        }
      }
      break;
    }
    default:
      std::cout << "[Unsupported datatype for printing]";
      break;
  }
  
  if (total_elements > max_elements_to_print) {
    std::cout << " ... (showing first " << max_elements_to_print << " of " << total_elements << " elements)";
  }
  std::cout << std::endl;
  std::cout << "=== End Dataset Values ===" << std::endl;
  
  // Cleanup
  H5Sclose(memspace_id);
  H5Tclose(datatype);
  H5Sclose(dataspace_id);
}

void Hdf5DatasetClient::PrintHyperslabValues(const void* buffer, const std::vector<hsize_t>& dimensions, 
                                            hid_t datatype, const std::string& dataset_name) {
  std::cout << "=== Hyperslab Values for: " << dataset_name << " ===" << std::endl;
  
  // Calculate total number of elements in the hyperslab
  size_t total_elements = 1;
  for (hsize_t dim : dimensions) {
    total_elements *= dim;
  }
  
  std::cout << "Hyperslab dimensions: ";
  for (size_t i = 0; i < dimensions.size(); ++i) {
    if (i > 0) std::cout << " x ";
    std::cout << dimensions[i];
  }
  std::cout << std::endl;
  
  std::cout << "Total elements in hyperslab: " << total_elements << std::endl;
  
  // Print datatype information for debugging
  H5T_class_t type_class = H5Tget_class(datatype);
  std::cout << "Datatype class: " << type_class << " (0=INTEGER, 1=FLOAT, 2=STRING, 3=BITFIELD, 4=OPAQUE, 5=COMPOUND, 6=REFERENCE, 7=ENUM, 8=VLEN, 9=ARRAY)" << std::endl;
  
  // Limit the number of elements to print to avoid overwhelming output
  const size_t max_elements_to_print = 100;
  size_t elements_to_print = std::min(total_elements, max_elements_to_print);
  
  std::cout << "First " << elements_to_print << " hyperslab values: ";
  
  // Determine the native type and print accordingly
  switch (type_class) {
    case H5T_INTEGER: {
      if (H5Tequal(datatype, H5T_NATIVE_INT)) {
        const int* data = reinterpret_cast<const int*>(buffer);
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else if (H5Tequal(datatype, H5T_NATIVE_LONG)) {
        const long* data = reinterpret_cast<const long*>(buffer);
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else if (H5Tequal(datatype, H5T_NATIVE_INT64)) {
        const int64_t* data = reinterpret_cast<const int64_t*>(buffer);
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else if (H5Tequal(datatype, H5T_NATIVE_UINT64)) {
        const uint64_t* data = reinterpret_cast<const uint64_t*>(buffer);
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else {
        // Try to print as generic integer with size information
        size_t type_size = H5Tget_size(datatype);
        std::cout << "[Integer type with size " << type_size << " bytes - raw values: ";
        const unsigned char* raw_data = reinterpret_cast<const unsigned char*>(buffer);
        for (size_t i = 0; i < std::min(elements_to_print, size_t(5)); ++i) {
          if (i > 0) std::cout << ", ";
          for (size_t j = 0; j < type_size; ++j) {
            std::cout << std::hex << static_cast<int>(raw_data[i * type_size + j]) << std::dec;
          }
        }
        if (elements_to_print > 5) std::cout << ", ...";
        std::cout << "]";
      }
      break;
    }
    case H5T_FLOAT: {
      if (H5Tequal(datatype, H5T_NATIVE_DOUBLE)) {
        const double* data = reinterpret_cast<const double*>(buffer);
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else if (H5Tequal(datatype, H5T_NATIVE_FLOAT)) {
        const float* data = reinterpret_cast<const float*>(buffer);
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << data[i];
        }
      } else {
        // Generic float printing
        for (size_t i = 0; i < elements_to_print; ++i) {
          if (i > 0) std::cout << ", ";
          std::cout << "float_val_" << i;
        }
      }
      break;
    }
    default:
      std::cout << "[Unsupported datatype for printing]";
      break;
  }
  
  if (total_elements > max_elements_to_print) {
    std::cout << " ... (showing first " << max_elements_to_print << " of " << total_elements << " elements)";
  }
  std::cout << std::endl;
  std::cout << "=== End Hyperslab Values ===" << std::endl;
}

} // namespace cae
