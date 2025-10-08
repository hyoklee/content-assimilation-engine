#define DEBUG 1
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

int main(int argc, char *argv[])
{
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <command> [options]" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  put <omni.yaml>    - Put data into buffer from YAML config" << std::endl;
    std::cerr << "  get <buffer>       - Get data from buffer and create YAML config" << std::endl;
    std::cerr << "  ls                 - List all buffers" << std::endl;
    return 1;
  }

  std::string command = argv[1];
  cae::OMNI omni;

  // Check for put/get/ls commands
  if (command == "put") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " put <omni.yaml>" << std::endl;
      return 1;
    }
    std::string name = argv[2];
    return omni.Put(name);

  } else if (command == "get") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " get <buffer>" << std::endl;
      return 1;
    }
    std::string name = argv[2];
    return omni.Get(name);

  } else if (command == "ls") {
    std::cout << "connecting runtime" << std::endl;
    return omni.List();

  } else {
    std::cerr << "Error: invalid command - " << command << std::endl;
    return 1;
  }
  return 0;
}
