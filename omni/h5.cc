#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "omni_processing.h"
#include "format/hdf5_dataset_client.h"

namespace cae {

bool ParseHdf5Uri(const std::string& uri, std::string& file_path, std::string& dataset_name) {
    const std::string prefix = "hdf5://";
    if (uri.find(prefix) != 0) {
        return false;
    }
    
    size_t path_end = uri.find(':', prefix.length());
    if (path_end == std::string::npos) {
        return false;
    }
    
    file_path = uri.substr(prefix.length(), path_end - prefix.length());
    dataset_name = uri.substr(path_end + 1);
    return !file_path.empty() && !dataset_name.empty();
}

} // namespace cae

void ProcessHdf5DataEntry(const OmniJobConfig::DataEntry &entry) {
  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << "Processing HDF5 Data Entry" << std::endl;
  std::cout << std::string(50, '=') << std::endl;
  std::string file_path, dataset_name;

#ifdef USE_HDF5
  std::cerr << "Error: " << entry.src << std::endl;

  // Parse the HDF5 source
  if (!cae::ParseHdf5Uri(entry.src, file_path, dataset_name)) {
    std::cerr << "Error: Invalid HDF5 source format: " << entry.src << std::endl;
    return;
  }

  std::cout << "File path: " << file_path << std::endl;
  std::cout << "Dataset name: " << dataset_name << std::endl;

  // Create HDF5 dataset client and read the dataset
  cae::Hdf5DatasetClient client;

  // Create DatasetConfig from entry
  cae::DatasetConfig config;
  config.name =std::string("dataset");
  config.uri = entry.src;
  // Convert from hsize_t to uint64_t
  config.start.assign(entry.hdf5_start.begin(), entry.hdf5_start.end());
  config.count.assign(entry.hdf5_count.begin(), entry.hdf5_count.end());
  config.stride.assign(entry.hdf5_stride.begin(), entry.hdf5_stride.end());
  config.run_script = entry.run_script;
  config.destination = entry.destination;
  
  try {
    size_t buffer_size = 0;
    unsigned char* buffer = client.ReadDataset(config, buffer_size);
    if (buffer) {
      std::cout << "✓ Successfully read HDF5 dataset, size: " << buffer_size << " bytes" << std::endl;
      // TODO: Process the buffer if needed
      delete[] buffer; // Don't forget to free the allocated memory
    } else {
      std::cerr << "✗ Failed to read HDF5 dataset" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "✗ Failed to process HDF5 dataset: " << e.what() << std::endl;
  }
#else
  (void)config; // Silence unused variable warning when HDF5 is disabled
#endif // USE_HDF5
  
}
