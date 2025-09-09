
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
