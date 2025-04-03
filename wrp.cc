#include <iostream>
#include <string>
#include <vector>
#include <sstream>

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
        if (1) {                // TO-DO: Check input later.
            std::cerr << "Invalid input: Please check format." << std::endl;
            return 1;
        }
        std::string name = argv[2];
        std::cout << "input: " << name << std::endl;
        
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
