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
