#include "format/dataset_config.h"
#include "format/hdf5_dataset_client.h"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <config_file.yaml>" << std::endl;
    std::cerr << "Example: " << argv[0] << " config/dataset_config.yaml" << std::endl;
    std::cerr << std::endl;
    std::cerr << "HDF5 OMNI Dataset Reader - Reads datasets from HDF5 files using YAML configuration" << std::endl;
    std::cerr << "Supports hyperslab selection, dataset metadata extraction, and external processing scripts" << std::endl;
    return 1;
  }

  std::string config_file = argv[1];
  
  try {
    std::cout << "=== HDF5 OMNI Dataset Reader ===" << std::endl;
    std::cout << "Reading dataset configuration from: " << config_file << std::endl;
    std::cout << std::endl;
    
    // Parse the dataset configuration
    cae::DatasetConfig config = cae::ParseDatasetConfig(config_file);
    
    std::cout << "Configuration loaded successfully:" << std::endl;
    std::cout << "  Name: " << config.name << std::endl;
    std::cout << "  URI: " << config.uri << std::endl;
    std::cout << "  Tags: ";
    for (const auto& tag : config.tags) {
      std::cout << tag << " ";
    }
    std::cout << std::endl;
    
    std::cout << "  Start: [";
    for (size_t i = 0; i < config.start.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << config.start[i];
    }
    std::cout << "]" << std::endl;
    
    std::cout << "  Count: [";
    for (size_t i = 0; i < config.count.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << config.count[i];
    }
    std::cout << "]" << std::endl;
    
    std::cout << "  Stride: [";
    for (size_t i = 0; i < config.stride.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << config.stride[i];
    }
    std::cout << "]" << std::endl;
    
    if (!config.run_script.empty()) {
      std::cout << "  Run script: " << config.run_script << std::endl;
    }
    
    if (!config.destination.empty()) {
      std::cout << "  Destination: " << config.destination << std::endl;
    }
    
    std::cout << std::endl;
    
    // Create HDF5 dataset client and read the dataset
    cae::Hdf5DatasetClient client;
    size_t buffer_size = 0;
    unsigned char* buffer = client.ReadDataset(config, buffer_size);
    
    if (buffer) {
      std::cout << "Successfully read " << buffer_size << " bytes of data" << std::endl;
      // TODO: Process the buffer if needed
      delete[] buffer; // Clean up the allocated buffer
      std::cout << std::endl;
      std::cout << "=== Dataset processing completed successfully! ===" << std::endl;
    } else {
      std::cerr << "Failed to read dataset" << std::endl;
      return 1;
    }
    
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  
  return 0;
}
