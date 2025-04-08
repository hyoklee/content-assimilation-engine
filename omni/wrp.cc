#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

int parse_yaml(std::string input_file) {

  std::ifstream ifs(input_file);

  if (!ifs.is_open()) {
    std::cerr << "Error: Could not open file " << input_file << std::endl;
    return 1;
  }

  try {
    YAML::Node root = YAML::Load(ifs);

    if (root.IsMap()) {
      for (YAML::const_iterator it = root.begin(); it != root.end(); ++it) {
        std::string key = it->first.as<std::string>();
        if(it->second.IsScalar()){
          std::string value = it->second.as<std::string>();
          std::cout << key << ": " << value << std::endl;
        } else if (it->second.IsSequence()){
          std::cout << key << ": " << std::endl;
          for(size_t i = 0; i < it->second.size(); ++i){
            if(it->second[i].IsScalar()){
              std::cout << " - " << it->second[i].as<std::string>()
			<< std::endl;
            }
          }
        } else if (it->second.IsMap()){
             std::cout << key << ": " << std::endl;
             for (YAML::const_iterator inner_it = it->second.begin();
		  inner_it != it->second.end(); ++inner_it) {
                std::string inner_key = inner_it->first.as<std::string>();
                if(inner_it->second.IsScalar()){
                  std::string inner_value = inner_it->second.as<std::string>();
                  std::cout << "  " << inner_key << ": " << inner_value
			    << std::endl;
                }
             }
        }
      }
    } else if (root.IsSequence()) {
      for(size_t i = 0; i < root.size(); ++i){
        if(root[i].IsScalar()){
          std::cout << " - " << root[i].as<std::string>() << std::endl;
        }
      }
    } else if (root.IsScalar()){
        std::cout << root.as<std::string>() << std::endl;
    }

  } catch (YAML::ParserException& e) {
    std::cerr << "Error parsing YAML: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> [options]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "put") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " put <input.omni>" << std::endl;
            return 1;
        }
        std::string name = argv[2];
        std::cout << "input: " << name << std::endl;
        parse_yaml(name);
	
    } else if (command == "get") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " get <output.omni>" << std::endl;
            return 1;
        }
        std::string name = argv[2];
        std::cout << "output: " << name << std::endl;
    } else if (command == "ls") {
      std::cout << "connecting runtime" << std::endl;
    }
    else {
        std::cerr << "Invalid command: " << command << std::endl;
        return 1;
    }

    return 0;
}
