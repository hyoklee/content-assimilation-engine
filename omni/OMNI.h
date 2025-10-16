///
/// OMNI.h
///
#ifndef CAE_OMNI_H_
#define CAE_OMNI_H_

#include <string>
#include <cstddef>
#include <sys/types.h>

#ifdef USE_HERMES
#include <hermes/hermes.h>
#endif

namespace cae {

class OMNI {
 public:
  OMNI() = default;
  ~OMNI() = default;

  // Main public API - these call private helper methods
  int Put(const std::string& input_file);
  int Get(const std::string& buffer);
  int List();

  // Set quiet mode (suppress stdout)
  void SetQuiet(bool quiet) { quiet_ = quiet; }

  // DataHub configuration (public for testing)
  bool CheckDataHubConfig();
  std::string ReadConfigFile(const std::string& config_path);

 private:
  // Core processing methods
  int ReadOmni(const std::string& input_file);
  int WriteOmni(const std::string& buf);
  int SetBlackhole();

  // Utility functions
#ifdef USE_POCO
  std::string Sha256File(const std::string& file_path);
#endif
#ifdef _WIN32
  std::string GetExt(const std::string& filename);
#endif
  std::string GetFileName(const std::string& uri);
  int ReadExactBytesFromOffset(const char* filename, off_t offset,
                               size_t num_bytes, unsigned char* buffer);

  // Metadata functions
  int WriteMeta(const std::string& name, const std::string& tags);
  std::string ReadTags(const std::string& buf);

  // DataHub integration functions
  std::string ReadDataHubAPIKey();
  int RegisterWithDataHub(const std::string& name, const std::string& tags);

  // Storage backend functions
#ifdef USE_HERMES
  int PutHermesTags(hermes::Context* ctx, hermes::Bucket* bkt,
                    const std::string& tags);
  int PutHermes(const std::string& name, const std::string& tags,
                const std::string& path, unsigned char* buffer, size_t nbyte);
  int GetHermes(const std::string& name, const std::string& path);
#endif
  int PutData(const std::string& name, const std::string& tags,
              const std::string& path, unsigned char* buffer, size_t nbyte);
#ifdef USE_AWS
  int WriteS3(const std::string& dest, char* ptr);
#endif

  // Download/transfer functions
#ifdef USE_POCO
  int Download(const std::string& url, const std::string& output_file_name,
               long long start_byte, long long end_byte = -1);
#endif
  int RunLambda(const std::string& lambda, const std::string& name,
                const std::string& dest);

  // Member variables
  bool quiet_ = false;
};

}  // namespace cae

#endif  // CAE_OMNI_H_
