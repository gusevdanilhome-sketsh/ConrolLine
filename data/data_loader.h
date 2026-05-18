#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include <string>
#include <vector>

class DataLoader {
public:
  void loadCSV(const std::string &filename,
               std::vector<std::vector<double>> &features,
               std::vector<int> &labels);
};

#endif