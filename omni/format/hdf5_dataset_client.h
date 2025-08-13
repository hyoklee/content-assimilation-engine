#ifndef CAE_FORMAT_HDF5_DATASET_CLIENT_H_
#define CAE_FORMAT_HDF5_DATASET_CLIENT_H_

#include "dataset_config.h"
#include "format_client.h"
#include <hdf5.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace cae {

/**
 * HDF5 Dataset Processing Client
 * 
 * This client can read datasets from HDF5 files according to a configuration
 * that specifies the dataset name, hyperslab parameters, and processing options.
 */
class Hdf5DatasetClient : public FormatClient {
private:
  static constexpr size_t DEFAULT_CHUNK_SIZE = 1024 * 1024; // 1MB chunks

public:
  /** Default constructor */
  Hdf5DatasetClient() = default;

  /** Destructor */
  ~Hdf5DatasetClient() override = default;

  /** Describe the dataset */
  std::string Describe(const FormatContext &ctx) override {
    return "HDF5 dataset: " + ctx.filename_ +
           " (size: " + std::to_string(ctx.size_) +
           " bytes, offset: " + std::to_string(ctx.offset_) + ")";
  }

  /** Process an HDF5 dataset according to configuration */
  void Import(const FormatContext &ctx) override;

  /** Read dataset using configuration */
  void ReadDataset(const DatasetConfig& config);

  /** Execute the run script if specified */
  void ExecuteRunScript(const std::string& script_path, const std::string& input_file, const std::string& output_file);

protected:
  virtual void OnChunkProcessed(size_t bytes_processed) {}
  virtual void OnDatasetRead(const std::string& dataset_name, const std::vector<hsize_t>& dimensions) {}

private:
  /** Open HDF5 file */
  hid_t OpenHdf5File(const std::string& file_path);
  
  /** Close HDF5 file */
  void CloseHdf5File(hid_t file_id);
  
  /** Read dataset hyperslab */
  bool ReadDatasetHyperslab(hid_t file_id, const std::string& dataset_name,
                           const std::vector<hsize_t>& start,
                           const std::vector<hsize_t>& count,
                           const std::vector<hsize_t>& stride,
                           void* buffer);
  
  /** Get dataset information */
  bool GetDatasetInfo(hid_t file_id, const std::string& dataset_name,
                     std::vector<hsize_t>& dimensions, hid_t& datatype);
  
  /** Allocate buffer for dataset */
  std::unique_ptr<char[]> AllocateBuffer(const std::vector<hsize_t>& dimensions, hid_t datatype);
  
  /** Calculate total size of dataset */
  size_t CalculateDatasetSize(const std::vector<hsize_t>& dimensions, hid_t datatype);
};

} // namespace cae

#endif // CAE_FORMAT_HDF5_DATASET_CLIENT_H_
