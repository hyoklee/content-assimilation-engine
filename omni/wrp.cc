#ifdef _WIN32
// Fix Winsock conflicts by including winsock2 first
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX  // Prevent Windows headers from defining min/max macros
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "omni_job_config.h"
#include "format/format_factory.h"
#include "repo/filesystem_repo_omni.h"
#include "repo/repo_factory.h"
#include "format/dataset_config.h"
#ifdef  USE_HDF5
#include "format/hdf5_dataset_client.h"
#include <hdf5.h>
#include "omni_processing.h"
#endif
#include <cstdlib>
#include <iostream>
#include <limits.h> // For PATH_MAX
#ifdef USE_MPI
#include <mpi.h>
#endif
#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#include <glob.h>
#else
// Windows-specific includes
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#endif
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <future>
#include <thread>
#include <algorithm>
#include <mutex>
#include <fstream>
#include <cctype> // For isspace
#include <cstdio> // For std::remove
#include <regex>

// Additional includes for put/get functionality
#include <errno.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <cstring>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#ifdef USE_HERMES
#include <hermes/data_stager/stager_factory.h>
#include <hermes/hermes.h>
#endif

#ifdef USE_POCO
#include <chrono>
#include <iomanip>
#include <thread>
#include "Poco/DigestEngine.h"
#include "Poco/Exception.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/Net/Context.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/NullStream.h"
#include "Poco/Path.h"
#include "Poco/Pipe.h"
#include "Poco/PipeStream.h"
#include "Poco/Process.h"
#include "Poco/SHA2Engine.h"
#include "Poco/SharedMemory.h"
#include "Poco/StreamCopier.h"
#include "Poco/TemporaryFile.h"
#include "Poco/URI.h"
#include "format/globus_utils.h"
#endif

#ifdef USE_AWS
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#endif

#include "OMNI.h"

using namespace cae;
namespace fs = std::filesystem;

#ifdef USE_AWS
// Global AWS SDK options for proper lifecycle management
static Aws::SDKOptions g_aws_options;
static bool g_aws_initialized = false;

// Cleanup function to be called at program exit
static void cleanup_aws_sdk() {
  if (g_aws_initialized) {
    Aws::ShutdownAPI(g_aws_options);
    g_aws_initialized = false;
  }
}

// Initialize AWS SDK once at program startup
static void init_aws_sdk() {
  if (!g_aws_initialized) {
    g_aws_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
    Aws::InitAPI(g_aws_options);
    g_aws_initialized = true;
    std::atexit(cleanup_aws_sdk);
  }
}
#endif

int main(int argc, char *argv[])
{
#ifdef USE_AWS
  // Initialize AWS SDK once at program startup
  init_aws_sdk();
#endif

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [-q] <command> [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -q                 - Quiet mode (suppress standard output)" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  put <omni.yaml>    - Put data into buffer from YAML config" << std::endl;
    std::cerr << "  get <buffer>       - Get data from buffer and create YAML config" << std::endl;
    std::cerr << "  ls                 - List all buffers" << std::endl;
    return 1;
  }

  // Parse options
  bool quiet = false;
  int arg_idx = 1;

  if (std::string(argv[1]) == "-q") {
    quiet = true;
    arg_idx = 2;
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " [-q] <command> [options]" << std::endl;
      return 1;
    }
  }

  std::string command = argv[arg_idx];

#ifdef USE_HERMES
  // Initialize Hermes/Chimaera client only if config files exist
  char* server_config_path = std::getenv("HERMES_CONF");
  char* client_config_path = std::getenv("HERMES_CLIENT_CONF");

  // Use default paths if environment variables are not set
  std::string server_path = server_config_path ? server_config_path : "hermes_server.yaml";
  std::string client_path = client_config_path ? client_config_path : "hermes_client.yaml";

  // Only initialize if both config files exist
  if (fs::exists(server_path) && fs::exists(client_path)) {
    try {
      bool start_server = false;  // Don't start server, just connect as client
      CHI_CLIENT->Create(server_path.c_str(), client_path.c_str(), start_server);
    } catch (...) {
      // Hermes initialization failed, continue without it
      if (!quiet) {
        std::cerr << "Warning: Hermes initialization failed, continuing without Hermes support" << std::endl;
      }
    }
  }
#endif

  cae::OMNI omni;
  omni.SetQuiet(quiet);

  // Check for put/get/ls commands
  if (command == "put") {
    if (argc < arg_idx + 2) {
      std::cerr << "Usage: " << argv[0] << " [-q] put <omni.yaml>" << std::endl;
      return 1;
    }
    std::string name = argv[arg_idx + 1];
    return omni.Put(name);

  } else if (command == "get") {
    if (argc < arg_idx + 2) {
      std::cerr << "Usage: " << argv[0] << " [-q] get <buffer>" << std::endl;
      return 1;
    }
    std::string name = argv[arg_idx + 1];
    return omni.Get(name);

  } else if (command == "ls") {
    if (!quiet) {
      std::cout << "connecting runtime" << std::endl;
    }
    return omni.List();

  } else {
    std::cerr << "Error: invalid command - " << command << std::endl;
    return 1;
  }
  return 0;
}
