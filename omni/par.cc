#include <iostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "omni_processing.h"
#ifdef USE_HDF5
#include <hdf5.h>
#endif

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

        if (entry["src"]) {
          std::string expanded_path = ExpandPath(entry["src"].as<std::string>());
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

        // Removed hash field as it's not defined in DataEntry
#ifdef  USE_HDF5
        // Parse source path/URL if present
        if (entry["src"]) {
          data_entry.src = entry["src"].as<std::string>();
        }

        // Parse HDF5 hyperslab parameters if present
        if (entry["start"]) {
          const YAML::Node &start_node = entry["start"];
          for (const auto &val : start_node) {
            data_entry.hdf5_start.push_back(val.as<size_t>());
          }
        }

        if (entry["count"]) {
          const YAML::Node &count_node = entry["count"];
          for (const auto &val : count_node) {
            data_entry.hdf5_count.push_back(val.as<size_t>());
          }
        }

        if (entry["stride"]) {
          const YAML::Node &stride_node = entry["stride"];
          for (const auto &val : stride_node) {
            data_entry.hdf5_stride.push_back(val.as<size_t>());
          }
        }

        if (entry["run"]) {
          data_entry.run_script = entry["run"].as<std::string>();
        }

        if (entry["dst"]) {
          data_entry.destination = entry["dst"].as<std::string>();
        }
#endif
        // File size detection removed as it's not part of the current implementation
        config.data_entries.push_back(data_entry);
      }
    }
#ifndef  USE_HDF5
    std::cout << "Checking for old format HDF5 entry..." << std::endl;
    if (yaml["src"] && yaml["src"].as<std::string>().find("hdf5://") == 0) {
      // Old format with direct HDF5 fields (compatibility with tf.yaml, tf3d.yaml)
      std::cout << "Found HDF5 entry in old format (direct fields)" << std::endl;
      OmniJobConfig::DataEntry data_entry;

      // Parse source path/URL
      if (yaml["src"]) {
        data_entry.src = yaml["src"].as<std::string>();
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

      if (yaml["dst"]) {
        data_entry.destination = yaml["dst"].as<std::string>();
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

    // Handle old format non-HDF5 entries with direct 'src' field
    std::cout << "Checking for old format non-HDF5 entry..." << std::endl;
    if (yaml["src"] && !yaml["data"]) {
      std::cout << "Found non-HDF5 entry in old format (direct 'src' field)" << std::endl;
      OmniJobConfig::DataEntry data_entry;

      // Parse src field
      if (yaml["src"]) {
	std::string s = yaml["src"].as<std::string>();
	// trim `hdf5://`.
	std::string s1 = s.substr(7);
	size_t p = s1.find(".h5");
	std::string s2 = s1.substr(0, p+3);
        std::string expanded_path = ExpandPath(s2);

        std::vector<std::string> expanded_files = ExpandFilePattern(expanded_path);
        data_entry.paths = expanded_files;
      }
      // Parse other fields
      if (yaml["offset"]) {
        data_entry.offset = yaml["offset"].as<size_t>();
      }
#ifdef USE_HDF5
      if (yaml["run"]) {
        data_entry.run_script = yaml["run"].as<std::string>();
      }
      if (yaml["dst"]) {
        data_entry.destination = yaml["dst"].as<std::string>();
      }
#endif
      // Add tags as description for compatibility
      if (yaml["tags"]) {
        const YAML::Node &tags_node = yaml["tags"];
        for (const auto &tag : tags_node) {
          data_entry.description.push_back(tag.as<std::string>());
        }
      }
      // File size detection and range derivation removed as it's not part of the current implementation
      config.data_entries.push_back(data_entry);
    }
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
