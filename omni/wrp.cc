#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif


#ifdef USE_HERMES
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
#include "Poco/Path.h"
#include "Poco/Pipe.h"
#include "Poco/PipeStream.h"
#include "Poco/Process.h"
#include "Poco/SHA2Engine.h"
#include "Poco/SharedMemory.h"
#include "Poco/StreamCopier.h"
#include "Poco/TemporaryFile.h"
#endif

#ifdef USE_AWS
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#endif

int read_exact_bytes_from_offset(const char *filename, off_t offset,
                                 size_t num_bytes, unsigned char *buffer);

int write_meta(std::string name, std::string tags) {
  std::filesystem::path file_path = ".blackhole/ls";
  std::ofstream outfile(file_path, std::ios::out | std::ios::app);
  if (outfile.is_open()) {
    outfile << name << ":";
    outfile << tags << std::endl;
    outfile.close();  
  }
  return 0;
}
int put(std::string name, std::string tags, std::string path,
	unsigned char* buffer, int nbyte) {

  std::cout << tags << std::endl;
#ifdef USE_HERMES
  CHIMAERA_CLIENT_INIT();  
  hermes::Context ctx;
  hermes::Bucket bkt(name);
  hermes::Blob blob(nbyte);
  memcpy(blob.data(), buffer, blob.size());
  hermes::BlobId blob_id = bkt.Put(path, blob, ctx);
#endif
#ifdef USE_POCO
  const std::size_t sharedMemorySize = nbyte+1;

  try {
    Poco::File file(name);
    if (file.exists()) {
      file.remove();
    }
    file.createFile();
    std::ofstream ofs(name, std::ios::binary);
    ofs.seekp(nbyte);
    ofs.put('\0');
    ofs.close();
    
    Poco::SharedMemory shm(file, Poco::SharedMemory::AM_WRITE);
	    
    char* data = static_cast<char*>(shm.begin());
    std::memcpy(data, (const char *)buffer, nbyte);
    std::cout << "Producer wrote: '" << shm.begin() << "' to shared memory."
	      << std::endl;


  } catch (Poco::Exception& e) {
    std::cerr << "Poco Exception: " << e.displayText() << std::endl;
    return -1;
  } catch (std::exception& e) {
    std::cerr << "Standard Exception: " << e.what() << std::endl;
    return -1;
  }
#endif
  return write_meta(name, tags);  
}

int run_lambda(std::string lambda, char* ptr, std::string name) {
#ifdef USE_POCO  
    try {
        std::string command = lambda;
        Poco::Process::Args args;
        args.push_back(name+".out");
	// args.push_back("poco_arg1");
        // args.push_back("poco argument with spaces");

        Poco::Pipe outPipe; // For stdout
        Poco::Pipe errPipe; // For stderr
        Poco::ProcessHandle ph = Poco::Process::launch(
            command,
            args,
            0,          // inPipe (stdin of child process)
            &outPipe,   // outPipe (stdout of child process)
            &errPipe    // errPipe (stderr of child process)
        );

        // Create input streams to read from the pipes
        Poco::PipeInputStream istr(outPipe); // Stream for stdout
        Poco::PipeInputStream estr(errPipe); // Stream for stderr

        std::string stdout_output;
        std::string stderr_output;

        // Read stdout
        Poco::StreamCopier::copyToString(istr, stdout_output);
        // Read stderr
        Poco::StreamCopier::copyToString(estr, stderr_output);

        // Wait for the process to complete and get its exit code
        int exitCode = ph.wait();

        std::cout << "\n--- Lambda Script Output (Poco) ---\n";
        std::cout << "STDOUT:\n" << stdout_output;
        std::cout << "STDERR:\n" << stderr_output;
        std::cout << "-----------------------------------\n";
        std::cout << "Lambda script exited with status: " << exitCode
		  << std::endl;

    } catch (Poco::SystemException& exc) {
        std::cerr << "Poco SystemException: " << exc.displayText() << std::endl;
        return 1;
    } catch (Poco::Exception& exc) {
        std::cerr << "Poco Exception: " << exc.displayText() << std::endl;
        return 1;
    } catch (std::exception& exc) {
        std::cerr << "Standard Exception: " << exc.what() << std::endl;
        return 1;
    }
#endif    
    return 0;  
}
  
#ifdef USE_AWS
int write_s3(std::string dest, char* ptr) {
  
  Aws::SDKOptions options;
  options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;

  const Aws::String prefix = "s3://";
  Aws::InitAPI(options);	  
  if (dest.find(prefix) != 0) {
    std::cerr << "Error: Not a valid S3 URL (missing 's3://' prefix)."
	      << std::endl;
    return -1;
  }
  Aws::String path = dest.substr(prefix.length());
  // Find the first '/' to separate bucket and key
  size_t first_slash = path.find('/');
  if (first_slash == Aws::String::npos) {
    std::cerr << "Invalid S3 URI: No path separator found"
	      << std::endl;
    return -1;
  }

  Aws::String bucket_name = path.substr(0, first_slash);
  Aws::String object_key = path.substr(first_slash + 1);

  if (bucket_name.empty())   {
    std::cerr << "Invalid S3 URI: Bucket name is empty" << std::endl;
    return -1;
  }
  if (object_key.empty())    {
    std::cerr << "Invalid S3 URI: Object key is empty" << std::endl;
    return -1;
  }

  Aws::Client::ClientConfiguration clientConfig;
  // clientConfig.endpointOverride = "localhost:4566"; // LocalStack endpoint without http://
  clientConfig.endpointOverride = "http://localhost:4566"; // LocalStack endpoint without http://
  clientConfig.scheme = Aws::Http::Scheme::HTTP;    // LocalStack usually runs on HTTP
  clientConfig.region = "us-east-1";               // Or any region, LocalStack is region-agnostic locally
  clientConfig.verifySSL = false;	
  // IMPORTANT: For LocalStack, you often need to force path style for S3
  clientConfig.useDualStack = false; // Disable dual stack for local endpoints; Gemini
  // clientConfig.forcePathStyle = true; // Perplexity
  // clientConfig.enableEndpointDiscovery = false; // Disable endpoint discovery for local endpoints
  //        clientConfig.useFips = false; // Disable FIPS for local endpoints

  // For LocalStack, dummy credentials are usually sufficient
  Aws::Auth::AWSCredentials credentials("test", "test");
  
  // Create an S3 client
  //	Aws::S3::S3Client s3_client(credentials, clientConfig);
  Aws::S3::S3Client s3_client(clientConfig, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);
  Aws::S3::Model::CreateBucketRequest createBucketRequest;
  createBucketRequest.SetBucket(bucket_name);
  // Important: For LocalStack S3, often requires an ACL or specific configuration
  // depending on LocalStack version.
  // createBucketRequest.SetACL(Aws::S3::Model::BucketCannedACL::public_read_write);

  std::cout << "Creating bucket: " << bucket_name << std::endl;
  auto createBucketOutcome = s3_client.CreateBucket(createBucketRequest);
  if (createBucketOutcome.IsSuccess()) {
    std::cout << "Bucket created successfully!" << std::endl;
  } else {
    std::cerr << "Error creating bucket: " << createBucketOutcome.GetError().GetMessage() << std::endl;
  }
	
  // Create a PutObject request
  Aws::S3::Model::PutObjectRequest put_request;
  put_request.SetBucket(bucket_name);
  put_request.SetKey(object_key);

  // Create a stream for the data
  auto input_data = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  *input_data << ptr;
  put_request.SetBody(input_data);

  // Optionally, set content type
  put_request.SetContentType("text/plain");

  // Execute the PutObject request
  auto put_object_outcome = s3_client.PutObject(put_request);

  if (put_object_outcome.IsSuccess())
    {
      std::cout << "Successfully uploaded object to " << bucket_name << "/" << object_key << std::endl;
    }
  else
    {
      std::cout << "Error while uploading object: " 
		<< put_object_outcome.GetError().GetMessage() << std::endl;
      return -1;
    }
  Aws::ShutdownAPI(options);  
  return 0;
  
}
#endif	  

int read_omni(std::string input_file) {
  
  std::string name;  
  std::string tags;
  std::string path;
  
  std::ifstream ifs(input_file);
  int offset;

  if (!ifs.is_open()) {
    std::cerr << "Error: Could not open file " << input_file << std::endl;
    return 1;
  }

  try {
    
    YAML::Node root = YAML::Load(ifs);

      
    if (root.IsMap()) {

      for (YAML::const_iterator it = root.begin(); it != root.end(); ++it) {
	
	int nbyte = 0;
	bool run = false;	
        std::string lambda;
        std::string key = it->first.as<std::string>();
	if(key == "name") {
	  name = it->second.as<std::string>();
	}

	if(key == "path") {
	  path = it->second.as<std::string>();
	}
	
	if(key == "offset") {
	  offset = it->second.as<int>();
	}
	if(key == "nbyte") {
	  nbyte = it->second.as<int>();
	  std::vector<char> buffer(nbyte);
          unsigned char* ptr =
	      reinterpret_cast<unsigned char*>(buffer.data());		  
	  if(read_exact_bytes_from_offset(path.c_str(), offset, nbyte, ptr) ==
	     0) {
	    std::cout << "buffer=" << ptr << std::endl;
	    put(name, tags, path, ptr, nbyte);
	  }
	  else {
	    return -1;
	  }
	}

        if (key == "run") {
          run = true;
          lambda = it->second.as<std::string>();
	}
	
        if (key == "dest") {
          std::string dest = it->second.as<std::string>();
          std::cout << "dest=" << dest << std::endl;	  
#ifdef USE_POCO
	  try {
	    Poco::File file(name);
	    if (!file.exists()) {
	      throw Poco::FileNotFoundException("Shared memory file not found: " + name + ".");
	    }
            Poco::SharedMemory shm2(file, Poco::SharedMemory::AM_READ);
	    std::cout << "read: '" <<  shm2.begin() << "' from shared memory."
		      << std::endl;

	    if(run) {
	      run_lambda(lambda, shm2.begin(), name);
	    }

#ifdef USE_AWS	    
	    write_s3(dest, shm2.begin());
#endif	    
	  }
	  catch (Poco::Exception& e) {
	    std::cerr << "Poco Exception: " << e.displayText() << std::endl;
	    return 1;
	  } catch (std::exception& e) {
	    std::cerr << "Standard Exception: " << e.what() << std::endl;
	    return 1;
	  }
#endif


#ifndef _WIN32
#ifdef USE_HERMES	  	  
          std::cout << "calling get" << dest << std::endl;
          // get(name, dest, nbyte);
#endif
#endif
	}
	
        if(it->second.IsScalar()){
          std::string value = it->second.as<std::string>();
          std::cout << key << ": " << value << std::endl;
        } else if (it->second.IsSequence()){
          std::cout << key << ": " << std::endl;
          for(size_t i = 0; i < it->second.size(); ++i){
            if(it->second[i].IsScalar()){
              std::cout << " - " << it->second[i].as<std::string>()
			<< std::endl;
	      if(key == "tags") {
		tags += it->second[i].as<std::string>();
		if (i < it->second.size() - 1) {
		  tags += ",";
		}
	      }
	      
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
        }  // for
      } // if 
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

int read_exact_bytes_from_offset(const char *filename, off_t offset,
                                 size_t num_bytes, unsigned char *buffer) {

    int fd = -1;
    ssize_t total_bytes_read = 0;
    ssize_t bytes_read;

    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking file");
        close(fd);
        return -1;
    }

    while (total_bytes_read < num_bytes) {
        bytes_read = read(fd, buffer + total_bytes_read,
			  num_bytes - total_bytes_read);
        if (bytes_read == -1) {
            perror("Error reading file");
            close(fd);
            return -1;
        }
        if (bytes_read == 0) {
            fprintf(stderr, "End of file reached after reading %zu bytes, expected %zu.\n", total_bytes_read, num_bytes);
            close(fd);
            return -2;
        }
        total_bytes_read += bytes_read;
    }

    if (close(fd) == -1) {
        perror("Error closing file");
        return -1;
    }
    
    if ((size_t)total_bytes_read == num_bytes)
      return 0;
    else
      return 1;

}

#ifdef USE_POCO
std::string sha256_file(const std::string& filePath) {
    try {
        // Open the file
        Poco::FileInputStream fis(filePath);
        
        // Create SHA256 engine
        Poco::SHA2Engine sha256(Poco::SHA2Engine::SHA_256);
        
        // Buffer for reading file
        const size_t bufferSize = 8192;
        char buffer[bufferSize];
        
        // Read file and update digest
        while (!fis.eof()) {
            fis.read(buffer, bufferSize);
            std::streamsize bytesRead = fis.gcount();
            if (bytesRead > 0) {
                sha256.update(buffer, static_cast<unsigned>(bytesRead));
            }
        }
        
        // Get the final digest
        const Poco::DigestEngine::Digest& digest = sha256.digest();
        
        // Convert to hexadecimal string
        std::stringstream ss;
        for (unsigned char b : digest) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
        }
        
        return ss.str();
    }
    catch (const Poco::Exception& ex) {
        throw std::runtime_error("Error calculating SHA256: " + ex.displayText());
    }
}
#endif

int write_omni(std::string buf) {

#ifdef USE_POCO
  std::string h = sha256_file(buf);
#endif 
  std::ofstream of(buf+".omni.yaml");
  of << "# OMNI" << std::endl;  
  of << "name: " << buf << std::endl;

#ifdef USE_POCO
  Poco::Path path(buf);
  of << "path: " << path.makeAbsolute().toString() << std::endl;
  if (!h.empty()) {
    of << "hash: " << h << std::endl;
  }
#endif
  of.close();
  
  return 0;
  
}

int set_blackhole(){
  
  std::cout << "checking IOWarp runtime...";
  if (std::filesystem::exists(".blackhole") == true) {
     std::cout << "...yes" << std::endl;
  }
  else {
    std::cout << "...no" << std::endl;
    std::cout << "launching a new IOWarp runtime....";
    if (std::filesystem::create_directory(".blackhole")) {
      std::cout << "...done" << std::endl;
      return 0;
    } else {
      std::cerr << "Error: failed to create .blackhole" << std::endl;
      return -1;
    }
  }
  return 0;
}

int list() {

  const std::string filename = ".blackhole/ls";
  
  std::ifstream inputFile(filename);

  if (!inputFile.is_open()) {
    std::cerr
      << "Error: Could not open the file \"" << filename << "\"" << std::endl;
    return 1;
  }

  std::string line;
  while (std::getline(inputFile, line)) {
    std::cout << line << std::endl;
  }

  if (inputFile.bad()) {
    std::cerr
      << "Error: An unrecoverable error occurred while reading the file."
      << std::endl;
    return 1;
  }
  inputFile.close();

  return 0; // Indicate success
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> [options]"
		  << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "put") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " put <omni.yaml>"
		      << std::endl;
            return 1;
        }
	if (set_blackhole() != 0) {
	  return 1;
	}
	
        std::string name = argv[2];
        std::cout << "input: " << name << std::endl;
        return read_omni(name);
	
    } else if (command == "get") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " get <buffer_name>"
		      << std::endl;
            return 1;
        }
        std::string name = argv[2];
        std::cout << "output: " << name << std::endl;
	return write_omni(name);
	
    } else if (command == "ls") {
      std::cout << "connecting runtime" << std::endl;
      return list();
      
    }
    else {
        std::cerr << "Invalid command: " << command << std::endl;
        return 1;
    }

    return 0;
}
