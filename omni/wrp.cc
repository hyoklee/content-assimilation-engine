#include "format/format_factory.h"
#include "repo/filesystem_repo_omni.h"
#include "repo/repo_factory.h"
#include "format/dataset_config.h"
#ifdef  OMNI_ENABLE_HDF5
#include "format/hdf5_dataset_client.h"
#include <hdf5.h>
#endif
#include <cstdlib>
#include <iostream>
#include <limits.h> // For PATH_MAX
#ifdef OMNI_ENABLE_MPI
#include <mpi.h>
#endif
#include <pwd.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <glob.h>
#include <future>
#include <thread>
#include <algorithm>
#include <mutex>
#include <fstream>
#include <cctype> // For isspace
#include <cstdio> // For std::remove
#include <regex>

using namespace cae;
namespace fs = std::filesystem;



std::string ExpandPath(const std::string &path) {
  if (path.empty() || path[0] != '~') {
    return path;
  }

  if (path.length() == 1 || path[1] == '/') {
    // ~/... case
    const char *home = getenv("HOME");
    if (!home) {
      struct passwd *pw = getpwuid(getuid());
      if (pw) {
        home = pw->pw_dir;
      }
    }
    if (home) {
      return std::string(home) + (path.length() > 1 ? path.substr(1) : "");
    }
  }

  // If we can't expand, return original path
  return path;
}

std::vector<std::string> ExpandFilePattern(const std::string &pattern) {
  std::vector<std::string> files;
  
  std::cout << "ExpandFilePattern called with pattern: " << pattern << std::endl;
  
  if (pattern.empty()) {
    std::cerr << "Warning: Empty file pattern provided" << std::endl;
    return files;
  }
  
  // Check if the pattern contains wildcards
  bool has_wildcards = (pattern.find('*') != std::string::npos) || 
                       (pattern.find('?') != std::string::npos) ||
                       (pattern.find('[') != std::string::npos);
  
  if (has_wildcards) {
    // Use glob to expand wildcards
    glob_t glob_result;
    int glob_ret = glob(pattern.c_str(), GLOB_TILDE | GLOB_BRACE, nullptr, &glob_result);
    
    if (glob_ret == 0) {
      for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
        std::string file_path = glob_result.gl_pathv[i];
        if (fs::is_regular_file(file_path)) {
          files.push_back(file_path);
        }
      }
      std::cout << "Expanded pattern '" << pattern << "' to " << files.size() << " files" << std::endl;
    } else if (glob_ret == GLOB_NOMATCH) {
      std::cerr << "Warning: No files match pattern: " << pattern << std::endl;
    } else {
      std::cerr << "Warning: Error expanding pattern: " << pattern << " (error code: " << glob_ret << ")" << std::endl;
    }
    
    globfree(&glob_result);
  } else {
    // Check if it's a directory
    if (fs::is_directory(pattern)) {
      for (const auto &entry : fs::directory_iterator(pattern)) {
        if (entry.is_regular_file()) {
          files.push_back(entry.path().string());
        }
      }
      std::cout << "Expanded directory '" << pattern << "' to " << files.size() << " files" << std::endl;
    } else if (fs::is_regular_file(pattern)) {
      // Single file
      files.push_back(pattern);
      std::cout << "Single file: " << pattern << std::endl;
    } else {
      std::cerr << "Warning: Path is neither a file nor directory: " << pattern << std::endl;
    }
  }
  
  // Sort files for consistent ordering
  std::sort(files.begin(), files.end());
  
  return files;
}

struct OmniJobConfig {
  std::string name;
  int max_scale;
  std::string hostfile; // Add hostfile to config

  struct DataEntry {
    std::vector<std::string> paths;  // Changed from single path to multiple paths
    std::vector<size_t> range;
    size_t offset;
    size_t size;
    std::vector<std::string> description;
    std::string hash;

#ifdef  OMNI_ENABLE_HDF5    
    // HDF5 support
    std::string uri;  // HDF5 URI (e.g., "hdf5://path/to/file.h5/dataset_name")
    std::vector<hsize_t> hdf5_start;   // HDF5 hyperslab start coordinates
    std::vector<hsize_t> hdf5_count;   // HDF5 hyperslab count
    std::vector<hsize_t> hdf5_stride;  // HDF5 hyperslab stride
    std::string run_script;            // HDF5 run script
    std::string destination;           // HDF5 destination
#endif
    
    DataEntry() : offset(0), size(0) {}
  };

  std::vector<DataEntry> data_entries;

  OmniJobConfig() : max_scale(100) {}
};

OmniJobConfig ParseOmniFile(const std::string &yaml_file) {
  OmniJobConfig config;

  try {
    std::cout << "Attempting to load YAML file: " << yaml_file << std::endl;
    YAML::Node yaml = YAML::LoadFile(yaml_file);
    std::cout << "YAML file loaded successfully" << std::endl;
    std::cout << "YAML keys: ";
    for (const auto& pair : yaml) {
      std::cout << pair.first.as<std::string>() << " ";
    }
    std::cout << std::endl;

    if (yaml["name"]) {
      config.name = yaml["name"].as<std::string>();
    }

    if (yaml["max_scale"]) {
      config.max_scale = yaml["max_scale"].as<int>();
    }

    // Handle both new format (with data section) and old format (direct HDF5 fields)
    std::cout << "Checking for data section..." << std::endl;
    if (yaml["data"]) {
      // New format with data section
      const YAML::Node &data_node = yaml["data"];
      std::cout << "Found " << data_node.size() << " data entries in YAML (new format)" << std::endl;
      for (const auto &entry : data_node) {
        std::cout << "Processing YAML entry" << std::endl;
        OmniJobConfig::DataEntry data_entry;

        if (entry["path"]) {
          std::string expanded_path = ExpandPath(entry["path"].as<std::string>());
          std::vector<std::string> expanded_files = ExpandFilePattern(expanded_path);
          data_entry.paths = expanded_files;
        }

        if (entry["range"]) {
          const YAML::Node &range_node = entry["range"];
          for (const auto &val : range_node) {
            data_entry.range.push_back(val.as<size_t>());
          }
        }

        if (entry["offset"]) {
          data_entry.offset = entry["offset"].as<size_t>();
        }

        if (entry["size"]) {
          data_entry.size = entry["size"].as<size_t>();
        }

        if (entry["description"]) {
          const YAML::Node &desc_node = entry["description"];
          for (const auto &desc : desc_node) {
            data_entry.description.push_back(desc.as<std::string>());
          }
        }

        if (entry["hash"]) {
          data_entry.hash = entry["hash"].as<std::string>();
        }
#ifdef  OMNI_ENABLE_HDF5
        // Parse HDF5 URI if present
        if (entry["uri"]) {
          data_entry.uri = entry["uri"].as<std::string>();
        }

        // Parse HDF5 hyperslab parameters if present
        if (entry["start"]) {
          const YAML::Node &start_node = entry["start"];
          for (const auto &val : start_node) {
            data_entry.hdf5_start.push_back(val.as<hsize_t>());
          }
        }

        if (entry["count"]) {
          const YAML::Node &count_node = entry["count"];
          for (const auto &val : count_node) {
            data_entry.hdf5_count.push_back(val.as<hsize_t>());
          }
        }

        if (entry["stride"]) {
          const YAML::Node &stride_node = entry["stride"];
          for (const auto &val : stride_node) {
            data_entry.hdf5_stride.push_back(val.as<hsize_t>());
          }
        }

        if (entry["run"]) {
          data_entry.run_script = entry["run"].as<std::string>();
        }

        if (entry["dest"]) {
          data_entry.destination = entry["dest"].as<std::string>();
        }
#endif
        // If size is not specified (0), automatically detect file size
        if (data_entry.size == 0 && !data_entry.paths.empty()) {
          FilesystemRepoClient fs_client;
          
          // For multiple files, we'll use the size of the first file as a template
          // In practice, you might want to handle each file individually
          size_t file_size = fs_client.GetFileSize(data_entry.paths[0]);
          if (file_size > 0) {
            // Calculate size as file_size - offset to read from offset to end
            // of file
            if (data_entry.offset < file_size) {
              data_entry.size = file_size - data_entry.offset;
              std::cout << "Auto-detected size for " << data_entry.paths[0] << ": "
                        << data_entry.size << " bytes (from offset "
                        << data_entry.offset << " to end of file)" << std::endl;
            } else {
              std::cerr << "Warning: Offset " << data_entry.offset
                        << " is beyond file size " << file_size << " for "
                        << data_entry.paths[0] << std::endl;
            }
          } else {
            std::cerr << "Warning: Could not determine file size for "
                      << data_entry.paths[0] << std::endl;
          }
          
          // Check if all files have the same size (optional validation)
          if (data_entry.paths.size() > 1) {
            std::cout << "Note: Processing " << data_entry.paths.size() 
                      << " files with same parameters" << std::endl;
          }
        }

        // Derive range from offset and size if range is not specified
        if (data_entry.range.empty() && data_entry.size > 0) {
          data_entry.range.push_back(data_entry.offset);
          data_entry.range.push_back(data_entry.offset + data_entry.size);
        }

        config.data_entries.push_back(data_entry);
      }
    }
#ifdef  OMNI_ENABLE_HDF5
    std::cout << "Checking for old format HDF5 entry..." << std::endl;
    if (yaml["uri"] && yaml["uri"].as<std::string>().find("hdf5://") == 0) {
      // Old format with direct HDF5 fields (compatibility with tf.yaml, tf3d.yaml)
      std::cout << "Found HDF5 entry in old format (direct fields)" << std::endl;
      OmniJobConfig::DataEntry data_entry;
      
      // Parse HDF5 URI
      if (yaml["uri"]) {
        data_entry.uri = yaml["uri"].as<std::string>();
      }
      
      // Parse HDF5 hyperslab parameters
      if (yaml["start"]) {
        const YAML::Node &start_node = yaml["start"];
        for (const auto &val : start_node) {
          data_entry.hdf5_start.push_back(val.as<hsize_t>());
        }
      }
      
      if (yaml["count"]) {
        const YAML::Node &count_node = yaml["count"];
        for (const auto &val : count_node) {
          data_entry.hdf5_count.push_back(val.as<hsize_t>());
        }
      }
      
      if (yaml["stride"]) {
        const YAML::Node &stride_node = yaml["stride"];
        for (const auto &val : stride_node) {
          data_entry.hdf5_stride.push_back(val.as<hsize_t>());
        }
      }
      
      if (yaml["run"]) {
        data_entry.run_script = yaml["run"].as<std::string>();
      }
      
      if (yaml["dest"]) {
        data_entry.destination = yaml["dest"].as<std::string>();
      }
      
      // Add tags as description for compatibility
      if (yaml["tags"]) {
        const YAML::Node &tags_node = yaml["tags"];
        for (const auto &tag : tags_node) {
          data_entry.description.push_back(tag.as<std::string>());
        }
      }
      
      config.data_entries.push_back(data_entry);
    }
#endif
    
  } catch (const YAML::Exception &e) {
    std::cerr << "YAML parsing error: " << e.what() << std::endl;
    throw;
  } catch (const std::exception &e) {
    std::cerr << "General error during YAML parsing: " << e.what() << std::endl;
    throw;
  } catch (...) {
    std::cerr << "Unknown error during YAML parsing" << std::endl;
    throw;
  }

  return config;
}

std::string BuildMpiCommand(const OmniJobConfig::DataEntry &entry, int nprocs,
                            const std::string &hostfile) {
  std::ostringstream cmd;

  // Build description string
  std::string description;
  if (!entry.description.empty()) {
    std::ostringstream desc_stream;
    for (size_t i = 0; i < entry.description.size(); ++i) {
      if (i > 0)
        desc_stream << ",";
      desc_stream << entry.description[i];
    }
    description = desc_stream.str();
  }

  // Construct MPI command with environment forwarding
  cmd << "mpirun -x LD_PRELOAD"; // Forward LD_PRELOAD explicitly

  // Add hostfile if provided
  if (!hostfile.empty()) {
    cmd << " --hostfile " << hostfile;
  }

  // Forward other important environment variables
  const char *important_env_vars[] = {"PATH",
                                      "HOME",
                                      "USER",
                                      "TMPDIR",
                                      "LD_LIBRARY_PATH",
                                      "PYTHONPATH",
                                      "CUDA_VISIBLE_DEVICES",
                                      "HERMES_CONF",
                                      "IOWARP_CAE_CONF",
                                      nullptr};

  for (int i = 0; important_env_vars[i] != nullptr; ++i) {
    if (getenv(important_env_vars[i])) {
      cmd << " -x " << important_env_vars[i];
    }
  }

  cmd << " -np " << nprocs;
  cmd << " wrp_binary_format_mpi";
  cmd << " \"" << entry.paths[0] << "\""; // Use the first (and only) path
  cmd << " " << entry.offset;
  cmd << " " << entry.size;

  if (!description.empty()) {
    cmd << " \"" << description << "\"";
  }

  if (!entry.hash.empty()) {
    cmd << " \"" << entry.hash << "\"";
  }

  return cmd.str();
}

void ProcessHdf5DataEntry(const OmniJobConfig::DataEntry &entry) {
  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << "Processing HDF5 Data Entry" << std::endl;
  std::cout << std::string(50, '=') << std::endl;
#ifdef  OMNI_ENABLE_HDF5    
  std::cout << "URI: " << entry.uri << std::endl;
#endif  
  // Parse the HDF5 URI
  std::string file_path, dataset_name;
#ifdef  OMNI_ENABLE_HDF5    
  if (!cae::ParseHdf5Uri(entry.uri, file_path, dataset_name)) {
    std::cerr << "Error: Invalid HDF5 URI format: " << entry.uri << std::endl;
    return;
  }
#endif  
  std::cout << "File path: " << file_path << std::endl;
  std::cout << "Dataset name: " << dataset_name << std::endl;

#ifdef  OMNI_ENABLE_HDF5
  // Create HDF5 dataset client and read the dataset
  cae::Hdf5DatasetClient client;
#endif
  
  // Create DatasetConfig from entry
  cae::DatasetConfig config;
  config.name = entry.description.empty() ? "dataset" : entry.description[0];
#ifdef  OMNI_ENABLE_HDF5  
  config.uri = entry.uri;
  config.start = entry.hdf5_start;
  config.count = entry.hdf5_count;
  config.stride = entry.hdf5_stride;
  config.run_script = entry.run_script;
  config.destination = entry.destination;
#endif
  
  // Add tags from description
  for (const auto& desc : entry.description) {
    config.tags.push_back(desc);
  }
  
#ifdef  OMNI_ENABLE_HDF5
  try {
    client.ReadDataset(config);
    std::cout << "✓ Successfully completed HDF5 processing" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "✗ Failed to process HDF5 dataset: " << e.what() << std::endl;
  }
#endif
  
}

void ProcessDataEntry(const OmniJobConfig::DataEntry &entry, int nprocs,
                      const std::string &hostfile) {
  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << "Processing Data Entry" << std::endl;
  std::cout << std::string(50, '=') << std::endl;
#ifdef  OMNI_ENABLE_HDF5  
  // Check if this is an HDF5 entry
  if (!entry.uri.empty() && entry.uri.find("hdf5://") == 0) {
    ProcessHdf5DataEntry(entry);
    return;
  }
#endif
  
  std::cout << "Files: " << entry.paths.size() << std::endl;
  for (size_t i = 0; i < entry.paths.size(); ++i) {
    std::cout << "  File " << (i + 1) << ": " << entry.paths[i] << std::endl;
  }
  std::cout << "Offset: " << entry.offset << " bytes" << std::endl;
  std::cout << "Size: " << entry.size << " bytes" << std::endl;
  std::cout << "MPI Processes: " << nprocs << std::endl;

  // Process each file in the entry
  for (size_t i = 0; i < entry.paths.size(); ++i) {
    std::cout << "\nProcessing file " << (i + 1) << "/" << entry.paths.size() 
              << ": " << entry.paths[i] << std::endl;
    
    // Create a single-file entry for this file
    OmniJobConfig::DataEntry single_file_entry;
    single_file_entry.paths = {entry.paths[i]};
    single_file_entry.range = entry.range;
    single_file_entry.offset = entry.offset;
    single_file_entry.size = entry.size;
    single_file_entry.description = entry.description;
    single_file_entry.hash = entry.hash;
    
    // Build and execute MPI command for this file
    std::string mpi_command = BuildMpiCommand(single_file_entry, nprocs, hostfile);
    std::cout << "Executing: " << mpi_command << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    int result = system(mpi_command.c_str());

    std::cout << std::string(50, '-') << std::endl;
    if (result == 0) {
      std::cout << "✓ Successfully completed processing " << entry.paths[i]
                << std::endl;
    } else {
      std::cerr << "✗ Failed to process " << entry.paths[i]
                << " (exit code: " << result << ")" << std::endl;
    }
  }
}

void ProcessDataEntryAsync(const OmniJobConfig::DataEntry &entry, int nprocs,
                          const std::string &hostfile) {
  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << "Processing Data Entry (Async)" << std::endl;
  std::cout << std::string(50, '=') << std::endl;
#ifdef  OMNI_ENABLE_HDF5  
  // Check if this is an HDF5 entry
  if (!entry.uri.empty() && entry.uri.find("hdf5://") == 0) {
    ProcessHdf5DataEntry(entry);
    return;
  }
#endif  
  std::cout << "Files: " << entry.paths.size() << std::endl;
  for (size_t i = 0; i < entry.paths.size(); ++i) {
    std::cout << "  File " << (i + 1) << ": " << entry.paths[i] << std::endl;
  }
  std::cout << "Offset: " << entry.offset << " bytes" << std::endl;
  std::cout << "Size: " << entry.size << " bytes" << std::endl;
  std::cout << "MPI Processes: " << nprocs << std::endl;

  // Create async tasks for each file
  std::vector<std::future<void>> futures;
  
  for (size_t i = 0; i < entry.paths.size(); ++i) {
    futures.push_back(std::async(std::launch::async, [&, i, nprocs, hostfile]() {
      std::cout << "\nProcessing file " << (i + 1) << "/" << entry.paths.size() 
                << ": " << entry.paths[i] << " (async)" << std::endl;
      
      // Create a single-file entry for this file
      OmniJobConfig::DataEntry single_file_entry;
      single_file_entry.paths = {entry.paths[i]};
      single_file_entry.range = entry.range;
      single_file_entry.offset = entry.offset;
      single_file_entry.size = entry.size;
      single_file_entry.description = entry.description;
      single_file_entry.hash = entry.hash;
      
      // Build and execute MPI command for this file
      std::string mpi_command = BuildMpiCommand(single_file_entry, nprocs, hostfile);
      std::cout << "Executing: " << mpi_command << std::endl;
      std::cout << std::string(50, '-') << std::endl;

      int result = system(mpi_command.c_str());

      std::cout << std::string(50, '-') << std::endl;
      if (result == 0) {
        std::cout << "✓ Successfully completed processing " << entry.paths[i]
                  << std::endl;
      } else {
        std::cerr << "✗ Failed to process " << entry.paths[i]
                  << " (exit code: " << result << ")" << std::endl;
      }
    }));
  }
  
  // Wait for all async tasks to complete
  for (auto &future : futures) {
    future.wait();
  }
}

// Parse hostfile into vector of hostnames
std::vector<std::string> ParseHostfile(const std::string &hostfile_path) {
    std::vector<std::string> hosts;
    std::ifstream infile(hostfile_path);
    std::string line;
    while (std::getline(infile, line)) {
        // Remove comments and whitespace
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (!line.empty()) hosts.push_back(line);
    }
    return hosts;
}

// Allocate least-loaded nodes for a job
std::vector<int> AllocateNodes(int num_nodes, std::vector<int> &node_proc_counts, std::mutex &mtx) {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::pair<int, int>> load_index;
    for (int i = 0; i < node_proc_counts.size(); ++i)
        load_index.push_back({node_proc_counts[i], i});
    std::sort(load_index.begin(), load_index.end());
    std::vector<int> selected;
    for (int i = 0; i < num_nodes; ++i) {
        selected.push_back(load_index[i].second);
        node_proc_counts[load_index[i].second]++;
    }
    return selected;
}

// Write a temporary hostfile for a job
std::string WriteTempHostfile(const std::vector<std::string> &hosts, const std::vector<int> &indices, int job_id) {
    std::string temp_hostfile = "hostfile_job_" + std::to_string(job_id) + ".tmp";
    std::ofstream outfile(temp_hostfile);
    for (int idx : indices) outfile << hosts[idx] << std::endl;
    outfile.close();
    
    // Log the node allocation for validation
    std::cout << "Job " << job_id << " allocated nodes: ";
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << hosts[indices[i]];
    }
    std::cout << " (hostfile: " << temp_hostfile << ")" << std::endl;
    
    return temp_hostfile;
}

int main(int argc, char *argv[]) {
  std::cout << "=== MAIN FUNCTION STARTED ===" << std::endl;
  
#ifdef OMNI_ENABLE_MPI
  // Initialize MPI for the main orchestrator
  MPI_Init(&argc, &argv);

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <omni_yaml_file> [hostfile]"
              << std::endl;
    MPI_Finalize();
    return 1;
  }

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#else
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <omni_yaml_file> [hostfile]"
              << std::endl;
    return 1;
  }

  int rank = 0; // Single process mode
#endif

  try {
    // Parse the OMNI YAML file
    OmniJobConfig config = ParseOmniFile(argv[1]);

    // Get hostfile from command line or config
    std::string hostfile;
    if (argc > 2) {
      hostfile = argv[2];
    } else if (getenv("OMNI_HOSTFILE")) {
      hostfile = getenv("OMNI_HOSTFILE");
    }

    std::vector<std::string> hosts;
    std::vector<int> node_proc_counts;
    std::mutex node_mutex;
    if (!hostfile.empty()) {
      hosts = ParseHostfile(hostfile);
      node_proc_counts.resize(hosts.size(), 0);
    }

    if (rank == 0) {
      std::cout << "OMNI Content Assimilation Engine" << std::endl;
      std::cout << "=================================" << std::endl;
      std::cout << "Job: " << config.name << std::endl;
      std::cout << "Max scale: " << config.max_scale << std::endl;
      std::cout << "Hostfile: "
                << (hostfile.empty() ? "not specified" : hostfile) << std::endl;
      std::cout << "Number of data entries: " << config.data_entries.size()
                << std::endl;

#ifdef  OMNI_ENABLE_HDF5
      // Display HDF5 entries
      int hdf5_count = 0;
      for (const auto& entry : config.data_entries) {
        if (!entry.uri.empty() && entry.uri.find("hdf5://") == 0) {
          hdf5_count++;
        }
      }
      if (hdf5_count > 0) {
        std::cout << "HDF5 entries: " << hdf5_count << std::endl;
      }
#endif
      
      // Launch all jobs concurrently, balancing node usage
      int job_id = 0;
      std::vector<std::future<void>> job_futures;
      std::cout << "About to process " << config.data_entries.size() << " data entries" << std::endl;
      
      for (size_t i = 0; i < config.data_entries.size(); ++i) {
        std::cout << "Processing entry " << i << std::endl;
        const auto &entry = config.data_entries[i];
        int nprocs, nthreads;
        
        std::cout << "Entry " << i << " has " << entry.paths.size() << " paths" << std::endl;
#ifdef  OMNI_ENABLE_HDF5        	
        if (!entry.uri.empty()) {
          std::cout << "Entry " << i << " has URI: " << entry.uri << std::endl;
        }

        // Skip HDF5 entries for MPI scaling calculation
        if (!entry.uri.empty() && entry.uri.find("hdf5://") == 0) {
          std::cout << "Entry " << i << " is HDF5 entry" << std::endl;
          nprocs = 1;  // Default for HDF5 processing
          nthreads = 1;
        } else {
#endif          
          // Regular file processing
          if (!entry.paths.empty()) {
            std::cout << "Entry " << i << " is regular file entry with path: " << entry.paths[0] << std::endl;
            FilesystemRepoClient fs_client;
            fs_client.RecommendScaleForFile(entry.paths[0], config.max_scale, nprocs, nthreads);
          } else {
            std::cout << "Entry " << i << " has no paths" << std::endl;
            nprocs = 1;
            nthreads = 1;
          }
#ifdef  OMNI_ENABLE_HDF5                  
        }
#endif
        int nodes_needed = (!hosts.empty()) ? std::min(nprocs, (int)hosts.size()) : nprocs;
        std::vector<int> node_indices;
        std::string temp_hostfile;
        if (!hosts.empty()) {
          node_indices = AllocateNodes(nodes_needed, node_proc_counts, node_mutex);
          temp_hostfile = WriteTempHostfile(hosts, node_indices, job_id);
        } else {
          temp_hostfile = hostfile; // fallback: use original hostfile or none
        }

        // Launch each job asynchronously
        job_futures.push_back(std::async(std::launch::async, [&, entry, nprocs, temp_hostfile, job_id]() {
          // Check if this is an HDF5 entry
#ifdef  OMNI_ENABLE_HDF5        	  
          if (!entry.uri.empty() && entry.uri.find("hdf5://") == 0) {
            ProcessHdf5DataEntry(entry);
          } else {
#endif	    
            // Regular file processing
            if (entry.paths.size() > 1)
              ProcessDataEntryAsync(entry, nprocs, temp_hostfile);
            else
              ProcessDataEntry(entry, nprocs, temp_hostfile);
#ifdef  OMNI_ENABLE_HDF5        	  	    
          }
#endif	  
          if (!temp_hostfile.empty() && temp_hostfile.find("hostfile_job_") == 0) {
            std::remove(temp_hostfile.c_str());
          }
        }));
        job_id++;
      }
      // Wait for all jobs to finish
      for (auto &f : job_futures) f.wait();

      std::cout << "\n" << std::string(50, '=') << std::endl;
      std::cout << "✓ All data entries processed successfully!" << std::endl;
      std::cout << std::string(50, '=') << std::endl;
    }

  } catch (const std::exception &e) {
    if (rank == 0) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
#ifdef OMNI_ENABLE_MPI
    MPI_Finalize();
#endif
    return 1;
  }

#ifdef OMNI_ENABLE_MPI
  MPI_Finalize();
#endif
  return 0;
}
