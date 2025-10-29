#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <regex>
#include <vector>
#include <algorithm>

#ifndef _WIN32
#include <glob.h>
#include <pwd.h>
#include <unistd.h>
#endif

#ifdef _WIN32
namespace fs = std::filesystem;
#else
namespace fs = std::filesystem;
#endif

std::string ExpandPath(const std::string &path) {
  if (path.empty() || path[0] != '~') {
    return path;
  }

  if (path.length() == 1 || path[1] == '/' || path[1] == '\\') {
    // ~/... case
    const char *home = nullptr;

#ifdef _WIN32
    // Windows: try USERPROFILE first, then HOMEDRIVE+HOMEPATH
    home = getenv("USERPROFILE");
    if (!home) {
      const char *drive = getenv("HOMEDRIVE");
      const char *path_part = getenv("HOMEPATH");
      if (drive && path_part) {
        static std::string home_path = std::string(drive) + std::string(path_part);
        home = home_path.c_str();
      }
    }
#else
    // POSIX systems
    home = getenv("HOME");
    if (!home) {
      struct passwd *pw = getpwuid(getuid());
      if (pw) {
        home = pw->pw_dir;
      }
    }
#endif
    
    if (home) {
      std::string separator = (path.length() > 1) ? 
#ifdef _WIN32
        (path[1] == '\\' ? "\\" : "/") :
#else
        "/" :
#endif
        "";
      return std::string(home) + separator + (path.length() > 1 ? path.substr(2) : "");
    }
  }

  // If we can't expand, return original path
  return path;
}

// Cross-platform file pattern matching using std::filesystem
void expandPattern(const fs::path& pattern_path,
		   std::vector<std::string>& files) {
  try {
    std::string pattern_str = pattern_path.string();
    fs::path parent_path = pattern_path.parent_path();
    std::string filename_pattern = pattern_path.filename().string();
    
    if (parent_path.empty()) {
      parent_path = ".";
    }
    
    if (!fs::exists(parent_path) || !fs::is_directory(parent_path)) {
      return;
    }
    
    // Convert simple wildcards to regex
    std::string regex_pattern = filename_pattern;
    // Escape regex special characters except * and ?
    std::regex special_chars(R"([\.^$+{}()[\]|])");
    regex_pattern = std::regex_replace(regex_pattern, special_chars, R"(\$&)");
    // Convert wildcards to regex
    std::regex star(R"(\\\*)"); // escaped *
    regex_pattern = std::regex_replace(regex_pattern, star, ".*");
    std::regex question(R"(\\\?)"); // escaped ?
    regex_pattern = std::regex_replace(regex_pattern, question, ".");
    
    std::regex pattern_regex(regex_pattern, std::regex_constants::icase);
    
    for (const auto& entry : fs::directory_iterator(parent_path)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        if (std::regex_match(filename, pattern_regex)) {
          files.push_back(entry.path().string());
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error expanding pattern: " << e.what() << std::endl;
  }
}

std::vector<std::string> ExpandFilePattern(const std::string &pattern) {
  std::vector<std::string> files;
  
  std::cout << "ExpandFilePattern called with pattern: " << pattern << std::endl;
  
  if (pattern.empty()) {
    std::cerr << "Warning: Empty file pattern provided" << std::endl;
    return files;
  }
  
  // First expand ~ if present
  std::string expanded_pattern = ExpandPath(pattern);
  
  // Check if the pattern contains wildcards
  bool has_wildcards = (expanded_pattern.find('*') != std::string::npos) || 
    (expanded_pattern.find('?') != std::string::npos);
  
  if (has_wildcards) {
#ifdef _WIN32
    // Use filesystem-based pattern matching for Windows
    expandPattern(fs::path(expanded_pattern), files);
#else
    // Use glob for POSIX systems
    glob_t glob_result;
    int glob_ret = glob(expanded_pattern.c_str(), GLOB_TILDE | GLOB_BRACE, nullptr, &glob_result);
    
    if (glob_ret == 0) {
      for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
        std::string file_path = glob_result.gl_pathv[i];
        if (fs::is_regular_file(file_path)) {
          files.push_back(file_path);
        }
      }
    } else if (glob_ret == GLOB_NOMATCH) {
      std::cerr << "Warning: No files match pattern: " << expanded_pattern << std::endl;
    } else {
      std::cerr << "Warning: Error expanding pattern: " << expanded_pattern << " (error code: " << glob_ret << ")" << std::endl;
    }
    
    globfree(&glob_result);
#endif
    
    std::cout << "Expanded pattern '" << pattern << "' to " << files.size() << " files" << std::endl;
  } else {
    // Check if it's a directory
    if (fs::is_directory(expanded_pattern)) {
      for (const auto &entry : fs::directory_iterator(expanded_pattern)) {
        if (entry.is_regular_file()) {
          files.push_back(entry.path().string());
        }
      }
      std::cout << "Expanded directory '" << expanded_pattern << "' to " << files.size() << " files" << std::endl;
    } else if (fs::is_regular_file(expanded_pattern)) {
      //     else if (fs::exists(expanded_pattern)) {
      // Single file
      files.push_back(expanded_pattern);
      std::cout << "Single file: " << expanded_pattern << std::endl;
    } else {
      std::cerr << "Warning: Path is neither a file nor directory: " << expanded_pattern << std::endl;
    }
  }
  
  // Sort files for consistent ordering
  std::sort(files.begin(), files.end());
  
  return files;
}